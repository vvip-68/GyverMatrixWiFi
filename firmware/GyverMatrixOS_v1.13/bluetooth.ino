#define UDP_PACKET_MAX_SIZE 1024
#define PARSE_AMOUNT 8          // максимальное количество значений в массиве, который хотим получить
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
char replyBuffer[7];                           // ответ клиенту - подтверждения получения команды: "ack;/r/n/0"
char incomeBuffer[UDP_PACKET_MAX_SIZE];        // Буфер для приема строки команды из wifi udp сокета. Также буфер используется для отправки строк в смартфон
                                               // Строка со списком эффектов может быть длинной, плюс кириллица в UTF занимает 2 байта - буфер должен быть большим.
unsigned long ackCounter = 0;
String receiveText = "", s_tmp = "";
byte tmpSaveMode = 0;

void bluetoothRoutine() {  
  
  parsing();                                    // принимаем данные

  if (tmpSaveMode != thisMode) {
    tmpSaveMode = thisMode;
    if (thisMode == DEMO_TEXT_0 || thisMode == DEMO_TEXT_1 || thisMode == DEMO_TEXT_2) {
      // Это бегущий текст  
      Serial.print(F("Включена бегущая строка "));
      Serial.println(thisMode);
    } else {
      byte tmp_effect = mapModeToEffect(thisMode);
      if (tmp_effect != 255) {
        s_tmp = String(EFFECT_LIST).substring(0,UDP_PACKET_MAX_SIZE);
        s_tmp = GetToken(s_tmp, tmp_effect+1, ',');
        Serial.print(F("Включен эффект "));
        Serial.println("'" + s_tmp + "'");
      } else {
        byte tmp_game = mapModeToGame(thisMode);
        if (tmp_game != 255) {
          s_tmp = String(GAME_LIST).substring(0,UDP_PACKET_MAX_SIZE);
          s_tmp = GetToken(s_tmp, tmp_game+1, ',');
          Serial.print(F("Включена игра "));
          Serial.println("'" + s_tmp + "'");
        } else {
          Serial.print(F("Включен режим "));
          Serial.println(thisMode);
        }
      }
    }
  }

  // на время принятия данных матрицу не обновляем!
  if (!parseStarted) {                          

    if (wifi_connected && useNtp) {
      if (ntp_t > 0 && millis() - ntp_t > 5000) {
        Serial.println(F("Таймаут NTP запроса!"));
        ntp_t = 0;
        ntp_cnt++;
        if (init_time && ntp_cnt >= 10) {
          Serial.println(F("Не удалось установить соединение с NTP сервером."));  
          refresh_time = false;
        }
      }
      bool timeToSync = ntpSyncTimer.isReady();
      if (timeToSync) { ntp_cnt = 0; refresh_time = true; }
      if (timeToSync || (refresh_time && ntp_t == 0 && (ntp_cnt < 10 || !init_time))) {
        getNTP();
        if (ntp_cnt >= 10) {
          if (init_time) {
            udp.flush();
          } else {
            //ESP.restart();
            ntp_cnt = 0;
            connectToNetwork();
          }
        }        
      }
    }

    if (useAutoBrightness && autoBrightnessTimer.isReady()) {
      // Во время работы будильника-рассвет, ночных часов, если матрица "выключена" или один из режимов "лампы" - освещения
      // авторегулировки яркости нет.    
      if (!(isAlarming || isNightClock || isTurnedOff || specialModeId == 2 || specialModeId == 3 || specialModeId == 6 || specialModeId == 7 || thisMode == DEMO_DAWN_ALARM)) {
        // 300 - это при макс. освещении, чтобы макс возможный 255 наступал не на грани порога чувствительности ФР, а немного раньше  
        int16_t val = (byte)brightness_filter.filtered((int16_t)map(analogRead(PHOTO_PIN),0,1023,0,300)); 
        if (val < autoBrightnessMin) val = autoBrightnessMin;
        if (val > 255) val = 255;
        if (specialMode) {
           specialBrightness = val;
        } else {
           globalBrightness = val;
        }        
        FastLED.setBrightness(val);
        // В режиме рисования нужно принудительно обновлять экран,
        // т.к статическое изображение не обновляется автоматически 
        if (drawingFlag) FastLED.show();
      }
    }
    
    if (!BTcontrol && effectsFlag && !isColorEffect(effect)) effectsFlag = false;

    // При яркости = 1 остаются гореть только красные светодиоды и все эффекты теряют вид.
    // поэтому отображать эффект "ночные часы"
    byte br = specialMode ? specialBrightness : globalBrightness;
    if (br == 1 && !(loadingFlag || isAlarming)) {
      doEffectWithOverlay(DEMO_CLOCK);    
    } 
    
    else if (runningFlag && !isAlarming) {                         // бегущая строка - Running Text
      String txt = runningText;
      uint32_t txtColor = globalColor;
      if (wifi_print_ip && (wifi_current_ip.length() > 0)) {
        txt = wifi_current_ip;     
        txtColor = 0xffffff;
      }
      if (txt.length() == 0) {
         txt = init_time
           ? clockCurrentText() + " " + dateCurrentTextLong()  // + dateCurrentTextShort()
           : TEXT_1; 
      }      
      fillString(txt, txtColor); 
      // Включенная бегущая строка только формирует строку в массиве точек матрицы, но не отображает ее
      // Если эффекты выключены - нужно принудительно вызывать отображение матрицы
      if (!effectsFlag) 
        FastLED.show();
      else if (isColorEffect(effect)) 
        effects();   
    }

    else if (drawingFlag && !isAlarming) {
      // Рисование. Если эффект цветов - применить
      if (effectsFlag && isColorEffect(effect)) {  
         effects();   
      }
    }
    
    // Один из режимов игры. На игры эффекты не налагаются
    else if (gamemodeFlag && (!gamePaused || loadingFlag) && !isAlarming) {
      // Для игр отключаем бегущую строку и эффекты
      effectsFlag = false;
      runningFlag = false;
      customRoutine();
    }

    // Бегущая строка или Часы в основном режиме и эффект Дыхание или Цвета, Радуга пикс
    else if ((thisMode == DEMO_TEXT_0 || thisMode == DEMO_TEXT_1 || thisMode == DEMO_TEXT_2 || thisMode == DEMO_CLOCK) && effectsFlag && isColorEffect(effect) && !isAlarming) { 

      // Подготовить изображение
      customModes(thisMode);
      // Наложить эффект Дыхание / Цвета и вывести в матрицу
      effects();
      
    } else {
      
      // Сформировать и вывести на матрицу текущий демо-режим
      if (!BTcontrol || effectsFlag || isAlarming) 
        customRoutine();
      else if (BTcontrol && effectsFlag && isColorEffect(effect)) {
        effects();  
      }      
    }            

    checkAlarmTime();
    checkAutoMode1Time();
    checkAutoMode2Time();

    butt.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
    byte clicks = 0;

    // Один клик
    if (butt.isSingle()) clicks = 1;    
    // Двойной клик
    if (butt.isDouble()) clicks = 2;
    // Тройной клик
    if (butt.isTriple()) clicks = 3;
    // Четверной и более клик
    if (butt.hasClicks()) clicks = butt.getClicks();
    
    if (butt.isHolded()) {
      // Нажатие и удержание
      isButtonHold = true;
      hold_start_time = millis();
    }
    
    if (butt.isPress()) {
      // Состояние - кнопку нажали  
    }
    
    if (butt.isRelease()) {
      // Состояние - кнопку отпустили
      isButtonHold = false;
      hold_start_time = 0;
    }

    // Любое нажатие кнопки останавливает будильник
    if ((isAlarming || isPlayAlarmSound) && (isButtonHold || clicks > 0)) {
      stopAlarm();
    }
            
    // Обработка нажатий кнопки
    else if (isButtonHold) {
      
      // Если работает будильник - любое количество нажатий или удержание прерывает будильник и включает часы на черном фоне     
      if ((isAlarming || isPlayAlarmSound)) {
        stopAlarm();
      }

      // Если с момента нажатия и удержания прошло более HOLD_TIMEOUT милисекунд...
      if (hold_start_time != 0 && (millis() - hold_start_time) > HOLD_TIMEOUT) {
        isButtonHold = false;
        if (isTurnedOff)
          // Если выключен - включить часы        
          setSpecialMode(1);
        else 
          // Если включен - выключить (включить спец-режим черный экран)
          setSpecialMode(0);
      }      
      
    } else if (!isTurnedOff) {
    
      // Прочие клики работают только если не выключено
      
      // Был одинарный клик
      if (clicks == 1) {
        if (wifi_print_ip) {
          wifi_print_ip = false;
          wifi_current_ip = "";
          runningFlag = false;
          effectsFlag = true;
        }
        if (specialMode) {
          // Если в спецрежиме - и белый экран - вкл.выкл часы на белом экране
          if (specialModeId == 2 || specialModeId == 3) {
             if (specialModeId == 2) setSpecialMode(3);
             else setSpecialMode(2);
          } else {
            // переключение по кругу между обычными (1) и ночными (4) часами и часами с огнем (5)
            if (specialModeId == 4) setSpecialMode(1);
            else if (specialModeId == 1) setSpecialMode(5);
            else setSpecialMode(4);              
          }
        } else {
          // Если в режиме демо - следующий режим
          nextMode();
        }        
      }

      // Был двойной клик
      if (clicks == 2) { 
        if (wifi_print_ip) {
          wifi_print_ip = false;
          wifi_current_ip = "";
          runningFlag = false;
          effectsFlag = true;
        }
        if (specialModeId < 0) {
          // Из любого режима - включить часы
          setSpecialMode(1);
        } else {
          // Если ярко-белый включен (лампа) - вернуться в часы
          if (specialModeId == 2 || specialModeId == 3) 
            // Включить обычные часы
            setSpecialMode(1);
          else  
            // Включить ярко белый экран (лампа)
            setSpecialMode(2);
        }
      }

      // Тройное нажатие      
      if (clicks == 3) {
        // Включить демо-режим
        idleTimer.setInterval(idleTime);
        idleTimer.reset();
        
        if (wifi_print_ip) {
          wifi_print_ip = false;
          wifi_current_ip = "";
          runningFlag = false;
          effectsFlag = true;
        }

        resetModes();  

        BTcontrol = false;
        AUTOPLAY = true;

        String s_tmp = String(ALARM_LIST);    
        uint32_t cnt = CountTokens(s_tmp, ','); 
        byte ef = random(0, cnt - 1); 
            
        // Включить указанный режим из списка доступных эффектов без дальнейшей смены
        // Значение ef может быть 0..N-1 - указанный режим из списка ALARM_LIST (приведенное к индексу с 0)      
        byte tmp = mapAlarmToEffect(ef);   
        // Если не опознали что за эффект - включаем режим "Камин"
        if (tmp != 255) setEffect(tmp);
        else            setEffect(EFFECT_FIRE); 
      }

      // Четверное нажатие - показать текущий IP WiFi-соединения
      if (clicks == 4) {
        showCurrentIP();
      }
      
      // ... и т.д.
    }

    // Есть ли изменение статуса MP3-плеера?
    if (dfPlayer.available()) {

      // Вывести детали об изменении статуса в лог
      byte msg_type = dfPlayer.readType();      
      printDetail(msg_type, dfPlayer.read());

      // Действия, которые нужно выполнить при изменении некоторых статусов:
      if (msg_type == DFPlayerCardRemoved) {
        // Карточка "отвалилась" - делаем недоступным все что связано с MP3 плеером
        isDfPlayerOk = false;
        alarmSoundsCount = 0;
        dawnSoundsCount = 0;
        Serial.println(F("MP3 плеер недоступен."));
      } else if (msg_type == DFPlayerCardOnline || msg_type == DFPlayerCardInserted) {
        // Плеер распознал карту - переинициализировать стадию 2
        InitializeDfPlayer2();
        if (!isDfPlayerOk) Serial.println(F("MP3 плеер недоступен."));
      }
    }

    // Проверить - если долгое время не было ручного управления - переключиться в автоматический режим
    if (!(isAlarming || isPlayAlarmSound)) checkIdleState();

    // Если есть несохраненные в EEPROM данные - сохранить их
    if (saveSettingsTimer.isReady()) {
      saveSettings();
    }
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
  String str, str1, str2;
  byte b_tmp;
  int8_t tmp_eff, idx;

  byte alarmDay;
  byte alarmHourVal;
  byte alarmMinuteVal;

  /*
    Протокол связи, посылка начинается с режима. Режимы:
    0 - отправка цвета $0 colorHEX;
    1 - отправка координат точки $1 X Y;
    2 - заливка - $2;
    3 - очистка - $3;
    4 - яркость - 
      $4 0 value   установить текущий уровень яркости
      $4 1 U V     установить режим авторегулировки яркости и мин. яркости впри авторегулировке, 
                   где U: 0 - выкл 1 - вкл; М - значение яркости - 1..255
    5 - картинка построчно $5 Y colorHEX X|colorHEX X|...|colorHEX X;
    6 - текст $6 N|some text, где N - назначение текста;
        0 - текст бегущей строки
        1 - имя сервера NTP
        2 - SSID сети подключения
        3 - пароль для подключения к сети 
        4 - имя точки доступа
        5 - пароль к точке доступа
        6 - настройки будильников
    7 - управление текстом: 
        $7 1 пуск; 
        $7 0 стоп; 
        $7 2 использовать в демо-режиме; 
        $7 3 НЕ использовать в демо-режиме; 
    8 - эффект
      - $8 0 номер эффекта;
      - $8 1 N X старт/стоп; N - номер эффекта, X=0 - стоп X=1 - старт 
      - $8 2 N X вкл/выкл использовать в демо-режиме; N - номер эффекта, X=0 - не использовать X=1 - использовать 
    9 - игра
      - $9 0 номер игры; 
      - $9 1 N X старт/стоп; N - номер игры, X=0 - стоп X=1 - старт 
      - $9 2 N X вкл/выкл использовать в демо-режиме; N - номер игры, X=0 - не использовать X=1 - использовать 
    10 - кнопка вверх
    11 - кнопка вправо
    12 - кнопка вниз
    13 - кнопка влево
    14 - быстрая установка ручных режимов с пред-настройками
       - $14 0; Черный экран (выкл);  
       - $14 1; Черный экран с часами;  
       - $14 2; Белый экран (освещение);  
       - $14 3; Белый экран с часами;  
       - $14 4; Черный экран с часами мин яркости - ночной режим;  
       - $14 5; Черный экран с эффектом огня и часами (камин);  
       - $14 6; Цветной экран;  
       - $14 7; Цветной экран с часами;  
    15 - скорость $15 скорость таймер; 0 - таймер эффектов, 1 - таймер скроллинга текста 2 - таймер игр
    16 - Режим смены эффектов: $16 value; N:  0 - Autoplay on; 1 - Autoplay off; 2 - PrevMode; 3 - NextMode
    17 - Время автосмены эффектов и бездействия: $17 сек сек;
    18 - Запрос текущих параметров программой: $18 page;  page: 1 - настройки; 2 - рисование; 3 - картинка; 4 - текст; 5 - эффекты; 6 - игра; 7 - часы; 8 - о приложении 
    19 - работа с настройками часов
    20 - настройки и управление будильников
       - $20 0;       - отключение будильника (сброс состояния isAlarming)
       - $20 1 DD EF WD;
           DD    - установка продолжительности рассвета (рассвет начинается за DD минут до установленного времени будильника)
           EF    - установка эффекта, который будет использован в качестве рассвета
           WD    - установка дней пн-вс как битовая маска
       - $20 2 X DD VV MA MB;
            X    - исп звук будильника X=0 - нет, X=1 - да 
           DD    - игратьзвук будильника DD минут послесрабатывания будильника
           VV    - максимальная громкость
           MA    - номер файла звука будильника
           MB    - номер файла звука рассвета
       - $20 3 NN VV X; - пример звука будильника
           NN - номер файла звука будильника из папки SD:/01
           VV - уровень громкости
           X  - 1 играть 0 - остановить
       - $20 4 NN VV X; - пример звука рассвета
           NN - номер файла звука рассвета из папки SD:/02
           VV - уровень громкости
           X  - 1 играть 0 - остановить
       - $20 5 VV; - установит уровень громкости проигрывания примеров (когда уже играет)
           VV - уровень громкости
       - $20 6 D HH MM; - установит время будильника для указанного дня недели
           D  - день недели
           HH - час времени будильника
           MM - минуты времени будильника
    21 - настройки подключения к сети . точке доступа
    22 - настройки включения режимов матрицы в указанное время
       - $22 X HH MM NN
           X  - номер режима 1 или 2
           HH - час срабатывания
           MM - минуты срабатывания
           NN - эффект: -2 - выключено; -1 - выключить матрицу; 0 - случайный режим и далее по кругу; 1 и далее - список режимов ALARM_LIST 
  */  
  if (recievedFlag) {      // если получены данные
    recievedFlag = false;

    // Режимы 16,17,18  не сбрасывают idleTimer
    if (intData[0] < 16 || intData[0] > 18) {
      idleTimer.reset();
      idleState = false;      
    }

    // Режимы кроме 4 (яркость), 14 (новый спец-режим) и 18 (запрос параметров страницы),
    // 19 (настройки часов), 20 (настройки будильника), 21 (настройки сети) сбрасывают спец-режим
    if (intData[0] != 4  && intData[0] != 14 && 
        intData[0] != 18 && intData[0] != 19 &&
        intData[0] != 20 && intData[0] != 21) {
      if (specialMode) {
        idleTimer.setInterval(idleTime);
        idleTimer.reset();
        specialMode = false;
        isNightClock = false;
        isTurnedOff = false;
        specialModeId = -1;
      }
    }

    // Режимы кроме 18 останавливают будильник, если он работает (идет рассвет)
    if (intData[0] != 18 && intData[0] != 20) {
      wifi_print_ip = false;
      wifi_current_ip = "";
      stopAlarm();
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
        if (intData[1] == 0) {
          globalBrightness = intData[2];
          breathBrightness = globalBrightness;
          saveMaxBrightness(globalBrightness);
          if (!(isNightClock || useAutoBrightness)) {          
            if (specialMode) specialBrightness = globalBrightness;
            FastLED.setBrightness(globalBrightness);
            FastLED.show();
          }
        }
        if (intData[1] == 1) {
          useAutoBrightness = intData[2] == 1;
          autoBrightnessMin = intData[3];
          if (autoBrightnessMin < 1) autoBrightnessMin = 1;
          setUseAutoBrightness(useAutoBrightness);
          setAutoBrightnessMin(autoBrightnessMin);
        }
        saveSettingsTimer.reset();
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
        // Получить номер строки (Y) для которой получили строку с данными (номер строки - сверху вниз, в то время как матрица - индекс строки снизу вверх)
        b_tmp = pictureLine.indexOf(" ");
        str = pictureLine.substring(0, b_tmp);
        pntY = str.toInt();
        pictureLine = pictureLine.substring(b_tmp+1);

        pntIdx = 0;
        idx = pictureLine.indexOf("|");
        while (idx>0)
        {
          str = pictureLine.substring(0, idx);
          pictureLine = pictureLine.substring(idx+1);
          
          pntPart[pntIdx++] = str;
          idx = pictureLine.indexOf("|");

          if (idx<0 && pictureLine.length()>0) {
            pntPart[pntIdx++] = pictureLine;  
          }          
          delay(0);
        }

        for (int i=0; i<pntIdx; i++) {
          str = pntPart[i];
          idx = str.indexOf(" ");
          str1 = str.substring(0, idx);
          str2 = str.substring(idx+1);

          pntColor=HEXtoInt(str1);
          pntX=str2.toInt();
          
          // начало картинки - очистить матрицу
          if (pntX == 0 && pntY == 0) {
            FastLED.clear(); 
            FastLED.show();
          }
          
          drawPixelXY(pntX, HEIGHT - pntY - 1, gammaCorrection(pntColor));
          delay(0);
        }

        // Выводить построчно для ускорения вывода на экран
        if (pntX == WIDTH - 1)
          FastLED.show();

        // Подтвердить прием строки изображения
        str = "$5 " + String(pntY)+ "-" + String(pntX) + " ack" + String(ackCounter++) + ";";
  
        str.toCharArray(incomeBuffer, str.length()+1);    
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write(incomeBuffer, str.length()+1);
        udp.endPacket();
        delay(0);
        break;
      case 6:
        loadingFlag = true;
        // строка принимается в переменную receiveText, формат строки N|text, где N:
        // 0 - текст для бегущей строки
        // 1 - имя сервера NTP
        // 2 - имя сети (SSID)
        // 3 - пароль к сети
        // 4 - имя точки доступа
        // 5 - пароль точки доступа
        // 6 - настройки будильника в формате $6 6|DD EF WD HH1 MM1 HH2 MM2 HH3 MM3 HH4 MM4 HH5 MM5 HH6 MM6 HH7 MM7
        tmp_eff = receiveText.indexOf("|");
        if (tmp_eff > 0) {
          b_tmp = receiveText.substring(0, tmp_eff).toInt();
          str = receiveText.substring(tmp_eff+1, receiveText.length()+1);
           switch(b_tmp) {
            case 0:
              runningText = str;
              break;
            case 1:
              str.toCharArray(ntpServerName, 30);
              setNtpServer(str);
              if (wifi_connected) {
                refresh_time = true; ntp_t = 0; ntp_cnt = 0;
              }
              break;
            case 2:
              str.toCharArray(ssid, 24);
              setSsid(str);
              break;
            case 3:
              str.toCharArray(pass, 16);
              setPass(str);
              break;
            case 4:
              str.toCharArray(apName, 10);
              setSoftAPName(str);
              break;
            case 5:
              str.toCharArray(apPass, 16);
              setSoftAPPass(str);
              // Передается в одном пакете - использовать SoftAP, имя точки и пароль
              // После получения пароля - перезапустить создание точки доступа
              if (useSoftAP) startSoftAP();
              break;
            case 6:
              // Настройки будильника в формате $6 6|DD EF WD HH1 MM1 HH2 MM2 HH3 MM3 HH4 MM4 HH5 MM5 HH6 MM6 HH7 MM7
              // DD    - установка продолжительности рассвета (рассвет начинается за DD минут до установленного времени будильника)
              // EF    - установка эффекта, который будет использован в качестве рассвета
              // WD    - установка дней пн-вс как битовая маска
              // HHx   - часы дня недели x (1-пн..7-вс)
              // MMx   - минуты дня недели x (1-пн..7-вс)
              //
              // Остановить будильнтк, если он сработал
              dfPlayer.stop();
              soundFolder = 0;
              soundFile = 0;
              isAlarming = false;
              isAlarmStopped = false;
              
              // Настройки содержат 17 элеиентов (см. формат выше)
              tmp_eff = CountTokens(str, ' ');
              if (tmp_eff == 17) {
              
                dawnDuration = constrain(GetToken(str, 1, ' ').toInt(),1,59);
                alarmEffect = mapAlarmToEffect(GetToken(str, 2, ' ').toInt());
                alarmWeekDay = GetToken(str, 3, ' ').toInt();
                saveAlarmParams(alarmWeekDay,dawnDuration,alarmEffect);
                
                for(byte i=0; i<7; i++) {
                  alarmHourVal = constrain(GetToken(str, i*2+4, ' ').toInt(), 0, 23);
                  alarmMinuteVal = constrain(GetToken(str, i*2+5, ' ').toInt(), 0, 59);
                  alarmHour[i] = alarmHourVal;
                  alarmMinute[i] = alarmMinuteVal;
                  setAlarmTime(i+1, alarmHourVal, alarmMinuteVal);
                }
                // Незамедлительно сохранить настройки будильника
                saveSettings();
                // Рассчитать время начала рассвета будильника
                calculateDawnTime();            
              }
              break;
           }
        }
        saveSettingsTimer.reset();
        if (b_tmp == 6) 
          sendPageParams(8);
        else
          sendAcknowledge();
        break;
      case 7:
        BTcontrol = true;
        if (intData[1] == 0 || intData[1] == 1) {
          if (intData[1] == 1) runningFlag = true;
          if (intData[1] == 0) runningFlag = false;          
          if (runningFlag) {
            startRunningText();
          }
        }
        else if (intData[1] == 2 || intData[1] == 3) {
          // Вкл/Выкл "Использовать в демо-режиме": 2 - вкл; 3 - выкл;
          setUseTextInDemo(intData[1] == 2);
        }
        saveSettingsTimer.reset();
        sendAcknowledge();
        break;
      case 8:      
        effect = intData[2];        
        // intData[1] : дейстие -> 0 - выбор эффекта;  1 - старт/стоп; 2 - вкл/выкл "использовать в демо-режиме"
        // intData[2] : номер эффекта
        // intData[3] : действие = 1: 0 - стоп 1 - старт; действие = 2: 0 - выкл; 1 - вкл;
        if (intData[1] == 0 || intData[1] == 1) {          
          setEffect(effect);
          if (!BTcontrol) BTcontrol = !isColorEffect(effect);                    // При установке эффекта дыхание / цвета / радуга пикс - переключаться в управление по BT не нужно
          loadingFlag = intData[1] == 0 && !isColorEffect(effect);
          effectsFlag = intData[1] == 0 || (intData[1] == 1 && intData[3] == 1); // выбор эффекта - сразу запускать           
          if (effect == EFFECT_FILL_COLOR && globalColor == 0x000000) setGlobalColor(0xffffff);
        } else if (intData[1] == 2) {
          // Вкл/выкл использование эффекта в демо-режиме
          saveEffectUsage(effect, intData[3] == 1); 
        }
        saveSettingsTimer.reset();                
        // Для "0" и "2" - отправляются параметры, подтверждение отправлять не нужно. Для остальных - нужно
        if (intData[1] == 0 || intData[1] == 2) {
          sendPageParams(5);
        } else { 
          sendAcknowledge();
        }
        break;
      case 9:        
        game = intData[2];
        // intData[1] : дейстие -> 0 - выбор игры;  1 - старт/стоп; 2 - вкл/выкл "использовать в демо-режиме"
        // intData[2] : номер игры
        // intData[3] : действие = 1: 0 - стоп 1 - старт; действие = 2: 0 - выкл; 1 - вкл;
        if (intData[1] == 0 || intData[1] == 1) {
          BTcontrol = true; 

          // начать новую игру при переходе со всех режимов кроме рисования
          // если игра в паузе змейка (game==0) - продолжить, иначе начать  новую игру 
          startGame(game, 
                    intData[1] == 0 && (!drawingFlag || (drawingFlag && game != GAME_SNAKE) || runningFlag), // new Game ?
                    intData[1] == 0 || (intData[1] == 1 && intData[3] == 0)                                  // is Paused ?
          );          
        } else if (intData[1] == 2) {
          // Вкл/выкл использование игры в демо-режиме
          saveGameUsage(game, intData[3] == 1); 
        }
        saveSettingsTimer.reset();

        // Для "0" и "2" - отправляются параметры, подтверждение отправлять не нужно. Для остальных - нужно
        if (intData[1] == 0 || intData[1] == 2) {
          sendPageParams(6);
        } else { 
          sendAcknowledge();
        }
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
        setSpecialMode(intData[1]);
        sendPageParams(1);
        break;
      case 15: 
        if (intData[2] == 0) {
          effectSpeed = intData[1]; 
          saveEffectSpeed(effect, effectSpeed);
          effectTimer.setInterval(effectSpeed);
          gifTimer.setInterval(effectSpeed * 10);
        } else if (intData[2] == 1) {
          scrollSpeed = intData[1]; 
          scrollTimer.setInterval(scrollSpeed);
          saveScrollSpeed(scrollSpeed);
        } else if (intData[2] == 2) {
          gameSpeed = map(constrain(intData[1],0,255),0,255,D_GAME_SPEED_MIN,D_GAME_SPEED_MAX);      // для игр скорость нужна меньше! вх 0..255 преобразовать в 25..375
          saveGameSpeed(game, gameSpeed);
          gameTimer.setInterval(gameSpeed);
        }
        saveSettingsTimer.reset();
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
          if (runningFlag) loadingFlag = true;       
          // если false - при переключении с эффекта бегущий текст на демо-режим "бегущий текст" текст демо режима не сначала, а с позиции где бежал текст эффекта
          // если true - текст начинает бежать сначала, потом плавно затухает на смену режима и потом опять начинает сначала.
          // И так и так не хорошо. Как починить? 
          
          runningFlag = false;
          controlFlag = false;      // После начала игры пока не трогаем кнопки - игра автоматическая 
          drawingFlag = false;
          gamemodeFlag = false;
          gamePaused = false;
          specialMode = false;
        } else {
          // Если при переключении в ручной режим был демонстрационный режим бегущей строки - включить ручной режим бегщей строки
          if (intData[1] == 0 || (intData[1] == 1 && (thisMode == DEMO_TEXT_0 || thisMode == DEMO_TEXT_1 || thisMode == DEMO_TEXT_2))) {
            loadingFlag = true;
            runningFlag = true;          
          }
        }

        saveSettingsTimer.reset();

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
        saveSettingsTimer.reset();
        sendAcknowledge();
        break;
      case 18: 
        if (intData[1] == 0) { // ping
          sendAcknowledge();
        } else {               // запрос параметров страницы приложения
          sendPageParams(intData[1]);
        }
        break;
      case 19: 
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
             if (wifi_connected) {
               refresh_time = true; ntp_t = 0; ntp_cnt = 0;
             }
             break;
           case 3:               // $19 3 N Z; - Период синхронизации часов NTP и Часовой пояс
             SYNC_TIME_PERIOD = intData[2];
             timeZoneOffset = (int8_t)intData[3];
             saveTimeZone(timeZoneOffset);
             saveNtpSyncTime(SYNC_TIME_PERIOD);
             saveTimeZone(timeZoneOffset);
             ntpSyncTimer.setInterval(1000 * 60 * SYNC_TIME_PERIOD);
             if (wifi_connected) {
               refresh_time = true; ntp_t = 0; ntp_cnt = 0;
             }
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
             checkClockOrigin();
             saveClockOrientation(CLOCK_ORIENT);
             break;
           case 5:               // $19 5 X; - Режим цвета часов  X: 0 - горизонтально, 1 - вертикально
             COLOR_MODE = intData[2];
             if (COLOR_MODE > 3) COLOR_MODE = 0;
             saveClockColorMode(COLOR_MODE);
             break;
           case 6:               // $19 6 X; - Показывать дату в режиме часов  X: 0 - нет, 1 - да
             showDateInClock = intData[2] == 1;
             setShowDateInClock(showDateInClock);
             break;
           case 7:               // $19 7 D I; - Продолжительность отображения даты / часов (в секундах)
             showDateDuration = intData[2];
             showDateInterval = intData[3];
             setShowDateDuration(showDateDuration);
             setShowDateInterval(showDateInterval);
             break;
           case 8:               // $19 8 YYYY MM DD HH MM; - Установить текущее время YYYY.MM.DD HH:MM
             setTime(intData[5],intData[6],0,intData[4],intData[3],intData[2]);
             init_time = true; refresh_time = false; ntp_cnt = 0;
             break;
        }
        saveSettingsTimer.reset();
        sendAcknowledge();
        break;
      case 20:
        switch (intData[1]) { 
          case 0:  
            if (isAlarming || isPlayAlarmSound) {
              stopAlarm();            
            }
            break;
          case 2:
            if (isDfPlayerOk) {
              // $20 2 X DD VV MA MB;
              //    X    - исп звук будильника X=0 - нет, X=1 - да 
              //   DD    - играть звук будильника DD минут послесрабатывания будильника
              //   VV    - максимальная громкость
              //   MA    - номер файла звука будильника
              //   MB    - номер файла звука рассвета
              dfPlayer.stop();
              soundFolder = 0;
              soundFile = 0;
              isAlarming = false;
              isAlarmStopped = false;

              useAlarmSound = intData[2] == 1;
              alarmDuration = constrain(intData[3],1,10);
              maxAlarmVolume = constrain(intData[4],0,30);
              alarmSound = intData[5] - 2;  // Индекс от приложения: 0 - нет; 1 - случайно; 2 - 1-й файл; 3 - ... -> -1 - нет; 0 - случайно; 1 - 1-й файл и т.д
              dawnSound = intData[6] - 2;   // Индекс от приложения: 0 - нет; 1 - случайно; 2 - 1-й файл; 3 - ... -> -1 - нет; 0 - случайно; 1 - 1-й файл и т.д
              saveAlarmSounds(useAlarmSound, alarmDuration, maxAlarmVolume, alarmSound, dawnSound);
              saveSettingsTimer.reset();
            }
            break;
          case 3:
            if (isDfPlayerOk) {
              // $20 3 X NN VV; - пример звука будильника
              //  X  - 1 играть 0 - остановить
              //  NN - номер файла звука будильника из папки SD:/01
              //  VV - уровень громкости
              if (intData[2] == 0) {
                StopSound(0);
                soundFolder = 0;
                soundFile = 0;
              } else {
                b_tmp = intData[3] - 2;  // Знач: -1 - нет; 0 - случайно; 1 и далее - файлы; -> В списке индексы: 1 - нет; 2 - случайно; 3 и далее - файлы
                if (b_tmp > 0 && b_tmp <= alarmSoundsCount) {
                  dfPlayer.stop();
                  soundFolder = 1;
                  soundFile = b_tmp;
                  dfPlayer.volume(constrain(intData[4],0,30));
                  dfPlayer.playFolder(soundFolder, soundFile);
                  dfPlayer.enableLoop();
                } else {
                  soundFolder = 0;
                  soundFile = 0;
                }
              }
            }  
            break;
          case 4:
            if (isDfPlayerOk) {
             // $20 4 X NN VV; - пример звука рассвета
             //    X  - 1 играть 0 - остановить
             //    NN - номер файла звука рассвета из папки SD:/02
             //    VV - уровень громкости
              if (intData[2] == 0) {
                StopSound(0);
                soundFolder = 0;
                soundFile = 0;
              } else {
                dfPlayer.stop();
                b_tmp = intData[3] - 2; // Знач: -1 - нет; 0 - случайно; 1 и далее - файлы; -> В списке индексы: 1 - нет; 2 - случайно; 3 и далее - файлы
                if (b_tmp > 0 && b_tmp <= dawnSoundsCount) {
                  soundFolder = 2;
                  soundFile = b_tmp;
                  dfPlayer.volume(constrain(intData[4],0,30));
                  dfPlayer.playFolder(soundFolder, soundFile);
                  dfPlayer.enableLoop();
                } else {
                  soundFolder = 0;
                  soundFile = 0;
                }
              }
            }
            break;
          case 5:
            if (isDfPlayerOk && soundFolder > 0) {
             // $20 5 VV; - установить уровень громкости проигрывания примеров (когда уже играет)
             //    VV - уровень громкости
             maxAlarmVolume = constrain(intData[2],0,30);
             dfPlayer.volume(maxAlarmVolume);
            }
            break;
        }
        if (intData[1] == 0) {
          sendPageParams(8);
        } else if (intData[1] == 1 || intData[1] == 2) { // Режимы установки параметров - сохранить
          // saveSettings();
          sendPageParams(8);
        } else {
          sendPageParams(96);
        }        
        break;
      case 21:
        // Настройки подключения к сети
        switch (intData[1]) { 
          // $21 0 0 - не использовать точку доступа $21 0 1 - использовать точку доступа
          case 0:  
            useSoftAP = intData[2] == 1;
            setUseSoftAP(useSoftAP);
            if (useSoftAP && !ap_connected) 
              startSoftAP();
            else if (!useSoftAP && ap_connected) {
              if (wifi_connected) { 
                ap_connected = false;              
                WiFi.softAPdisconnect(true);
                Serial.println(F("Точка доступа отключена."));
              }
            }      
            saveSettingsTimer.reset();
            break;
          case 1:  
            // $21 1 IP1 IP2 IP3 IP4 - установить статический IP адрес подключения к локальной WiFi сети, пример: $21 1 192 168 0 106
            // Локальная сеть - 10.х.х.х или 172.16.х.х - 172.31.х.х или 192.168.х.х
            // Если задан адрес не локальной сети - сбросить его в 0.0.0.0, что означает получение динамического адреса 
            if (!(intData[2] == 10 || intData[2] == 172 && intData[3] >= 16 && intData[3] <= 31 || intData[2] == 192 && intData[3] == 168)) {
              intData[2] = 0;
              intData[3] = 0;
              intData[4] = 0;
              intData[5] = 0;
            }
            saveStaticIP(intData[2], intData[3], intData[4], intData[5]);
            saveSettingsTimer.reset();
            break;
          case 2:  
            // $21 2; Выполнить переподключение к сети WiFi
            FastLED.clear();
            FastLED.show();
            startWiFi();
            showCurrentIP();
            break;
        }
        if (intData[1] == 0 || intData[1] == 1) {
          sendAcknowledge();
        } else {
          sendPageParams(9);
        }
        break;
      case 22:
      /*  22 - настройки включения режимов матрицы в указанное время
       - $22 HH1 MM1 NN1 HH2 MM2 NN2
           HHn - час срабатывания
           MMn - минуты срабатывания
           NNn - эффект: -5 - выключено; -4 - выключить матрицу; -3 - ночные часы; -2 - камин с часами; -1 бегущая строка ..  0 - случайный режим и далее по кругу; 1 и далее - список режимов ALARM_LIST 
      */    
        AM1_hour = intData[1];
        AM1_minute = intData[2];
        AM1_effect_id = intData[3];
        if (AM1_hour < 0) AM1_hour = 0;
        if (AM1_hour > 23) AM1_hour = 23;
        if (AM1_minute < 0) AM1_minute = 0;
        if (AM1_minute > 59) AM1_minute = 59;
        if (AM1_effect_id < -5) AM1_effect_id = -5;
        setAM1params(AM1_hour, AM1_minute, AM1_effect_id);
        AM2_hour = intData[4];
        AM2_minute = intData[5];
        AM2_effect_id = intData[6];
        if (AM2_hour < 0) AM2_hour = 0;
        if (AM2_hour > 23) AM2_hour = 23;
        if (AM2_minute < 0) AM2_minute = 0;
        if (AM2_minute > 59) AM2_minute = 59;
        if (AM2_effect_id < -5) AM2_effect_id = -5;
        setAM2params(AM2_hour, AM2_minute, AM2_effect_id);

        saveSettings();
        sendPageParams(10);
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
        int len = udp.read(incomeBuffer, UDP_PACKET_MAX_SIZE);
        if (len > 0) {          
          incomeBuffer[len] =0;
        }
        bufIdx = 0;
        
        delay(0);            // ESP8266 при вызове delay отпрабатывает стек IP протокола, дадим ему поработать        

        Serial.print(F("UDP пакeт размером "));
        Serial.print(packetSize);
        Serial.print(F(" от "));
        IPAddress remote = udp.remoteIP();
        for (int i = 0; i < 4; i++) {
          Serial.print(remote[i], DEC);
          if (i < 3) {
            Serial.print(F("."));
          }
        }
        Serial.print(F(", порт "));
        Serial.println(udp.remotePort());
        if (udp.remotePort() == localPort) {
          Serial.print(F("Содержимое: "));
          Serial.println(incomeBuffer);
        }
      }

      // NTP packet from time server
      if (haveIncomeData && udp.remotePort() == 123) {
        parseNTP();
        haveIncomeData = false;
      }
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
          } else if (intData[0] == 6) {  // текст
            receiveText = String(&incomeBuffer[bufIdx]);
            receiveText.trim();
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
            setGlobalColor((uint32_t)HEXtoInt(string_convert));
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
  // W:число     ширина матрицы
  // H:число     высота матрицы
  // DM:Х        демо режим, где Х = 0 - выкл (ручное управление); 1 - вкл
  // AP:Х        автосменарежимов, где Х = 0 - выкл; 1 - вкл
  // PD:число    продолжительность режима в секундах
  // IT:число    время бездействия в секундах
  // BR:число    яркость
  // CL:HHHHHH   текущий цвет рисования, HEX
  // TX:[текст]  текст, ограничители [] обязательны
  // TS:Х        состояние бегущей строки, где Х = 0 - выкл; 1 - вкл
  // ST:число    скорость прокрутки текста
  // EF:число    текущий эффект
  // ES:Х        состояние эффектов, где Х = 0 - выкл; 1 - вкл
  // EC:X        оверлей часов для эффекта вкл/выкл, где Х = 0 - выкл; 1 - вкл
  // SE:число    скорость эффектов
  // GM:число    текущая игра
  // GS:Х        состояние игры, где Х = 0 - выкл; 1 - вкл
  // SG:число    скорость игры
  // CE:X        оверлей часов вкл/выкл, где Х = 0 - выкл; 1 - вкл
  // CС:X        режим цвета часов: 0,1,2
  // NP:Х        использовать NTP, где Х = 0 - выкл; 1 - вкл
  // NT:число    период синхронизации NTP в минутах
  // NZ:число    часовой пояс -12..+12
  // NS:[текст]  сервер NTP, ограничители [] обязательны
  // UT:X        использовать бегущую строку в демо-режиме 0-нет, 1-да
  // UE:X        использовать эффект в демо-режиме 0-нет, 1-да
  // UG:X        использовать игру в демо-режиме 0-нет, 1-да
  // DC:X        показывать дату вместе с часами 0-нет, 1-да
  // DD:число    время показа даты при отображении часов (в секундах)
  // DI:число    интервал показа даты при отображении часов (в секундах)
  // LG:[список] список игр, разделенный запятыми, ограничители [] обязательны        
  // LE:[список] список эффектов, разделенный запятыми, ограничители [] обязательны        
  // LA:[список] список эффектов для будильника, разделенный запятыми, ограничители [] обязательны        
  // AL:X        сработал будильник 0-нет, 1-да
  // AT:HH MM     часы-минуты времени будильника -> например "09 15"
  // AW:число    битовая маска дней недели будильника b6..b0: b0 - пн .. b7 - вс
  // AD:число    продолжительность рассвета, мин
  // AE:число    эффект, использующийся для будильника
  // AO:X        включен будильник 0-нет, 1-да
  // NW:[текст]  SSID сети подключения
  // NA:[текст]  пароль подключения к сети
  // AU:X        создавать точку доступа 0-нет, 1-да
  // AN:[текст]  имя точки доступа
  // AA:[текст]  парольточки доступа
  // MX:X        MP3 плеер доступен для использования 0-нет, 1-да
  // MU:X        использовать звук в будильнике 0-нет, 1-да
  // MD:число    сколько минут звучит будильник, если его не отключили
  // MV:число    максимальная громкость будильника
  // MA:число    номер файла звука будильника из SD:/01
  // MB:число    номер файла звука рассвета из SD:/02
  // MP:папка.файл  номер папки и файла звука который проигрывается
  // BU:X        использовать авторегулировку яркости 0-нет, 1-да
  // BY:число    минимальное значений япкости при авторегулировке
  // IP:xx.xx.xx.xx Текущий IP адрес WiFi соединения в сети
  // AM1H:HH     час включения режима 1     00..23
  // AM1M:MM     минуты включения режима 1  00..59
  // AM1E:NN     номер эффекта режима 1:   -2 - не используется; -1 - выключить матрицу; 0 - включить случайный с автосменой; 1 - номер режима из спписка ALARM_LIST
  // AM2H:HH     час включения режима 2     00..23
  // AM2M:MM     минуты включения режима 2  00..59
  // AM2E:NN     номер эффекта режима 1:   -2 - не используется; -1 - выключить матрицу; 0 - включить случайный с автосменой; 1 - номер режима из спписка ALARM_LIST
  
  String str = "", color, text;
  boolean allowed;
  byte b_tmp;
  switch (page) { 
    case 1:  // Настройки. Вернуть: Ширина/Высота матрицы; Яркость; Деморежм и Автосмена; Время смены режимо
      str="$18 W:"+String(WIDTH)+"|H:"+String(HEIGHT)+"|DM:";
      if (BTcontrol)  str+="0|AP:"; else str+="1|AP:";
      if (AUTOPLAY)   str+="1|BR:"; else str+="0|BR:";
      str+=String(globalBrightness) + "|PD:" + String(autoplayTime / 1000) + "|IT:" + String(idleTime / 60 / 1000) +  "|AL:";
      if ((isAlarming || isPlayAlarmSound) && !isAlarmStopped) str+="1"; else str+="0";
      str+="|BU:" + String(useAutoBrightness ? "1" : "0");    
      str+="|BY:" + String(autoBrightnessMin);       
      str+=";";
      break;
    case 2:  // Рисование. Вернуть: Яркость; Цвет точки;
      color = ("000000" + String(globalColor, HEX));
      color = color.substring(color.length() - 6); // FFFFFF             
      str="$18 BR:"+String(globalBrightness) + "|CL:" + color;
      str+="|BU:" + String(useAutoBrightness ? "1" : "0");    
      str+="|BY:" + String(autoBrightnessMin);       
      str+=";";
      break;
    case 3:  // Картинка. Вернуть: Яркость;
      str="$18 BR:"+String(globalBrightness);
      str+="|BU:" + String(useAutoBrightness ? "1" : "0");    
      str+="|BY:" + String(autoBrightnessMin);       
      str+=";";
      break;
    case 4:  // Текст. Вернуть: Яркость; Скорость текста; Вкл/Выкл; Текст; Использовать в демо
      text = runningText;
      text.replace(";","~");
      str="$18 BR:"+String(globalBrightness) + "|ST:" + String(constrain(map(scrollSpeed, D_TEXT_SPEED_MIN,D_TEXT_SPEED_MAX, 0, 255), 0,255)) + "|TS:";
      if (runningFlag)  str+="1|TX:["; else str+="0|TX:[";
      str += text + "]" + "|UT:";
      if (getUseTextInDemo())  str+="1"; else str+="0";
      str+="|BU:" + String(useAutoBrightness ? "1" : "0");    
      str+="|BY:" + String(autoBrightnessMin);       
      str+=";";
      break;
    case 5:  // Эффекты. Вернуть: Номер эффекта, Остановлен или играет; Яркость; Скорость эффекта; Оверлей часов; Использовать в демо 
      allowed = false;
#if (OVERLAY_CLOCK == 1)      
      b_tmp = mapEffectToModeCode(effect); 
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
      str+="|BU:" + String(useAutoBrightness ? "1" : "0");    
      str+="|BY:" + String(autoBrightnessMin);       
      if (isColorEffect(effect) || !allowed || effect == EFFECT_CLOCK) 
          str+="|EC:X";  // X - параметр не используется (неприменим)
      else    
          str+="|EC:" + String(getEffectClock(effect));
      if (isColorEffect(effect)) 
          str+="|UE:X";  // X - параметр не используется (неприменим)
      else if (getEffectUsage(effect))
          str+="|UE:1";
      else    
          str+="|UE:0";
      str+=";";
      break;
    case 6:  // Игры. Вернуть: Номер игры; Вкл.выкл; Яркость; Скорость игры; Использовать в демо
      str="$18 GM:"+String(game+1) + "|GS:";
      if (gamemodeFlag && !gamePaused)  str+="1|BR:"; else str+="0|BR:";
      str+=String(globalBrightness) + "|SG:" + String(constrain(map(gameSpeed, D_GAME_SPEED_MIN,D_GAME_SPEED_MAX, 0, 255), 0,255)); 
      str+="|BU:" + String(useAutoBrightness ? "1" : "0");    
      str+="|BY:" + String(autoBrightnessMin);       
      str+="|UG:" + String(getGameUsage(game) ? "1" : "0");    
      str+=";";
      break;
    case 7:  // Настройки часов. Вернуть: Оверлей вкл/выкл
      str="$18 CE:"+String(getClockOverlayEnabled()) + "|CC:" + String(COLOR_MODE) + "|NP:"; 
      if (useNtp)  str+="1|NT:"; else str+="0|NT:";
      str+=String(SYNC_TIME_PERIOD) + "|NZ:" + String(timeZoneOffset) + "|DC:"; 
      if (showDateInClock)  str+="1|DD:"; else str+="0|DD:";
      str+=String(showDateDuration) + "|DI:" + String(showDateInterval) + "|NS:["; 
      str+=String(ntpServerName)+"]";
      str+=";";
      break;
    case 8:  // Настройки будильника
      str="$18 AL:"; 
      if ((isAlarming || isPlayAlarmSound) && !isAlarmStopped) str+="1|AD:"; else str+="0|AD:";
      str+=String(dawnDuration)+"|AW:";
      for (int i=0; i<7; i++) {
         if (((alarmWeekDay>>i) & 0x01) == 1) str+="1"; else str+="0";  
         if (i<6) str+='.';
      }
      for (int i=0; i<7; i++) {      
            str+="|AT:"+String(i+1)+" "+String(alarmHour[i])+" "+String(alarmMinute[i]);
      }
      str+="|AE:" + String(mapEffectToAlarm(alarmEffect) + 1); // Индекс в списке в приложении смартфона начинается с 1
      str+="|MX:" + String(isDfPlayerOk ? "1" : "0");          // 1 - MP3 доступен; 0 - MP3 не доступен
      str+="|MU:" + String(useAlarmSound ? "1" : "0");         // 1 - использовать звук; 0 - MP3 не использовать звук
      str+="|MD:" + String(alarmDuration); 
      str+="|MV:" + String(maxAlarmVolume); 
      if (soundFolder == 0) {      
        str+="|MA:" + String(alarmSound+2);                      // Знач: -1 - нет; 0 - случайно; 1 и далее - файлы; -> В списке индексы: 1 - нет; 2 - случайно; 3 и далее - файлы
        str+="|MB:" + String(dawnSound+2);                       // Знач: -1 - нет; 0 - случайно; 1 и далее - файлы; -> В списке индексы: 1 - нет; 2 - случайно; 3 и далее - файлы
      } else if (soundFolder == 1) {      
        str+="|MB:" + String(dawnSound+2);                       // Знач: -1 - нет; 0 - случайно; 1 и далее - файлы; -> В списке индексы: 1 - нет; 2 - случайно; 3 и далее - файлы
      } else if (soundFolder == 2) {      
        str+="|MA:" + String(alarmSound+2);                      // Знач: -1 - нет; 0 - случайно; 1 и далее - файлы; -> В списке индексы: 1 - нет; 2 - случайно; 3 и далее - файлы
      }
      str+="|MP:" + String(soundFolder) + '~' + String(soundFile+2); 
      str+=";";
      break;
    case 9:  // Настройки подключения
      str="$18 AU:"; 
      if (useSoftAP) str+="1|AN:["; else str+="0|AN:[";
      str+=String(apName) + "]|AA:[";
      str+=String(apPass) + "]|NW:[";
      str+=String(ssid) + "]|NA:[";
      str+=String(pass) + "]|IP:";
      if (wifi_connected) str += WiFi.localIP().toString(); 
      else                str += String(F("нет подключения"));
      str+=";";
      break;
    case 10:  // Настройки режимов автовключения по времени
      str="$18 AM1T:"+String(AM1_hour)+" "+String(AM1_minute)+"|AM1A:"+String(AM1_effect_id)+
             "|AM2T:"+String(AM2_hour)+" "+String(AM2_minute)+"|AM2A:"+String(AM2_effect_id); 
      str+=";";
      break;
    case 95:  // Ответ состояния будильника - сообщение по инициативе сервера
      str = "$18 AL:"; 
      if ((isAlarming || isPlayAlarmSound) && !isAlarmStopped) str+="1;"; else str+="0;";
      cmd95 = str;
      break;
    case 96:  // Ответ демо-режима звука - сообщение по инициативе сервера
      str ="$18 MP:" + String(soundFolder) + '~' + String(soundFile+2) + ";"; 
      cmd96 = str;
      break;
    case 97:  // Запрос списка эффектов для будильника
      str="$18 LA:[" + String(ALARM_LIST).substring(0,UDP_PACKET_MAX_SIZE-12) + "];"; 
      break;
    case 98:  // Запрос списка игр
      str="$18 LG:[" + String(GAME_LIST).substring(0,UDP_PACKET_MAX_SIZE-12) + "];"; 
      break;
    case 99:  // Запрос списка эффектов
      str="$18 LE:[" + String(EFFECT_LIST).substring(0,UDP_PACKET_MAX_SIZE-12) + "];"; 
      break;
  }
  
  if (str.length() > 0) {
    // Отправить клиенту запрошенные параметры страницы / режимов
    str.toCharArray(incomeBuffer, str.length()+1);    
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(incomeBuffer, str.length()+1);
    udp.endPacket();
    delay(0);
    Serial.println(String(F("Ответ на ")) + udp.remoteIP().toString() + ":" + String(udp.remotePort()) + " >> " + String(incomeBuffer));
  } else {
    sendAcknowledge();
  }
}

