#define PARSE_AMOUNT 4          // максимальное количество значений в массиве, который хотим получить
#define header '$'              // стартовый символ
#define divider ' '             // разделительный символ
#define ending ';'              // завершающий символ

int16_t intData[PARSE_AMOUNT];  // массив численных значений после парсинга - для WiFi часы время синхр м.б отрицательным + 
                                // период синхронизации м.б больше 255 мин - нужен тип int16_t
uint32_t prevColor;
boolean recievedFlag;
byte lastMode = 0;
boolean parseStarted;
String pictureLine;

char incomeBuffer[UDP_TX_PACKET_MAX_SIZE];        // Буфер для приема строки команды из wifi udp сокета
char replyBuffer[7];                              // ответ клиенту - подтверждения получения команды: "ack;/r/n/0"

unsigned long ackCounter = 0;

void bluetoothRoutine() {  
  parsing();                                    // принимаем данные

  // на время принятия данных матрицу не обновляем!
  if (!parseStarted) {                          

    #if (USE_CLOCK == 1)
      if (WifiTimer.isReady() && wifi_connected) {  
        if (useNtp) {
          if (ntp_t > 0 && millis() - ntp_t > 3000) {
            Serial.println("NTP request timeout!");
            init_time = 0;
            ntp_t = 0;
          }
          if (ntpTimer.isReady() || (init_time == 0 && ntp_t == 0)) {
            getNTP();
          }
        }
      }
    #endif    

    if (!BTcontrol && effectsFlag && !isColorEffect(effect)) effectsFlag = false;

    if (runningFlag) {                         // бегущая строка - Running Text
      String text = runningText;
      if (text == "") {
        #if (USE_CLOCK == 1)          
           text = init_time ==0 
             ? clockCurrentText()
             : clockCurrentText() + " " + dateCurrentTextLong();  // + dateCurrentTextShort()
        #else
           text = "Gyver Matrix";
        #endif           
      }
      fillString(text, globalColor); 
      // Включенная бегущая строка только формирует строку в массиве точек матрицы, но не отображает ее
      // Если эффекты выключены - нужно принудительно вызывать отображение матрицы
      if (!effectsFlag) 
        FastLED.show();
      else if (isColorEffect(effect)) 
        effects();   
    }

    else if (drawingFlag) {
      // Рисование. Если эффект цветов - применить
      if (effectsFlag && isColorEffect(effect)) {  
         effects();   
      }
    }
    
    // Один из режимов игры. На игры эффекты не налагаются
    else if (gamemodeFlag && (!gamePaused || loadingFlag)) {
      // Для игр отключаем бегущую строку и эффекты
      effectsFlag = false;
      runningFlag = false;
      customRoutine();        
    }

    // Бегущая строка или Часы в основном режиме и эффект Дыхание или Цвета, Радуга пикс
    else if ((thisMode == DEMO_TEXT_0 || thisMode == DEMO_TEXT_1 || thisMode == DEMO_TEXT_2 || thisMode == DEMO_CLOCK) && effectsFlag && isColorEffect(effect)) { 

      // Подготовить изображение
      customModes();
      // Наложить эффект Дыхание / Цвета и вывести в матрицу
      effects();
    } else {
      // Сформировать и вывести на матрицу текущий демо-режим
      if (!BTcontrol || effectsFlag) 
        customRoutine();
      else if (BTcontrol && effectsFlag && isColorEffect(effect)) {
        effects();  
      }
    }            

    // Проверить - если долгое время не было ручного управления - переключиться в автоматический режим
    checkIdleState();
  }
}

// Блок эффектов наложения цветовых эффектов на сформированное изображение
void effects() {
  
  // Эффекты наложения цвета на изображение, имеющееся на матрице
  // Только эффекты 0, 1 и 5 совместимы с бегущей строкой - они меняют цвет букв
  // Остальные эффекты портят бегущую строку - ее нужно отключать  
  if (runningFlag && !isColorEffect(effect)) runningFlag = false;  // Дыхание, Цвет, Радуга пикс
    
  if (effectTimer.isReady()) {
    switch (effect) {
      case EFFECT_BREATH: brightnessRoutine(); break;         // Дыхание
      case EFFECT_COLOR: colorsRoutine(); break;              // Цвет
      case EFFECT_RAINBOW_PIX: rainbowColorsRoutine(); break; // Радуга пикс.
    }
    FastLED.show();
  }
}

byte parse_index;
String string_convert = "";
enum modes {NORMAL, COLOR, TEXT} parseMode;

bool haveIncomeData = false;
char incomingByte;

int16_t  bufIdx = 0;         // Могут приниматься пакеты > 255 байт - nbg int16_t
int16_t  packetSize = 0;

// разбор строки картинки - команда $5
char *pch;
int pntX, pntY, pntColor, pntIdx;
char buf[14];               // точка картинки FFFFFF XXX YYY
String pntPart[WIDTH];      // массив разобранной входной строки на строки точек

