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
bool saveSpecialMode;
bool saveRunningFlag;
int8_t saveSpecialModeId;
byte saveThisMode;

byte clockHue;

#if (OVERLAY_CLOCK == 1)
CRGB overlayLEDs[165];                // По максимуму календарь - шрифт 3x5 - 4 цифры в два ряда, по одному пробелу между цифрами и рядами - 15x11
                                      // По максимуму часы шрифт 3x5 гориз - 4 цифры по одному пробелу между цифрами - 15x5, 
                                      //                             верт - два ряда по две цифры по одному пробелу между цифрами и рядами - 7x11, 
byte listSize = sizeof(overlayList);
#endif

CRGB clockLED[5] = {HOUR_COLOR, HOUR_COLOR, DOT_COLOR, MIN_COLOR, MIN_COLOR};

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
  Serial.print(F("Отправка NTP пакета на сервер "));
  Serial.println(ntpServerName);
  // set all bytes in the buffer to 0
  // memset(packetBuffer, 0, NTP_PACKET_SIZE);
  for (byte i=0; i<NTP_PACKET_SIZE; i++) packetBuffer[i] = 0;
  
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
  udp.beginPacket(address, 123); // NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void parseNTP() {
    Serial.println(F("Разбор пакета NTP"));
    ntp_t = 0; ntp_cnt = 0; init_time = true; refresh_time = false;
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
  IPAddress ip;
  ip.fromString(F("0.0.0.0"));
#if defined(ESP8266)
  if (!timeServerIP.isSet()) timeServerIP.fromString(F("192.36.143.130"));  // Один из ru.pool.ntp.org // 195.3.254.2
#endif
#if defined(ESP32)
  if (timeServerIP==ip) timeServerIP.fromString(F("192.36.143.130"));  // Один из ru.pool.ntp.org // 195.3.254.2
#endif
  printNtpServerName();
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  ntp_t = millis();  
}

boolean overlayAllowed() {
#if (OVERLAY_CLOCK == 1)  

  // Оверлей не разрешен, если часы еще не инициализированы
  if (!init_time) return false;

  // В играх оверлея часов нет;
  if (gamemodeFlag) return false;

  // Часы влазят на матрицу?
  if (!(allowHorizontal || allowVertical)) return false;
  
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

  // Оверлей разрешен настройками параметров эффекта? (в эффекте "Часы" оверлей часов не разрешен)
  if (allowed && BTcontrol && effectsFlag && effect != MC_CLOCK) {
    allowed = getEffectClock(effect);   
  }
    
  // Если в режиме авто - найти соответствие номера отображаемого режима номеру эффекта и проверить - разрешен ли для него оверлей часов
  if (allowed && !BTcontrol) {
    byte tmp_effect = mapModeToEffect(thisMode);
    if (tmp_effect <= MAX_EFFECT) {
      allowed = getEffectClock(tmp_effect);   
    } else {
      allowed = false;  
    }
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
    if (h10 == 1 && m01 != 1 && X > 0) X--;
    // 0 в часах не выводим, для центрирования сдвигаем остальные цифры влево на место нуля
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
      drawPixelXY(X + 3, Y + 5, clockLED[2]);
    } else {
      if (modeCode == MC_CLOCK) {
        drawPixelXY(X + 3, Y + 5, 0);
      }
    }
    drawDigit3x5(m10, X, Y, clockLED[3]);
    drawDigit3x5(m01, X + 4, Y, clockLED[4]);
  }
}

