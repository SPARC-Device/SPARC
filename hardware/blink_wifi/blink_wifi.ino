// IR Eye Blink Detection System for ESP32
// Author: Gorang Singh 
// Features: 4-blink emergency, blink LED, emergency LED, buzzer siren, one-time HELP message

#include <WiFi.h>
#include <Preferences.h>

// Preferences for persistent storage
Preferences prefs;

// Default values
#define DEFAULT_SSID "Pushpa"
#define DEFAULT_PASS "*#@09password"
#define DEFAULT_MIN_BLINK_DURATION 400
#define DEFAULT_BLINK_INTERVAL 2000

// Configurable variables
char ssid[33];
char password[65];
unsigned long minBlinkDuration;
unsigned long blinkInterval;

// GPIO Pin Definitions
const int IR_SENSOR_PIN = 36;
const int BUZZER_PIN = 26;
const int BLINK_LED_PIN = 19;
const int EMERGENCY_LED_PIN = 2;

// Timing Constants (non-configurable)
const unsigned long DEBOUNCE_DELAY = 50;        // Debounce delay for IR sensor
const unsigned long EMERGENCY_TIMEOUT = 3000;   // Timeout to reset emergency/blinks in ms

// WiFi and TCP server
IPAddress local_IP(192, 168, 1, 184); // Set your static IP
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
WiFiServer server(3333);
WiFiClient client;
bool clientConnected = false;

// State Variables
bool currentEyeState = true;
bool lastSensorReading = HIGH;
unsigned long eyeCloseTime = 0;
unsigned long lastDebounceTime = 0;

int consecutiveBlinks = 0;
unsigned long lastBlinkTime = 0;
bool emergencyMode = false;
unsigned long emergencyStartTime = 0;
bool helpMessageSent = false;

// Function declarations
void handleEyesClosed();
void handleEyesOpened();
void triggerEmergency();
void handleEmergencySignals();
void processCommand(String cmd, Stream &out, bool fromWifi);
void reconnectWiFi();
void loadConfig();
void saveConfig();
void sendStatus(Stream &out);

void setup() {
  Serial.begin(115200);

  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BLINK_LED_PIN, OUTPUT);
  pinMode(EMERGENCY_LED_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(BLINK_LED_PIN, LOW);
  digitalWrite(EMERGENCY_LED_PIN, LOW);

  // Preferences init and load
  prefs.begin("blinkcfg", false);
  loadConfig();

  Serial.println("=== IR Eye Blink Detection System (ESP32) ===");
  Serial.println("-> 4 valid blinks trigger emergency");
  Serial.println("-> Blink LED ON when eye is closed");
  Serial.println("-> Emergency LED & Buzzer flash in siren mode");
  Serial.println("-> HELP message sent once during emergency");
  Serial.println("-> Send commands via Serial or TCP client");
  Serial.println("Commands: SET_SSID:<ssid>, SET_PASS:<pass>, SET_MINBLINK:<ms>, SET_BLINKINT:<ms>, STATUS");

  // WiFi setup
  reconnectWiFi();
  server.begin();
  Serial.println("TCP server started on port 3333");
}

String wifiCmdBuffer = "";
String serialCmdBuffer = "";

void loop() {
  // Handle new client connection
  if (!clientConnected) {
    client = server.available();
    if (client) {
      clientConnected = true;
      // Send config string: minBlinkDuration;blinkInterval;ssid;password (no newline)
      String config = String(minBlinkDuration) + ";" + String(blinkInterval) + ";" + ssid + ";" + password;
      client.print(config); // No newline
      Serial.print("Sent config to client: ");
      Serial.println(config);
      Serial.println("[INFO] Client connected.");
    }
  } else if (!client.connected()) {
    clientConnected = false;
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("[INFO] Client disconnected.");
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
        handleEyesClosed();
      } else {
        // Eye just opened
        handleEyesOpened();
      }
      currentEyeState = eyeOpen;
    }
  }

  lastSensorReading = sensorReading;

  // Handle emergency siren if active
  if (emergencyMode) {
    handleEmergencySignals();
  }

  // Reset emergency & blink count if no activity for 3s
  if (millis() - lastBlinkTime > EMERGENCY_TIMEOUT && consecutiveBlinks > 0) {
    consecutiveBlinks = 0;
    if (emergencyMode) {
      emergencyMode = false;
      helpMessageSent = false;
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(EMERGENCY_LED_PIN, LOW);
      Serial.println("Emergency mode deactivated.");
    }
  }

  delay(10);
}

