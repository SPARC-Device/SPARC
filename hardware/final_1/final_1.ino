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
  if (blinkWifiCheckSingleBlink()) gui3OnSingleBlink();
  if (blinkWifiCheckDoubleBlink()) gui3OnDoubleBlink();
}