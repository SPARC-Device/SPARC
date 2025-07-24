#include "settings.h"

#include "../../include/common_variables.h"

#include "../network/blink_wifi.h"

#include <EEPROM.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <SPI.h>


extern Preferences prefs;
extern int uiState;
extern void gui3Setup();

// --- Forward declarations for T9 helpers ---
void drawMessageBar(const String& msg);
void drawT9Cell(int index, bool highlightYellow, bool blinkMode);
void drawPopupBar();
void clearPopupBar();
void drawValidBlinkT9EditScreen();
void drawConsecutiveGapT9EditScreen();

// --- Popup state for T9 keyboard in settings edit screen (move to top for visibility) ---
int popupCountEdit = 0;
String lastPopupCharsEdit[6];
int popupXPositionsEdit[6];
int popupWidthEdit = 50;
int popupSpacingEdit = 5;
int popupBarYEdit = 130;
int popupBarHeightEdit = 30;
bool popupActiveEdit = false;
int popupIndexEdit = 0;
unsigned long popupStartTimeEdit = 0;
const unsigned long popupTimeoutEdit = 3000;
void setupPopupEdit(int index);
void drawPopupEdit();
void clearPopupTextEdit();

// All global variables and functions from setting_2.ino
#define EEPROM_SIZE 200
#define SSID_ADDR 0
#define PASS_ADDR 64
#define USERID_ADDR 140 // EEPROM address for userId (5 chars + null)

TFT_eSPI tft = TFT_eSPI();

bool inEditScreen = false;
int blinkEditField = 0; // 0: minBlinkDuration, 1: consecutiveGap

// Add new variables for edit mode
String editHeading = "";
bool popupActive = false;
int selectedT9Key = -1;
int selectedPopupLetter = -1;
int settingsUiState = 0; // 0 = main menu, 1 = wifi, 2 = blink, 3 = edit, etc.
String userId = ""; // Only define here


const int popupBarY = 130;
const int popupBarHeight = 30;

const char* labels[12] = {
  "1 ABC", "2 DEF", "3 GHI",
  "4 JKL", "5 MNO", "6 PQR",
  "7 STU", "8 VWX", "9 YZ",
  "SAVE", "0 _<", "CLEAR"
};

unsigned long popupStartTime = 0;

// Add missing variable definitions
String prevssid;
String prevpassword;
unsigned long prevBlinkDuration = 400;
unsigned long prevBlinkGap = 1200;

// Add a flag to track if the T9 keyboard is active in edit mode
bool editKeyboardActive = false;
int editSelectedT9Cell = -1; // -1 means no cell selected

// Add a flag to track minimal edit mode (for WiFi Name/Password)
bool minimalEditMode = false;

// Add a variable to store the previous WiFi name before editing
String prevSSIDBeforeEdit = "";

// Add a flag and buffer for WiFi Password T9 edit mode
bool wifiPassT9EditMode = false;

// Add a flag and buffer for Valid Blink T9 edit mode
bool validBlinkT9EditMode = false;

// Add a flag and buffer for Consecutive Gap T9 edit mode
bool consecutiveGapT9EditMode = false;

// Add debounce flags for blink T9 keyboard
bool blinkT9TouchActive = false;
int blinkT9LastCell = -1;

// --- Add new labels for blink settings T9 keyboard ---
const char* blinkLabels[12] = {
  "1", "2", "3",
  "4", "5", "6",
  "7", "8", "9",
  "SAVE", "0_<", "CLEAR"
};

String trimString(const String& str) {   //triming 
  int start = 0;
  int end = str.length() - 1;
  while (start <= end && str[start] == ' ') start++;
  while (end >= start && str[end] == ' ') end--;
  return str.substring(start, end + 1);
}


String removeAllSpaces(const String& str) {  // remove all spaces
  String out = "";
  for (int i = 0; i < str.length(); ++i) {
    if (str[i] != ' ') out += str[i];
  }
  return out;
}

// Draw minimal edit screen: heading, message box, T9 keyboard only
void drawMinimalEditScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(15, 10);
    tft.print(editHeading);
    // Draw Back button (top left)
    tft.fillRect(10, 10, 60, 40, TFT_DARKGREY);
    tft.drawRect(10, 10, 60, 40, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(25, 25);
    tft.print("Back");
    // Message box
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawRect(15, 80, 290, 40, TFT_WHITE);
    tft.fillRect(16, 81, 288, 38, TFT_BLACK);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(20, 100);
    tft.print(ssid); // Changed from editBuffer to ssid
    // T9 grid
    for (int i = 0; i < 12; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = 15 + col * (90 + 10);
        int y = 160 + row * (60 + 10);
        uint16_t keyColor = (editSelectedT9Cell == i) ? TFT_YELLOW : TFT_DARKGREY;
        tft.fillRect(x, y, 90, 60, keyColor);
        tft.drawRect(x, y, 90, 60, TFT_WHITE);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(2);
        int textWidth = tft.textWidth(labels[i]);
        tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
        tft.print(labels[i]);
    }
    if (popupActiveEdit) drawPopupEdit();
}

// Add Preferences-based save/load for WiFi credentials
void saveWiFiToPreferences() {
    // Trim SSID and password before saving
    Serial.println("saveWiFiToPreferences function called");
    String trimmedSSID = trimString(ssid);
    String trimmedPassword = removeAllSpaces(password);
    prefs.begin("blinkcfg", false);
    prefs.putString("ssid", trimmedSSID);
    prefs.putString("pass", trimmedPassword);
    prefs.end();

    Serial.print("trimmedSSID:"); // debug
    Serial.println(trimmedSSID);
    Serial.print("trimmedPassword:"); // debug
    Serial.println(trimmedPassword);
    Serial.print("wifi prefrences saved"); // debug
    // Update the variables everywhere
    ssid = trimmedSSID;
    password = trimmedPassword;

    Serial.println("reconnecting to wifi");
}

void loadWiFiFromPreferences() {
    prefs.begin("blinkcfg", false);
    String loadedSSID = prefs.getString("ssid", "");
    String loadedPassword = prefs.getString("pass", "");
    prefs.end();
    // Trim after loading
    ssid = trimString(loadedSSID);
    password = removeAllSpaces(loadedPassword);
}

void saveBlinkSettingsToPreferences() {
  if (blinkDuration < 100) blinkDuration = 100;
  if (blinkDuration > 2000) blinkDuration = 2000;
  if (blinkGap < 500) blinkGap = 500;
  if (blinkGap > 5000) blinkGap = 5000;
  prefs.begin("blinkcfg", false);
  prefs.putULong("blinkDuration", blinkDuration);
  prefs.putULong("blinkGap", blinkGap);
  prefs.end();
}

void loadBlinkSettingsFromPreferences() {
  prefs.begin("blinkcfg", false);
  blinkDuration = prefs.getULong("blinkDuration", 400);
  blinkGap = prefs.getULong("blinkGap", 1200);
  prefs.end();
  if (blinkDuration < 100) blinkDuration = 100;
  if (blinkDuration > 2000) blinkDuration = 2000;
  if (blinkGap < 500) blinkGap = 500;
  if (blinkGap > 5000) blinkGap = 5000;
}


String getEditHeading(int field) {
  switch (field) {
    case 0: return "WIFI-NAME";
    case 1: return "WIFI-PASSWORD";
    case 2: return "Valid Blink";
    case 3: return "Consecutive Blink Gap";
    default: return "";
  }
}

