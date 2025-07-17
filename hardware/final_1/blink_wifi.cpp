#include "blink_wifi.h"
#include <Arduino.h>

// Pin assignments (update as needed)
static const int IR_SENSOR_PIN = 36;
static const int BLINK_LED_PIN = 25; // Changed from 19 to 25
static const int EMERGENCY_LED_PIN = 27; // Changed from 2 to 27
static const int BUZZER_PIN = 26;
static const int NAVIGATION_LED_PIN = 14; // Added navigation LED

// Configurable blink detection
static unsigned long minBlinkDuration = 400; // Default is 400 ms
static unsigned long blinkInterval = 1500;    // Default is 800 ms
static const unsigned long DEBOUNCE_DELAY = 50;
static const unsigned long EMERGENCY_TIMEOUT = 7000;

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
    } else if (consecutiveBlinks == 2) {
      doubleBlinkDetected = true;
    } else if (consecutiveBlinks == 4 && !emergencyMode) {
      quadBlinkDetected = true;
      emergencyMode = true;
      emergencyStartTime = millis();
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
    // Reset emergency & blink count if no activity for 7s
    if (millis() - lastBlinkTime > EMERGENCY_TIMEOUT) {
      consecutiveBlinks = 0;
      emergencyMode = false;
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(EMERGENCY_LED_PIN, LOW);
    }
  } else {
    digitalWrite(EMERGENCY_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    // Reset blink count if no activity for 7s
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

// Optionally, for emergency/quad blink
bool blinkWifiCheckQuadBlink() {
  if (quadBlinkDetected) {
    quadBlinkDetected = false;
    return true;
  }
  return false;
} 