#include "blink_wifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "setting_2.h" // For userId
#include "notif.h" // For sendNotificationRequest
#include <EEPROM.h>
// Pin assignments (update as needed)
static const int IR_SENSOR_PIN = 36;
static const int BLINK_LED_PIN = 25; // Changed from 19 to 25
static const int EMERGENCY_LED_PIN = 27; // Changed from 2 to 27
static const int BUZZER_PIN = 26;
// static const int NAVIGATION_LED_PIN = 14; // Removed navigation LED
static const int EMERGENCY_BUTTON_PIN = 12; // Emergency reset button moved to 14 from 22

// Configurable blink detection
static unsigned long minBlinkDuration = 400; // Default is 400 ms
static unsigned long blinkInterval = 1200;    // Default is 1500 ms
static const unsigned long DEBOUNCE_DELAY = 50;
static const unsigned long EMERGENCY_TIMEOUT = 7000;
static unsigned long emergency_blink_interval = 250; // For emergency blink detection

// State variables
static bool currentEyeState = true;
static bool lastSensorReading = HIGH;
static unsigned long eyeCloseTime = 0;
static unsigned long lastDebounceTime = 0;
static int consecutiveBlinks = 0;
static unsigned long lastBlinkTime = 0;
static unsigned long lastBlinkEndTime = 0; // For correct blink gap calculation
static bool emergencyMode = false;
static unsigned long emergencyStartTime = 0;

// Blink event flags for GUI
static bool singleBlinkDetected = false;
static bool doubleBlinkDetected = false;
static bool quadBlinkDetected = false; // For 4 blinks (emergency)

// For deferred blink event processing
static unsigned long lastBlinkEventTime = 0;

// Preferences for persistent storage
Preferences prefs;

// WiFi and TCP server
IPAddress local_IP(192, 168, 1, 13); // Set your static IP
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
WiFiServer server(33333);
WiFiClient client;
bool clientConnected = false;

// Configurable variables (moved from .ino)
char ssid[33] = "Pushpa";
char password[65] = "*#@09password";
// minBlinkDuration and blinkInterval already exist

// Use userId and readUserIdFromEEPROM() as declared in the header.

// Function declarations for WiFi logic
void processCommand(String cmd, Stream &out, bool fromWifi);
void reconnectWiFi();
void loadConfig();
void saveConfig();
void sendStatus(Stream &out);

// Command buffers
String wifiCmdBuffer = "";
String serialCmdBuffer = "";

void blinkWifiSetup() {
  Serial.begin(115200);
  Serial.println("Booting...");
  
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
  EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM
  userId = readUserIdFromEEPROM();
  loadConfig();
  reconnectWiFi();
  server.begin();
}

