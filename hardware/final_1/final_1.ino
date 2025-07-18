#include "blink_wifi.h"
#include "gui_3.h"
#include "setting_2.h"
#include "notif.h"
#include <WiFi.h>
#include "variable.h"

int uiState = 0;
bool tftConnected = true;

void openSettingsInterface() {
    uiState = 1;
    setting2Setup();
}

IPAddress local_IP(192, 168, 1, 13); // Set your static IP
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);


void setup() {
  Serial.begin(115200);  
  loadWiFiFromPreferences();
    WiFi.config(local_IP, gateway, subnet);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    notificationServerSetup();
    blinkWifiSetup();
    gui3Setup();
  }

void loop() {
  getBlinks();
  if (tftConnected) {
    if (uiState == 0) {
      notificationServerLoop();
      gui3Loop();

      if (blinkWifiCheckSingleBlink()) {
       // Serial.println("[DEBUG] Handling Single blink: calling gui3OnSingleBlink()");
        gui3OnSingleBlink();
        } else if (blinkWifiCheckDoubleBlink()) {
        Serial.println("[DEBUG] Handling Double blink: calling gui3OnDoubleBlink()");
        gui3OnDoubleBlink();
      }
    } else if (uiState == 1) {
      setting2Loop();
    }
  } else {
    blinkWifiLoop();
  }
  blinkWifiResetFlags();
}
