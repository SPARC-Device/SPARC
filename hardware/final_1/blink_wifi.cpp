#include "blink_wifi.h"
#include <Arduino.h>

// Pin assignments (update as needed)
static const int IR_SENSOR_PIN = 36;
static const int BLINK_LED_PIN = 25; // Changed from 19 to 25
static const int EMERGENCY_LED_PIN = 27; // Changed from 2 to 27
static const int BUZZER_PIN = 26;
static const int NAVIGATION_LED_PIN = 14; // Added navigation LED

// Configurable blink detection
static unsigned long minBlinkDuration = 400;
static unsigned long blinkInterval = 2000;
static const unsigned long DEBOUNCE_DELAY = 50;
static const unsigned long EMERGENCY_TIMEOUT = 3000;

// State variables
static bool currentEyeState = true;
static bool lastSensorReading = HIGH;
static unsigned long eyeCloseTime = 0;
static unsigned long lastDebounceTime = 0;
static int consecutiveBlinks = 0;
static unsigned long lastBlinkTime = 0;
static bool emergencyMode = false;
static unsigned long emergencyStartTime = 0;

// Blink event flags for GUI
static bool singleBlinkDetected = false;
static bool doubleBlinkDetected = false;

void blinkWifiSetup() {
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(BLINK_LED_PIN, OUTPUT);
  pinMode(EMERGENCY_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(NAVIGATION_LED_PIN, OUTPUT); // Navigation LED
  digitalWrite(BLINK_LED_PIN, LOW);
  digitalWrite(EMERGENCY_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(NAVIGATION_LED_PIN, LOW); // Ensure navigation LED is off at start
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
        if (blinkDuration >= minBlinkDuration) {
          if (millis() - lastBlinkTime < blinkInterval) {
            consecutiveBlinks++;
          } else {
            consecutiveBlinks = 1;  // Reset count if too much time passed
          }
          lastBlinkTime = millis();
          // Set blink event flags for GUI
          if (consecutiveBlinks == 1) singleBlinkDetected = true;
          if (consecutiveBlinks == 2) doubleBlinkDetected = true;
          // Emergency trigger
          if (consecutiveBlinks >= 4 && !emergencyMode) {
            emergencyMode = true;
            emergencyStartTime = millis();
          }
        }
      }
      currentEyeState = eyeOpen;
    }
  }
  lastSensorReading = sensorReading;

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
    // Reset emergency & blink count if no activity for 3s
    if (millis() - lastBlinkTime > EMERGENCY_TIMEOUT) {
      consecutiveBlinks = 0;
      emergencyMode = false;
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(EMERGENCY_LED_PIN, LOW);
    }
  } else {
    digitalWrite(EMERGENCY_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    // Reset blink count if no activity for 3s
    if (millis() - lastBlinkTime > EMERGENCY_TIMEOUT) {
      consecutiveBlinks = 0;
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