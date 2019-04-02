#if (USE_EEPROM == 1)
#define EEPROM_OK 0xA5        // Флаг, показывающий, что EEPROM инициализирована корректными данными 
#define EFFECT_EEPROM 150     // начальная ячейка eeprom с параметрами эффектов
#define GAME_EEPROM 230       // начальная ячейка eeprom с параметрами игр

void loadSettings() {

  // Адреса в EEPROM:
  //    0 - если EEPROM_OK - EEPROM инициализировано, если другое значение - нет 
  //    1 - максимальная яркость ленты 1-255
  //    2 - скорость прокрутки текста по умолчанию
  //    3 - скорость эффектов по умолчанию
  //    4 - скорость игр по умолчанию
  //    5 - разрешен оверлей часов для эффектов
  //    6 - автосмена режима в демо: вкл/выкл
  //    7 - время автосмены режимов
  //    8 - время бездействия до переключения в авторежим
  //    9 - использовать синхронизацию времени через NTP
  //10,11 - период синхронизации NTP (int16_t - 2 байта)
  //   12 - time zone
  //   13 - ориентация часов: 0 - горизонтально; 1 - вертикально
  //   14 - цветовая схема часов 0 - монохром; 1 - каждая цифра свой цвет плавно; 2 - часы, точки, минуты - свой цвет плавно; 3 - часы, точки, минуты - цвет пользователя
  //   15 - использовать эффекты бегущего текста в демо режиме
  //   16 - Отображать с часами текущую дату 
  //   17 - Кол-во секунд отображения даты
  //   18 - Кол-во секунд отображения часов
  //   19 - Будильник вкал/выкл 1 - вкл; 0 -выкл
  //   20 - Будильник, время: часы
  //   21 - Будильник, время: минуты
  //   22 - Будильник, дни недели
  //   23 - Будильник, продолжительность "рассвета"
  //   24 - Будильник, эффект "рассвета"
  //   25 - зарезервировано
  //  ... - зарезервировано
  //  80-103  - имя сети  WiFi
  //  104-119 - пароль сети  WiFi
  //  120-149 - имя NTP сервера
  //  ... - зарезервировано
  //  149 - зарезервировано
  //  150 - 150+(Nэфф*3)   - скорость эффекта
  //  151 - 150+(Nэфф*3)+1 - 1 - оверлей часов разрешен; 0 - нет оверлея часов
  //  152 - 150+(Nэфф*3)+2 - эффект в авторежиме: 1 - использовать; 0 - не использовать
  // ....
  //  230 - 100+(Nигр*2)   - скорость игры
  //  230 - 100+(Nигр*2)+1 - использовать игру в демо-режиме

  // Инициализировано ли EEPROM
  bool isInitialized = EEPROMread(0) == EEPROM_OK;  
    
  if (isInitialized) {    
    globalBrightness = EEPROMread(1);
    scrollSpeed = map(EEPROMread(2),0,255,D_TEXT_SPEED_MIN,D_TEXT_SPEED_MAX);
    effectSpeed = map(EEPROMread(3),0,255,D_EFFECT_SPEED_MIN,D_EFFECT_SPEED_MAX);
    gameSpeed = map(EEPROMread(4),0,255,D_GAME_SPEED_MIN,D_GAME_SPEED_MAX);   
    AUTOPLAY = EEPROMread(6) == 1;
    autoplayTime = EEPROMread(7) * 1000L;  // секунды -> миллисек 
    idleTime = EEPROMread(8) * 60 * 1000L; // минуты -> миллисек
    overlayEnabled = EEPROMread(5);
    useNtp = EEPROMread(9) == 1;
    SYNC_TIME_PERIOD = EEPROM_int_read(10);
    timeZoneOffset = (uint8_t)EEPROMread(12);
    CLOCK_ORIENT = EEPROMread(13) == 1 ? 1 : 0;
    COLOR_MODE = EEPROMread(14);
    showDateInClock = EEPROMread(16) == 1;  
    showDateDuration = EEPROMread(17);
    showDateInterval = EEPROMread(18);
    alarmOnOff = EEPROMread(19) == 1; 
    alarmHour = constrain(EEPROMread(20), 0, 23);
    alarmMinute = constrain(EEPROMread(21), 0, 59);
    alarmWeekDay = EEPROMread(22);
    alarmDuration = constrain(EEPROMread(23),1,59);
    alarmEffect = EEPROMread(24);

    EEPROM_string_read(80,24).toCharArray(ssid, 24);            //  80-103  - имя сети  WiFi    (24 байта макс)
    EEPROM_string_read(104,16).toCharArray(pass, 16);           //  104-119 - пароль сети  WiFi (16 байт макс)
    EEPROM_string_read(120,30).toCharArray(ntpServerName, 30);  //  120-149 - имя NTP сервера   (30 байт макс)    
    if (strlen(ntpServerName) == 0) strcpy(ntpServerName, "time.nist.gov");
    
  } else {
    globalBrightness = BRIGHTNESS;
    scrollSpeed = D_TEXT_SPEED;
    effectSpeed = D_EFFECT_SPEED;
    gameSpeed = D_GAME_SPEED;
    AUTOPLAY = true;
    autoplayTime = ((long)AUTOPLAY_PERIOD * 1000);     // секунды -> миллисек
    idleTime = ((long)IDLE_TIME * 60 * 1000);          // минуты -> миллисек
    overlayEnabled = true;
    useNtp = true;
    SYNC_TIME_PERIOD = 60;
    timeZoneOffset = 7;
    CLOCK_ORIENT = 0;
    COLOR_MODE = 0;
    showDateInClock = true;  
    showDateDuration = 5;
    showDateInterval = 20;
    alarmOnOff = false;
    alarmHour = 0;
    alarmMinute = 0;
    alarmWeekDay = 0;
    alarmDuration = 20;
    alarmEffect = EFFECT_DAWN_ALARM;
    
    strcpy(ssid, "");
    strcpy(pass, "");
    strcpy(ntpServerName, "time.nist.gov");
  }

  scrollTimer.setInterval(scrollSpeed);
  effectTimer.setInterval(effectSpeed);
  gameTimer.setInterval(gameSpeed);
  idleTimer.setInterval(idleTime);  
  ntpSyncTimer.setInterval(1000 * 60 * SYNC_TIME_PERIOD);
  
  // После первой инициализации значений - сохранить их принудительно
  if (!isInitialized) {
    saveDefaults();
    saveSettings();
  }
}