void drawValueBar() {
  tft.setTextColor(TFT_WHITE);
  tft.drawRect(15, 50, 290, 40, TFT_WHITE);
  tft.fillRect(16, 51, 288, 38, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  if (blinkEditField == 0 || blinkEditField == 1) tft.setCursor(23, 70);
  else tft.setCursor(23, 70);
  tft.print(ssid); // Changed from editBuffer to ssid
}

// Remove the old clearPopupBar function (the one using popupBarY = 100, popupBarHeight = 40)
// Only keep the new clearPopupBar for the T9 keyboard

void drawT9Grid() {
  int t9Y = 160;
  for (int i = 0; i < 12; i++) {
    int col = i % 3;
    int row = i / 3;
    int x = 15 + col * (90 + 10);
    int y = t9Y + row * (60 + 10);
    uint16_t keyColor = (selectedT9Key == i) ? TFT_YELLOW : TFT_DARKGREY;
    tft.fillRect(x, y, 90, 60, keyColor);
    tft.drawRect(x, y, 90, 60, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    int textWidth = tft.textWidth(labels[i]);
    tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
    tft.print(labels[i]);
  }
}

void drawMainMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  int segmentHeight = 480 / 3;
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(50, segmentHeight/2 - 16);
  tft.print("WiFi Settings ->");
  tft.drawRect(10, 10, 300, segmentHeight-20, TFT_WHITE);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(50, segmentHeight + segmentHeight/2 - 16);
  tft.print("Blink Settings ->");
  tft.drawRect(10, segmentHeight+10, 300, segmentHeight-20, TFT_WHITE);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 2*segmentHeight + 20);
  tft.print("User ID");
  tft.drawRect(10, 2*segmentHeight+40, 300, 40, TFT_WHITE);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(20, 2*segmentHeight+50);
  tft.print(userId);
  int btnY = 420;
  int btnW = 120;
  int btnH = 40;
  tft.setTextColor(TFT_WHITE);
  tft.fillRect(30, btnY, btnW, btnH, TFT_DARKGREY);
  tft.drawRect(30, btnY, btnW, btnH, TFT_WHITE);
  tft.setCursor(60, btnY+12);
  tft.print("Save");
  tft.fillRect(170, btnY, btnW, btnH, TFT_DARKGREY);
  tft.drawRect(170, btnY, btnW, btnH, TFT_WHITE);
  tft.setCursor(200, btnY+12);
  tft.print("Cancel");
  // Store previous values for Save/Cancel
  // prevssid = ssid;
  // prevpassword = password;
  // prevBlinkDuration = blinkDuration;
  // prevBlinkGap = blinkGap;
  // Initialize staging variables
  // editBlinkDuration = blinkDuration; // Removed
  // editBlinkGap = blinkGap; // Removed
}

