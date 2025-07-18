#include "blink_wifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "setting_2.h" // For userId
#include "notif.h" // For sendNotificationRequest
#include <EEPROM.h>
#include "variable.h"


// Pin assignments (update as needed)
static const int IR_SENSOR_PIN = 36;
static const int BLINK_LED_PIN = 25; // Changed from 19 to 25
static const int EMERGENCY_LED_PIN = 27; // Changed from 2 to 27
static const int BUZZER_PIN = 26;
// static const int NAVIGATION_LED_PIN = 14; // Removed navigation LED
static const int EMERGENCY_BUTTON_PIN = 12; // Emergency reset button moved to 14 from 22

// Configurable blink detection
int blinkDuration = 400; // ms, default
int blinkGap = 1200;     // ms, default

static const unsigned int DEBOUNCE_DELAY = 50;
static const unsigned int EMERGENCY_TIMEOUT = 7000;
static unsigned int emergency_blink_interval = 250; // For emergency blink detection

// State variables
static bool currentEyeState = true;
static bool lastSensorReading = HIGH;
static unsigned int eyeCloseTime = 0;
static unsigned int lastDebounceTime = 0;
static int consecutiveBlinks = 0;
static unsigned int lastBlinkTime = 0;
static unsigned int lastBlinkEndTime = 0; // For correct blink gap calculation
static bool emergencyMode = false;
static unsigned int emergencyStartTime = 0;

// Blink event flags for GUI
static bool singleBlinkDetected = false;
static bool doubleBlinkDetected = false;
static bool quadBlinkDetected = false; // For 4 blinks (emergency)

static bool blinkProcessed = false;


// For deferred blink event processing
static unsigned int lastBlinkEventTime = 0;

// Preferences for persistent storage
Preferences prefs;

// WiFi and TCP server

WiFiServer server(33333);
WiFiClient client;
bool clientConnected = false;

// Configurable variables (moved from .ino)
String ssid = "Pushpa";
String password = "*#@09password";


// Function declarations for WiFi logic
void processCommand(String cmd, Stream &out, bool fromWifi);
void reconnectWiFi();
void loadConfig();
void saveConfig();
void sendStatus(Stream &out);
void emergencyModeCheck();

// Command buffers
String wifiCmdBuffer = "";
String serialCmdBuffer = "";

// Helper functions for Preferences persistence

int getBlinks(){
   // IR sensor logic
   bool sensorReading = digitalRead(IR_SENSOR_PIN);  // LOW = Eye closed

   // Debounce check
   if (sensorReading != lastSensorReading) {
     lastDebounceTime = millis();
   }
 
   if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
     bool eyeOpen = (sensorReading == HIGH);
     if (eyeOpen != currentEyeState) {
       if (!eyeOpen) {
         // Eye just closed
         eyeCloseTime = millis();
         digitalWrite(BLINK_LED_PIN, HIGH); // Blink LED ON
       } else {
         // Eye just opened
         unsigned long actualblinkDuration = millis() - eyeCloseTime;
         digitalWrite(BLINK_LED_PIN, LOW); // Blink LED OFF
         // Use different min duration for emergency blinks
         unsigned long currentBlinkDuration = (consecutiveBlinks >= 2) ? emergency_blink_interval : blinkDuration;
         if (actualblinkDuration >= currentBlinkDuration) {
           unsigned long actualblinkGap = 0;
           if (lastBlinkEndTime != 0) {
             actualblinkGap = millis() - lastBlinkEndTime;
           }
           if (millis() - lastBlinkTime < blinkGap) {
             consecutiveBlinks++;
           } else {
             consecutiveBlinks = 1;  // Reset count if too much time passed
           }
           lastBlinkTime = millis();
           lastBlinkEndTime = millis(); // Update for next gap calculation
           lastBlinkEventTime = millis(); // Update for deferred event
           // Debug print
           Serial.print("Blink #");
           Serial.print(consecutiveBlinks);
           Serial.print(", blinkDuration:");
           Serial.print(blinkDuration);
           Serial.print(", Duration: ");
           Serial.print(actualblinkDuration);
           Serial.print(" ms, Gap: ");
           Serial.print(blinkGap);
           Serial.print(" ms, Acutal Gap: ");
           Serial.print(actualblinkGap);
           Serial.println(" ms");
         }
       }
       currentEyeState = eyeOpen;
     }
   }
   lastSensorReading = sensorReading;

   if (consecutiveBlinks > 0 && (millis() - lastBlinkEventTime > blinkGap+300)) {
    consecutiveBlinks = 0;
    blinkProcessed = false; // reset flag after full blink gap
  }
 

  if (!blinkProcessed){
    if (consecutiveBlinks == 1 &&(millis() - lastBlinkEventTime > blinkGap)) {
      singleBlinkDetected = true;
      blinkProcessed = true;
      Serial.println("[DEBUG] Single blink detected and flagged.");
    } else if (consecutiveBlinks == 2&&(millis() - lastBlinkEventTime > blinkGap)) {
      doubleBlinkDetected = true;
      blinkProcessed = true;
      Serial.println("[DEBUG] Double blink detected and flagged.");
    } 
   
      if (consecutiveBlinks >= 4 && !emergencyMode && (millis() - lastBlinkEventTime > blinkGap)) {
      quadBlinkDetected = true;
      blinkProcessed = true;
      Serial.println("[DEBUG] Quad blink (emergency) detected and flagged.");
      emergencyMode = true;
      emergencyStartTime = millis();
      Serial.println("EMERGENCY MODE ACTIVATED!");

      emergencyModeCheck(); // added emergency mode check
  }
}
  return consecutiveBlinks;

}