void saveDefaults() {

  EEPROMwrite(1, globalBrightness);

  EEPROMwrite(2, constrain(map(scrollSpeed, D_TEXT_SPEED_MIN, D_TEXT_SPEED_MAX, 0, 255), 0, 255));
  EEPROMwrite(3, constrain(map(effectSpeed, D_EFFECT_SPEED_MIN, D_EFFECT_SPEED_MAX, 0, 255), 0, 255));
  EEPROMwrite(4, constrain(map(gameSpeed, D_GAME_SPEED_MIN, D_GAME_SPEED_MAX, 0, 255), 0, 255));

  EEPROMwrite(6, AUTOPLAY ? 1 : 0);
  EEPROMwrite(7, autoplayTime / 1000);
  EEPROMwrite(8, constrain(idleTime / 60 / 1000, 0, 255));

  EEPROMwrite(5, overlayEnabled);
  EEPROMwrite(9, useNtp ? 1 : 0);
  EEPROM_int_write(10, SYNC_TIME_PERIOD);
  EEPROMwrite(12, (byte)timeZoneOffset);
  EEPROMwrite(13, CLOCK_ORIENT == 1 ? 1 : 0);
  EEPROMwrite(14, COLOR_MODE);
  EEPROMwrite(16, showDateInClock ? 1 : 0);
  EEPROMwrite(17, showDateDuration);
  EEPROMwrite(18, showDateInterval);
  saveAlarmParams(alarmOnOff,alarmHour,alarmMinute,alarmWeekDay,alarmDuration,alarmEffect);

  EEPROMwrite(15, 1);    // Использовать бегущий текст в демо-режиме: 0 - нет; 1 - да

  // Настройки по умолчанию для эффектов
  int addr = EFFECT_EEPROM;
  for (int i = 0; i < MAX_EFFECT; i++) {
    saveEffectParams(i, effectSpeed, false, true);
  }

  // Настройки по умолчанию для игр
  addr = GAME_EEPROM;
  for (int i = 0; i < MAX_GAME; i++) {
    saveGameParams(i, gameSpeed, true);
  }
    
  strcpy(ntpServerName, "time.nist.gov");
  EEPROM_string_write(120, ntpServerName);
      
  eepromModified = true;
}