// ********************* ПРИНИМАЕМ ДАННЫЕ **********************
void parsing() {
// ****************** ОБРАБОТКА *****************
  String str;
  byte b_tmp;
  /*
    Протокол связи, посылка начинается с режима. Режимы:
    0 - отправка цвета $0 colorHEX;
    1 - отправка координат точки $1 X Y;
    2 - заливка - $2;
    3 - очистка - $3;
    4 - яркость - $4 value;
    5 - картинка построчно $5 Y colorHEX X|colorHEX X|...|colorHEX X;
    6 - текст $6 some text
    7 - управление текстом: $7 1; пуск, $7 0; стоп
    8 - эффект
      - $8 0 номерЭффекта;
      - $8 1 N X старт/стоп; N - номер эффекта, X=0 - стоп X=1 - старт 
    9 - игра
    10 - кнопка вверх
    11 - кнопка вправо
    12 - кнопка вниз
    13 - кнопка влево
    14 - пауза в игре
    15 - скорость $15 скорость таймер; 0 - таймер эффектов, 1 - таймер скроллинга текста 2 - таймер игр
    16 - Режим смены эффектов: $16 value; N:  0 - Autoplay on; 1 - Autoplay off; 2 - PrevMode; 3 - NextMode
    17 - Время автосмены эффектов и бездействия: $17 сек сек;
    18 - Запрос текущих параметров программой: $18 page;  page: 1 - настройки; 2 - рисование; 3 - картинка; 4 - текст; 5 - эффекты; 6 - игра; 7 - часы; 8 - о приложении 
    19 - работа с настройками часов
  */  
  if (recievedFlag) {      // если получены данные
    recievedFlag = false;

    // Режимы 16,17,18  не сбрасывают idleTimer
    if (intData[0] < 16 || intData[0] > 18) {
      idleTimer.reset();
      idleState = false;
    }

    switch (intData[0]) {
      case 0:
        if (!runningFlag) drawingFlag = true;
        sendAcknowledge();
        break;
      case 1:
        BTcontrol = true;
        drawingFlag = true;
        runningFlag = false;
        if (gamemodeFlag && game==1) gamePaused = true;
        else {
          gamemodeFlag = false;
          gamePaused = false;
        }
        if (!isColorEffect(effect)) {
            effectsFlag = false;
        }
        drawPixelXY(intData[1], intData[2], gammaCorrection(globalColor));
        FastLED.show();
        sendAcknowledge();
        break;
      case 2:
        BTcontrol = true;
        runningFlag = false;
        drawingFlag = true;
        gamemodeFlag = false;
        gamePaused = false;
        if (!isColorEffect(effect)) {
            effectsFlag = false;
        }
        fillAll(gammaCorrection(globalColor));
        FastLED.show();
        sendAcknowledge();
        break;
      case 3:
        BTcontrol = true;
        runningFlag = false;
        gamemodeFlag = false;
        gamePaused = false;
        drawingFlag = true;
        if (!isColorEffect(effect)) {
            effectsFlag = false;
        }
        FastLED.clear();
        FastLED.show();
        sendAcknowledge();
        break;
      case 4:
        globalBrightness = intData[1];
        breathBrightness = globalBrightness;
        saveMaxBrightness(globalBrightness);
        FastLED.setBrightness(globalBrightness);
        FastLED.show();
        sendAcknowledge();
        break;
      case 5:
        BTcontrol = true;

        if (!drawingFlag) {
          FastLED.clear(); 
        }
        
        effectsFlag = false;        
        runningFlag = false;
        gamemodeFlag = false;
        drawingFlag = true;
        loadingFlag = false;

        // Разбираем СТРОКУ из принятого буфера формата 'Y colorHEX X|colorHEX X|...|colorHEX X'
        // Получить номер строки (Y) для которой получили строку с данными 
        b_tmp = pictureLine.indexOf(" ");
        str = pictureLine.substring(0, b_tmp);
        pntY = str.toInt();
        pictureLine = pictureLine.substring(b_tmp+1);
        
        pictureLine.toCharArray(incomeBuffer, pictureLine.length()+1);
        pch = strtok (incomeBuffer,"|");
        pntIdx = 0;
        while (pch != NULL)
        {
          pntPart[pntIdx++] = String(pch);
          pch = strtok (NULL, "|");
        }
        
        for (int i=0; i<pntIdx; i++) {
          str = pntPart[i];
          str.toCharArray(buf, str.length()+1);

          pntColor=HEXtoInt(String(strtok(buf," ")));
          pntX=atoi(strtok(NULL," "));

          // начало картинки - очистить матрицу
          if ((pntX == 0) && (pntY == HEIGHT - 1)) {
            FastLED.clear(); 
            FastLED.show();
          }
          
          drawPixelXY(pntX, pntY, gammaCorrection(pntColor));
        }

        // Выводить построчно для ускорения вывода на экран
        if (pntX == WIDTH - 1)
          FastLED.show();

        // Подтвердить прием строки изображения
        str = "$5 " + String(pntY)+ "-" + String(pntX) + " ack" + String(ackCounter++) + ";";
  
        str.toCharArray(incomeBuffer, str.length()+1);    
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write(incomeBuffer);
        udp.endPacket();
        delay(0);
        break;
      case 6:
        loadingFlag = true;
        // строка принимается в переменную runningText
        sendAcknowledge();
        break;
      case 7:
        BTcontrol = true;
        if (intData[1] == 1) runningFlag = true;
        if (intData[1] == 0) runningFlag = false;
        if (runningFlag) {
          gamemodeFlag = false;
          drawingFlag = false;
          if (!isColorEffect(effect)) {
            effectsFlag = false;
          }
        }
        sendAcknowledge();
        break;
      case 8:
        effect = intData[2];
        gamemodeFlag = false;
        loadingFlag = !isColorEffect(effect);
        effectsFlag = true;
        if (!BTcontrol) BTcontrol = !isColorEffect(effect);     // При установке эффекта дыхание / цвета / радуга пикс - переключаться в управление по BT не нужно
        if (!isColorEffect(effect)) {
          drawingFlag = false;
          runningFlag = false;
        }
        
        effectSpeed = getEffectSpeed(effect);
        effectTimer.setInterval(effectSpeed);
                
        // intData[1] : дейстие -> 0 - выбор эффекта  = 1 - старт/стоп
        // intData[2] : номер эффекта
        // intData[3] : 0 - стоп 1 - старт
        effectsFlag = intData[1] == 0 || (intData[1] == 1 && intData[3] == 1); // выбор эффекта - сразу запускать
        
        // Найти соответствие thisMode указанному эффекту. 
        // Дльнейшее отображение изображения эффекта будет выполняться стандартной процедурой customRoutin()
        if (!isColorEffect(effect)) {            
           b_tmp = mapEffectToMode(effect);
           if (b_tmp != 255) thisMode = b_tmp;
        }

        breathBrightness = globalBrightness;
        FastLED.setBrightness(globalBrightness);    
        
        // Для "0" - отправляются параметры, подтверждение отправлять не нужно. Для остальных - нужно
        if (intData[1] == 0) {
          sendPageParams(5);
        } else { 
          sendAcknowledge();
        }
        break;
      case 9:        
        BTcontrol = true;        
        if (!drawingFlag || runningFlag) {        // начать новую игру при переходе со всех режимов кроме рисования
          loadingFlag = true;    
          FastLED.clear(); 
          FastLED.show(); 
        }
        effectsFlag = false;
        runningFlag = false;
        controlFlag = false;                      // Посе начала игры пока не трогаем кнопки - игра автоматическая 
        drawingFlag = false;
        gamemodeFlag = true;
        gamePaused = true;
        game = intData[1];
        
        // Найти соответствие thisMode указанной игре. 
        // Дльнейшее отображение изображения эффекта будет выполняться стандартной процедурой customRoutin()
        b_tmp = mapGameToMode(game);
        if (b_tmp != 255) thisMode = b_tmp;

        gameSpeed = getGameSpeed(game);
        gameTimer.setInterval(gameSpeed);        
        // Отправить программе актуальное состояние параметров эффектов (6 - страница "Игры")
        sendPageParams(6);
        break;        
      case 10:
        BTcontrol = true;        
        buttons = 0;
        controlFlag = true;
        gamePaused = false;
        gameDemo = false;
        sendAcknowledge();
        break;
      case 11:
        BTcontrol = true;
        buttons = 1;
        controlFlag = true;
        gamePaused = false;
        gameDemo = false;
        sendAcknowledge();
        break;
      case 12:
        BTcontrol = true;
        buttons = 2;
        controlFlag = true;
        gamePaused = false;
        gameDemo = false;
        sendAcknowledge();
        break;
      case 13:
        BTcontrol = true;
        buttons = 3;
        controlFlag = true;
        gamePaused = false;
        gameDemo = false;
        sendAcknowledge();
        break;
      case 14:
        BTcontrol = true;
        b_tmp = mapGameToMode(game);
        if (b_tmp != 255) {
          thisMode = b_tmp;
          if (!drawingFlag || (drawingFlag && game != 0) || runningFlag) {        // начать новую игру при переходе со всех режимов кроме рисования
            loadingFlag = true;                                                   // если игра в паузе змейка - продолжить, иначе начать  новую игру 
            FastLED.clear(); 
            FastLED.show(); 
          }
          effectsFlag = false;
          drawingFlag = false;
          runningFlag = false;
          gamemodeFlag = true;
          gamePaused = intData[1] == 0;  
          gameSpeed = getGameSpeed(game);
          gameTimer.setInterval(gameSpeed);        
        }
        sendAcknowledge();
        break;
      case 15: 
        if (intData[2] == 0) {
          effectSpeed = intData[1]; 
          saveEffectSpeed(effect, effectSpeed);
          effectTimer.setInterval(effectSpeed);
        } else if (intData[2] == 1) {
          scrollSpeed = intData[1]; 
          scrollTimer.setInterval(scrollSpeed);
          saveScrollSpeed(scrollSpeed);
        } else if (intData[2] == 2) {
          gameSpeed = map(constrain(intData[1],0,255),0,255,D_GAME_SPEED_MIN,D_GAME_SPEED_MAX);      // для игр скорость нужна меньше! вх 0..255 преобразовать в 25..375
          saveGameSpeed(game, gameSpeed);
          gameTimer.setInterval(gameSpeed);
        }
        sendAcknowledge();
        break;
      case 16:
        BTcontrol = intData[1] == 1;                
        if (intData[1] == 0) AUTOPLAY = true;
        else if (intData[1] == 1) AUTOPLAY = false;
        else if (intData[1] == 2) prevMode();
        else if (intData[1] == 3) nextMode();
        else if (intData[1] == 4) AUTOPLAY = intData[2] == 1;

        idleState = !BTcontrol && AUTOPLAY; 
        if (AUTOPLAY) {
          autoplayTimer = millis(); // При включении автоматического режима сбросить таймер автосмены режимов
          controlFlag = false; 
          gameDemo = true;
        }
        saveAutoplay(AUTOPLAY);

        if (!BTcontrol) {
          runningFlag = false;
          controlFlag = false;      // После начала игры пока не трогаем кнопки - игра автоматическая 
          drawingFlag = false;
          gamemodeFlag = false;
          gamePaused = false;
          loadingFlag = true;       // если false - при переключении с эффекта бегущий текст на демо-режим "бегущий текст" текст демо режима не сначала, а с позиции где бежал текст эффекта
                                    // если true - текст начинает бежать сначала, потом плавно затухает на смену режима и потом опять начинает сначала.
                                    // И так и так не хорошо. Как починить? 
        } else {
          // Если при переключении в ручной режим был демонстрационный режим бегущей строки - включить ручной режим бегщей строки
          if (intData[1] == 0 || (intData[1] == 1 && (thisMode == DEMO_TEXT_0 || thisMode == DEMO_TEXT_1 || thisMode == DEMO_TEXT_2))) {
            loadingFlag = true;
            runningFlag = true;          
          }
        }

        if (!BTcontrol && AUTOPLAY) {
          sendPageParams(1);
        } else {        
          sendAcknowledge();
        }
        
        break;
      case 17: 
        autoplayTime = ((long)intData[1] * 1000);   // секунды -> миллисек 
        idleTime = ((long)intData[2] * 60 * 1000);  // минуты -> миллисек
        saveAutoplayTime(autoplayTime);
        saveIdleTime(idleTime);
        if (AUTOPLAY) {
          autoplayTimer = millis();
          BTcontrol = false;
          controlFlag = false; 
          gameDemo = true;
        }
        idleState = !BTcontrol && AUTOPLAY; 
        if (idleState) {
          if (idleTime == 0) // тамймер отключен
            idleTimer.setInterval(4294967295);
          else
            idleTimer.setInterval(idleTime);
          idleTimer.reset();
        }
        sendAcknowledge();
        break;
      case 18: 
        if (intData[1] == 0)  // ping
          sendAcknowledge();
        else {                // запрос параметров страницы приложения
          sendPageParams(intData[1]);
          saveSettings();
        }
        break;
      case 19: 
#if (USE_CLOCK == 1)
         switch (intData[1]) {
           case 0:               // $19 0 N X; - сохранить настройку X "Часы в эффекте" для эффекта N
             saveEffectClock(intData[2], intData[3] == 1);
             break;
           case 1:               // $19 1 X; - сохранить настройку X "Часы в эффектах"
             saveClockOverlayEnabled(intData[2] == 1);
             break;
           case 2:               // $19 2 X; - Использовать синхронизацию часов NTP  X: 0 - нет, 1 - да
             useNtp = intData[2] == 1;
             saveUseNtp(useNtp);
             init_time = 0; ntp_t = 0;
             break;
           case 3:               // $19 3 N Z; - Период синхронизации часов NTP и Часовой пояс
             SYNC_TIME_PERIOD = intData[2];
             timeZoneOffset = (int8_t)intData[3];
             saveTimeZone(timeZoneOffset);
             saveNtpSyncTime(SYNC_TIME_PERIOD);
             saveTimeZone(timeZoneOffset);
             ntpTimer.setInterval(1000 * 60 * SYNC_TIME_PERIOD);
             init_time = 0; ntp_t = 0;
             break;
           case 4:               // $19 4 X; - Ориентация часов  X: 0 - горизонтально, 1 - вертикально
             CLOCK_ORIENT = intData[2] == 1 ? 1  : 0;
             // Центрируем часы по горизонтали/вертикали по ширине / высоте матрицы
             if (CLOCK_ORIENT == 0) {
               CLOCK_X = CLOCK_X_H;
               CLOCK_Y = CLOCK_Y_H;
             } else {
               CLOCK_X = CLOCK_X_V;
               CLOCK_Y = CLOCK_Y_V;
             }
             if (CLOCK_X < 0) CLOCK_X = 0;
             if (CLOCK_Y < 0) CLOCK_Y = 0;
             saveClockOrientation(CLOCK_ORIENT);
             break;
           case 5:               // $19 5 X; - Режим цвета часов  X: 0 - горизонтально, 1 - вертикально
             COLOR_MODE = intData[2];
             if (COLOR_MODE > 3) COLOR_MODE = 0;
             saveClockColorMode(COLOR_MODE);
             break;
        }
        sendAcknowledge();
#endif        
        break;
    }
    lastMode = intData[0];  // запомнить предыдущий режим
  }

  // ****************** ПАРСИНГ *****************
  haveIncomeData = false;

  if (!haveIncomeData) {

    // Если предыдущий буфер еще не разобран - новых данных из сокета не читаем, продолжаем разбор уже считанного буфера
    haveIncomeData = bufIdx > 0 && bufIdx < packetSize; 
    if (!haveIncomeData) {
      packetSize = udp.parsePacket();      
      haveIncomeData = packetSize > 0;      
    
      if (haveIncomeData) {                
        // read the packet into packetBufffer
        int len = udp.read(incomeBuffer, UDP_TX_PACKET_MAX_SIZE);
        if (len > 0) {
          incomeBuffer[len] =0;
        }
        bufIdx = 0;
        
        delay(0);            // ESP8266 при вызове delay отпрабатывает стек IP протокола, дадим ему поработать        

        Serial.print("UDP пакeт размером ");
        Serial.print(packetSize);
        Serial.print(" от ");
        IPAddress remote = udp.remoteIP();
        for (int i = 0; i < 4; i++) {
          Serial.print(remote[i], DEC);
          if (i < 3) {
            Serial.print(".");
          }
        }
        Serial.print(", port ");
        Serial.println(udp.remotePort());
        if (udp.remotePort() == localPort) {
          Serial.print("Содержимое: ");
          Serial.println(incomeBuffer);
        }
      }

#if (USE_CLOCK == 1)
      // NTP packet from time server
      if (udp.remotePort() == 123) {
        parseNTP();
        haveIncomeData = 0;
      }
#endif      
    }

    if (haveIncomeData) {         
      if (parseMode == TEXT) {                         // если нужно принять строку - принимаем всю
          // Из за ошибки в компоненте UdpSender в Thunkable - теряются половина отправленных 
          // символов, если их кодировка - двухбайтовый UTF8, т.к. оно вычисляет длину строки без учета двухбайтовости
          // Чтобы символы не терялись - при отправке строки из андроид-программы, она добивается с конца пробелами
          // Здесь эти конечные пробелы нужно предварительно удалить
          while (packetSize > 0 && incomeBuffer[packetSize-1] == ' ') packetSize--;
          incomeBuffer[packetSize] = 0;

          // Оставшийся буфер преобразуем с строку
          if (intData[0] == 5) {  // строка картинки
            pictureLine = String(&incomeBuffer[bufIdx]);
          } if (intData[0] == 6) {  // текст бегущей строки
            runningText = String(&incomeBuffer[bufIdx]);
            runningText.trim();
          }
                    
          incomingByte = ending;                       // сразу завершаем парс
          parseMode = NORMAL;
          bufIdx = 0; 
          packetSize = 0;                              // все байты из входящего пакета обработаны
        } else {
          incomingByte = incomeBuffer[bufIdx++];       // обязательно ЧИТАЕМ входящий символ
      } 
    }       
  }
  
  if (haveIncomeData) {

    if (parseStarted) {                                             // если приняли начальный символ (парсинг разрешён)
      if (incomingByte != divider && incomingByte != ending) {      // если это не пробел И не конец
        string_convert += incomingByte;                             // складываем в строку
      } else {                                                      // если это пробел или ; конец пакета
        if (parse_index == 0) {
          byte cmdMode = string_convert.toInt();
          intData[0] = cmdMode;
          if (cmdMode == 0) parseMode = COLOR;                      // передача цвета (в отдельную переменную)
          else if (cmdMode == 6 || cmdMode == 5) {
            parseMode = TEXT;
          }
          else parseMode = NORMAL;
          // if (cmdMode != 7 || cmdMode != 0) runningFlag = false;
        }

        if (parse_index == 1) {       // для второго (с нуля) символа в посылке
          if (parseMode == NORMAL) intData[parse_index] = string_convert.toInt();             // преобразуем строку в int и кладём в массив}
          if (parseMode == COLOR) {                                                           // преобразуем строку HEX в цифру
            globalColor = (uint32_t)HEXtoInt(string_convert);           
            if (intData[0] == 0) {
              if (runningFlag && effectsFlag) effectsFlag = false;   
              incomingByte = ending;
              parseStarted = false;
              BTcontrol = true;
            } else {
              parseMode = NORMAL;
            }
          }
        } else {
          intData[parse_index] = string_convert.toInt();  // преобразуем строку в int и кладём в массив
        }
        string_convert = "";                        // очищаем строку
        parse_index++;                              // переходим к парсингу следующего элемента массива
      }
    }

    if (incomingByte == header) {                   // если это $
      parseStarted = true;                          // поднимаем флаг, что можно парсить
      parse_index = 0;                              // сбрасываем индекс
      string_convert = "";                          // очищаем строку
    }

    if (incomingByte == ending) {                   // если таки приняли ; - конец парсинга
      parseMode == NORMAL;
      parseStarted = false;                         // сброс
      recievedFlag = true;                          // флаг на принятие
    }

    if (bufIdx >= packetSize) {                     // Весь буфер разобран 
      bufIdx = 0;
      packetSize = 0;
    }
  }
}