// нарисовать дату календаря
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
  if (init_time)
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
  if (init_time)
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
  if (init_time)
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
      modeCode == MC_FIRE ||
      modeCode == MC_PAINTBALL ||
      modeCode == MC_SWIRL) return true;
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
    CRGB color = CONTRAST_COLOR_3;
    CRGB gc = CRGB(globalColor);
    if (color == gc) color = -color;
    for (byte i = 0; i < 5; i++) clockLED[i] = color;  
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
      case MC_NOISE_PLASMA:
      case MC_LIGHTERS:
      case MC_PAINTBALL:
      case MC_SWIRL:
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

  byte alrmHour;
  byte alrmMinute;
  
  dawnWeekDay = 0;
  if (!init_time || alarmWeekDay == 0) return;       // Время инициализировано? Хотя бы на один день будильник включен?

  int8_t alrmWeekDay = weekday()-1;                  // day of the week, Sunday is day 0   
  if (alrmWeekDay == 0) alrmWeekDay = 7;             // Sunday is day 7, Monday is day 1;

  byte h = hour();
  byte m = minute();
  byte w = weekday()-1;
  if (w == 0) w = 7;

  byte cnt = 0;
  while (cnt < 2) {
    cnt++;
    while ((alarmWeekDay & (1 << (alrmWeekDay - 1))) == 0) {
      alrmWeekDay++;
      if (alrmWeekDay == 8) alrmWeekDay = 1;
    }
      
    alrmHour = alarmHour[alrmWeekDay-1];
    alrmMinute = alarmMinute[alrmWeekDay-1];
  
    // "Сегодня" время будильника уже прошло? 
    if (alrmWeekDay == w && (h * 60L + m > alrmHour * 60L + alrmMinute)) {
      alrmWeekDay++;
      if (alrmWeekDay == 8) alrmWeekDay = cnt == 1 ? 1 : 7;
    }
  }

  // Serial.printf("Alarm: h:%d m:%d wd:%d\n", alrmHour, alrmMinute, alrmWeekDay);
  
  // расчёт времени рассвета
  if (alrmMinute > dawnDuration) {                  // если минут во времени будильника больше продолжительности рассвета
    dawnHour = alrmHour;                            // час рассвета равен часу будильника
    dawnMinute = alrmMinute - dawnDuration;         // минуты рассвета = минуты будильника - продолж. рассвета
    dawnWeekDay = alrmWeekDay;
  } else {                                          // если минут во времени будильника меньше продолжительности рассвета
    dawnWeekDay = alrmWeekDay;
    dawnHour = alrmHour - 1;                        // значит рассвет будет часом раньше
    if (dawnHour < 0) {
      dawnHour = 23;     
      dawnWeekDay--;
      if (dawnWeekDay == 0) dawnWeekDay = 7;                           
    }
    dawnMinute = 60 - (dawnDuration - alrmMinute);  // находим минуту рассвета в новом часе
    if (dawnMinute == 60) {
      dawnMinute=0; dawnHour++;
      if (dawnHour == 24) {
        dawnHour=0; dawnWeekDay++;
        if (dawnWeekDay == 8) dawnWeekDay = 1;
      }
    }
  }

  // Serial.printf("Dawn: h:%d m:%d wd:%d\n", dawnHour, dawnMinute, dawnWeekDay);

  Serial.print(String(F("Следующий рассвет в "))+String(dawnHour)+ F(":") + String(dawnMinute));
  switch(dawnWeekDay) {
    case 1: Serial.println(F(", понедельник")); break;
    case 2: Serial.println(F(", вторник")); break;
    case 3: Serial.println(F(", среда")); break;
    case 4: Serial.println(F(", четверг")); break;
    case 5: Serial.println(F(", пятница")); break;
    case 6: Serial.println(F(", суббота")); break;
    case 7: Serial.println(F(", воскресенье")); break;
    default: Serial.println(); break;
  }  
}