void saveSettings() {

  if (!eepromModified) return;
  
  // Поставить отметку, что EEPROM инициализировано параметрами эффектов
  EEPROMwrite(0, EEPROM_OK);
  
  EEPROM.commit();
  Serial.println(F("Настройки сохранены в EEPROM"));
  
  eepromModified = false;
}

void saveEffectParams(byte effect, int speed, boolean overlay, boolean use) {
  const int addr = EFFECT_EEPROM;  
  EEPROMwrite(addr + effect*3, constrain(map(speed, D_EFFECT_SPEED_MIN, D_EFFECT_SPEED_MAX, 0, 255), 0, 255));        // Скорость эффекта
  EEPROMwrite(addr + effect*3 + 1, overlay ? 1 : 0);             // По умолчанию оверлей часов для эффекта отключен  
  EEPROMwrite(addr + effect*3 + 2, use ? 1 : 0);                 // По умолчанию эффект доступен в демо-режиме  
  eepromModified = true;
}

void saveEffectSpeed(byte effect, int speed) {
  if (speed != getEffectSpeed(effect)) {
    const int addr = EFFECT_EEPROM;  
    EEPROMwrite(addr + effect*3, constrain(map(speed, D_EFFECT_SPEED_MIN, D_EFFECT_SPEED_MAX, 0, 255), 0, 255));        // Скорость эффекта
    eepromModified = true;
  }
}

int getEffectSpeed(byte effect) {
  const int addr = EFFECT_EEPROM;
  return map(EEPROMread(addr + effect*3),0,255,D_EFFECT_SPEED_MIN,D_EFFECT_SPEED_MAX);
}

void saveEffectUsage(byte effect, boolean use) {
  if (use != getEffectUsage(effect)) {
    const int addr = EFFECT_EEPROM;  
    EEPROMwrite(addr + effect*3 + 2, use ? 1 : 0);             // По умолчанию оверлей часов для эффекта отключен  
    eepromModified = true;
  }
}

boolean getEffectUsage(byte effect) {
  const int addr = EFFECT_EEPROM;
  return EEPROMread(addr + effect*3 + 2) == 1;
}

void saveGameParams(byte game, int speed, boolean use) {
  const int addr = GAME_EEPROM;  
  EEPROMwrite(addr + game*2, constrain(map(speed, D_GAME_SPEED_MIN, D_GAME_SPEED_MAX, 0, 255), 0, 255));         // Скорость эффекта
  EEPROMwrite(addr + game*2 + 1, use ? 1 : 0);                                                                   // Использовать игру в демо-режиме
  eepromModified = true;
}

void saveGameSpeed(byte game, int speed) {
  if (speed != getGameSpeed(game)) {
    const int addr = GAME_EEPROM;  
    EEPROMwrite(addr + game*2, constrain(map(speed, D_GAME_SPEED_MIN, D_GAME_SPEED_MAX, 0, 255), 0, 255));         // Скорость эффекта
    eepromModified = true;
  }
}

