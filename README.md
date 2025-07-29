# SPARC

**SPARC** (Smart Paralysis Assistance through Responsive Communication) is an open-source assistive technology platform designed for people who are non-verbal and immobile. By detecting intentional blinks using IR sensors, SPARC enables users to communicate basic needs, emergencies, and messages via a TFT display and WiFi-connected notification system. This is our submission for the IEEE SSCS 2025 Arduino Competition.

The complete system integrates:
- **SPARC-Device** (ESP32-based blink detection and TFT user interface)
- **SPARC-GUI** (Python desktop application for configuration and monitoring)
- **SPARC-Notify** (Android app for caretakers to receive instant notifications)

---

## System Architecture

### 1. SPARC Device (ESP32)

- **Blink Detection:** IR sensors detect user blinks, which are interpreted as navigation/selection commands.
- **TFT Display Interface:** Users navigate a grid (T9-style, icons, and emojis) using blinks to select messages (e.g., Emergency, Help, Food, Restroom).
- **WiFi Communication:** Device connects to WiFi and communicates with a proxy server (`notif-server`) to send notifications.
- **Settings Management:** Blink duration/gap, WiFi credentials, and UserID are configurable and saved in EEPROM.
- **Emoji & Text Messaging:** Quickly send pre-defined text or emoji messages to caretakers.

### 2. SPARC-GUI (Python)

- **Final Vision GUI:** Desktop application mirroring the device's T9 grid, allowing configuration of blink settings.
- **Accessibility:** Real-time visualization, text display, customizable parameters and Text-to-Speech (TTS) for optimal user experience.

### 3. SPARC-Notify (Android/Kotlin)

- **Caretaker Notifications:** Receives categorized, high-priority alerts (Emergency, Help, Food, Restroom) from patients.
- **Patient Code Subscription:** Caretakers subscribe to 5-character patient codes and select notification types to receive.
- **Emergency Alerts:** Dedicated notification channel with unique sound, vibration, and lock-screen visibility for emergencies.
- **Local Preferences:** Notification filters and codes are saved locally for privacy and reliability.

---

## Communication Flow

1. **User blinks** to navigate/select a need or emergency message on the TFT display.
2. **Device sends a notification** over WiFi to a proxy server (`notif-server`).
3. **Caregiver’s phone** (running SPARC-Notify) receives the alert instantly, with priority and filtering.

---

## Features

- **Assistive Communication:** Enables immobile/non-verbal users to communicate basic needs and emergencies.
- **Customizable Sensitivity:** Blink detection parameters (duration/gap/sensitivity) and WiFi settings are user-tunable.
- **Multi-modal Output:** TFT display shows text and emojis; notifications sent to mobile devices.
- **Secure & Private:** No personal data transmitted; only non-identifying patient codes used.
- **Caregiver Integration:** Real-time, filtered, high-priority notifications for efficient response.

---

## Hardware Requirements

- ESP32 – Main microcontroller, handles all processing and connectivity.

- TFT Display – For GUI navigation and message selection.

- IR Sensor – Detects user blinks, mounted on spectacles.

- Spectacle Frame – Holds the IR sensor for eye-level blink detection.

- Buzzer – Triggers emergency alarms when needed.

- Red LED – Indicates emergency mode status.

- Blink LED – Blinks on every detected eye blink.

- DFPlayer MP3 Module with SD Card – Stores and plays audio files for message feedback.

- Speaker – Outputs audio from DFPlayer.

- Push Button – Used to manually turn off emergency mode.

- Resistor – Pull-up resistor for stable push button input.

- Power Supply: Standard USB or battery power

<p align="center">
  <img src="./assets/schematic-diagram.jpg" width="400"/>
</p>

---

## Software Requirements

- **SPARC-DEVICE:** C++/C Arduino firmware (in `SPARC-DEVICE/`)
- **SPARC-GUI:** Python 3.x, PyQt5, pyttsx3
- **SPARC-Notify:** Android app (Kotlin), Firebase Cloud Messaging
- **notif-server:** Proxy server for message routing (setup instructions forthcoming)

---

## Getting Started

### Device Setup

1. **Hardware assembly:** Connect IR sensor and TFT display to ESP32/Arduino.
2. **Flash firmware:** Use Arduino IDE to upload the code from `SPARC-DEVICE/`.
3. **Configure WiFi & Blink Settings:** Set SSID, password, blink duration/gap via TFT or SPARC-GUI.

---

## Repository Structure

- `SPARC-DEVICE/` : Arduino firmware, blink detection, TFT logic, WiFi comms.
  - `include/emoji/` : Emoji arrays for TFT.
  - `include/common_variables.h` : Shared variables (WiFi, blink settings).
  - `src/settings/` : Settings and EEPROM logic.
- `SPARC-GUI/` : Python desktop GUI.
- `SPARC-Notify/` : Android app for notifications.

---

## License

This project is licensed under the GNU General Public License v3.0.

---

## Team Members

- Praful Kumar Singh
- Gorang Singh
- Aaradhya Bhardwaj