// Add missing function definitions from setting_2.ino
void drawWiFiMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(15, 10);
  tft.print("WiFi Settings");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(15, 50);
  tft.print("Name");
  tft.drawRect(15, 80, 290, 40, TFT_WHITE);
  tft.fillRect(16, 81, 288, 38, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(20, 100);
  tft.print(ssid);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(15, 140);
  tft.print("Password");
  tft.drawRect(15, 170, 290, 40, TFT_WHITE);
  tft.fillRect(16, 171, 288, 38, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(20, 190);
  tft.print(password);
  tft.setTextColor(TFT_YELLOW);
  tft.drawRect(10, 370, 100, 40, TFT_WHITE);
  tft.setCursor(30, 380);
  tft.print("Back");
}

void drawBlinkMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(15, 10);
  tft.print("Blink Settings");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(15, 50);
  tft.print("Valid Blink");
  int barX = 70;
  int barY1 = 80;
  int barW = 180;
  int barH = 40;
  tft.drawRect(barX, barY1, barW, barH, TFT_WHITE);
  tft.fillRect(barX+1, barY1+1, barW-2, barH-2, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(barX+10, barY1+20-8);
  tft.print(String(blinkDuration));
  tft.fillRect(barX-40, barY1, 35, barH, TFT_RED);
  tft.drawRect(barX-40, barY1, 35, barH, TFT_WHITE);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(barX-30, barY1+8);
  tft.print("-");
  tft.fillRect(barX+barW+5, barY1, 35, barH, TFT_GREEN);
  tft.drawRect(barX+barW+5, barY1, 35, barH, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(barX+barW+15, barY1+8);
  tft.print("+");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(15, 140);
  tft.print("Consecutive Gap");
  int barY2 = 170;
  tft.drawRect(barX, barY2, barW, barH, TFT_WHITE);
  tft.fillRect(barX+1, barY2+1, barW-2, barH-2, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(barX+10, barY2+20-8);
  tft.print(String(blinkGap));
  tft.fillRect(barX-40, barY2, 35, barH, TFT_RED);
  tft.drawRect(barX-40, barY2, 35, barH, TFT_WHITE);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(barX-30, barY2+8);
  tft.print("-");
  tft.fillRect(barX+barW+5, barY2, 35, barH, TFT_GREEN);
  tft.drawRect(barX+barW+5, barY2, 35, barH, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(barX+barW+15, barY2+8);
  tft.print("+");
  int noteY = 280;
  tft.setTextColor(TFT_YELLOW);
  tft.drawRect(10,noteY-20,300,100,TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(35, noteY-5);
  tft.print("Default Settings");
  tft.setCursor(15, noteY + 25);
  tft.setTextColor(TFT_WHITE);
  tft.print("Valid Blink : 400ms");
  tft.setCursor(15, noteY + 55);
  tft.print("Consecutive Gap : 1200ms");
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.drawRect(10, 370, 100, 40, TFT_WHITE);
  tft.setCursor(30, 380);
  tft.print("Back");
}


// Add forward declaration for drawEditScreen
void drawEditScreen();

// Draw WiFi Name T9 edit interface
void drawWifiNameT9EditScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(15, 10);
    tft.print("WIFI-NAME");
    drawMessageBar(ssid);
    clearPopupBar();
    if (popupActiveEdit) drawPopupBar();
    for (int i = 0; i < 12; i++) drawT9Cell(i, false);
}

// Add a flag for WiFi Name T9 edit mode
bool wifiNameT9EditMode = false;

// Draw WiFi Password T9 edit interface
void drawWifiPassT9EditScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(15, 10);
    tft.print("WIFI-PASSWORD");
    drawMessageBar(password);
    clearPopupBar();
    if (popupActiveEdit) drawPopupBar();
    for (int i = 0; i < 12; i++) drawT9Cell(i, false);
}

void handleTouch() {
  uint16_t tx, ty;
  // WiFi Name T9 edit mode: handle first so it takes priority
  if (wifiNameT9EditMode) {
    if (tft.getTouch(&tx, &ty)) {
      // T9 grid area
      int t9Y = 160;
      int prevCell = editSelectedT9Cell;
      for (int i = 0; i < 12; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = 15 + col * (90 + 10);
        int y = t9Y + row * (60 + 10);
        if (tx >= x && tx <= x+90 && ty >= y && ty <= y+60) {
          // Handle special cells
          if (i == 9) { // SAVE for wifi name 
            // Draw green background and black text for 100ms
            tft.fillRect(x, y, 90, 60, TFT_GREEN);
            tft.drawRect(x, y, 90, 60, TFT_WHITE);
            tft.setTextColor(TFT_BLACK);
            tft.setTextSize(2);
            int textWidth = tft.textWidth(labels[i]);
            tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
            tft.print(labels[i]);
            delay(100);
           
            drawT9Cell(i, false);

            ssid = trimString(ssid); // added this line 

            Serial.print("prevssid:"); // debug
            Serial.println(prevssid);
            Serial.print("ssid:"); 
            Serial.println(ssid);

            wifiNameT9EditMode = false;
            editSelectedT9Cell = -1;
            popupActiveEdit = false;
            drawWiFiMenu();
            return;
          } else if (i == 10) { // 0 _<
            if (prevCell != -1 && prevCell != i) drawT9Cell(prevCell, false);
            editSelectedT9Cell = i;
            drawT9Cell(i, true);
            setupPopupEdit(i); // This will set up "0", "_", "<"
            if (popupCountEdit > 0) {
              popupActiveEdit = true;
              popupIndexEdit = -1;
              popupStartTimeEdit = millis();
              drawPopupBar();
            }
            return;
          } else if (i == 11) { // CLEAR
            // Draw red background for 100ms
            tft.fillRect(x, y, 90, 60, TFT_RED);
            tft.drawRect(x, y, 90, 60, TFT_WHITE);
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(2);
            int textWidth = tft.textWidth(labels[i]);
            tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
            tft.print(labels[i]);
            delay(100);
            drawT9Cell(i, false);
            ssid = "";
            drawMessageBar(ssid);
            return;
          } else {
            // Normal T9 cell: highlight and show popup
            if (prevCell != -1 && prevCell != i) drawT9Cell(prevCell, false);
            editSelectedT9Cell = i;
            drawT9Cell(i, true);
            setupPopupEdit(i);
            if (popupCountEdit > 0) {
              popupActiveEdit = true;
              popupIndexEdit = -1;
              popupStartTimeEdit = millis();
              drawPopupBar();
            }
            return;
          }
        }
      }
      // Popup bar area (fixed position)
      int popupBarYFixed = 110;
      int popupBarHeightFixed = 40;
      int totalWidth = popupCountEdit * popupWidthEdit + (popupCountEdit - 1) * popupSpacingEdit;
      int popupX = (320 - totalWidth) / 2;
      for (int i = 0; i < popupCountEdit; i++) {
        int px = popupX + i * (popupWidthEdit + popupSpacingEdit);
        if (tx >= px && tx <= px+popupWidthEdit && ty >= popupBarYFixed && ty <= popupBarYFixed+popupBarHeightFixed) {
          String sel = lastPopupCharsEdit[i];
          tft.fillRect(px, popupBarYFixed, popupWidthEdit, popupBarHeightFixed, TFT_BLACK);
          for (int t = 0; t < 3; ++t) tft.drawRect(px + t, popupBarYFixed + t, popupWidthEdit - 2 * t, popupBarHeightFixed - 2 * t, TFT_GREEN);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextSize(2);
          int tw = tft.textWidth(sel);
          int tx2 = px + (popupWidthEdit - tw) / 2;
          int ty2 = popupBarYFixed + (popupBarHeightFixed / 2) - 6;
          tft.setCursor(tx2, ty2);
          tft.print(sel);
          delay(120);
          if (ssid.length() < 24 || sel == "<") {
            if (sel == "<") {
              if (ssid.length()) ssid.remove(ssid.length()-1);
            } else if (sel == "_") {
              ssid += " ";
            } else if (sel == "0") {
              ssid += "0";
            } else {
              ssid += sel;
            }
          }
          popupActiveEdit = false;
          if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false);
          editSelectedT9Cell = -1;
          clearPopupBar();
          drawMessageBar(ssid);
          return;
        }
      }
      // If touch outside popup, close popup and deselect cell
      if (popupActiveEdit) {
        popupActiveEdit = false;
        if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false);
        editSelectedT9Cell = -1;
        clearPopupBar();
      }
    }
    // --- Popup timeout logic ---
    if (popupActiveEdit && (millis() - popupStartTimeEdit > popupTimeoutEdit)) {
      popupActiveEdit = false;
      if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false);
      editSelectedT9Cell = -1;
      clearPopupBar();
    }
    return;
  }
  // Minimal edit screen: handle first so it takes priority
  if (minimalEditMode) {
    if (tft.getTouch(&tx, &ty)) {
      // Back button (top left)
      if (tx >= 10 && tx <= 70 && ty >= 10 && ty <= 50) {
        minimalEditMode = false;
        editSelectedT9Cell = -1;
        popupActiveEdit = false;
        drawWiFiMenu();
        return;
      }
      // Message box area (no-op, already in edit mode)
      // If popup is active, check popup buttons
      if (popupActiveEdit && editSelectedT9Cell != -1) {
        for (int i = 0; i < popupCountEdit; i++) {
          int px = popupXPositionsEdit[i];
          if (tx >= px && tx <= px+popupWidthEdit && ty >= popupBarYEdit && ty <= popupBarYEdit+popupBarHeightEdit) {
            String sel = lastPopupCharsEdit[i];
            // Highlight popup button green
            tft.fillRect(px, popupBarYEdit, popupWidthEdit, popupBarHeightEdit, TFT_BLACK);
            for (int t = 0; t < 3; ++t) tft.drawRect(px + t, popupBarYEdit + t, popupWidthEdit - 2 * t, popupBarHeightEdit - 2 * t, TFT_GREEN);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setTextSize(2);
            int tw = tft.textWidth(sel);
            int tx2 = px + (popupWidthEdit - tw) / 2;
            int ty2 = popupBarYEdit + (popupBarHeightEdit / 2) - 6;
            tft.setCursor(tx2, ty2);
            tft.print(sel);
            delay(120);
            if (sel == "<") {
              ssid = ssid.substring(0, ssid.length() - 1);
            } else if (sel == "_") {
              ssid += " ";
            } else {
              ssid += sel;
            }
            popupActiveEdit = false;
            editSelectedT9Cell = -1;
            clearPopupTextEdit();
            drawMinimalEditScreen();
            return;
          }
        }
        // If touch outside popup, close popup and deselect cell
        popupActiveEdit = false;
        editSelectedT9Cell = -1;
        clearPopupTextEdit();
        drawMinimalEditScreen();
        return;
      }
      // T9 grid area
      int t9Y = 160;
      for (int i = 0; i < 12; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = 15 + col * (90 + 10);
        int y = t9Y + row * (60 + 10);
        if (tx >= x && tx <= x+90 && ty >= y && ty <= y+60) {
          // Highlight cell and show popup
          editSelectedT9Cell = i;
          setupPopupEdit(i);
          if (popupCountEdit > 0) {
            popupActiveEdit = true;
            popupIndexEdit = 0;
            popupStartTimeEdit = millis();
            drawMinimalEditScreen();
          }
          return;
        }
      }
    }
    // Handle popup timeout
    if (popupActiveEdit && (millis() - popupStartTimeEdit > popupTimeoutEdit)) {
      popupActiveEdit = false;
      editSelectedT9Cell = -1;
      clearPopupTextEdit();
      drawMinimalEditScreen();
    }
    return;
  }
  // WiFi Password T9 edit mode: handle first so it takes priority
  if (wifiPassT9EditMode) {
    if (tft.getTouch(&tx, &ty)) {
      // T9 grid area
      int t9Y = 160;
      int prevCell = editSelectedT9Cell;
      for (int i = 0; i < 12; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = 15 + col * (90 + 10);
        int y = t9Y + row * (60 + 10);
        if (tx >= x && tx <= x+90 && ty >= y && ty <= y+60) {
          // Handle special cells
          if (i == 9) { // SAVE
            tft.fillRect(x, y, 90, 60, TFT_GREEN);
            tft.drawRect(x, y, 90, 60, TFT_WHITE);
            tft.setTextColor(TFT_BLACK);
            tft.setTextSize(2);
            int textWidth = tft.textWidth(labels[i]);
            tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
            tft.print(labels[i]);
            delay(100);
            drawT9Cell(i, false);

            password = removeAllSpaces(password); // added this line
            wifiPassT9EditMode = false;
            editSelectedT9Cell = -1;
            popupActiveEdit = false;
            drawWiFiMenu();
            return;
          } else if (i == 10) { // 0 _<
            if (prevCell != -1 && prevCell != i) drawT9Cell(prevCell, false);
            editSelectedT9Cell = i;
            drawT9Cell(i, true);
            setupPopupEdit(i); // This will set up "0", "_", "<"
            if (popupCountEdit > 0) {
              popupActiveEdit = true;
              popupIndexEdit = -1;
              popupStartTimeEdit = millis();
              drawPopupBar();
            }
            return;
          } else if (i == 11) { // CLEAR
            tft.fillRect(x, y, 90, 60, TFT_RED);
            tft.drawRect(x, y, 90, 60, TFT_WHITE);
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(2);
            int textWidth = tft.textWidth(labels[i]);
            tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
            tft.print(labels[i]);
            delay(100);
            drawT9Cell(i, false);
            password = "";
            drawMessageBar(password);
            return;
          } else {
            // Normal T9 cell: highlight and show popup
            if (prevCell != -1 && prevCell != i) drawT9Cell(prevCell, false);
            editSelectedT9Cell = i;
            drawT9Cell(i, true);
            setupPopupEdit(i);
            if (popupCountEdit > 0) {
              popupActiveEdit = true;
              popupIndexEdit = -1;
              popupStartTimeEdit = millis();
              drawPopupBar();
            }
            return;
          }
        }
      }
      // Popup bar area (fixed position)
      int popupBarYFixed = 110;
      int popupBarHeightFixed = 40;
      int totalWidth = popupCountEdit * popupWidthEdit + (popupCountEdit - 1) * popupSpacingEdit;
      int popupX = (320 - totalWidth) / 2;
      for (int i = 0; i < popupCountEdit; i++) {
        int px = popupX + i * (popupWidthEdit + popupSpacingEdit);
        if (tx >= px && tx <= px+popupWidthEdit && ty >= popupBarYFixed && ty <= popupBarYFixed+popupBarHeightFixed) {
          String sel = lastPopupCharsEdit[i];
          tft.fillRect(px, popupBarYFixed, popupWidthEdit, popupBarHeightFixed, TFT_BLACK);
          for (int t = 0; t < 3; ++t) tft.drawRect(px + t, popupBarYFixed + t, popupWidthEdit - 2 * t, popupBarHeightFixed - 2 * t, TFT_GREEN);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextSize(2);
          int tw = tft.textWidth(sel);
          int tx2 = px + (popupWidthEdit - tw) / 2;
          int ty2 = popupBarYFixed + (popupBarHeightFixed / 2) - 6;
          tft.setCursor(tx2, ty2);
          tft.print(sel);
          delay(120);
          if (password.length() < 24 || sel == "<") {
            if (sel == "<") {
              if (password.length()) password.remove(password.length()-1);
            } else if (sel == "_") {
              password += " ";
            } else if (sel == "0") {
              password += "0";
            } else {
              password += sel;
            }
          }
          popupActiveEdit = false;
          if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false);
          editSelectedT9Cell = -1;
          clearPopupBar();
          drawMessageBar(password);
          return;
        }
      }
      // If touch outside popup, close popup and deselect cell
      if (popupActiveEdit) {
        popupActiveEdit = false;
        if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false);
        editSelectedT9Cell = -1;
        clearPopupBar();
      }
    }
    // --- Popup timeout logic ---
    if (popupActiveEdit && (millis() - popupStartTimeEdit > popupTimeoutEdit)) {
      popupActiveEdit = false;
      if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false);
      editSelectedT9Cell = -1;
      clearPopupBar();
    }
    return;
  }
  // In handleTouch, update Valid Blink and Consecutive Gap T9 edit mode logic to only append digits from popups, ignore all other popup selections
  if (validBlinkT9EditMode) {
    if (tft.getTouch(&tx, &ty)) {
      int t9Y = 160;
      int prevCell = editSelectedT9Cell;
      int touchedCell = -1;
      for (int i = 0; i < 12; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = 15 + col * (90 + 10);
        int y = t9Y + row * (60 + 10);
        if (tx >= x && tx <= x+90 && ty >= y && ty <= y+60) {
          touchedCell = i;
          break;
        }
      }
      if (touchedCell != -1) {
        if (!blinkT9TouchActive || blinkT9LastCell != touchedCell) {
          blinkT9TouchActive = true;
          blinkT9LastCell = touchedCell;
          int i = touchedCell;
          int col = touchedCell % 3;
          int row = touchedCell / 3;
          int x = 15 + col * (90 + 10);
          int y = 160 + row * (60 + 10);
          if (i == 9) { // SAVE
            tft.fillRect(x, y, 90, 60, TFT_GREEN);
            tft.drawRect(x, y, 90, 60, TFT_WHITE);
            tft.setTextColor(TFT_BLACK);
            tft.setTextSize(2);
            int textWidth = tft.textWidth(blinkLabels[i]);
            tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
            tft.print(blinkLabels[i]);
            delay(100);
            drawT9Cell(i, false, true);
            validBlinkT9EditMode = false;
            editSelectedT9Cell = -1;
            popupActiveEdit = false;
            drawBlinkMenu();
            return;
          } else if (i == 10) { // 0_<=
            if (prevCell != -1 && prevCell != i) drawT9Cell(prevCell, false, true);
            editSelectedT9Cell = i;
            drawT9Cell(i, true, true);
            // Only show popup for 0, _, <
            popupCountEdit = 0;
            lastPopupCharsEdit[popupCountEdit++] = "0";
            lastPopupCharsEdit[popupCountEdit++] = "_";
            lastPopupCharsEdit[popupCountEdit++] = "<";
            popupWidthEdit = 50;
            int totalWidth = popupCountEdit * popupWidthEdit + (popupCountEdit - 1) * popupSpacingEdit;
            int popupX = (320 - totalWidth) / 2;
            for (int j = 0; j < popupCountEdit; j++) {
                popupXPositionsEdit[j] = popupX + j * (popupWidthEdit + popupSpacingEdit);
            }
            popupActiveEdit = true;
            popupIndexEdit = -1;
            popupStartTimeEdit = millis();
            drawPopupBar();
            return;
          } else if (i == 11) { // CLEAR
            tft.fillRect(x, y, 90, 60, TFT_RED);
            tft.drawRect(x, y, 90, 60, TFT_WHITE);
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(2);
            int textWidth = tft.textWidth(blinkLabels[i]);
            tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
            tft.print(blinkLabels[i]);
            delay(100);
            drawT9Cell(i, false, true);
            blinkDuration = 0;
            drawMessageBar(String(blinkDuration));
            return;
          } else if (i >= 0 && i <= 8) { // 1-9
            if (String(blinkDuration).length() < 10) {
                blinkDuration = blinkDuration * 10 + (i + 1);
                drawMessageBar(String(blinkDuration));
            }
            return;
          }
        }
      }
      // Popup bar area (no debounce needed for popup)
      int popupBarYFixed = 110;
      int popupBarHeightFixed = 40;
      int totalWidth = popupCountEdit * popupWidthEdit + (popupCountEdit - 1) * popupSpacingEdit;
      int popupX = (320 - totalWidth) / 2;
      for (int i = 0; i < popupCountEdit; i++) {
        int px = popupX + i * (popupWidthEdit + popupSpacingEdit);
        if (tx >= px && tx <= px+popupWidthEdit && ty >= popupBarYFixed && ty <= popupBarYFixed+popupBarHeightFixed) {
          String sel = lastPopupCharsEdit[i];
          tft.fillRect(px, popupBarYFixed, popupWidthEdit, popupBarHeightFixed, TFT_BLACK);
          for (int t = 0; t < 3; ++t) tft.drawRect(px + t, popupBarYFixed + t, popupWidthEdit - 2 * t, popupBarHeightFixed - 2 * t, TFT_GREEN);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextSize(2);
          int tw = tft.textWidth(sel);
          int tx2 = px + (popupWidthEdit - tw) / 2;
          int ty2 = popupBarYFixed + (popupBarHeightFixed / 2) - 6;
          tft.setCursor(tx2, ty2);
          tft.print(sel);
          delay(120);
          if (sel == "<") {
            blinkDuration = blinkDuration / 10; // Remove last digit
          } else if (sel == "0") {
            if (String(blinkDuration).length() < 10) blinkDuration = blinkDuration * 10;
          } // ignore _ for numeric
          popupActiveEdit = false;
          if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false, true);
          editSelectedT9Cell = -1;
          clearPopupBar();
          drawMessageBar(String(blinkDuration));
          return;
        }
      }
      // If touch outside popup, close popup and deselect cell
      if (popupActiveEdit) {
        popupActiveEdit = false;
        if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false, true);
        editSelectedT9Cell = -1;
        clearPopupBar();
        return;
      }
    } else {
      // No touch: reset debounce
      blinkT9TouchActive = false;
      blinkT9LastCell = -1;
    }
    // --- Popup timeout logic ---
    if (popupActiveEdit && (millis() - popupStartTimeEdit > popupTimeoutEdit)) {
      popupActiveEdit = false;
      if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false, true);
      editSelectedT9Cell = -1;
      clearPopupBar();
    }
    return;
  }