void sendPageParams(int page) {
  // W:число    ширина матрицы
  // H:число    высота матрицы
  // DM:Х       демо режим, где Х = 0 - выкл (ручное управление); 1 - вкл
  // AP:Х       автосменарежимов, где Х = 0 - выкл; 1 - вкл
  // PD:число   продолжительность режима в секундах
  // IT:число   время бездействия в секундах
  // BR:число   яркость
  // CL:HHHHHH  текущий цвет рисования, HEX
  // TX:[текст] текст, ограничители [] обязательны
  // TS:Х       состояние бегущей строки, где Х = 0 - выкл; 1 - вкл
  // ST:число   скорость прокрутки текста
  // EF:число   текущий эффект
  // ES:Х       состояние эффектов, где Х = 0 - выкл; 1 - вкл
  // EC:X       оверлей часов для эффекта вкл/выкл, где Х = 0 - выкл; 1 - вкл
  // SE:число   скорость эффектов
  // GM:число   текущая игра
  // GS:Х       состояние игры, где Х = 0 - выкл; 1 - вкл
  // SG:число   скорость игры
  // CE:X       оверлей часов вкл/выкл, где Х = 0 - выкл; 1 - вкл
  // CС:X       режим цвета часов: 0,1,2
  // NP:Х       использовать NTP, где Х = 0 - выкл; 1 - вкл
  // NT:число   период синхронизации NTP в минутах
  // NZ:число   часовой пояс -12..+12
  String str = "", color, text;
  boolean allowed;
  byte b_tmp;
  switch (page) { 
    case 1:  // Настройки. Вернуть: Ширина/Высота матрицы; Яркость; Деморежм и Автосмена; Время смены режимо
      str="$18 W:"+String(WIDTH)+"|H:"+String(HEIGHT)+"|DM:";
      if (BTcontrol) str+="0|AP:"; else str+="1|AP:";
      if (AUTOPLAY)  str+="1|BR:"; else str+="0|BR:";
      str+=String(globalBrightness) + "|PD:" + String(autoplayTime / 1000) + "|IT:" + String(idleTime / 60 / 1000) +  ";";
      break;
    case 2:  // Рисование. Вернуть: Яркость; Цвет точки;
      color = ("000000" + String(globalColor, HEX));
      color = color.substring(color.length() - 6); // FFFFFF             
      str="$18 BR:"+String(globalBrightness) + "|CL:" + color + ";";
      break;
    case 3:  // Картинка. Вернуть: Яркость;
      str="$18 BR:"+String(globalBrightness) + ";";
      break;
    case 4:  // Текст. Вернуть: Яркость; Скорость текста; Вкл/Выкл; Текст
      text = runningText;
      text.replace(";","~");
      str="$18 BR:"+String(globalBrightness) + "|ST:" + String(constrain(map(scrollSpeed, D_TEXT_SPEED_MIN,D_TEXT_SPEED_MAX, 0, 255), 0,255)) + "|ST:";
      if (runningFlag)  str+="1|TX:["; else str+="0|TX:[";
      str += text + "]" + ";";
      break;
    case 5:  // Эффекты. Вернуть: Номер эффекта, Остановлен или играет; Яркость; Скорость эффекта; Оверлей часов 
      allowed = false;
#if (USE_CLOCK == 1 && OVERLAY_CLOCK == 1)      
      b_tmp = mapEffectToMode(effect);
      if (b_tmp != 255) {
        for (byte i = 0; i < sizeof(overlayList); i++) {
          allowed = (b_tmp == overlayList[i]);
          if (allowed) break;
        }
      }
#endif      
      str="$18 EF:"+String(effect+1) + "|ES:";
      if (effectsFlag)  str+="1|BR:"; else str+="0|BR:";
      str+=String(globalBrightness) + "|SE:" + String(constrain(map(effectSpeed, D_EFFECT_SPEED_MIN,D_EFFECT_SPEED_MAX, 0, 255), 0,255));
#if (USE_CLOCK == 1)      
      if (isColorEffect(effect) || !allowed || effect == 9) 
          str+="|EC:X;";  // X - параметр не используется (неприменим)
      else    
          str+="|EC:" + String(getEffectClock(effect)) + ";";
#else
      str+="|EC:X;";  // X - параметр не используется (неприменим)
#endif      
      break;
    case 6:  // Игры. Вернуть: Номер игры; Вкл.выкл; Яркость; Скорость игры
      str="$18 GM:"+String(game+1) + "|GS:";
      if (gamemodeFlag && !gamePaused)  str+="1|BR:"; else str+="0|BR:";
      str+=String(globalBrightness) + "|SG:" + String(constrain(map(gameSpeed, D_GAME_SPEED_MIN,D_GAME_SPEED_MAX, 0, 255), 0,255)) + ";"; 
      break;
    case 7:  // Настройки часов. Вернуть: Оверлей вкл/выкл
#if (USE_CLOCK == 1)      
      str="$18 CE:"+String(getClockOverlayEnabled()) + "|CC:" + String(COLOR_MODE) + "|NP:"; 
      if (useNtp)  str+="1|NT:"; else str+="0|NT:";
      str+=String(SYNC_TIME_PERIOD) + "|NZ:" + String(timeZoneOffset)  
      + ";";
#endif      
      break;
  }
  
  if (str.length() > 0) {
    // Отправить клиенту запрошенные параметры страницы / режимов
    str.toCharArray(incomeBuffer, str.length()+1);    
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(incomeBuffer);
    udp.endPacket();
    delay(0);
  } else {
    sendAcknowledge();
  }
}