// Проверка времени срабатывания будильника
void checkAlarmTime() {
  
  // Будильник включен?
  if (init_time && dawnWeekDay > 0) {

    byte h = hour();
    byte m = minute();
    byte w = weekday()-1;
    if (w == 0) w = 7;

    // Время срабатывания будильника после завершения рассвета
    byte alrmWeekDay = dawnWeekDay;
    byte alrmHour = dawnHour;
    byte alrmMinute = dawnMinute + dawnDuration;
    if (alrmMinute >= 60) {
      alrmMinute = 60 - alrmMinute;
      alrmHour++;
    }
    if (alrmHour > 23) {
      alrmHour = 24 - alrmHour;
      alrmWeekDay++;
    }
    if (alrmWeekDay > 7) alrmWeekDay = 1;

    // Текущий день недели совпадает с вычисленным днём недели рассвета?
    if (w == dawnWeekDay) {
       // Часы / минуты начала рассвета наступили? Еще не запущен рассвет? Еще не остановлен пользователем?
       if (!isAlarming && !isAlarmStopped && ((w * 1000L + h * 60L + m) >= (dawnWeekDay * 1000L + dawnHour * 60L + dawnMinute)) && ((w * 1000L + h * 60L + m) < (alrmWeekDay * 1000L + alrmHour * 60L + alrmMinute))) {
         // Сохранить параметры текущего режима для восстановления после завершения работы будильника
         saveSpecialMode = specialMode;
         saveSpecialModeId = specialModeId;
         saveThisMode = thisMode;
         saveRunningFlag = runningFlag;
         // Включить будильник
         isAlarming = true; 
         isAlarmStopped = false;
         setEffect(EFFECT_DAWN_ALARM);
         // Реальная продолжительность рассвета
         realDawnDuration = (alrmHour * 60L + alrmMinute) - (dawnHour * 60L + dawnMinute);
         if (realDawnDuration > dawnDuration) realDawnDuration = dawnDuration;
         // Отключмить таймер автоперехода в демо-режим
         idleTimer.setInterval(4294967295);
         #if (USE_MP3 ==1)
         if (useAlarmSound) PlayDawnSound();
         #endif
         sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника
         Serial.println(String(F("Рассвет ВКЛ в "))+String(h)+ ":" + String(m));
       }
    }
    
    delay(0); // Для предотвращения ESP8266 Watchdog Timer
    
    // При наступлении времени срабатывания будильника, если он еще не выключен пользователем - запустить режим часов и звук будильника
    if (alrmWeekDay == w && alrmHour == h && alrmMinute == m && isAlarming) {
      Serial.println(String(F("Рассвет Авто-ВЫКЛ в "))+String(h)+ ":" + String(m));
      isAlarming = false;
      #if (USE_MP3 == 1)
      isAlarmStopped = false;
      if (isDfPlayerOk && useAlarmSound) {
        setSpecialMode(1);
        PlayAlarmSound();
      } else {
        isAlarmStopped = true;
        setModeByModeId(saveThisMode);
      }
      #else
      isAlarmStopped = true;
      #endif
      sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника
    }

    delay(0); // Для предотвращения ESP8266 Watchdog Timer

    // Если рассвет начинался и остановлен пользователем и время начала рассвета уже прошло - сбросить флаги, подготовив их к следующему циклу
    if (isAlarmStopped && ((w * 1000L + h * 60L + m) > (alrmWeekDay * 1000L + alrmHour * 60L + alrmMinute + alarmDuration))) {
      isAlarming = false;
      isAlarmStopped = false;
      StopSound(0);
    }
  }
  
  // Подошло время отключения будильника - выключить, включить демо-режим
  if (alarmSoundTimer.isReady()) {
    alarmSoundTimer.setInterval(4294967295);
    StopSound(1500);   

    resetModes();

    BTcontrol = false;
    AUTOPLAY = true;

    if (saveSpecialMode){
       setSpecialMode(saveSpecialModeId);
    } else {       
       setModeByModeId(saveThisMode);
    }

    // setRandomMode();
       
    sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника
  }

  delay(0); // Для предотвращения ESP8266 Watchdog Timer

  // Плавное изменение громкости будильника
  #if (USE_MP3 == 1)
  if (fadeSoundTimer.isReady() && isDfPlayerOk) {
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
    delay(0); // Для предотвращения ESP8266 Watchdog Timer    
  }
  #endif  
}