if (consecutiveGapT9EditMode) {
    if (tft.getTouch(&tx, &ty)) {
      int t9Y = 160;
      int prevCell = editSelectedT9Cell;
      int touchedCell = -1;
      for (int i = 0; i < 12; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = 15 + col * (90 + 10);
        int y = t9Y + row * (60 + 10);
        if (tx >= x && tx <= x+90 && ty >= y && ty <= y+60) {
          touchedCell = i;
          break;
        }
      }
      if (touchedCell != -1) {
        if (!blinkT9TouchActive || blinkT9LastCell != touchedCell) {
          blinkT9TouchActive = true;
          blinkT9LastCell = touchedCell;
          int i = touchedCell;
          int col = touchedCell % 3;
          int row = touchedCell / 3;
          int x = 15 + col * (90 + 10);
          int y = 160 + row * (60 + 10);
          if (i == 9) { // SAVE for blink input
            tft.fillRect(x, y, 90, 60, TFT_GREEN);
            tft.drawRect(x, y, 90, 60, TFT_WHITE);
            tft.setTextColor(TFT_BLACK);
            tft.setTextSize(2);
            int textWidth = tft.textWidth(blinkLabels[i]);
            tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
            tft.print(blinkLabels[i]);
            delay(100);
            drawT9Cell(i, false, true);
            consecutiveGapT9EditMode = false;
            editSelectedT9Cell = -1;
            popupActiveEdit = false;
            drawBlinkMenu();
            return;
          } else if (i == 10) { // 0_<=
            if (prevCell != -1 && prevCell != i) drawT9Cell(prevCell, false, true);
            editSelectedT9Cell = i;
            drawT9Cell(i, true, true);
            // Only show popup for 0, _, <
            popupCountEdit = 0;
            lastPopupCharsEdit[popupCountEdit++] = "0";
            lastPopupCharsEdit[popupCountEdit++] = "_";
            lastPopupCharsEdit[popupCountEdit++] = "<";
            popupWidthEdit = 50;
            int totalWidth = popupCountEdit * popupWidthEdit + (popupCountEdit - 1) * popupSpacingEdit;
            int popupX = (320 - totalWidth) / 2;
            for (int j = 0; j < popupCountEdit; j++) {
                popupXPositionsEdit[j] = popupX + j * (popupWidthEdit + popupSpacingEdit);
            }
            popupActiveEdit = true;
            popupIndexEdit = -1;
            popupStartTimeEdit = millis();
            drawPopupBar();
            return;
          } else if (i == 11) { // CLEAR
            tft.fillRect(x, y, 90, 60, TFT_RED);
            tft.drawRect(x, y, 90, 60, TFT_WHITE);
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(2);
            int textWidth = tft.textWidth(blinkLabels[i]);
            tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
            tft.print(blinkLabels[i]);
            delay(100);
            drawT9Cell(i, false, true);
            blinkGap = 0;
            drawMessageBar(String(blinkGap));
            return;
          } else if (i >= 0 && i <= 8) { // 1-9
            if (String(blinkGap).length() < 10) {
                blinkGap = blinkGap * 10 + (i + 1);
                drawMessageBar(String(blinkGap));
            }
            return;
          }
        }
      }
      // Popup bar area (no debounce needed for popup)
      int popupBarYFixed = 110;
      int popupBarHeightFixed = 40;
      int totalWidth = popupCountEdit * popupWidthEdit + (popupCountEdit - 1) * popupSpacingEdit;
      int popupX = (320 - totalWidth) / 2;
      for (int i = 0; i < popupCountEdit; i++) {
        int px = popupX + i * (popupWidthEdit + popupSpacingEdit);
        if (tx >= px && tx <= px+popupWidthEdit && ty >= popupBarYFixed && ty <= popupBarYFixed+popupBarHeightFixed) {
          String sel = lastPopupCharsEdit[i];
          tft.fillRect(px, popupBarYFixed, popupWidthEdit, popupBarHeightFixed, TFT_BLACK);
          for (int t = 0; t < 3; ++t) tft.drawRect(px + t, popupBarYFixed + t, popupWidthEdit - 2 * t, popupBarHeightFixed - 2 * t, TFT_GREEN);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextSize(2);
          int tw = tft.textWidth(sel);
          int tx2 = px + (popupWidthEdit - tw) / 2;
          int ty2 = popupBarYFixed + (popupBarHeightFixed / 2) - 6;
          tft.setCursor(tx2, ty2);
          tft.print(sel);
          delay(120);
          if (sel == "<") {
            blinkGap = blinkGap / 10; // Remove last digit
          } else if (sel == "0") {
            if (String(blinkGap).length() < 10) blinkGap = blinkGap * 10;
          } // ignore _ for numeric
          popupActiveEdit = false;
          if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false, true);
          editSelectedT9Cell = -1;
          clearPopupBar();
          drawMessageBar(String(blinkGap));
          return;
        }
      }
      // If touch outside popup, close popup and deselect cell
      if (popupActiveEdit) {
        popupActiveEdit = false;
        if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false, true);
        editSelectedT9Cell = -1;
        clearPopupBar();
        return;
      }
    } else {
      // No touch: reset debounce
      blinkT9TouchActive = false;
      blinkT9LastCell = -1;
    }
    // --- Popup timeout logic ---
    if (popupActiveEdit && (millis() - popupStartTimeEdit > popupTimeoutEdit)) {
      popupActiveEdit = false;
      if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false, true);
      editSelectedT9Cell = -1;
      clearPopupBar();
    }
    return;
  }
  // Main menu touch handling (settingsUiState == 0)
  if (settingsUiState == 0) {
    if (tft.getTouch(&tx, &ty)) {
      int segmentHeight = 480 / 3;
      int gap = 10;
      int wifiTop = 10;
      int wifiBottom = segmentHeight - gap;
      int blinkTop = segmentHeight + gap;
      int blinkBottom = 2*segmentHeight - gap;
      // WiFi Settings segment
      if (tx >= 10 && tx <= 310 && ty >= wifiTop && ty <= wifiBottom) {
        settingsUiState = 1;
        drawWiFiMenu();
        return;
      }
      // Blink Settings segment
      if (tx >= 10 && tx <= 310 && ty >= blinkTop && ty <= blinkBottom) {
        settingsUiState = 2;
        drawBlinkMenu();
        return;
      }
      // Save button
      if (tx >= 30 && tx <= 150 && ty >= 420 && ty <= 460) {
        tft.fillRect(30, 420, 120, 40, TFT_GREEN);
        tft.drawRect(30, 420, 120, 40, TFT_WHITE);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(60, 432);
        tft.print("Save");
        delay(120);
        Serial.println("SAVE Button pressed.");
        // Save all current values to Preferences and update prev* variables
        Serial.print("prevssid:"); // debug
        Serial.println(prevssid);
        Serial.print("ssid:"); // debug
        Serial.println(ssid);

        if(prevssid != ssid || prevpassword != password){
          prevssid = ssid;
          prevpassword = password;
          saveWiFiToPreferences();
          Serial.println("WiFi credentials saved.");
          uiState = 0;
          gui3Setup();
          reconnectWiFi();
          return;
        }
        if(prevBlinkDuration != blinkDuration || prevBlinkGap != blinkGap){
          prevBlinkDuration = blinkDuration;
          prevBlinkGap = blinkGap;
          saveBlinkSettingsToPreferences();
          Serial.println("Blink settings saved.");
        }
        uiState = 0;
        gui3Setup();
        return;
      }
      // Cancel button
      if (tx >= 170 && tx <= 290 && ty >= 420 && ty <= 460) {
        tft.fillRect(170, 420, 120, 40, TFT_RED);
        tft.drawRect(170, 420, 120, 40, TFT_WHITE);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(200, 432);
        tft.print("Cancel");
        delay(120);
        // Restore all values from prev* variables
        ssid = prevssid;
        password = prevpassword;
        blinkDuration = prevBlinkDuration;
        blinkGap = prevBlinkGap;
        // Do NOT call any save function here
        // Return to main T9 interface
        uiState = 0;
        gui3Setup();
        return;
      }
    }
    return;
  }
  // WiFi menu
  if (settingsUiState == 1) {
    if (tft.getTouch(&tx, &ty)) {
      // Name bar
      if (tx >= 15 && tx <= 305 && ty >= 80 && ty <= 120) {
        blinkEditField = 0; // SSID
        editHeading = getEditHeading(blinkEditField);
      
        wifiNameT9EditMode = true;
        editSelectedT9Cell = -1;
        popupActiveEdit = false;
        drawWifiNameT9EditScreen();
        return;
      }
      // Password bar
      if (tx >= 15 && tx <= 305 && ty >= 170 && ty <= 210) {
        blinkEditField = 1; // Password
        editHeading = getEditHeading(blinkEditField);
      
        wifiPassT9EditMode = true;
        editSelectedT9Cell = -1;
        popupActiveEdit = false;
        drawWifiPassT9EditScreen();
        return;
      }
      // Back button
      if (tx >= 10 && tx <= 110 && ty >= 370 && ty <= 410) {
        settingsUiState = 0;
        drawMainMenu();
        return;
      }
    }
    return;
  }
  // Blink menu
  if (settingsUiState == 2) {
    if (tft.getTouch(&tx, &ty)) {
      int barX = 70;
      int barY1 = 80;
      int barY2 = 170;
      int barW = 180;
      int barH = 40;
      // Valid Blink '-' button
      if (tx >= barX-55 && tx <= barX-5 && ty >= barY1 && ty <= barY1+barH) {
        tft.fillRect(barX-40, barY1, 35, barH, TFT_RED);
        tft.drawRect(barX-40, barY1, 35, barH, TFT_WHITE);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(3);
        tft.setCursor(barX-30, barY1+8);
        tft.print("-");
        delay(120);
        blinkDuration = (blinkDuration >= 10) ? blinkDuration-10 : 0;
        // Only redraw the value inside the white rectangle
        tft.fillRect(barX+1, barY1+1, barW-2, barH-2, TFT_BLACK);
        tft.setTextColor(TFT_CYAN);
        tft.setTextSize(2);
        tft.setCursor(barX+10, barY1+20-8);
        tft.print(String(blinkDuration));
        return;
      }
      // Valid Blink '+' button
      if (tx >= barX+barW+5 && tx <= barX+barW+40 && ty >= barY1 && ty <= barY1+barH) {
        tft.fillRect(barX+barW+5, barY1, 35, barH, TFT_GREEN);
        tft.drawRect(barX+barW+5, barY1, 35, barH, TFT_WHITE);
        tft.setTextColor(TFT_BLACK);
        tft.setTextSize(3);
        tft.setCursor(barX+barW+15, barY1+8);
        tft.print("+");
        delay(120);
        blinkDuration += 10;
        // Only redraw the value inside the white rectangle
        tft.fillRect(barX+1, barY1+1, barW-2, barH-2, TFT_BLACK);
        tft.setTextColor(TFT_CYAN);
        tft.setTextSize(2);
        tft.setCursor(barX+10, barY1+20-8);
        tft.print(String(blinkDuration));
        return;
      }
      // Valid Blink bar (edit)
      if (tx >= barX && tx <= barX+barW && ty >= barY1 && ty <= barY1+barH) {
        validBlinkT9EditMode = true;
        editSelectedT9Cell = -1;
        popupActiveEdit = false;
        drawValidBlinkT9EditScreen();
        return;
      }
      // Consecutive Gap '-' button
      if (tx >= barX-55 && tx <= barX-5 && ty >= barY2 && ty <= barY2+barH) {
        tft.fillRect(barX-40, barY2, 35, barH, TFT_RED);
        tft.drawRect(barX-40, barY2, 35, barH, TFT_WHITE);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(3);
        tft.setCursor(barX-30, barY2+8);
        tft.print("-");
        delay(120);
        blinkGap = (blinkGap >= 10) ? blinkGap-10 : 0;
        // Only redraw the value inside the white rectangle
        tft.fillRect(barX+1, barY2+1, barW-2, barH-2, TFT_BLACK);
        tft.setTextColor(TFT_CYAN);
        tft.setTextSize(2);
        tft.setCursor(barX+10, barY2+20-8);
        tft.print(String(blinkGap));
        return;
      }
      // Consecutive Gap '+' button
      if (tx >= barX+barW+5 && tx <= barX+barW+40 && ty >= barY2 && ty <= barY2+barH) {
        tft.fillRect(barX+barW+5, barY2, 35, barH, TFT_GREEN);
        tft.drawRect(barX+barW+5, barY2, 35, barH, TFT_WHITE);
        tft.setTextColor(TFT_BLACK);
        tft.setTextSize(3);
        tft.setCursor(barX+barW+15, barY2+8);
        tft.print("+");
        delay(120);
        blinkGap += 10;
        // Only redraw the value inside the white rectangle
        tft.fillRect(barX+1, barY2+1, barW-2, barH-2, TFT_BLACK);
        tft.setTextColor(TFT_CYAN);
        tft.setTextSize(2);
        tft.setCursor(barX+10, barY2+20-8);
        tft.print(String(blinkGap));
        return;
      }
      // Consecutive Gap bar (edit)
      if (tx >= barX && tx <= barX+barW && ty >= barY2 && ty <= barY2+barH) {
        consecutiveGapT9EditMode = true;
        editSelectedT9Cell = -1;
        popupActiveEdit = false;
        drawConsecutiveGapT9EditScreen();
        return;
      }
      // Back button
      if (tx >= 10 && tx <= 110 && ty >= 370 && ty <= 410) {
        settingsUiState = 0;
        drawMainMenu();
        return;
      }
    }
    return;
  }
  // Edit screen (T9 keyboard for SSID/Password)
  if (settingsUiState == 3) {
    if (tft.getTouch(&tx, &ty)) {
      // Message bar area (activate keyboard)
      if (tx >= 15 && tx <= 305 && ty >= 80 && ty <= 120) {
        editKeyboardActive = true;
        editSelectedT9Cell = -1;
        popupActiveEdit = false;
        drawEditScreen();
        return;
      }
      // If keyboard is not active, ignore other touches
      if (!editKeyboardActive) return;
      // If popup is active, check popup buttons
      if (popupActiveEdit && editSelectedT9Cell != -1) {
        for (int i = 0; i < popupCountEdit; i++) {
          int px = popupXPositionsEdit[i];
          if (tx >= px && tx <= px+popupWidthEdit && ty >= popupBarYEdit && ty <= popupBarYEdit+popupBarHeightEdit) {
            String sel = lastPopupCharsEdit[i];
            // Highlight popup button green
            tft.fillRect(px, popupBarYEdit, popupWidthEdit, popupBarHeightEdit, TFT_BLACK);
            for (int t = 0; t < 3; ++t) tft.drawRect(px + t, popupBarYEdit + t, popupWidthEdit - 2 * t, popupBarHeightEdit - 2 * t, TFT_GREEN);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setTextSize(2);
            int tw = tft.textWidth(sel);
            int tx2 = px + (popupWidthEdit - tw) / 2;
            int ty2 = popupBarYEdit + (popupBarHeightEdit / 2) - 6;
            tft.setCursor(tx2, ty2);
            tft.print(sel);
            delay(120);
            if (sel == "<") {
              ssid = ssid.substring(0, ssid.length() - 1);
            } else if (sel == "_") {
              ssid += " ";
            } else {
              ssid += sel;
            }
            popupActiveEdit = false;
            editSelectedT9Cell = -1;
            clearPopupTextEdit();
            drawEditScreen();
            return;
          }
        }
        // If touch outside popup, close popup and deselect cell
        popupActiveEdit = false;
        editSelectedT9Cell = -1;
        clearPopupTextEdit();
        drawEditScreen();
        return;
      }
      // T9 grid area
      int t9Y = 160;
      for (int i = 0; i < 12; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = 15 + col * (90 + 10);
        int y = t9Y + row * (60 + 10);
        if (tx >= x && tx <= x+90 && ty >= y && ty <= y+60) {
          // Highlight cell and show popup
          editSelectedT9Cell = i;
          setupPopupEdit(i);
          if (popupCountEdit > 0) {
            popupActiveEdit = true;
            popupIndexEdit = 0;
            popupStartTimeEdit = millis();
            drawEditScreen();
          }
          return;
        }
      }
      // // Save button
      // if (tx >= 30 && tx <= 150 && ty >= 420 && ty <= 460) {
      //   if (blinkEditField == 0) ssid = ssid;
      //   else if (blinkEditField == 1) password = password;
      //   saveWiFiToPreferences();
      //   settingsUiState = 1;
      //   editKeyboardActive = false;
      //   editSelectedT9Cell = -1;
      //   popupActiveEdit = false;
      //   drawWiFiMenu();
      //   return;
      // }
      // // Cancel button
      // if (tx >= 170 && tx <= 290 && ty >= 420 && ty <= 460) {
      //   settingsUiState = 1;
      //   editKeyboardActive = false;
      //   editSelectedT9Cell = -1;
      //   popupActiveEdit = false;
      //   drawWiFiMenu();
      //   return;
      // }
    }
    // Handle popup timeout
    if (popupActiveEdit && (millis() - popupStartTimeEdit > popupTimeoutEdit)) {
      popupActiveEdit = false;
      editSelectedT9Cell = -1;
      clearPopupTextEdit();
      drawEditScreen();
    }
    return;
  }
  // ... (rest of handleTouch from setting_2.ino)
}
// ... (copy the rest of the real settings UI code as needed)