void sendAcknowledge() {
  // Отправить подтверждение, чтобы клиентский сокет прервал ожидание
  String reply = "ack" + String(ackCounter++) + ";";
  reply.toCharArray(replyBuffer, reply.length()+1);    
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write(replyBuffer);
  udp.endPacket();
  delay(0);
}

// hex string to uint32_t
uint32_t HEXtoInt(String hexValue) {
  byte tens, ones, number1, number2, number3;
  tens = (hexValue[0] < '9') ? hexValue[0] - '0' : hexValue[0] - '7';
  ones = (hexValue[1] < '9') ? hexValue[1] - '0' : hexValue[1] - '7';
  number1 = (16 * tens) + ones;

  tens = (hexValue[2] < '9') ? hexValue[2] - '0' : hexValue[2] - '7';
  ones = (hexValue[3] < '9') ? hexValue[3] - '0' : hexValue[3] - '7';
  number2 = (16 * tens) + ones;

  tens = (hexValue[4] < '9') ? hexValue[4] - '0' : hexValue[4] - '7';
  ones = (hexValue[5] < '9') ? hexValue[5] - '0' : hexValue[5] - '7';
  number3 = (16 * tens) + ones;

  return ((uint32_t)number1 << 16 | (uint32_t)number2 << 8 | number3 << 0);
}