void stopAlarm() {
  #if (USE_MP3 == 1)  
  if ((isAlarming || isPlayAlarmSound) && !isAlarmStopped) {
    Serial.println(String(F("Рассвет ВЫКЛ в ")) + String(hour())+ ":" + String(minute()));
    isAlarming = false;
    isAlarmStopped = true;
    isPlayAlarmSound = false;
    cmd95 = "";
    alarmSoundTimer.setInterval(4294967295);
    StopSound(1500);

    resetModes();  

    BTcontrol = false;
    AUTOPLAY = false;

    if (saveSpecialMode){
       setSpecialMode(saveSpecialModeId);
    } else {
       setModeByModeId(saveThisMode);
    }
    
    // setRandomMode();

    delay(0);    
    sendPageParams(95);  // Параметры, статуса IsAlarming (AL:1), чтобы изменить в смартфоне отображение активности будильника
  }
  #endif
}

void setModeByModeId(byte aMode) {
  if (saveRunningFlag) {
    thisMode = aMode;
    startRunningText();
  } else {
    byte tmp = mapModeToEffect(aMode);
    if (tmp <= MAX_EFFECT) {
      setEffect(tmp);
    } else {  
      tmp = mapModeToGame(aMode);
      if (tmp <= MAX_GAME) {
        startGame(tmp, true, false);
      } else if (aMode == DEMO_TEXT_0 || aMode == DEMO_TEXT_1 || aMode == DEMO_TEXT_2) {
        thisMode = aMode;
        startRunningText();
        loadingFlag = true;
      }
      else {
        setEffect(EFFECT_FIRE);  
      }
    }
  }
  FastLED.setBrightness(globalBrightness);
}