int getGameSpeed(byte game) {
  const int addr = GAME_EEPROM;  
  return map(EEPROMread(addr + game*2),0,255,D_GAME_SPEED_MIN,D_GAME_SPEED_MAX);
}

void saveGameUsage(byte game, boolean use) {
  if (use != getGameUsage(game)) {
    const int addr = GAME_EEPROM;  
    EEPROMwrite(addr + game*2 + 1, use ? 1 : 0);             // По умолчанию оверлей часов для эффекта отключен  
    eepromModified = true;
  }
}

boolean getGameUsage(byte game) {
  const int addr = GAME_EEPROM;
  return EEPROMread(addr + game*2 + 1) == 1;
}

void saveScrollSpeed(int speed) {
  if (speed != getScrollSpeed()) {
    EEPROMwrite(2, constrain(map(speed, D_TEXT_SPEED_MIN, D_TEXT_SPEED_MAX, 0, 255), 0, 255));
    eepromModified = true;
  }
}

int getScrollSpeed() {
  return map(EEPROMread(2),0,255,D_TEXT_SPEED_MIN,D_TEXT_SPEED_MAX);
}

byte getMaxBrightness() {
  return EEPROMread(1);
}

void saveMaxBrightness(byte brightness) {
  if (brightness != getMaxBrightness()) {
    EEPROMwrite(1, globalBrightness);
    eepromModified = true;
  }
}

void saveAutoplay(boolean value) {
  if (value != getAutoplay()) {
    EEPROMwrite(6, value);
    eepromModified = true;
  }
}

bool getAutoplay() {
  return EEPROMread(6) == 1;
}

void saveAutoplayTime(long value) {
  if (value != getAutoplayTime()) {
    EEPROMwrite(7, constrain(value / 1000, 0, 255));
    eepromModified = true;
  }
}

long getAutoplayTime() {
  long time = EEPROMread(7) * 1000L;  
  if (time == 0) time = ((long)AUTOPLAY_PERIOD * 1000);
  return time;
}

void saveIdleTime(long value) {
  if (value != getIdleTime()) {
    EEPROMwrite(8, constrain(value / 60 / 1000, 0, 255));
    eepromModified = true;
  }
}

long getIdleTime() {
  long time = EEPROMread(8) * 60 * 1000L;  
  if (time == 0) time = ((long)IDLE_TIME * 60 * 1000);
  return time;
}

void saveEffectClock(byte effect, boolean overlay) {
  if (overlay != getEffectClock(effect)) {
    const int addr = EFFECT_EEPROM;  
    EEPROMwrite(addr + effect*3 + 1, overlay ? 1 : 0);             // По умолчанию оверлей часов для эффекта отключен  
    eepromModified = true;
  }
}

boolean getEffectClock(byte effect) {
  const int addr = EFFECT_EEPROM;
  return EEPROMread(addr + effect*3 + 1) == 1;
}

boolean getClockOverlayEnabled() {
  return EEPROMread(5) == 1;
}

void saveClockOverlayEnabled(boolean enabled) {
  if (enabled != getClockOverlayEnabled()) {
    EEPROMwrite(5, enabled ? 1 : 0);
    eepromModified = true;
  }
}

void saveUseNtp(boolean value) {
  if (value != getUseNtp()) {
    EEPROMwrite(9, value);
    eepromModified = true;
  }
}

bool getUseNtp() {
  return EEPROMread(9) == 1;
}

void saveNtpSyncTime(uint16_t value) {
  if (value != getNtpSyncTime()) {
    EEPROM_int_write(10, SYNC_TIME_PERIOD);
    eepromModified = true;
  }
}

uint16_t getNtpSyncTime() {
  uint16_t time = EEPROM_int_read(10);  
  if (time == 0) time = 60;
  return time;
}

void saveTimeZone(int8_t value) {
  if (value != getTimeZone()) {
    EEPROMwrite(12, (byte)value);
    eepromModified = true;
  }
}

