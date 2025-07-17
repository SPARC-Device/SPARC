#include "blink_wifi.h"
#include "gui_3.h"

void setup() {
  blinkWifiSetup();
  gui3Setup();
}

void loop() {
  blinkWifiLoop();
  gui3Loop();

  // --- Blink navigation integration ---
  if (blinkWifiCheckDoubleBlink()) {
    gui3OnDoubleBlink();
  } else if (blinkWifiCheckSingleBlink()) {
    gui3OnSingleBlink();
  }
}