bool isColorEffect(byte effect) {
  // Цветовые эффекты - Дыхание, Цвет или Радуга пикс.
  // Они могут работать с custom демо режимами
  return effect == EFFECT_BREATH || effect == EFFECT_COLOR || effect == EFFECT_RAINBOW_PIX;
}

byte mapEffectToMode(byte effect) {
  byte tmp_mode = 255;
  
  switch (effect) {
    case EFFECT_SNOW:                tmp_mode = DEMO_SNOW; break;                 // snowRoutine();
    case EFFECT_BALL:                tmp_mode = DEMO_BALL; break;                 // ballRoutine();
    case EFFECT_RAINBOW:             tmp_mode = DEMO_RAINBOW; break;              // rainbowRoutine();
    case EFFECT_FIRE:                tmp_mode = DEMO_FIRE; break;                 // fireRoutine()
    case EFFECT_MATRIX:              tmp_mode = DEMO_MATRIX; break;               // matrixRoutine();
    case EFFECT_BALLS:               tmp_mode = DEMO_BALLS; break;                // ballsRoutine();
    case EFFECT_CLOCK:               tmp_mode = DEMO_CLOCK; break;                // clockRoutine();
    case EFFECT_STARFALL:            tmp_mode = DEMO_STARFALL; break;             // starfallRoutine()
    case EFFECT_SPARKLES:            tmp_mode = DEMO_SPARKLES; break;             // sparklesRoutine()
    case EFFECT_RAINBOW_DIAG:        tmp_mode = DEMO_RAINBOW_DIAG; break;         // rainbowDiagonalRoutine();
    case EFFECT_NOISE_MADNESS:       tmp_mode = DEMO_NOISE_MADNESS;  break;       // madnessNoise();
    case EFFECT_NOISE_CLOUD:         tmp_mode = DEMO_NOISE_CLOUD;  break;         // cloudNoise();
    case EFFECT_NOISE_LAVA:          tmp_mode = DEMO_NOISE_LAVA;  break;          // lavaNoise();
    case EFFECT_NOISE_PLASMA:        tmp_mode = DEMO_NOISE_PLASMA;  break;        // plasmaNoise();
    case EFFECT_NOISE_RAINBOW:       tmp_mode = DEMO_NOISE_RAINBOW;  break;       // rainbowNoise();
    case EFFECT_NOISE_RAINBOW_STRIP: tmp_mode = DEMO_NOISE_RAINBOW_STRIP;  break; // rainbowStripeNoise();
    case EFFECT_NOISE_ZEBRA:         tmp_mode = DEMO_NOISE_ZEBRA;  break;         // zebraNoise();
    case EFFECT_NOISE_FOREST:        tmp_mode = DEMO_NOISE_FOREST; break;         // forestNoise();
    case EFFECT_NOISE_OCEAN:         tmp_mode = DEMO_NOISE_OCEAN; break;          // oceanNoise();
    case EFFECT_ANIMATION:           tmp_mode = DEMO_ANIMATION; break;            // animation();

    // Нет соответствия - выполняются для текущего режима thisMode
    case EFFECT_BREATH:              // Дыхание
    case EFFECT_COLOR:               // Цвет
    case EFFECT_RAINBOW_PIX:         // Радуга пикс
      break;
  }

  return tmp_mode;
}

