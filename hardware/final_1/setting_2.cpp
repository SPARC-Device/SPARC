#include "setting_2.h"
#include <EEPROM.h>
#include <TFT_eSPI.h>
#include <SPI.h>

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
#define MIN_BLINK_DURATION_ADDR 128
#define CONSECUTIVE_GAP_ADDR 132

TFT_eSPI tft = TFT_eSPI();

String currentSSID = "";
String currentPassword = "";
unsigned long minBlinkDuration = 400;
unsigned long consecutiveGap = 1200;
const unsigned long debounceDelay = 50;

bool currentEyeState = true;
bool lastSensorReading = HIGH;

unsigned long eyeCloseTime = 0;
unsigned long lastDebounceTime = 0;

int consecutiveBlinks = 0;
unsigned long lastBlinkTime = 0;

bool emergencyMode = false;
unsigned long emergencyStartTime = 0;

String typedSSID = "";
String typedPassword = "";
bool ssidInputMode = true;
bool inKeyboardMode = false;
bool inBlinkEditMode = false;
bool inEditScreen = false;
int blinkEditField = 0; // 0: minBlinkDuration, 1: consecutiveGap
String blinkEditBuffer = "";

// Add new variables for edit mode
String editBuffer = "";
int editField = -1; // 0: Name, 1: PASS, 2: Valid Blink, 3: Consecutive Gap
String editHeading = "";
bool popupActive = false;
int selectedT9Key = -1;
int selectedPopupLetter = -1;
int settingsUiState = 0; // 0 = main menu, 1 = wifi, 2 = blink, 3 = edit, etc.
String userId = "USER123456"; // Placeholder for user ID

const int keyWidth = 90;
const int keyHeight = 60;
const int spacing = 10;
const int xOffset = 15;
const int yOffset = 180;
const int popupBarY = 130;
const int popupBarHeight = 30;

const char* labels[12] = {
  "1 ABC", "2 DEF", "3 GHI",
  "4 JKL", "5 MNO", "6 PQR",
  "7 STU", "8 VWX", "9 YZ",
  "SAVE", "0 _<", "CLEAR"
};

unsigned long popupStartTime = 0;
int activePopupIndex = -1;
int popupCount = 0;
String lastPopupChars[6];
int popupWidth = 50;
int popupSpacing = 5;
int popupXPositions[6];

// Add missing variable definitions
String prevTypedSSID = "";
String prevTypedPassword = "";
unsigned long prevMinBlinkDuration = 400;
unsigned long prevConsecutiveGap = 2000;

// Add a flag to track if the T9 keyboard is active in edit mode
bool editKeyboardActive = false;
int editSelectedT9Cell = -1; // -1 means no cell selected

// Add a flag to track minimal edit mode (for WiFi Name/Password)
bool minimalEditMode = false;

// Add a variable to store the previous WiFi name before editing
String prevSSIDBeforeEdit = "";

// Add a flag and buffer for WiFi Password T9 edit mode
bool wifiPassT9EditMode = false;
String editPassBuffer = "";
String prevTypedPasswordBeforeEdit = "";

// Add a flag and buffer for Valid Blink T9 edit mode
bool validBlinkT9EditMode = false;
String editValidBlinkBuffer = "";
String prevValidBlinkValue = "";

// Add a flag and buffer for Consecutive Gap T9 edit mode
bool consecutiveGapT9EditMode = false;
String editConsecutiveGapBuffer = "";
String prevConsecutiveGapValue = "";

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
    tft.print(editBuffer);
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

// EEPROM helpers
void writeStringToEEPROM(int addrOffset, const String &str) {
  int len = str.length();
  for (int i = 0; i < len; i++) EEPROM.write(addrOffset + i, str[i]);
  EEPROM.write(addrOffset + len, '\0');
  EEPROM.commit();
}

String readStringFromEEPROM(int addrOffset) {
  char data[64];
  int len = 0;
  unsigned char k = EEPROM.read(addrOffset);
  while (k != '\0' && len < 63) {
    data[len] = k;
    len++;
    k = EEPROM.read(addrOffset + len);
  }
  data[len] = '\0';
  return String(data);
}