void sendAcknowledge() {
  // Отправить подтверждение, чтобы клиентский сокет прервал ожидание
  String reply = "";
  bool isCmd = false; 
  if (cmd95.length() > 0) { reply += cmd95; cmd95 = ""; isCmd = true;}
  if (cmd96.length() > 0) { reply += cmd96; cmd96 = ""; isCmd = true; }
  reply += "ack" + String(ackCounter++) + ";";  
  reply.toCharArray(replyBuffer, reply.length()+1);    
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write(replyBuffer, reply.length()+1);
  udp.endPacket();
  delay(0);
  if (isCmd) {
    Serial.println(String(F("Ответ на ")) + udp.remoteIP().toString() + ":" + String(udp.remotePort()) + " >> " + String(replyBuffer));
  }
}

void setSpecialMode(int spc_mode) {
        
  AUTOPLAY = false;
  BTcontrol = true;
  effectsFlag = true;
  gamemodeFlag = false;
  drawingFlag = false;
  runningFlag = false;
  loadingFlag = true;
  isNightClock = false;
  isTurnedOff = false;
  specialModeId = -1;

  String str;
  byte tmp_eff = -1;
  specialBrightness = globalBrightness;

  switch(spc_mode) {
    case 0:  // Черный экран (выкл);
      tmp_eff = EFFECT_FILL_COLOR;
      specialClock = false;
      setGlobalColor(0x000000);
      specialBrightness = 0;
      isTurnedOff = true;
      break;
    case 1:  // Черный экран с часами;  
      tmp_eff = EFFECT_FILL_COLOR;
      specialClock = true;
      setGlobalColor(0x000000);
      break;
    case 2:  // Белый экран (освещение);
      tmp_eff = EFFECT_FILL_COLOR;
      specialClock = false;
      setGlobalColor(0xffffff);
      break;
    case 3:  // Белый экран с часами;
      tmp_eff = EFFECT_FILL_COLOR;
      specialClock = true;
      setGlobalColor(0xffffff);
      break;
    case 4:  // Черный экран с часами минимальной яркости - ночной режим;
      tmp_eff = EFFECT_FILL_COLOR;
      specialClock = true;
      setGlobalColor(0x000000);
      COLOR_MODE = 0; // Монохром, т.к все что не белоена миним. яркости черное, белое - красным.
      specialBrightness = 1;
      isNightClock = true;
      break;
    case 5:  // Черный экран с эффектом огня и часами (камин);
      tmp_eff = EFFECT_FIRE;
      specialClock = true;
      break;
    case 6:  // Экран указанного цвета;
      tmp_eff = EFFECT_FILL_COLOR;
      str = String(incomeBuffer).substring(6,12); // $14 6 00FFAA;
      specialClock = false;
      setGlobalColor((uint32_t)HEXtoInt(str));
      break;
    case 7:  // Экран указанного цвета с часами;
      tmp_eff = EFFECT_FILL_COLOR;
      str = String(incomeBuffer).substring(6,12); // $14 6 00FFAA;
      specialClock = true;
      setGlobalColor((uint32_t)HEXtoInt(str));
      break;
  }

  if (tmp_eff >=0) {    
    // Найти соответствие thisMode указанному эффекту. 
    // Дльнейшее отображение изображения эффекта будет выполняться стандартной процедурой customRoutin()
    byte b_tmp = mapEffectToMode(tmp_eff);
    if (b_tmp != 255) {
      effect = (byte)tmp_eff;
      thisMode = b_tmp;
      specialMode = true;
      effectSpeed = getEffectSpeed(effect);
      effectTimer.setInterval(effectSpeed);
      // Таймер возврата в авторежим отключен    
      idleTimer.setInterval(4294967295);
      idleTimer.reset();
      FastLED.setBrightness(specialBrightness);
      specialModeId = spc_mode;
    }
  }  
  
  setCurrentSpecMode(spc_mode);
}

