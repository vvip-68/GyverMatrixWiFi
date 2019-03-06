// режим часов

#if (USE_CLOCK == 1)

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
  Serial.println(F("Отправка NTP пакета..."));
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
    ntp_t = 0;
    init_time = 1;
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
    for (byte i = 0; i < 5; i++) clockLED[i] = NORMAL_CLOCK_COLOR;
  } else if (COLOR_MODE == 1) {
    for (byte i = 0; i < 5; i++) clockLED[i] = CHSV(clockHue + HUE_GAP * i, 255, 255);
    clockLED[2] = CHSV(clockHue + 128 + HUE_GAP * 1, 255, 255); // точки делаем другой цвет
  } else if (COLOR_MODE == 2) {
    clockLED[0] = CHSV(clockHue + HUE_GAP * 0, 255, 255);
    clockLED[1] = CHSV(clockHue + HUE_GAP * 0, 255, 255);
    clockLED[2] = CHSV(clockHue + 128 + HUE_GAP * 1, 255, 255); // точки делаем другой цвет
    clockLED[3] = CHSV(clockHue + HUE_GAP * 2, 255, 255);
    clockLED[4] = CHSV(clockHue + HUE_GAP * 2, 255, 255);
  } else if (COLOR_MODE == 3) {// для календаря -  
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
    //if (hrs > 9) 
    drawDigit3x5(h10, X + (h10 == 1 ? 1 : 0), Y, clockLED[0]); // шрифт 3x5 в котором 1 - по центру знакоместа - смещать вправо на 1 колонку
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
  for (byte i = 0; i < 5; i++) clockLED[i] = CONTRAST_COLOR_3;  
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
        contrastClockC();
        break;
    }
  }
  else
    clockColor();
}

#else

void clockOverlayWrapH(byte posX, byte posY) {
  return;
}
void clockOverlayUnwrapH(byte posX, byte posY) {
  return;
}
void clockOverlayWrapV(byte posX, byte posY) {
  return;
}
void clockOverlayUnwrapV(byte posX, byte posY) {
  return;
}
boolean needUnwrap() {
  return true;
}
#endif

#else

void clockRoutine() { }

#endif
