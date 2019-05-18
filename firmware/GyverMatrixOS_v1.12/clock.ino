// режим часов

// ****************** НАСТРОЙКИ ЧАСОВ *****************
#define MIN_COLOR CRGB::White          // цвет минут
#define HOUR_COLOR CRGB::White         // цвет часов
#define DOT_COLOR CRGB::White          // цвет точек

#define NORMAL_CLOCK_COLOR CRGB::White // Нормальный цвет монохромных цветов

#define CONTRAST_COLOR_1 CRGB::Orange  // контрастный цвет часов
#define CONTRAST_COLOR_2 CRGB::Green   // контрастный цвет часов
#define CONTRAST_COLOR_3 CRGB::Black   // контрастный цвет часов

#define HUE_STEP 5          // шаг цвета часов в режиме радужной смены
#define HUE_GAP 30          // шаг цвета между цифрами в режиме радужной смены

// ****************** ДЛЯ РАЗРАБОТЧИКОВ ****************
byte clockHue;

#if (OVERLAY_CLOCK == 1)
CRGB overlayLEDs[165];                // По максимому календарь - шрифт 3x5 - 4 цифры в два ряда, по одному пробелу между цифрами и рядами - 15x11
                                      // По максимуму часы шрифт 3x5 гориз - 4 цифры по одному пробелу между цифрами - 15x5, 
                                      //                             верт - два ряда по две цифры по одному пробелу между цифрами и рядами - 7x11, 
byte listSize = sizeof(overlayList);
#endif

bool overlayEnabled = getClockOverlayEnabled();
CRGB clockLED[5] = {HOUR_COLOR, HOUR_COLOR, DOT_COLOR, MIN_COLOR, MIN_COLOR};