// Random string generator function (A-Z, a-z, 0-9)
String generateRandomString(int length) {
  String charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  String result = "";
  for (int i = 0; i < length; i++) {
    int randomIndex = random(charset.length());
    result += charset[randomIndex];
  }
  return result;
}

// Custom random user ID generator with pattern: XdXdX (X=alphanumeric, d=digit)
String generatePatternedUserId() {
  String alphaNum = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  String digits = "0123456789";
  String id = "";
  for (int i = 0; i < 5; i++) {
    if (i == 1 || i == 3) {
      int idx = random(digits.length());
      id += digits[idx];
    } else {
      int idx = random(alphaNum.length());
      id += alphaNum[idx];
    }
  }
  return id;
}

// Validate userId matches pattern: XdXdX
bool isValidPatternedUserId(const String &id) {
  if (id.length() != 5) return false;
  for (int i = 0; i < 5; i++) {
    char c = id[i];
    if (i == 1 || i == 3) {
      if (c < '0' || c > '9') return false;
    } else {
      if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return false;
    }
  }
  return true;
}

// Helper to write userId to EEPROM
void writeUserIdToEEPROM(const String &id) {
  for (int i = 0; i < 5; i++) {
    EEPROM.write(USERID_ADDR + i, (i < id.length()) ? id[i] : '\0');
  }
  EEPROM.write(USERID_ADDR + 5, '\0'); // Null terminator
  EEPROM.commit();
}