void resetModes() {
  // Отключение спец-режима перед включением других
  specialMode = false;
  isNightClock = false;
  isTurnedOff = false;
  specialModeId = -1;

  // Отключение ВСЕХ режимов: рисование/текст/эффекты/игры  
  effectsFlag = false;        
  runningFlag = false;
  gamemodeFlag = false;
  drawingFlag = false;
  loadingFlag = false;
  controlFlag = false;

  breathBrightness = globalBrightness;
}

void setEffect(byte eff) {

  effect = eff;
  
  // Эффект динамического цвета не отключает текущий режим
  if (!isColorEffect(effect)) resetModes();

  loadingFlag = !isColorEffect(effect);
  effectsFlag = true;
    
  effectSpeed = getEffectSpeed(effect);
  effectTimer.setInterval(effectSpeed);

  // Найти соответствие thisMode указанному эффекту. 
  if (!isColorEffect(effect)) {            
    byte b_tmp = mapEffectToMode(effect);           
    if (b_tmp != 255) {
      thisMode = b_tmp;
      if (!useAutoBrightness) {
        FastLED.setBrightness(globalBrightness);      
      }  
    }
  }
  setCurrentSpecMode(-1);
}

void startGame(byte game, bool newGame, bool paused) {
  
  // Найти соответствие thisMode указанной игре. 
  byte b_tmp = mapGameToMode(game);
  
  if (b_tmp != 255) {
    resetModes();
      
    gamemodeFlag = true;
    gamePaused = paused;  
  
    gameSpeed = getGameSpeed(game);
    gameTimer.setInterval(gameSpeed);        
    
    thisMode = b_tmp;
    if (!useAutoBrightness) {
      FastLED.setBrightness(globalBrightness);      
    }
  
    if (newGame) {  
      loadingFlag = true;                                                                  
      FastLED.clear(); 
      FastLED.show(); 
    }    
  }  
  setCurrentSpecMode(-1);
}

void startRunningText() {
  runningFlag = true;
  if (!isColorEffect(effect)) {
    effectsFlag = false;
  }
  // Если при включении режима "Бегущая строка" цвет текста - черный -- включить белый, т.к черный на черном не видно
  if (globalColor == 0x000000) setGlobalColor(0xffffff);
  if (!useAutoBrightness)
    FastLED.setBrightness(globalBrightness);    
  setCurrentSpecMode(-1);
}

void showCurrentIP() {
  resetModes();          
  runningFlag = true;  
  BTcontrol = false;
  AUTOPLAY = true;
  wifi_print_ip = true;
  wifi_current_ip = wifi_connected ? WiFi.localIP().toString() : String(F("Нет подключения к сети WiFi"));
}
