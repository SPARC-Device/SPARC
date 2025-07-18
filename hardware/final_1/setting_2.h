#ifndef SETTING_2_H
#define SETTING_2_H

#include <Arduino.h>

// Main entry points
void setting2Setup();
void setting2Loop();

// If you want to expose any config or UI functions, declare them here
// For now, only the main entry points are declared
void drawT9Cell(int index, bool highlightYellow, bool blinkMode = false);

void saveBlinkSettingsToPreferences();
void loadBlinkSettingsFromPreferences();
void loadWiFiFromPreferences();

#ifndef SETTING_2_H_USERID_GUARD
#define SETTING_2_H_USERID_GUARD


#define USERID_ADDR 140
#define EEPROM_SIZE 200



String readUserIdFromEEPROM();

#endif

#endif // SETTING_2_H 