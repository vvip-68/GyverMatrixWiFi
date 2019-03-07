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
  //   19 - зарезервировано
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
#if (USE_CLOCK == 1)
    overlayEnabled = EEPROMread(5);
    useNtp = EEPROMread(9) == 1;
    SYNC_TIME_PERIOD = EEPROM_int_read(10);
    timeZoneOffset = (uint8_t)EEPROMread(12);
    CLOCK_ORIENT = EEPROMread(13) == 1 ? 1 : 0;
    COLOR_MODE = EEPROMread(14);
    showDateInClock = EEPROMread(16) == 1;  
    showDateDuration = EEPROMread(17);
    showDateInterval = EEPROMread(18);
#endif    
  } else {
    globalBrightness = BRIGHTNESS;
    scrollSpeed = D_TEXT_SPEED;
    effectSpeed = D_EFFECT_SPEED;
    gameSpeed = D_GAME_SPEED;
    AUTOPLAY = true;
    autoplayTime = ((long)AUTOPLAY_PERIOD * 1000);     // секунды -> миллисек
    idleTime = ((long)IDLE_TIME * 60 * 1000);          // минуты -> миллисек
#if (USE_CLOCK == 1)  
    overlayEnabled = true;
    useNtp = true;
    SYNC_TIME_PERIOD = 60;
    timeZoneOffset = 7;
    CLOCK_ORIENT = 0;
    COLOR_MODE = 0;
    showDateInClock = true;  
    showDateDuration = 5;
    showDateInterval = 20;
#endif    
  }

  scrollTimer.setInterval(scrollSpeed);
  effectTimer.setInterval(effectSpeed);
  gameTimer.setInterval(gameSpeed);
  idleTimer.setInterval(idleTime);
  
#if (USE_CLOCK == 1)    
  ntpTimer.setInterval(1000 * 60 * SYNC_TIME_PERIOD);
#endif    
  
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

#if (USE_CLOCK == 1)  
  EEPROMwrite(5, overlayEnabled);
  EEPROMwrite(9, useNtp ? 1 : 0);
  EEPROM_int_write(10, SYNC_TIME_PERIOD);
  EEPROMwrite(12, (byte)timeZoneOffset);
  EEPROMwrite(13, CLOCK_ORIENT == 1 ? 1 : 0);
  EEPROMwrite(14, COLOR_MODE);
  EEPROMwrite(16, showDateInClock ? 1 : 0);
  EEPROMwrite(17, showDateDuration);
  EEPROMwrite(18, showDateInterval);
#endif

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

#if (USE_CLOCK == 1)
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

#endif

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

// запись
void EEPROM_int_write(byte addr, uint16_t num) {
  byte raw[2];
  (uint16_t&)raw = num;
  for (byte i = 0; i < 2; i++) EEPROMwrite(addr+i, raw[i]);
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
#if (USE_CLOCK == 1)
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
#endif
bool getUseTextInDemo() { return true; }
void setUseTextInDemo(boolean use) {  }

#endif