void blinkWifiSetup() {
  Serial.println("inside the wifi setup...");
  
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(BLINK_LED_PIN, OUTPUT);
  pinMode(EMERGENCY_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  // pinMode(NAVIGATION_LED_PIN, OUTPUT); // Removed navigation LED
  pinMode(EMERGENCY_BUTTON_PIN, INPUT_PULLUP); // Emergency reset button
  digitalWrite(BLINK_LED_PIN, LOW);
  digitalWrite(EMERGENCY_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  // digitalWrite(NAVIGATION_LED_PIN, LOW); // Removed navigation LED

  // WiFi/Preferences setup
  prefs.begin("blinkcfg", false);
  EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM for userId only
  userId = readUserIdFromEEPROM();
 // reconnectWiFi();
  
}

void emergencyModeCheck() {
  if (!emergencyMode) return;

  Serial.println("emergencyModeCheck function");

  while (emergencyMode) {
    unsigned long elapsed = millis() - emergencyStartTime;

    // 500ms ON, 500ms OFF
    if ((elapsed % 1000) < 500) {
      digitalWrite(EMERGENCY_LED_PIN, HIGH);
      //tone(BUZZER_PIN, 3000); // 2kHz tone
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(EMERGENCY_LED_PIN, LOW);
      //noTone(BUZZER_PIN);
      digitalWrite(BUZZER_PIN, LOW);
    }

    // Emergency button clears the mode
    if (digitalRead(EMERGENCY_BUTTON_PIN) == HIGH) {
      emergencyMode = false;
      digitalWrite(EMERGENCY_LED_PIN, LOW);
      noTone(BUZZER_PIN);
      Serial.println("EMERGENCY MODE CLEARED!");
    }

    delay(10);  // Prevent ESP32 watchdog reset
  }
}



void blinkWifiLoop() {
  server.begin();
  if (clientConnected && singleBlinkDetected) {
    client.print('1');
  } else if (clientConnected && doubleBlinkDetected) {
    client.print('2');
  } else if (clientConnected && quadBlinkDetected && !emergencyMode) {
    client.print('4');

    // Send emergency notification
    sendNotificationRequest(userId, "EMERGENCY");
  }
  // added emergency 
  emergencyModeCheck(); 

  // Handle new client connection
  if (!clientConnected) {
    client = server.available();
    if (client) {
      loadConfig();
      clientConnected = true;
      // Send config string: minBlinkDuration;blinkInterval;ssid;password;userId (added userId)
      String config = String(blinkDuration) + ";" + String(blinkGap) + ";" + ssid + ";" + password + ";" + userId;
      client.print(config); // No newline
    }
  } else if (!client.connected()) {
    clientConnected = false;
    client.stop();
  }

  // Handle Serial commands
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialCmdBuffer.length() > 0) {
        processCommand(serialCmdBuffer, Serial, false);
        serialCmdBuffer = "";
      }
    } else {
      serialCmdBuffer += c;
    }
  }

  // Handle TCP client commands
  if (clientConnected && client.connected()) {
    while (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        if (wifiCmdBuffer.length() > 0) {
          processCommand(wifiCmdBuffer, client, true);
          wifiCmdBuffer = "";
        }
      } else {
        wifiCmdBuffer += c;
      }
    }
  }
}