void handleEyesClosed() {
  Serial.println("Eyes: CLOSED");
  digitalWrite(BLINK_LED_PIN, HIGH); // Blink LED ON
}

void handleEyesOpened() {
  unsigned long blinkDuration = millis() - eyeCloseTime;
  Serial.println("Eyes: OPEN");
  digitalWrite(BLINK_LED_PIN, LOW); // Blink LED OFF

  if (blinkDuration >= minBlinkDuration) {
    Serial.print("Blink duration: ");
    Serial.print(blinkDuration);
    Serial.println(" ms - VALID");

    if (millis() - lastBlinkTime < blinkInterval) {
      consecutiveBlinks++;
    } else {
      consecutiveBlinks = 1;  // Reset count if too much time passed
    }

    lastBlinkTime = millis();
    Serial.print("Consecutive blinks: ");
    Serial.println(consecutiveBlinks);

    // Send blink count to client (1, 2, or 4 only)
    if (clientConnected && client.connected()) {
      if (consecutiveBlinks == 1) {
        client.print(1);
      } else if (consecutiveBlinks == 2) {
        client.print(2);
      } else if (consecutiveBlinks == 4) {
        client.print(4);
      }
    }

    if (consecutiveBlinks >= 4 && !emergencyMode) {
      triggerEmergency();
    }
  } else {
    Serial.print("Blink duration: ");
    Serial.print(blinkDuration);
    Serial.println(" ms - FALSE BLINK (ignored)");
  }
}

void triggerEmergency() {
  emergencyMode = true;
  emergencyStartTime = millis();
  helpMessageSent = false;

  Serial.println("!!! EMERGENCY DETECTED !!!");
  Serial.println("4 valid blinks detected! Activating signals...");

  // Send HELP only once here
  if (!helpMessageSent) {
    Serial.println("HELP! Emergency condition detected!");
    helpMessageSent = true;
  }
}

void handleEmergencySignals() {
  unsigned long elapsed = millis() - emergencyStartTime;

  // Siren pattern: 500ms ON, 500ms OFF
  if ((elapsed % 1000) < 500) {
    digitalWrite(EMERGENCY_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(EMERGENCY_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
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
      Serial.print("[UPDATE] SSID updated to: "); Serial.println(ssid);
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
      Serial.println("[UPDATE] Password updated.");
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
      Serial.print("[UPDATE] Min blink duration updated to: "); Serial.println(minBlinkDuration);
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
      Serial.print("[UPDATE] Blink interval updated to: "); Serial.println(blinkInterval);
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
  // Load or set defaults
  if (prefs.isKey("ssid")) {
    prefs.getString("ssid").toCharArray(ssid, 33);
  } else {
    strncpy(ssid, DEFAULT_SSID, 33);
  }
  if (prefs.isKey("pass")) {
    prefs.getString("pass").toCharArray(password, 65);
  } else {
    strncpy(password, DEFAULT_PASS, 65);
  }
  minBlinkDuration = prefs.getULong("minblink", DEFAULT_MIN_BLINK_DURATION);
  blinkInterval = prefs.getULong("blinkint", DEFAULT_BLINK_INTERVAL);
}

void saveConfig() {
  prefs.putString("ssid", String(ssid));
  prefs.putString("pass", String(password));
  prefs.putULong("minblink", minBlinkDuration);
  prefs.putULong("blinkint", blinkInterval);
}

void reconnectWiFi() {
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed.");
  }
}