// Helper to read userId from EEPROM
String readUserIdFromEEPROM() {
  char data[6];
  int len = 0;
  unsigned char k = EEPROM.read(USERID_ADDR);
  while (k != '\0' && k != 0xFF && len < 5) {
    data[len] = k;
    len++;
    k = EEPROM.read(USERID_ADDR + len);
  }
  data[len] = '\0';
  return String(data);
}

// Helper functions for Preferences persistence


void setting2Setup() {
    // No EEPROM for settings, only for userId if needed
    tft.setRotation(2);
    uint16_t calData[5] = { 471, 2859, 366, 3388, 2 }; // Same as gui3Setup
    tft.setTouch(calData);
    loadWiFiFromPreferences();
    loadBlinkSettingsFromPreferences();
    prevssid = ssid;
    prevpassword = password;
    prevBlinkDuration = blinkDuration;
    prevBlinkGap = blinkGap;
    // Persistent userId logic with pattern check
    userId = readUserIdFromEEPROM();
    if (!isValidPatternedUserId(userId)) {
        randomSeed(analogRead(0) + millis());
        userId = generatePatternedUserId();
        writeUserIdToEEPROM(userId);
    }
    drawMainMenu();
}

void setting2Loop() {
    handleTouch();
    // --- Popup timeout logic ---
    if (inEditScreen && popupActive && selectedT9Key != -1 && popupStartTime > 0) {
        if (millis() - popupStartTime > 2000) { // 2 seconds timeout
            selectedT9Key = -1;
            selectedPopupLetter = -1;
            popupActive = false;
            popupStartTime = 0;
            drawT9Grid();
            drawValueBar();
            clearPopupBar();
        }
    }
    // --- Return to main UI if top-left corner is touched ---
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
        if (x < 40 && y < 40) {
            // You may want to call a function to switch back to the main UI here
            // For example: switchToMainUI();
        }
    }
} 