String trimString(const String& str) {
  int start = 0;
  int end = str.length() - 1;
  while (start <= end && str[start] == ' ') start++;
  while (end >= start && str[end] == ' ') end--;
  return str.substring(start, end + 1);
}

String removeAllSpaces(const String& str) {
  String out = "";
  for (int i = 0; i < str.length(); ++i) {
    if (str[i] != ' ') out += str[i];
  }
  return out;
}

void saveWiFiToEEPROM() {
  currentSSID = trimString(typedSSID);
  currentPassword = trimString(typedPassword);
  typedSSID = currentSSID;
  typedPassword = currentPassword;
  writeStringToEEPROM(SSID_ADDR, currentSSID);
  writeStringToEEPROM(PASS_ADDR, currentPassword);
  EEPROM.commit();
}

void saveConfigToEEPROM() {
  EEPROM.put(MIN_BLINK_DURATION_ADDR, minBlinkDuration);
  EEPROM.put(CONSECUTIVE_GAP_ADDR, consecutiveGap);
  EEPROM.commit();
}

void loadConfigFromEEPROM() {
  currentSSID = readStringFromEEPROM(SSID_ADDR);
  currentPassword = readStringFromEEPROM(PASS_ADDR);
  EEPROM.get(MIN_BLINK_DURATION_ADDR, minBlinkDuration);
  EEPROM.get(CONSECUTIVE_GAP_ADDR, consecutiveGap);
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
  if (editField == 0 || editField == 1) tft.setCursor(23, 70);
  else tft.setCursor(23, 70);
  tft.print(editBuffer);
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
  tft.print(typedSSID);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(15, 140);
  tft.print("Password");
  tft.drawRect(15, 170, 290, 40, TFT_WHITE);
  tft.fillRect(16, 171, 288, 38, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(20, 190);
  tft.print(typedPassword);
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
  tft.print(String(minBlinkDuration));
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
  tft.print(String(consecutiveGap));
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

void drawPlaceholderPage() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(40, 200);
  tft.print("Placeholder Page");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(40, 240);
  tft.print("(Redirected after Save/Cancel)");
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
    drawMessageBar(editBuffer);
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
    drawMessageBar(editPassBuffer);
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
          if (i == 9) { // SAVE
            prevSSIDBeforeEdit = typedSSID;
            typedSSID = editBuffer;
            wifiNameT9EditMode = false;
            editSelectedT9Cell = -1;
            popupActiveEdit = false;
            drawWiFiMenu();
            return;
          } else if (i == 10) { // 0 _<
            // Show popup bar for 0 _ <
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
            editBuffer = "";
            drawMessageBar(editBuffer);
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
          // Highlight popup button green
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
          if (editBuffer.length() < 24 || sel == "<") {
            if (sel == "<") {
              if (editBuffer.length()) editBuffer.remove(editBuffer.length()-1);
            } else if (sel == "_") {
              editBuffer += " ";
            } else if (sel == "0") {
              editBuffer += "0";
            } else {
              editBuffer += sel;
            }
          }
          popupActiveEdit = false;
          if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false);
          editSelectedT9Cell = -1;
          clearPopupBar();
          drawMessageBar(editBuffer);
          return;
        }
      }
      // If touch outside popup, close popup and deselect cell
      if (popupActiveEdit) {
        popupActiveEdit = false;
        if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false);
        editSelectedT9Cell = -1;
        clearPopupBar();
        return;
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
              if (editBuffer.length()) editBuffer.remove(editBuffer.length()-1);
            } else if (sel == "_") {
              editBuffer += " ";
            } else {
              editBuffer += sel;
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
            prevTypedPassword = typedPassword;
            typedPassword = editPassBuffer;
            wifiPassT9EditMode = false;
            editSelectedT9Cell = -1;
            popupActiveEdit = false;
            drawWiFiMenu();
            return;
          } else if (i == 10) { // 0 _<
            // Show popup bar for 0 _ <
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
            editPassBuffer = "";
            drawMessageBar(editPassBuffer);
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
          // Highlight popup button green
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
          if (editPassBuffer.length() < 24 || sel == "<") {
            if (sel == "<") {
              if (editPassBuffer.length()) editPassBuffer.remove(editPassBuffer.length()-1);
            } else if (sel == "_") {
              editPassBuffer += " ";
            } else if (sel == "0") {
              editPassBuffer += "0";
            } else {
              editPassBuffer += sel;
            }
          }
          popupActiveEdit = false;
          if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false);
          editSelectedT9Cell = -1;
          clearPopupBar();
          drawMessageBar(editPassBuffer);
          return;
        }
      }
      // If touch outside popup, close popup and deselect cell
      if (popupActiveEdit) {
        popupActiveEdit = false;
        if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false);
        editSelectedT9Cell = -1;
        clearPopupBar();
        return;
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
          if (i == 9) { // SAVE
            prevValidBlinkValue = String(minBlinkDuration);
            String digits = editValidBlinkBuffer;
            int firstNonZero = 0;
            while (firstNonZero < digits.length() && digits[firstNonZero] == '0') firstNonZero++;
            digits = digits.substring(firstNonZero);
            if (digits.length() == 0) minBlinkDuration = 0;
            else minBlinkDuration = digits.substring(0, 10).toInt();
            saveConfigToEEPROM();
            validBlinkT9EditMode = false;
            editSelectedT9Cell = -1;
            popupActiveEdit = false;
            drawBlinkMenu();
            return;
          } else if (i == 10) { // 0_<
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
            editValidBlinkBuffer = "0";
            drawMessageBar(editValidBlinkBuffer);
            return;
          } else if (i >= 0 && i <= 8) { // 1-9
            if (editValidBlinkBuffer.length() < 10) {
                editValidBlinkBuffer += String(i+1);
                drawMessageBar(editValidBlinkBuffer);
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
            if (editValidBlinkBuffer.length()) editValidBlinkBuffer.remove(editValidBlinkBuffer.length()-1);
          } else if (sel == "0") {
            if (editValidBlinkBuffer.length() < 10) editValidBlinkBuffer += "0";
          } // ignore _ for numeric
          popupActiveEdit = false;
          if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false, true);
          editSelectedT9Cell = -1;
          clearPopupBar();
          drawMessageBar(editValidBlinkBuffer);
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
          if (i == 9) { // SAVE
            prevConsecutiveGapValue = String(consecutiveGap);
            String digits = editConsecutiveGapBuffer;
            int firstNonZero = 0;
            while (firstNonZero < digits.length() && digits[firstNonZero] == '0') firstNonZero++;
            digits = digits.substring(firstNonZero);
            if (digits.length() == 0) consecutiveGap = 0;
            else consecutiveGap = digits.substring(0, 10).toInt();
            saveConfigToEEPROM();
            consecutiveGapT9EditMode = false;
            editSelectedT9Cell = -1;
            popupActiveEdit = false;
            drawBlinkMenu();
            return;
          } else if (i == 10) { // 0_<
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
            editConsecutiveGapBuffer = "0";
            drawMessageBar(editConsecutiveGapBuffer);
            return;
          } else if (i >= 0 && i <= 8) { // 1-9
            if (editConsecutiveGapBuffer.length() < 10) {
                editConsecutiveGapBuffer += String(i+1);
                drawMessageBar(editConsecutiveGapBuffer);
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
            if (editConsecutiveGapBuffer.length()) editConsecutiveGapBuffer.remove(editConsecutiveGapBuffer.length()-1);
          } else if (sel == "0") {
            if (editConsecutiveGapBuffer.length() < 10) editConsecutiveGapBuffer += "0";
          } // ignore _ for numeric
          popupActiveEdit = false;
          if (editSelectedT9Cell != -1) drawT9Cell(editSelectedT9Cell, false, true);
          editSelectedT9Cell = -1;
          clearPopupBar();
          drawMessageBar(editConsecutiveGapBuffer);
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
        drawPlaceholderPage();
        settingsUiState = 4;
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
        typedSSID = prevTypedSSID;
        typedPassword = prevTypedPassword;
        minBlinkDuration = prevMinBlinkDuration;
        consecutiveGap = prevConsecutiveGap;
        saveWiFiToEEPROM();
        saveConfigToEEPROM();
        drawPlaceholderPage();
        settingsUiState = 4;
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
        editField = 0; // SSID
        editHeading = getEditHeading(editField);
        editBuffer = typedSSID;
        prevSSIDBeforeEdit = typedSSID;
        wifiNameT9EditMode = true;
        editSelectedT9Cell = -1;
        popupActiveEdit = false;
        drawWifiNameT9EditScreen();
        return;
      }
      // Password bar
      if (tx >= 15 && tx <= 305 && ty >= 170 && ty <= 210) {
        editField = 1; // Password
        editHeading = getEditHeading(editField);
        editPassBuffer = typedPassword;
        prevTypedPasswordBeforeEdit = typedPassword;
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
        minBlinkDuration = (minBlinkDuration >= 10) ? minBlinkDuration-10 : 0;
        saveConfigToEEPROM();
        drawBlinkMenu();
        return;
      }
      // Valid Blink '+' button
      if (tx >= barX+barW+5 && tx <= barX+barW+40 && ty >= barY1 && ty <= barY1+barH) {
        tft.fillRect(barX+barW+5, barY1, 35, barH, TFT_GREEN);
        tft.drawRect(barX+barW+5, barY1, 35, barH, TFT_WHITE);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(3);
        tft.setCursor(barX+barW+15, barY1+8);
        tft.print("+");
        delay(120);
        minBlinkDuration += 10;
        saveConfigToEEPROM();
        drawBlinkMenu();
        return;
      }
      // Valid Blink bar (edit)
      if (tx >= barX && tx <= barX+barW && ty >= barY1 && ty <= barY1+barH) {
        validBlinkT9EditMode = true;
        editValidBlinkBuffer = String(minBlinkDuration);
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
        consecutiveGap = (consecutiveGap >= 10) ? consecutiveGap-10 : 0;
        saveConfigToEEPROM();
        drawBlinkMenu();
        return;
      }
      // Consecutive Gap '+' button
      if (tx >= barX+barW+5 && tx <= barX+barW+40 && ty >= barY2 && ty <= barY2+barH) {
        tft.fillRect(barX+barW+5, barY2, 35, barH, TFT_GREEN);
        tft.drawRect(barX+barW+5, barY2, 35, barH, TFT_WHITE);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(3);
        tft.setCursor(barX+barW+15, barY2+8);
        tft.print("+");
        delay(120);
        consecutiveGap += 10;
        saveConfigToEEPROM();
        drawBlinkMenu();
        return;
      }
      // Consecutive Gap bar (edit)
      if (tx >= barX && tx <= barX+barW && ty >= barY2 && ty <= barY2+barH) {
        consecutiveGapT9EditMode = true;
        editConsecutiveGapBuffer = String(consecutiveGap);
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
              if (editBuffer.length()) editBuffer.remove(editBuffer.length()-1);
            } else if (sel == "_") {
              editBuffer += " ";
            } else {
              editBuffer += sel;
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
      // Save button
      if (tx >= 30 && tx <= 150 && ty >= 420 && ty <= 460) {
        if (editField == 0) typedSSID = editBuffer;
        else if (editField == 1) typedPassword = editBuffer;
        saveWiFiToEEPROM();
        settingsUiState = 1;
        editKeyboardActive = false;
        editSelectedT9Cell = -1;
        popupActiveEdit = false;
        drawWiFiMenu();
        return;
      }
      // Cancel button
      if (tx >= 170 && tx <= 290 && ty >= 420 && ty <= 460) {
        settingsUiState = 1;
        editKeyboardActive = false;
        editSelectedT9Cell = -1;
        popupActiveEdit = false;
        drawWiFiMenu();
        return;
      }
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

void setting2Setup() {
    tft.setRotation(2);
    uint16_t calData[5] = { 471, 2859, 366, 3388, 2 }; // Same as gui3Setup
    tft.setTouch(calData);
    loadConfigFromEEPROM();
    typedSSID = currentSSID;
    typedPassword = currentPassword;
    prevTypedSSID = typedSSID;
    prevTypedPassword = typedPassword;
    prevMinBlinkDuration = minBlinkDuration;
    prevConsecutiveGap = consecutiveGap;
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
    tft.print(editBuffer);
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
    drawMessageBar(editValidBlinkBuffer);
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
    drawMessageBar(editConsecutiveGapBuffer);
    clearPopupBar();
    if (popupActiveEdit) drawPopupBar();
    for (int i = 0; i < 12; i++) drawT9Cell(i, false, true);
} 