void blinkWifiLoop() {
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
        unsigned long blinkDuration = millis() - eyeCloseTime;
        digitalWrite(BLINK_LED_PIN, LOW); // Blink LED OFF
        // Use different min duration for emergency blinks
        unsigned long currentMinBlinkDuration = (consecutiveBlinks >= 2) ? emergency_blink_interval : minBlinkDuration;
        if (blinkDuration >= currentMinBlinkDuration) {
          unsigned long blinkGap = 0;
          if (lastBlinkEndTime != 0) {
            blinkGap = millis() - lastBlinkEndTime;
          }
          if (millis() - lastBlinkTime < blinkInterval) {
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
          Serial.print(", Duration: ");
          Serial.print(blinkDuration);
          Serial.print(" ms, Gap: ");
          Serial.print(blinkGap);
          Serial.println(" ms");
        }
      }
      currentEyeState = eyeOpen;
    }
  }
  lastSensorReading = sensorReading;

  // Deferred blink event processing
  if (consecutiveBlinks > 0 && (millis() - lastBlinkEventTime > blinkInterval)) {
    if (consecutiveBlinks == 1) {
      singleBlinkDetected = true;
      if (clientConnected && client.connected()) {
        client.print('1');
      }
    } else if (consecutiveBlinks == 2) {
      doubleBlinkDetected = true;
      if (clientConnected && client.connected()) {
        client.print('2');
      }
    } else if (consecutiveBlinks >= 4 && !emergencyMode) {
      quadBlinkDetected = true;
      if (clientConnected && client.connected()) {
        client.print('4');
      }
      emergencyMode = true;
      emergencyStartTime = millis();
      Serial.println("EMERGENCY MODE ACTIVATED!");
      // Send emergency notification
      sendNotificationRequest(userId, "EMERGENCY");
    }
    // Reset blink count after event
    consecutiveBlinks = 0;
  }

  // Emergency siren logic
  if (emergencyMode) {
    unsigned long elapsed = millis() - emergencyStartTime;
    // Siren pattern: 500ms ON, 500ms OFF
    if ((elapsed % 1000) < 500) {
      digitalWrite(EMERGENCY_LED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(EMERGENCY_LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
    }
    // Emergency mode stays on until button is pressed
    if (digitalRead(EMERGENCY_BUTTON_PIN) == HIGH) { // Button pressed (active low)
      emergencyMode = false;
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(EMERGENCY_LED_PIN, LOW);
      Serial.println("EMERGENCY MODE CLEARED!");
    }
  } else {
    digitalWrite(EMERGENCY_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
  // No more emergency timeout logic

  // Handle new client connection
  if (!clientConnected) {
    client = server.available();
    if (client) {
      clientConnected = true;
      // Send config string: minBlinkDuration;blinkInterval;ssid;password;userId (added userId)
      String config = String(minBlinkDuration) + ";" + String(blinkInterval) + ";" + ssid + ";" + password + ";" + userId;
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
    return true;
  }
  return false;
}

bool blinkWifiCheckDoubleBlink() {
  if (doubleBlinkDetected) {
    doubleBlinkDetected = false;
    return true;
  }
  return false;
}

// Optionally, for emergency/quad blink
bool blinkWifiCheckQuadBlink() {
  if (quadBlinkDetected) {
    quadBlinkDetected = false;
    return true;
  }
  return false;
}

void processCommand(String cmd, Stream &out, bool fromWifi) {
  cmd.trim();
  cmd.toUpperCase();
  if (cmd.startsWith("SET_SSID:")) {
    String newSsid = cmd.substring(9);
    newSsid.trim();
    if (newSsid.length() > 0 && newSsid.length() < 33) {
      newSsid.toCharArray(ssid, 33);
      saveConfig();
      out.print("SSID updated to: "); out.print(ssid);
      out.print("\n");
      reconnectWiFi();
    } else {
      out.print("Invalid SSID length\n");
    }
  } else if (cmd.startsWith("SET_PASS:")) {
    String newPass = cmd.substring(9);
    newPass.trim();
    if (newPass.length() > 0 && newPass.length() < 65) {
      newPass.toCharArray(password, 65);
      saveConfig();
      out.print("Password updated\n");
      reconnectWiFi();
    } else {
      out.print("Invalid password length\n");
    }
  } else if (cmd.startsWith("SET_MINBLINK:")) {
    String val = cmd.substring(13);
    unsigned long v = val.toInt();
    if (v >= 100 && v <= 5000) {
      minBlinkDuration = v;
      saveConfig();
      out.print("Min blink duration updated to: "); out.print(minBlinkDuration); out.print("\n");
    } else {
      out.print("Invalid min blink duration\n");
    }
  } else if (cmd.startsWith("SET_BLINKINT:")) {
    String val = cmd.substring(13);
    unsigned long v = val.toInt();
    if (v >= 200 && v <= 10000) {
      blinkInterval = v;
      saveConfig();
      out.print("Blink interval updated to: "); out.print(blinkInterval); out.print("\n");
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
  out.print("Min Blink Duration: "); out.print(minBlinkDuration); out.print("\n");
  out.print("Blink Interval: "); out.print(blinkInterval); out.print("\n");
  out.print("WiFi IP: "); out.print(WiFi.localIP()); out.print("\n");
}

void loadConfig() {
  Serial.println("Loading configuration from Preferences...");
  // Load or set defaults
  if (prefs.isKey("ssid")) {
    prefs.getString("ssid").toCharArray(ssid, 33);
    Serial.print("Loaded SSID: ");
    Serial.println(ssid);
  } else {
    Serial.print("Using default SSID: ");
    Serial.println(ssid);
  }
  if (prefs.isKey("pass")) {
    prefs.getString("pass").toCharArray(password, 65);
    Serial.println("Loaded password from Preferences");
  } else {
    Serial.println("Using default password");
  }
  minBlinkDuration = prefs.getULong("minblink", minBlinkDuration);
  blinkInterval = prefs.getULong("blinkint", blinkInterval);
  Serial.print("Min blink duration: ");
  Serial.println(minBlinkDuration);
  Serial.print("Blink interval: ");
  Serial.println(blinkInterval);
}

void saveConfig() {
  prefs.putString("ssid", String(ssid));
  prefs.putString("pass", String(password));
  prefs.putULong("minblink", minBlinkDuration);
  prefs.putULong("blinkint", blinkInterval);
}

void reconnectWiFi() {
  Serial.println("Disconnecting from WiFi...");
  WiFi.disconnect();
  delay(100);
  Serial.println("Setting WiFi mode to STA...");
  WiFi.mode(WIFI_STA);
  Serial.println("Configuring static IP: 192.168.1.184");
  WiFi.config(local_IP, gateway, subnet);
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