// --- Add missing drawEditScreen definition to resolve linker error ---
void drawEditScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(15, 10);
    tft.print(editHeading);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(15, 50);
    tft.print("Value:");
    // Message bar (touchable)
    tft.drawRect(15, 80, 290, 40, TFT_WHITE);
    tft.fillRect(16, 81, 288, 38, TFT_BLACK);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(20, 100);
    tft.print(ssid); // Changed from editBuffer to ssid
    // If keyboard is active, draw T9 grid
    if (editKeyboardActive) {
        for (int i = 0; i < 12; i++) {
            int col = i % 3;
            int row = i / 3;
            int x = 15 + col * (90 + 10);
            int y = 160 + row * (60 + 10);
            uint16_t keyColor = (editSelectedT9Cell == i) ? TFT_YELLOW : TFT_DARKGREY;
            tft.fillRect(x, y, 90, 60, keyColor);
            tft.drawRect(x, y, 90, 60, TFT_WHITE);
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(2);
            int textWidth = tft.textWidth(labels[i]);
            tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
            tft.print(labels[i]);
        }
        if (popupActiveEdit) drawPopupEdit();
    }
    // Draw Save and Cancel buttons
    int btnY = 420;
    int btnW = 120;
    int btnH = 40;
    tft.setTextColor(TFT_WHITE);
    tft.fillRect(30, btnY, btnW, btnH, TFT_DARKGREY);
    tft.drawRect(30, btnY, btnW, btnH, TFT_WHITE);
    tft.setCursor(60, btnY+12);
    tft.print("Save");
    tft.fillRect(170, btnY, btnW, btnH, TFT_DARKGREY);
    tft.drawRect(170, btnY, btnW, btnH, TFT_WHITE);
    tft.setCursor(200, btnY+12);
    tft.print("Cancel");
} 