bool blinkWifiCheckSingleBlink() {
  if (singleBlinkDetected) {
    singleBlinkDetected = false;
    Serial.println("[DEBUG] Single blink event consumed by GUI.");
    return true;
  }
  return false;
}

bool blinkWifiCheckDoubleBlink() {
  if (doubleBlinkDetected) {
    doubleBlinkDetected = false;
    Serial.println("[DEBUG] Double blink event consumed by GUI.");
    return true;
  }
  return false;
}

// Optionally, for emergency/quad blink
bool blinkWifiCheckQuadBlink() {
  if (quadBlinkDetected) {
    quadBlinkDetected = false;
    Serial.println("[DEBUG] Quad blink event consumed by GUI.");
    return true;
  }
  return false;
}

void processCommand(String cmd, Stream &out, bool fromWifi) {
  cmd.trim();
  cmd.toUpperCase();
  
   if (cmd.startsWith("SET_MINBLINK:")) {
    String val = cmd.substring(13);
    unsigned long v = val.toInt();
    if (v >= 100 && v <= 5000) {
      blinkDuration = v;
      saveBlinkSettingsToPreferences();
      out.print("Min blink duration updated to: "); out.print(blinkDuration); out.print("\n");
    } else {
      out.print("Invalid min blink duration\n");
    }
  } else if (cmd.startsWith("SET_BLINKINT:")) {
    String val = cmd.substring(13);
    unsigned long v = val.toInt();
    if (v >= 200 && v <= 10000) {
      blinkGap = v;
      saveBlinkSettingsToPreferences();
      out.print("Blink interval updated to: "); out.print(blinkGap); out.print("\n");
    } else {
      out.print("Invalid blink interval\n");
    }
  } else if (cmd == "STATUS") {
    sendStatus(out);
  } else {
    out.print("Unknown command\n");
  }
}

void sendStatus(Stream &out) {
  out.print("SSID: "); out.print(ssid); out.print("\n");
  out.print("Password: "); out.print(password); out.print("\n");
  out.print("Min Blink Duration: "); out.print(blinkDuration); out.print("\n");
  out.print("Blink Interval: "); out.print(blinkGap); out.print("\n");
  out.print("WiFi IP: "); out.print(WiFi.localIP()); out.print("\n");
}

void loadConfig() {
  Serial.println("Loading configuration from Preferences...");
  // Load or set defaults
  if (prefs.isKey("ssid")) {
    ssid=prefs.getString("ssid","");
    Serial.print("Loaded SSID: ");
    Serial.println(ssid);
  } else {
    Serial.print("Using default SSID: ");
    Serial.println(ssid);
  }
  if (prefs.isKey("pass")) {
    password=prefs.getString("pass","");
    Serial.println("Loaded password from Preferences");
  } else {
    Serial.println("Using default password");
  }
  blinkDuration = prefs.getULong("blinkDuration", blinkDuration);
  blinkGap = prefs.getULong("blinkGap", blinkGap);
  Serial.print("Min blink duration: ");
  Serial.println(blinkDuration);
  Serial.print("Blink interval: ");
  Serial.println(blinkGap);
}

void saveConfig() {
  prefs.putULong("blinkDuration", blinkDuration);
  prefs.putULong("blinkGap", blinkGap);
}

void reconnectWiFi() {
  Serial.println("Disconnecting from WiFi...");
  WiFi.disconnect();
  delay(100);
  Serial.println("Setting WiFi mode to STA...");
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  unsigned long startAttempt = millis();
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(500);
    attempts++;
    Serial.print("WiFi connection attempt ");
    Serial.print(attempts);
    Serial.print(" - Status: ");
    Serial.println(WiFi.status());
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected to WiFi! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed after 10 seconds.");
    Serial.print("Final WiFi status: ");
    Serial.println(WiFi.status());
  }
} 


void blinkWifiResetFlags() {
  doubleBlinkDetected = false;
  singleBlinkDetected = false;
  quadBlinkDetected = false;
}
