#ifndef SETTING_H
#define SETTING_H

#include <Arduino.h>

// Main entry points
void setting2Setup();
void setting2Loop();

void drawT9Cell(int index, bool highlightYellow, bool blinkMode = false);

void saveBlinkSettingsToPreferences();
void loadBlinkSettingsFromPreferences();

void loadWiFiFromPreferences();

#ifndef SETTING_H_USERID_GUARD
#define SETTING_H_USERID_GUARD

#define USERID_ADDR 140
#define EEPROM_SIZE 200

String readUserIdFromEEPROM();

#endif

#endif // SETTING_H