byte mapGameToMode(byte game) {
  byte tmp_mode = 255;
  
  switch (game) {
    case GAME_SNAKE:    tmp_mode = DEMO_SNAKE;    break;  // snakeRoutine(); 
    case GAME_TETRIS:   tmp_mode = DEMO_TETRIS;   break;  // tetrisRoutine();
    case GAME_MAZE:     tmp_mode = DEMO_MAZE;     break;  // mazeRoutine();
    case GAME_RUNNER:   tmp_mode = DEMO_RUNNER;   break;  // runnerRoutine();
    case GAME_FLAPPY:   tmp_mode = DEMO_FLAPPY;   break;  // flappyRoutine();
    case GAME_ARKANOID: tmp_mode = DEMO_ARKANOID; break;  // arkanoidRoutine();
  }

  return tmp_mode;
}

byte mapModeToEffect(byte aMode) {
  byte tmp_effect = 255;
  // Если режима нет в списке - ему нет соответствия среди эффектов - значит это игра или бегущий текст
  switch (aMode) {
    case DEMO_NOISE_MADNESS:        tmp_effect = EFFECT_NOISE_MADNESS; break;       // madnessNoise();
    case DEMO_NOISE_CLOUD:          tmp_effect = EFFECT_NOISE_CLOUD; break;         // cloudNoise();
    case DEMO_NOISE_LAVA:           tmp_effect = EFFECT_NOISE_LAVA; break;          // lavaNoise();
    case DEMO_NOISE_PLASMA:         tmp_effect = EFFECT_NOISE_PLASMA; break;        // plasmaNoise();
    case DEMO_NOISE_RAINBOW:        tmp_effect = EFFECT_NOISE_RAINBOW; break;       // rainbowNoise();
    case DEMO_NOISE_RAINBOW_STRIP:  tmp_effect = EFFECT_NOISE_RAINBOW_STRIP; break; // rainbowStripeNoise();
    case DEMO_NOISE_ZEBRA:          tmp_effect = EFFECT_NOISE_ZEBRA; break;         // zebraNoise();
    case DEMO_NOISE_FOREST:         tmp_effect = EFFECT_NOISE_FOREST; break;        // forestNoise();
    case DEMO_NOISE_OCEAN:          tmp_effect = EFFECT_NOISE_OCEAN; break;         // oceanNoise();
    case DEMO_SNOW:                 tmp_effect = EFFECT_SNOW;  break;               // snowRoutine();
    case DEMO_SPARKLES:             tmp_effect = EFFECT_SPARKLES; break;            // sparklesRoutine()
    case DEMO_MATRIX:               tmp_effect = EFFECT_MATRIX;  break;             // matrixRoutine();
    case DEMO_STARFALL:             tmp_effect = EFFECT_STARFALL; break;            // starfallRoutine()
    case DEMO_BALL:                 tmp_effect = EFFECT_BALL;  break;               // ballRoutine();
    case DEMO_BALLS:                tmp_effect = EFFECT_BALLS;  break;              // ballsRoutine();
    case DEMO_RAINBOW:              tmp_effect = EFFECT_RAINBOW;  break;            // rainbowRoutine();
    case DEMO_RAINBOW_DIAG:         tmp_effect = EFFECT_RAINBOW_DIAG; break;        // rainbowDiagonalRoutine();
    case DEMO_FIRE:                 tmp_effect = EFFECT_FIRE;  break;               // fireRoutine()
    case DEMO_ANIMATION:            tmp_effect = EFFECT_ANIMATION; break;           // animation();

    case DEMO_TEXT_0 :  break;      // Бегущий текст
    case DEMO_TEXT_1 :  break;      // Бегущий текст
    case DEMO_TEXT_2 :  break;      // Бегущий текст

    case DEMO_SNAKE: break;         // snakeRoutine(); 
    case DEMO_TETRIS: break;        // tetrisRoutine();
    case DEMO_MAZE: break;          // mazeRoutine();
    case DEMO_RUNNER: break;        // runnerRoutine();
    case DEMO_FLAPPY: break;        // flappyRoutine();
    case DEMO_ARKANOID: break;      // arkanoidRoutine();
    
    case DEMO_CLOCK: break;         // clockRoutine();     
  }
  return tmp_effect;
}

