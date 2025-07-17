#include "blink_wifi.h"
#include "gui_3.h"
#include "setting_2.h"

int uiState = 0; // 0 = main, 1 = settings

void openSettingsInterface() {
    uiState = 1;
    setting2Setup();
}

void setup() {
  blinkWifiSetup();
  gui3Setup();
}

void loop() {
  if (uiState == 0) {
    blinkWifiLoop();
    gui3Loop();

    // --- Blink navigation integration ---
    if (blinkWifiCheckDoubleBlink()) {
      gui3OnDoubleBlink();
    } else if (blinkWifiCheckSingleBlink()) {
      gui3OnSingleBlink();
    }
  } else if (uiState == 1) {
    setting2Loop();
  }
}