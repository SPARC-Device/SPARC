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

IPAddress local_IP(192, 168, 120, 13); // Set your static IP
IPAddress gateway(192, 168, 120, 1);
IPAddress subnet(255, 255, 255, 0);


void setup() {
  Serial.begin(115200);  
  loadWiFiFromPreferences();
  loadBlinkSettingsFromPreferences();

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  
  // Try to connect for 10 seconds
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 10000) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
  } else {
    Serial.println("WiFi connection failed - continuing without WiFi");
  }
  
  notificationServerSetup();
  blinkWifiSetup();
  gui3Setup();
  //gui3InitAudio(); 
}

void loop() {
  getBlinks();
    // 1. Always check for new client connection
    if (!clientFound && !clientConnected) {
        WiFiClient tempClient = server.available();
        if (tempClient) {
            client = tempClient;
            Serial.println("*** NEW CLIENT CONNECTED - SENDING CONFIG ***");
            delay(500); // Give client time to be ready
            String config = String(blinkDuration) + ";" + String(blinkGap) + ";" + ssid + ";" + password + ";" + userId;
            size_t bytesWritten = client.print(config);
            client.flush();
            Serial.print("Config string sent to client: ");
            Serial.println(config);
            Serial.print("Bytes written: ");
            Serial.println(bytesWritten);
            clientConnected = true;
            clientFound = true;
            Serial.println("*** CLIENT CONNECTED, CONFIG SENT, SWITCHING TO CLIENT MODE ***");
        }
    }

    // 2. Set tftConnected based on clientConnected
    tftConnected = !clientConnected;

    // 3. Run the appropriate mode
    if (tftConnected) {
        // GUI mode
        if (uiState == 0) {
            notificationServerLoop();
            gui3Loop();
            if (blinkWifiCheckSingleBlink()) gui3OnSingleBlink();
            else if (blinkWifiCheckDoubleBlink()) gui3OnDoubleBlink();
        } else if (uiState == 1) {
            setting2Loop();
        }
    } else {
        // Client mode
        blinkWifiLoop();
        // blinkWifiLoop() should handle client disconnection and set clientConnected = false if needed
    }

    blinkWifiResetFlags();
}