// --- Popup state for T9 keyboard in settings edit screen (move to top for visibility) ---
void setupPopupEdit(int index) {
    popupCountEdit = 0;
    if (index == 9) {
        // No emojis in settings, skip
        return;
    } else if (index == 10) {
        lastPopupCharsEdit[popupCountEdit++] = "0";
        lastPopupCharsEdit[popupCountEdit++] = "_";
        lastPopupCharsEdit[popupCountEdit++] = "<";
        popupWidthEdit = 50;
    } else {
        String label = labels[index];
        // Always include the number as the first popup button
        if (label.length() > 0 && label[0] >= '0' && label[0] <= '9') {
            lastPopupCharsEdit[popupCountEdit++] = String(label[0]);
        }
        // Add all letters (skip spaces and the number at the start)
        for (int i = 1; i < label.length(); i++) {
            if (label[i] != ' ') lastPopupCharsEdit[popupCountEdit++] = String(label[i]);
        }
        popupWidthEdit = 50;
    }
    int totalWidth = popupCountEdit * popupWidthEdit + (popupCountEdit - 1) * popupSpacingEdit;
    int popupX = (320 - totalWidth) / 2;
    for (int i = 0; i < popupCountEdit; i++) {
        popupXPositionsEdit[i] = popupX + i * (popupWidthEdit + popupSpacingEdit);
    }
}

void drawPopupEdit() {
    for (int i = 0; i < popupCountEdit; i++) {
        int px = popupXPositionsEdit[i];
        uint16_t fill = TFT_BLACK;
        uint16_t border = (i == popupIndexEdit) ? TFT_YELLOW : TFT_WHITE;
        int thickness = (i == popupIndexEdit) ? 3 : 1;
        tft.fillRect(px, popupBarYEdit, popupWidthEdit, popupBarHeightEdit, fill);
        for (int t = 0; t < thickness; ++t) tft.drawRect(px + t, popupBarYEdit + t, popupWidthEdit - 2 * t, popupBarHeightEdit - 2 * t, border);
        if (thickness == 1) tft.drawRect(px, popupBarYEdit, popupWidthEdit, popupBarHeightEdit, border);
        tft.setTextColor(TFT_WHITE, fill);
        tft.setTextSize(2);
        String boxText = lastPopupCharsEdit[i];
        int tw = tft.textWidth(boxText);
        int tx = px + (popupWidthEdit - tw) / 2;
        int ty = popupBarYEdit + (popupBarHeightEdit / 2) - 6;
        tft.setCursor(tx, ty);
        tft.print(boxText);
    }
}

void clearPopupTextEdit() {
    for (int i = 0; i < popupCountEdit; i++) {
        int px = popupXPositionsEdit[i];
        tft.fillRect(px, popupBarYEdit, popupWidthEdit, popupBarHeightEdit, TFT_BLACK);
    }
} 

// Helper to draw a single T9 cell
void drawT9Cell(int index, bool highlightYellow, bool blinkMode) {
    int col = index % 3;
    int row = index / 3;
    int x = 15 + col * (90 + 10);
    int y = 160 + row * (60 + 10);
    uint16_t keyColor = highlightYellow ? TFT_YELLOW : TFT_DARKGREY;
    tft.fillRect(x, y, 90, 60, keyColor);
    tft.drawRect(x, y, 90, 60, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    const char* label = blinkMode ? blinkLabels[index] : labels[index];
    int textWidth = tft.textWidth(label);
    tft.setCursor(x + (90 - textWidth) / 2, y + 30 - 12);
    tft.print(label);
}

// Helper to draw the popup bar
void drawPopupBar() {
    int popupBarYFixed = 110;
    int popupBarHeightFixed = 40;
    tft.fillRect(0, popupBarYFixed, 320, popupBarHeightFixed, TFT_BLACK); // Always clear
    int totalWidth = popupCountEdit * popupWidthEdit + (popupCountEdit - 1) * popupSpacingEdit;
    int popupX = (320 - totalWidth) / 2;
    for (int i = 0; i < popupCountEdit; i++) {
        int px = popupX + i * (popupWidthEdit + popupSpacingEdit);
        uint16_t fill = TFT_DARKGREY;
        uint16_t border = TFT_WHITE;
        if (popupIndexEdit == i) border = TFT_YELLOW;
        tft.fillRect(px, popupBarYFixed, popupWidthEdit, popupBarHeightFixed, fill);
        tft.drawRect(px, popupBarYFixed, popupWidthEdit, popupBarHeightFixed, border);
        tft.setTextColor(TFT_WHITE, fill);
        tft.setTextSize(2);
        String boxText = lastPopupCharsEdit[i];
        int tw = tft.textWidth(boxText);
        int tx = px + (popupWidthEdit - tw) / 2;
        int ty = popupBarYFixed + (popupBarHeightFixed / 2) - 6;
        tft.setCursor(tx, ty);
        tft.print(boxText);
    }
}

// Helper to clear the popup bar area
void clearPopupBar() {
    int popupBarYFixed = 110;
    int popupBarHeightFixed = 40;
    tft.fillRect(0, popupBarYFixed, 320, popupBarHeightFixed, TFT_BLACK);
}

// Helper to draw just the message bar
void drawMessageBar(const String& msg) {
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawRect(15, 60, 290, 40, TFT_WHITE);
    tft.fillRect(16, 61, 288, 38, TFT_BLACK);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(20, 80);
    int maxMsgLen = 24;
    String displayMsg = msg;
    if (displayMsg.length() > maxMsgLen) displayMsg = displayMsg.substring(displayMsg.length() - maxMsgLen);
    tft.print(displayMsg);
} 

// Draw Valid Blink T9 edit interface (no extra buttons, just like WiFi Name/Password)
void drawValidBlinkT9EditScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(15, 10);
    tft.print("Valid Blink");
    drawMessageBar(String(blinkDuration));
    clearPopupBar();
    if (popupActiveEdit) drawPopupBar();
    for (int i = 0; i < 12; i++) drawT9Cell(i, false, true); // true = blink mode
}
// Draw Consecutive Gap T9 edit interface (no extra buttons, just like WiFi Name/Password)
void drawConsecutiveGapT9EditScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(15, 10);
    tft.print("Consecutive Gap");
    drawMessageBar(String(blinkGap));
    clearPopupBar();
    if (popupActiveEdit) drawPopupBar();
    for (int i = 0; i < 12; i++) drawT9Cell(i, false, true);
} 