int8_t getTimeZone() {
  return (int8_t)EEPROMread(12);
}

byte getClockOrientation() {
  return EEPROMread(13) == 1 ? 1 : 0;
}

void saveClockOrientation(byte orientation) {
  if (orientation != getClockOrientation()) {
    EEPROMwrite(13, orientation == 1 ? 1 : 0);
    eepromModified = true;
  }
}

byte getClockColorMode() {
  return EEPROMread(14);
}

void saveClockColorMode(byte ColorMode) {
  if (ColorMode != getClockColorMode()) {
    EEPROMwrite(14, COLOR_MODE);
    eepromModified = true;
  }
}

bool getShowDateInClock() {
  return EEPROMread(16) == 1;
}

void setShowDateInClock(boolean use) {  
  if (use != getShowDateInClock()) {
    EEPROMwrite(16, use ? 1 : 0);
    eepromModified = true;
  }
}

byte getShowDateDuration() {
  return EEPROMread(17);
}

void setShowDateDuration(byte Duration) {
  if (Duration != getShowDateDuration()) {
    EEPROMwrite(17, Duration);
    eepromModified = true;
  }
}

byte getShowDateInterval() {
  return EEPROMread(18);
}

void setShowDateInterval(byte Interval) {
  if (Interval != getShowDateInterval()) {
    EEPROMwrite(18, Interval);
    eepromModified = true;
  }
}

void saveAlarmParams(boolean alarmOnOff, byte alarmHour, byte alarmMinute, byte alarmWeekDay, byte alarmDuration, byte alarmEffect) {
  //   19 - Будильник вкал/выкл 1 - вкл; 0 -выкл
  //   20 - Будильник, время: часы
  //   21 - Будильник, время: минуты
  //   22 - Будильник, дни недели
  //   23 - Будильник, продолжительность "рассвета"
  //   24 - Будильник, эффект "рассвета"
  //   25 - зарезервировано
  if (alarmOnOff != getAlarmOnOff()) {
    EEPROMwrite(19, alarmOnOff ? 1 : 0);
    eepromModified = true;
  }
  if (alarmHour != getAlarmHour()) {
    EEPROMwrite(20, alarmHour);
    eepromModified = true;
  }
  if (alarmMinute != getAlarmMinute()) {
    EEPROMwrite(21, alarmMinute);
    eepromModified = true;
  }
  if (alarmWeekDay != getAlarmWeekDay()) {
    EEPROMwrite(22, alarmWeekDay);
    eepromModified = true;
  }
  if (alarmDuration != getAlarmDuration()) {
    EEPROMwrite(23, alarmDuration);
    eepromModified = true;
  }
  if (alarmEffect != getAlarmEffect()) {
    EEPROMwrite(24, alarmEffect);
    eepromModified = true;
  }
}

bool getAlarmOnOff() { 
  return EEPROMread(19) == 1;
}

byte getAlarmHour() { 
  return EEPROMread(20);
}

byte getAlarmMinute() { 
  return EEPROMread(21);
}

byte getAlarmWeekDay() { 
  return EEPROMread(22);
}

byte getAlarmDuration() { 
  return EEPROMread(23);
}

byte getAlarmEffect() { 
  return EEPROMread(24);
}

bool getUseTextInDemo() {
  return EEPROMread(15) == 1;
}

void setUseTextInDemo(boolean use) {  
  if (use != getUseTextInDemo()) {
    EEPROMwrite(15, use ? 1 : 0);
    eepromModified = true;
  }
}

// ----------------------------------------------------------

byte EEPROMread(byte addr) {    
  return EEPROM.read(addr);
}

void EEPROMwrite(byte addr, byte value) {    
  EEPROM.write(addr, value);
}

// чтение uint16_t
uint16_t EEPROM_int_read(byte addr) {    
  byte raw[2];
  for (byte i = 0; i < 2; i++) raw[i] = EEPROMread(addr+i);
  uint16_t &num = (uint16_t&)raw;
  return num;
}