void setRandomMode() {
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

// Проверка необходимости включения режима 1 по установленному времени
void checkAutoMode1Time() {
  if (AM1_effect_id <= -5 || AM1_effect_id >= MAX_EFFECT || !init_time) return;
  
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

// Проверка необходимости включения режима 2 по установленному времени
void checkAutoMode2Time() {

  // Действие отличается от "Нет действия" и время установлено?
  if (AM2_effect_id <= -5 || AM2_effect_id >= MAX_EFFECT || !init_time) return;

  // Если сработал будильник - рассвет - режим не переключать - остаемся в режими обработки будильника
  if ((isAlarming || isPlayAlarmSound) && !isAlarmStopped) return;

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

// Выполнение включения режима 1 или 2 (amode) по установленному времени
void SetAutoMode(byte amode) {
  
  Serial.print(F("Авторежим "));
  Serial.print(amode);
  Serial.print(F(" ["));
  Serial.print(amode == 1 ? AM1_hour : AM2_hour);
  Serial.print(":");
  Serial.print(amode == 1 ? AM1_minute : AM2_minute);
  Serial.print(F("] - "));

  int8_t ef = (amode == 1 ? AM1_effect_id : AM2_effect_id);

  //ef: -5 - нет действия; 
  //    -4 - выключить матрицу (черный экран); 
  //    -3 - ночные часы, 
  //    -2 - камин с часами, 
  //    -1 - бегущая строка, 
  //     0 - случайный,
  //     1 и далее - эффект из ALARM_LIST по списку

  // включить указанный режим
  if (ef <= -5 || ef >= MAX_EFFECT) {
    Serial.print(F("нет действия"));
  } else if (ef == -4) {

    // Выключить матрицу (черный экран)
    Serial.print(F("выключение матрицы"));
    setSpecialMode(0);
    
  } else if (ef == -3) {

    // Ночные часы
    Serial.print(F("включение режима "));    
    Serial.print(F("ночные часы"));
    setSpecialMode(4);
    
  } else if (ef == -2) {

    // Камин с часами
    Serial.print(F("включение режима "));    
    Serial.print(F("камин с часами"));
    setSpecialMode(5);
    
  } else if (ef == -1) {

    // Бегущая строка
    Serial.print(F("включение режима "));    
    Serial.print(F("бегущая строка"));

    resetModes();  
    startRunningText();
    
  } else {

    Serial.print(F("включение режима "));    
    // Если режим включения == 0 - случайный режим и автосмена по кругу
    idleTimer.setInterval(ef == 0 ? idleTime : 4294967295); 
    idleTimer.reset();

    resetModes();  

    AUTOPLAY = ef == 0;
    if (AUTOPLAY) {
      autoplayTimer = millis();
      idleState = true;
      BTcontrol = false;
    }
    
    String s_tmp = String(ALARM_LIST);
    
    if (ef == 0) {
      // "Случайный" режим и далее автосмена
      Serial.print(F("демонcтрации эффектов:"));
      uint32_t cnt = CountTokens(s_tmp, ','); 
      ef = random(0, cnt - 1); 
    } else {
      ef -= 1; // Приведение номера эффекта (номер с 1) к индексу в массиве ALARM_LIST (индекс c 0)
    }

    s_tmp = GetToken(s_tmp, ef+1, ',');
    Serial.print(F(" эффект "));
    Serial.print("'" + s_tmp + "'");
    
    // Включить указанный режим из списка доступных эффектов без дальнейшей смены
    // Значение ef может быть 0..N-1 - указанный режим из списка ALARM_LIST (приведенное к индексу с 0)      
    byte tmp = mapAlarmToEffect(ef);   
    // Если не опознали что за эффект - включаем режим "Камин"
    if (tmp != 255) setEffect(tmp);
    else            setEffect(EFFECT_FIRE); 
  }
  
  Serial.println();
}

void checkClockOrigin() {
  if (allowVertical || allowHorizontal) {
    // Если ширина матрицы не позволяет расположить часы горизонтально - переключить в вертикальный режим
    if (CLOCK_ORIENT == 1 && !allowVertical) {
      CLOCK_ORIENT = 0;
      saveClockOrientation(CLOCK_ORIENT);
    }
    // Если высота матрицы не позволяет расположить часы вертикально - переключить в горизонтальный режим
    if (CLOCK_ORIENT == 0 && !allowHorizontal) {
      CLOCK_ORIENT = 1;
      saveClockOrientation(CLOCK_ORIENT);
    }
  } else {
    overlayEnabled = false;
    saveClockOverlayEnabled(overlayEnabled);
    return;
  }

  CLOCK_X = CLOCK_ORIENT == 0 ? CLOCK_X_H : CLOCK_X_V;
  CLOCK_Y = CLOCK_ORIENT == 0 ? CLOCK_Y_H : CLOCK_Y_V;

  // ширина и высота отображения часов  
  byte cw = CLOCK_ORIENT == 0 ? 4*3 + 3*1 : 2*3 + 1; // гориз: 4 цифры * (шрифт 3 пикс шириной) 3 + пробела между цифрами) // ширина горизонтальных часов
                                                     // верт:  2 цифры * (шрифт 3 пикс шириной) 1 + пробел между цифрами)  // ширина вертикальных часов
  byte ch = CLOCK_ORIENT == 0 ? 1*5 : 2*5 + 1;       // гориз: Одна строка цифр 5 пикс высотой                             // высота горизонтальных часов
                                                     // верт:  Две строки цифр 5 пикс высотой + 1 пробел между строкми     // высота вертикальных часовв
  while (CLOCK_X > 0 && CLOCK_X + cw > WIDTH)  CLOCK_X--;
  while (CLOCK_Y > 0 && CLOCK_Y + ch > HEIGHT) CLOCK_Y--;

  cw = 4*3 + 1;                                     // 4 цифры * (шрифт 3 пикс шириной) 1 + пробел между цифрами)          // ширина календаря
  ch = 2*5 + 1;                                     // Две строки цифр 5 пикс высотой + 1 пробел между строкми             // высота календаря
  
  while (CALENDAR_X > 0 && CALENDAR_X + cw > WIDTH)  CALENDAR_X--; 
  while (CALENDAR_Y > 0 && CALENDAR_Y + ch > HEIGHT) CALENDAR_Y--;
}
