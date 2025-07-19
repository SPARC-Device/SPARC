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
WiFiServer server(45454);
WiFiClient client;
bool clientConnected = false;
static bool clientFound = false;  // Keep clientFound outside/static so it persists


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

bool isServerAvailable(){  // bool 
  return clientConnected;
}


void blinkWifiSetup() {
  Serial.println("*** INSIDE BLINK WIFI SETUP ***");
  
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
  Serial.println("*** ALL PINS INITIALIZED ***");

  // WiFi/Preferences setup
  Serial.println("*** INITIALIZING PREFERENCES ***");
  prefs.begin("blinkcfg", false);
  Serial.println("*** INITIALIZING EEPROM ***");
  EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM for userId only
  Serial.println("*** READING USER ID FROM EEPROM ***");
  userId = readUserIdFromEEPROM();
  Serial.print("*** USER ID READ: ");
  Serial.print(userId);
  Serial.println(" ***");
  Serial.println("*** RECONNECTING TO WIFI ***");
  reconnectWiFi();
  Serial.println("*** STARTING SERVER ON PORT 45454 ***");
  server.begin();
  Serial.println("*** SERVER STARTED SUCCESSFULLY ON PORT 45454 ***");
  
}

void emergencyModeCheck() {
  if (!emergencyMode) return;

  sendNotificationRequest(userId, "EMERGENCY");

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
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("EMERGENCY MODE CLEARED!");
    }

    delay(10);  // Prevent ESP32 watchdog reset
  }
}



void blinkWifiLoop() {
    
    if (WiFi.status() == WL_CONNECTED) {
        // Only do the 5-second wait if we haven't found a client yet
        if (!clientFound && !clientConnected) {
            Serial.println("*** WAITING 5 SECONDS FOR CLIENT CONNECTIONS ***");
            unsigned long waitStart = millis();
            
            while (millis() - waitStart < 5000 && !clientFound) {
                Serial.println("*** CHECKING FOR AVAILABLE CLIENT ***");
                WiFiClient tempClient = server.available();
                
                if (tempClient) {
                    Serial.println("*** SERVER.AVAILABLE() RETURNED A CLIENT! ***");
                    Serial.print("Client valid: ");
                    Serial.println(tempClient ? "YES" : "NO");
                    Serial.print("Client connected(): ");
                    Serial.println(tempClient.connected());
                    Serial.print("Client available(): ");
                    Serial.println(tempClient.available());
                    
                    // Accept the client regardless of connected() status
                    client = tempClient;
                    Serial.println("Client assigned to global variable");
                    
                    // ADD THIS DELAY - Give client time to be ready
                    Serial.println("*** WAITING 500ms FOR CLIENT TO BE READY ***");
                    delay(500);  // 500ms delay
                    Serial.println("*** DELAY COMPLETED ***");
                    
                    Serial.print("Global client connected(): ");
                    Serial.println(client.connected());
                    Serial.print("Global client available(): ");
                    Serial.println(client.available());
                    
                    loadConfig();
                    Serial.println("Config loaded");
                    
                    clientConnected = true;
                    clientFound = true;
                    Serial.println("*** CLIENT CONNECTED FLAG SET TO TRUE ***");
                    
                    String config = String(blinkDuration) + ";" + String(blinkGap) + ";" + ssid + ";" + password + ";" + userId;
                    Serial.print("Config string to send: '");
                    Serial.print(config);
                    Serial.println("'");
                    Serial.print("Config string length: ");
                    Serial.println(config.length());
                    
                    Serial.println("*** ATTEMPTING TO SEND CONFIG DATA ***");
                    size_t bytesWritten = client.print(config);
                    Serial.print("Bytes written to client: ");
                    Serial.println(bytesWritten);
                    
                    if (bytesWritten > 0) {
                        Serial.println("*** CONFIG SENT SUCCESSFULLY! ***");
                    } else {
                        Serial.println("*** ERROR: NO BYTES WRITTEN! ***");
                        Serial.print("Client still connected: ");
                        Serial.println(client.connected());
                        Serial.print("Client available: ");
                        Serial.println(client.available());
                    }
                    
                    // Try to flush the data
                    Serial.println("*** FLUSHING CLIENT BUFFER ***");
                    client.flush();
                    Serial.println("Client buffer flushed");
                    Serial.println("*** CLIENT CONNECTED! ***");
                    
                    break; // Exit the wait loop
                } else {
                    Serial.println("*** NO CLIENT AVAILABLE YET ***");
                }
                delay(100); // Small delay to prevent busy waiting
            }
            
            if (!clientFound) {
                Serial.println("*** 5-SECOND WAIT PERIOD COMPLETED - NO CLIENT CONNECTED ***");
            }
        }
        
        // Continue with rest of your WiFi loop code...
        Serial.println("*** INSIDE MAIN WIFI LOOP ***");
        
        if (clientConnected && singleBlinkDetected) {
            Serial.println("*** SENDING SINGLE BLINK TO CLIENT ***");
            client.print('1');
        } else if (clientConnected && doubleBlinkDetected) {
            Serial.println("*** SENDING DOUBLE BLINK TO CLIENT ***");
            client.print('2');
        } else if (clientConnected && quadBlinkDetected && !emergencyMode) {
            Serial.println("*** SENDING QUAD BLINK TO CLIENT ***");
            client.print('4');
        }
        
        // added emergency 
        emergencyModeCheck(); 
        Serial.println("*** CHECKING FOR CLIENT CONNECTION STATUS ***");
        
        // Handle new client connection (only if we haven't found one yet)
        if (!clientConnected && !clientFound) {
            Serial.println("=== CHECKING FOR NEW CLIENT ===");
            WiFiClient tempClient = server.available();
            
            if (tempClient) {
                // Same client handling code as above - you could extract this to a separate function
                Serial.println("*** SERVER.AVAILABLE() RETURNED A CLIENT! ***");
                // ... (same client handling code)
                clientFound = true;
                clientConnected = true;
            } else {
                // Only print this occasionally to avoid spam
                static unsigned long lastNoClientMsg = 0;
                if (millis() - lastNoClientMsg > 5000) {  // Every 5 seconds
                    Serial.println("*** NO CLIENT AVAILABLE ***");
                    lastNoClientMsg = millis();
                }
            }
        }
    } else {
        Serial.println("*** WIFI NOT CONNECTED - CANNOT HANDLE CLIENTS ***");
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