// запись uint16_t
void EEPROM_int_write(byte addr, uint16_t num) {
  byte raw[2];
  (uint16_t&)raw = num;
  for (byte i = 0; i < 2; i++) EEPROMwrite(addr+i, raw[i]);
}

// чтение стоки (макс 32 байта)
String EEPROM_string_read(byte addr, byte len) {
     if (len>32) len = 32;
     char buffer[len+1];

     byte i = 0;
     while (i < len) {
       char c = EEPROMread(addr+i);
       if (isAlphaNumeric(c)) buffer[i] = c;
       else break;
       i++;
     }
     buffer[i] = 0;
     return String(buffer);
}

// запись строки (макс 32 байта)
void EEPROM_string_write(byte addr, String buffer) {
     uint16_t len = buffer.length();
     if (len>32) len = 32;
     byte i = 0;
     while (i < len) {
       EEPROMwrite(addr+i, buffer[i++]);
     }
     if (i < len-1) EEPROMwrite(addr+i,0);
}
// ----------------------------------------------------------

#else

void loadSettings() { }
void saveSettings() { eepromModified = false; }
void saveEffectParams(byte effect, int speed, boolean overlay, boolean use) { }
void saveEffectSpeed(byte effect, int speed) { }
int getEffectSpeed(byte effect) { return effectSpeed; }
void saveEffectUsage(byte effect, boolean use) { }
boolean getEffectUsage(byte effect) { return true;  }
void saveGameParams(byte game, int speed, boolean use) { }
void saveGameSpeed(byte game, int speed) { }
int getGameSpeed(byte game) { return gameSpeed; }
void saveGameUsage(byte game, boolean use) { }
boolean getGameUsage(byte game) { return true;  }
void saveScrollSpeed(int speed) { }
int getScrollSpeed() { return scrollSpeed; }
byte getMaxBrightness(byte brightness) { return globalBrightness; }
void saveMaxBrightness(byte brightness) {}
void saveAutoplay(boolean value) { }
bool getAutoplay() { return AUTOPLAY; }
void saveAutoplayTime(long value) { }
long getAutoplayTime() { return autoplayTime; }
void saveIdleTime(long value) { }
long getIdleTime() { return autoplayTime; }
void saveEffectClock(byte effect, boolean overlay) { }
boolean getEffectClock(byte effect) { return overlayAllowed(); }
boolean getClockOverlayEnabled() { return overlayEnabled; }
void saveClockOverlayEnabled(boolean enabled) { }
void saveUseNtp(boolean value) { }
bool getUseNtp() { return useNtp;}
void saveNtpSyncTime(uint16_t value) { }
uint16_t getNtpSyncTime() { return SYNC_TIME_PERIOD; }
void saveTimeZone(int8_t value) { }
int8_t getTimeZone() { return timeZoneOffset; }
byte getClockOrientation() { return CLOCK_ORIENT; }
void saveClockOrientation(byte orientation) { }
byte getClockColorMode() { return COLOR_MODE; }
void saveClockColorMode(byte ColorMode) { }
bool getShowDateInClock() { return true; }
void setShowDateInClock(boolean use) {  }
byte getShowDateDuration() { return 5; }
void setShowDateDuration(byte Duration) { }
byte getShowDateInterval() { return 20; }
void setShowDateInterval(byte Interval) { }
void saveAlarmParams(boolean alarmOnOff, byte alarmHour, byte alarmMinute, byte alarmWeekDay, byte alarmDuration, byte alarmEffect) { }
bool getAlarmOnOff() { return false; }
byte getAlarmHour() { return 0;}
byte getAlarmMinute() { return 0;}
byte getAlarmWeekDay() { return 0;}
byte getAlarmDuration() { return 1;}
byte getAlarmEffect() { return EFFECT_DAWN_ALARM;}
bool getUseTextInDemo() { return true; }
void setUseTextInDemo(boolean use) {  }

#endif