// send an NTP request to the time server at the given address
 unsigned long sendNTPpacket(IPAddress& address) {
  Serial.print(F("Отправка NTP пакета на сервер "));
  Serial.println(ntpServerName);
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void parseNTP() {
    Serial.println(F("Разбор пакета NTP"));
    ntp_t = 0; ntp_cnt = 0; init_time = true; refresh_time = false;
    //udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord = word(incomeBuffer[40], incomeBuffer[41]);
    unsigned long lowWord = word(incomeBuffer[42], incomeBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    unsigned long seventyYears = 2208988800UL ;
    time_t t = secsSince1900 - seventyYears + (timeZoneOffset) * 3600;
    Serial.print(F("Секунд с 1970: "));
    Serial.println(t);
    setTime(t);
    calculateDawnTime();
}

void getNTP() {
  if (!wifi_connected) return;

  WiFi.hostByName(ntpServerName, timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  ntp_t = millis();
}

boolean overlayAllowed() {
#if (OVERLAY_CLOCK == 1)  

  // Оверлей не разрешен, если часы еще не инициализированы
  if (!init_time) return false;
  
  // Оверлей разрешен текущими параметрами спец.режима?
  if (specialMode) return specialClock;
  
  // Оверлей разрешен общими настройками часов? 
  bool allowed = getClockOverlayEnabled();
  // Оверлей разрешен настройками списка разрешенных для оверлея эффектов?
  if (allowed) {
    for (byte i = 0; i < listSize; i++) {
      allowed = (modeCode == overlayList[i]);
      if (allowed) break;
    }
  }

  // Оверлей разрешен настройками параметров эффекта? (в эффектах Дыхание, Цвета, Радуга пикс. и Часы оверлей часов не разрешен)
  if (allowed && BTcontrol && effectsFlag && !(isColorEffect(effect) || effect == MC_CLOCK)) {
    allowed = getEffectClock(effect);   
  }
  
  // Если в режиме авто - найти соответствие номера отображаемого режима номеру эффекта и проверить - разрешен ли для него оверлей часов
  if (allowed && !BTcontrol) {
    byte tmp_effect = mapModeToEffect(thisMode);
    if (tmp_effect <= MAX_EFFECT) 
      allowed = getEffectClock(tmp_effect);   
  }

  return allowed;
#else
  return false;
#endif
}

String clockCurrentText() {
  
  hrs = hour();
  mins = minute();

  String sHrs = "0" + String(hrs);  
  String sMin = "0" + String(mins);
  if (sHrs.length() > 2) sHrs = sHrs.substring(1);
  if (sMin.length() > 2) sMin = sMin.substring(1);
  return sHrs + ":" + sMin;
}

String dateCurrentTextShort() {
  
  aday = day();
  amnth = month();
  ayear = year();

  String sDay = "0" + String(aday);  
  String sMnth = "0" + String(amnth);
  String sYear = String(ayear);
  if (sDay.length() > 2) sDay = sDay.substring(1);
  if (sMnth.length() > 2) sMnth = sMnth.substring(1);
  return sDay + "." + sMnth + "." + sYear;
}

String dateCurrentTextLong() {
  
  aday = day();
  amnth = month();
  ayear = year();

  String sDay = "0" + String(aday);  
  String sMnth = "";
  String sYear = String(ayear);
  switch (amnth) {
    case  1: sMnth = F(" января ");   break;
    case  2: sMnth = F(" февраля ");  break;
    case  3: sMnth = F(" марта ");    break;
    case  4: sMnth = F(" апреля ");   break;
    case  5: sMnth = F(" мая ");      break;
    case  6: sMnth = F(" июня ");     break;
    case  7: sMnth = F(" июля ");     break;
    case  8: sMnth = F(" августа ");  break;
    case  9: sMnth = F(" сентября "); break;
    case 10: sMnth = F(" октября ");  break;
    case 11: sMnth = F(" ноября ");   break;
    case 12: sMnth = F(" декабря ");  break;
  }  
  if (sDay.length() > 2) sDay = sDay.substring(1);
  return sDay + sMnth + sYear + " года";
}

void clockColor() {
  if (COLOR_MODE == 0) {     
    // Монохромные часы  
    for (byte i = 0; i < 5; i++) clockLED[i] = NORMAL_CLOCK_COLOR;
  } else if (COLOR_MODE == 1) {
    // Каждая цифра своим цветом, плавная смкна цвета
    for (byte i = 0; i < 5; i++) clockLED[i] = CHSV(clockHue + HUE_GAP * i, 255, 255);
    clockLED[2] = CHSV(clockHue + 128 + HUE_GAP * 1, 255, 255); // точки делаем другой цвет
  } else if (COLOR_MODE == 2) {
    // Часы, точки, минуты своим цветом, плавная смкна цвета
    clockLED[0] = CHSV(clockHue + HUE_GAP * 0, 255, 255);
    clockLED[1] = CHSV(clockHue + HUE_GAP * 0, 255, 255);
    clockLED[2] = CHSV(clockHue + 128 + HUE_GAP * 1, 255, 255); // точки делаем другой цвет
    clockLED[3] = CHSV(clockHue + HUE_GAP * 2, 255, 255);
    clockLED[4] = CHSV(clockHue + HUE_GAP * 2, 255, 255);
  } else if (COLOR_MODE == 3) {// для календаря -  
    // Часы, точки, минуты своим заданным цветом
    clockLED[0] = HOUR_COLOR;  // число
    clockLED[1] = HOUR_COLOR;  // месяц
    clockLED[2] = DOT_COLOR;   // разделитель числа/месяца
    clockLED[3] = MIN_COLOR;   // год 1-е две цифры
    clockLED[4] = MIN_COLOR;   // год 2-е две цифры
  }
}

// нарисовать часы
void drawClock(byte hrs, byte mins, boolean dots, byte X, byte Y) {
  byte h10 = hrs / 10;
  byte h01 = hrs % 10;
  byte m10 = mins / 10;
  byte m01 = mins % 10;
  
  if (CLOCK_ORIENT == 0) {
    if (h10 == 1 && m01 != 1) X--;
    // 0 в часах невыводим, для центрирования сдвигаем остальные цифры влево на место нуля
    if (h10 > 0) 
      drawDigit3x5(h10, X + (h10 == 1 ? 1 : 0), Y, clockLED[0]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
    else 
      X -= 2;
    drawDigit3x5(h01, X + 4, Y, clockLED[1]);
    if (dots) {
      drawPixelXY(X + 7, Y + 1, clockLED[2]);
      drawPixelXY(X + 7, Y + 3, clockLED[2]);
    } else {
      if (modeCode == MC_CLOCK) {
        drawPixelXY(X + 7, Y + 1, 0);
        drawPixelXY(X + 7, Y + 3, 0);
      }
    }
    drawDigit3x5(m10, X + 8, Y, clockLED[3]);
    drawDigit3x5(m01, X + 12 + (m01 == 1 ? -1 : 0) + (m10 == 1 && m01 != 1 ? -1 : 0) , Y, clockLED[4]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать влево на 1 колонку
  } else { // Вертикальные часы
    //if (hrs > 9) // Так реально красивей
    drawDigit3x5(h10, X, Y + 6, clockLED[0]);
    drawDigit3x5(h01, X + 4, Y + 6, clockLED[1]);
    if (dots) { // Мигающие точки легко ассоциируются с часами
      drawPixelXY(X + 1, Y + 5, clockLED[2]);
      drawPixelXY(X + 5, Y + 5, clockLED[2]);
    } else {
      if (modeCode == MC_CLOCK) {
        drawPixelXY(X + 1, Y + 5, 0);
        drawPixelXY(X + 5, Y + 5, 0);
      }
    }
    drawDigit3x5(m10, X, Y, clockLED[3]);
    drawDigit3x5(m01, X + 4, Y, clockLED[4]);
  }
}

// нарисовать часы
void drawCalendar(byte aday, byte amnth, int16_t ayear, boolean dots, byte X, byte Y) {

  // Число месяца
  drawDigit3x5(aday / 10, X, Y + 6, clockLED[0]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
  drawDigit3x5(aday % 10, X + 4, Y + 6, clockLED[0]);

  // разделитель числа/месяца
  if (dots) {
    drawPixelXY(X + 7, Y + 5, clockLED[2]);
  } else {
    if (modeCode == MC_CLOCK) {
      drawPixelXY(X + 7, Y + 5, 0);
    }
  }
  
  // Месяц
  drawDigit3x5(amnth / 10, X + 8, Y + 6, clockLED[1]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
  drawDigit3x5(amnth % 10, X + 12, Y + 6, clockLED[1]);

  // Год  
  drawDigit3x5(ayear / 1000, X, Y, clockLED[3]);
  drawDigit3x5((ayear / 100) % 10, X + 4, Y, clockLED[3]);
  drawDigit3x5((ayear / 10) % 10, X + 8, Y, clockLED[4]);
  drawDigit3x5(ayear % 10, X + 12, Y, clockLED[4]);
}

void clockRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    modeCode = MC_CLOCK;
  }

  FastLED.clear();
  clockTicker();

  if (!showDateInClock) {
    // Отображать только часы
    drawClock(hrs, mins, dotFlag, CLOCK_X, CLOCK_Y);
  } else {
    // Отображать попеременно часы и календарь
    if (showDateState)
      drawCalendar(aday, amnth, ayear, dotFlag, CALENDAR_X, CALENDAR_Y);
    else  
      drawClock(hrs, mins, dotFlag, CLOCK_X, CLOCK_Y);
    
    if (millis() - showDateStateLastChange > (showDateState ? showDateDuration : showDateInterval) * 1000L) {
      showDateStateLastChange = millis();
      showDateState = !showDateState;
    }
  }
}

void calendarRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    modeCode = MC_CLOCK;
  }

  FastLED.clear();
  clockTicker();
  drawCalendar(aday, amnth, ayear, dotFlag, CALENDAR_X, CALENDAR_Y);
}

void clockTicker() {
  hrs = hour();
  mins = minute();
  aday = day();
  amnth = month();
  ayear = year();
  
  if (halfsecTimer.isReady()) {
    
    clockHue += HUE_STEP;

    #if (OVERLAY_CLOCK == 1)
      setOverlayColors();
    #endif

    dotFlag = !dotFlag;
  }
}

#if (OVERLAY_CLOCK == 1)
void clockOverlayWrapH(byte posX, byte posY) {
  byte thisLED = 0;
  for (byte i = posX; i < posX + 15; i++) {
    for (byte j = posY; j < posY + 5; j++) {
      overlayLEDs[thisLED] = leds[getPixelNumber(i, j)];
      thisLED++;
    }
  }
  clockTicker();
  drawClock(hrs, mins, dotFlag, posX, posY);
}

void clockOverlayUnwrapH(byte posX, byte posY) {
  byte thisLED = 0;
  for (byte i = posX; i < posX + 15; i++) {
    for (byte j = posY; j < posY + 5; j++) {
      leds[getPixelNumber(i, j)] = overlayLEDs[thisLED];
      thisLED++;
    }
  }
}

void clockOverlayWrapV(byte posX, byte posY) {
  byte thisLED = 0;
  for (byte i = posX; i < posX + 7; i++) {
    for (byte j = posY; j < posY + 11; j++) {
      overlayLEDs[thisLED] = leds[getPixelNumber(i, j)];
      thisLED++;
    }
  }
  clockTicker();
  drawClock(hrs, mins, dotFlag, posX, posY);
}

void clockOverlayUnwrapV(byte posX, byte posY) {
  byte thisLED = 0;
  for (byte i = posX; i < posX + 7; i++) {
    for (byte j = posY; j < posY + 11; j++) {
      leds[getPixelNumber(i, j)] = overlayLEDs[thisLED];
      thisLED++;
    }
  }
}

void calendarOverlayWrap(byte posX, byte posY) {
  byte thisLED = 0;
  for (byte i = posX; i < posX + 15; i++) {
    for (byte j = posY; j < posY + 11; j++) {
      overlayLEDs[thisLED] = leds[getPixelNumber(i, j)];
      thisLED++;
    }
  }
  clockTicker();
  drawCalendar(aday, amnth, ayear, dotFlag, CALENDAR_X, CALENDAR_Y);
}

void calendarOverlayUnwrap(byte posX, byte posY) {
  byte thisLED = 0;
  for (byte i = posX; i < posX + 15; i++) {
    for (byte j = posY; j < posY + 11; j++) {
      leds[getPixelNumber(i, j)] = overlayLEDs[thisLED];
      thisLED++;
    }
  }
}

boolean needUnwrap() {
  if (modeCode == MC_SNOW ||
      modeCode == MC_SPARKLES ||
      modeCode == MC_MATRIX ||
      modeCode == MC_STARFALL ||
      modeCode == MC_BALLS ||
      modeCode == MC_FIRE) return true;
  else return false;
}
#endif

void contrastClock() {  
  for (byte i = 0; i < 5; i++) clockLED[i] = NORMAL_CLOCK_COLOR;
}

void contrastClockA() {  
  for (byte i = 0; i < 5; i++) clockLED[i] = CONTRAST_COLOR_1;
}

void contrastClockB() {
  for (byte i = 0; i < 5; i++) clockLED[i] = CONTRAST_COLOR_2;
}

void contrastClockC(){
  if (specialMode && specialClock){
    CRGB color = -CRGB(globalColor);
    for (byte i = 0; i < 5; i++) clockLED[i] = color;  
  } else {  
    for (byte i = 0; i < 5; i++) clockLED[i] = CONTRAST_COLOR_3;  
  }
}

void setOverlayColors() {
  if (COLOR_MODE == 0) {
    switch (modeCode) {
      case MC_CLOCK: 
      case MC_GAME: 
      case MC_SPARKLES:
      case MC_MATRIX:
      case MC_STARFALL:
      case MC_FIRE: 
      case MC_BALL:
      case MC_BALLS: 
      case MC_NOISE_RAINBOW:
      case MC_NOISE_RAINBOW_STRIP: 
      case MC_RAINBOW:
      case MC_RAINBOW_DIAG: 
        contrastClock();
        break;
      case MC_SNOW:
      case MC_NOISE_ZEBRA: 
      case MC_NOISE_MADNESS:
      case MC_NOISE_CLOUD:
      case MC_NOISE_FOREST:
      case MC_NOISE_OCEAN: 
        contrastClockA();
        break;
      case MC_NOISE_PLASMA:
      case MC_NOISE_LAVA:
        contrastClockB();
        break;
      case MC_DAWN_ALARM:
      case MC_FILL_COLOR:
        contrastClockC();
        break;
    }
  }
  else
    clockColor();
}

// расчет времени начала рассвета
void calculateDawnTime() {
  
  if (!init_time) return;
  
  // расчёт времени рассвета
  if (alarmMinute > dawnDuration) {                 // если минут во времени будильника больше продолжительности рассвета
    dawnHour = alarmHour;                            // час рассвета равен часу будильника
    dawnMinute = alarmMinute - dawnDuration;        // минуты рассвета = минуты будильника - продолж. рассвета
  } else {                                           // если минут во времени будильника меньше продолжительности рассвета
    dawnHour = alarmHour - 1;                        // значит рассвет будет часом раньше
    if (dawnHour < 0) dawnHour = 23;                 // защита от совсем поехавших
    dawnMinute = 60 - (dawnDuration - alarmMinute); // находим минуту рассвета в новом часе
  }
  
  dawnWeekDay = weekday()-1;                         // day of the week, Sunday is day 0 
  if (dawnWeekDay == 0) dawnWeekDay = 7;             // Sunday is day 7, Monday is day 1;
  if (dawnHour * 60L + dawnMinute <= hour() * 60 + minute()) dawnWeekDay++;             // Если час рассвета меньше текущего - это "завтра"
  if (dawnWeekDay == 8) dawnWeekDay = 1;             // Если переход через вс (7) - это пн (1)

  Serial.print(String(F("Следующий рассвет в "))+String(dawnHour)+ F(":") + String(dawnMinute));
  switch(dawnWeekDay) {
    case 1: Serial.println(F(", понедельник")); break;
    case 2: Serial.println(F(", вторник")); break;
    case 3: Serial.println(F(", среда")); break;
    case 4: Serial.println(F(", четверг")); break;
    case 5: Serial.println(F(", пятница")); break;
    case 6: Serial.println(F(", суббота")); break;
    case 7: Serial.println(F(", воскресенье")); break;
  }  
}

// Проверка времени срабатывания будильника
void checkAlarmTime() {
  
  // Будильник включен?
  if (init_time && alarmOnOff) {

    int h = hour();
    int m = minute();

    // Включен ли будильник для текущего дня недели?       
    if ((alarmWeekDay & (1 << (dawnWeekDay - 1))) > 0) {
       // Часы / минуты начала рассвета наступили? Еще не запущен рассвет? Еще не остановлен пользователем?
       if (!isAlarming && !isAlarmStopped && ((h * 60L + m) >= (dawnHour * 60L + dawnMinute)) && ((h * 60L + m) < (alarmHour * 60L + alarmMinute))) {
         specialMode = false;
         specialModeId = -1;
         isAlarming = true;
         isAlarmStopped = false;
         loadingFlag = true;
         thisMode = DEMO_DAWN_ALARM;
         realDawnDuration = (alarmHour * 60L + alarmMinute) - (dawnHour * 60L + dawnMinute);
         if (realDawnDuration > dawnDuration) realDawnDuration = dawnDuration;
         idleTimer.setInterval(4294967295);
         if (useAlarmSound) PlayDawnSound();
         sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника
         Serial.println(String(F("Рассвет ВКЛ в "))+String(h)+ ":" + String(m));
       }
    }
    
    delay(0); // Для предотвращения ESP8266 Watchdog Timer
    
    // При наступлении времени срабатывания будильника, если он еще не выключен пользователем - запустить режим часов и звук будильника
    if (alarmHour == h && alarmMinute == m && isAlarming) {
      Serial.println(String(F("Рассвет Авто-ВЫКЛ в "))+String(h)+ ":" + String(m));
      isAlarming = false;
      isAlarmStopped = false;
      setSpecialMode(1);
      if (useAlarmSound) {
        PlayAlarmSound();
      }
      sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника
    }

    delay(0); // Для предотвращения ESP8266 Watchdog Timer

    // Если рассвет начинался и остановлен пользователем и время начала рассвета уже прошло - сбросить флаги, подготовив их к следующему циклу
    if (isAlarmStopped && ((h * 60L + m) > (alarmHour * 60L + alarmMinute))) {
      isAlarming = false;
      isAlarmStopped = false;
    }
  }
  
  // Подошло время отключения будильника - выключить
  if (alarmSoundTimer.isReady()) {
    alarmSoundTimer.setInterval(4294967295);
    StopSound(2500);      
    sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника
  }

  delay(0); // Для предотвращения ESP8266 Watchdog Timer

  //Плавное изменение громкости будильника
  if (fadeSoundTimer.isReady()) {
    if (fadeSoundDirection > 0) {
      // увеличение громкости
      dfPlayer.volumeUp();
      fadeSoundStepCounter--;
      if (fadeSoundStepCounter <= 0) {
        fadeSoundDirection = 0;
        fadeSoundTimer.setInterval(4294967295);
      }
    } else if (fadeSoundDirection < 0) {
      // Уменьшение громкости
      dfPlayer.volumeDown();
      fadeSoundStepCounter--;
      if (fadeSoundStepCounter <= 0) {
        isPlayAlarmSound = false;
        fadeSoundDirection = 0;
        fadeSoundTimer.setInterval(4294967295);
        StopSound(0);
      }
    }
  }
  
  delay(0); // Для предотвращения ESP8266 Watchdog Timer    
}

void stopAlarm() {
  if ((isAlarming || isPlayAlarmSound) && !isAlarmStopped) {
    Serial.println(String(F("Рассвет ВЫКЛ в ")) + String(hour())+ ":" + String(minute()));
    isAlarming = false;
    isAlarmStopped = true;
    isPlayAlarmSound = false;
    cmd95 = "";
    alarmSoundTimer.setInterval(4294967295);
    StopSound(2500);
    setSpecialMode(1);
    delay(0);    
    sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника
  }
}

void checkAutoMode1Time() {
  if (AM1_effect_id == -2 || !init_time) return;
  
  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало врема активации режима - активировать
  if (!AM1_running && AM1_hour == hrs && AM1_minute == mins) {
    AM1_running = true;
    SetAutoMode(1);
  }

  // Режим активирован и время срабатывания режима прошло - сбросить флаг для подготовки к следующему циклу
  if (AM1_running && (AM1_hour != hrs || AM1_minute != mins)) {
    AM1_running = false;
  }
}

void checkAutoMode2Time() {
  if (AM2_effect_id == -2 || !init_time) return;

  hrs = hour();
  mins = minute();

  // Режим по времени включен (enable) и настало врема активации режима - активировать
  if (!AM2_running && AM2_hour == hrs && AM2_minute == mins) {
    AM2_running = true;
    SetAutoMode(2);
  }

  // Режим активирован и время срабатывания режима прошло - сбросить флаг для подготовки к следующему циклу
  if (AM2_running && (AM2_hour != hrs || AM2_minute != mins)) {
    AM2_running = false;
  }
}

void SetAutoMode(byte amode) {
  Serial.print(F("Авторежим "));
  Serial.print(amode);
  Serial.print(F(" ["));
  Serial.print(amode == 1 ? AM1_hour : AM2_hour);
  Serial.print(":");
  Serial.print(amode == 1 ? AM1_minute : AM2_minute);
  Serial.print(F("] - "));

  byte ef = amode == 1 ? AM1_effect_id : AM2_effect_id;
  
  // включить указанный режим
  if (ef == -1) {

    // Выключить матрицу (черный экран)
    Serial.print(F("выключение матрицы"));
    setSpecialMode(0);
    
  } else {
    Serial.print(F("включение матрицы"));

    // Если режим включения == 0 - случайный режим и автосмена по кругу
    AUTOPLAY = ef == 0;
    if (!AUTOPLAY) {
      // Таймер возврата в авторежим отключен    
      idleTimer.setInterval(4294967295);
      idleTimer.reset();
    }
      
    if (ef == 0) {
      // "Случайный режим и далее автосмена
      nextModeHandler();
    } else {  
      // Для применения доступен список эффектов из ALARM_LIST; 
      // Эффекты в списке ALARM_LIST начинаются c 0; Значение ef может быть -2 (выключено); -1 - выключать матрицу; 0 - случайный и длее с 1 - указанный
      // Ножно привести номер указанного эффекта ef к индексу в ALARM_LIST, и дальше к номеру используемого эффекта, поэтому mapAlarmToEffect(ef - 1);
      byte tmp = mapAlarmToEffect(ef - 1);   
      // Если не опознали что за эффект - включаем "следующий" режим
      if (tmp != 255) setEffect(ef);
      else            nextModeHandler(); 
    }
  }
  
  Serial.println();
}