byte mapModeToGame(byte aMode) {
  byte tmp_game = 255;
  // Если режима нет в списке - ему нет соответствия среди тгр - значит это эффект или бегущий текст
  switch (aMode) {
    case DEMO_NOISE_MADNESS:        break;       // madnessNoise();
    case DEMO_NOISE_CLOUD:          break;       // cloudNoise();
    case DEMO_NOISE_LAVA:           break;       // lavaNoise();
    case DEMO_NOISE_PLASMA:         break;       // plasmaNoise();
    case DEMO_NOISE_RAINBOW:        break;       // rainbowNoise();
    case DEMO_NOISE_RAINBOW_STRIP:  break;       // rainbowStripeNoise();
    case DEMO_NOISE_ZEBRA:          break;       // zebraNoise();
    case DEMO_NOISE_FOREST:         break;       // forestNoise();
    case DEMO_NOISE_OCEAN:          break;       // oceanNoise();
    case DEMO_SNOW:                 break;       // snowRoutine();
    case DEMO_SPARKLES:             break;       // sparklesRoutine()
    case DEMO_MATRIX:               break;       // matrixRoutine();
    case DEMO_STARFALL:             break;       // starfallRoutine()
    case DEMO_BALL:                 break;       // ballRoutine();
    case DEMO_BALLS:                break;       // ballsRoutine();
    case DEMO_RAINBOW:              break;       // rainbowRoutine();
    case DEMO_RAINBOW_DIAG:         break;       // rainbowDiagonalRoutine();
    case DEMO_FIRE:                 break;       // fireRoutine()
    case DEMO_ANIMATION:            break;       // animation();

    case DEMO_TEXT_0:               break;       // Бегущий текст
    case DEMO_TEXT_1:               break;       // Бегущий текст
    case DEMO_TEXT_2:               break;       // Бегущий текст

    case DEMO_SNAKE:    tmp_game = GAME_SNAKE;    break;     // snakeRoutine(); 
    case DEMO_TETRIS:   tmp_game = GAME_TETRIS;   break;     // tetrisRoutine();
    case DEMO_MAZE:     tmp_game = GAME_MAZE;     break;     // mazeRoutine();
    case DEMO_RUNNER:   tmp_game = GAME_RUNNER;   break;     // runnerRoutine();
    case DEMO_FLAPPY:   tmp_game = GAME_FLAPPY;   break;     // flappyRoutine();
    case DEMO_ARKANOID: tmp_game = GAME_ARKANOID; break;     // arkanoidRoutine();

    case DEMO_CLOCK: break;  // clockRoutine();     
  }
  return tmp_game;
}
