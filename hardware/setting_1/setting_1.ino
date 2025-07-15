// === FULL INTEGRATED CODE: TFT + EEPROM + T9 Keyboard (NO WIFI) ===

#include <EEPROM.h>
#include <TFT_eSPI.h>
#include <SPI.h>

#define EEPROM_SIZE 200
#define SSID_ADDR 0
#define PASS_ADDR 64
#define MIN_BLINK_DURATION_ADDR 128
#define CONSECUTIVE_GAP_ADDR 132

TFT_eSPI tft = TFT_eSPI();

String currentSSID = "";
String currentPassword = "";
unsigned long minBlinkDuration = 400;
unsigned long consecutiveGap = 2000;
const unsigned long debounceDelay = 50;

const int IR_SENSOR_PIN = A0;
const int LED_PIN = 2;

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
int blinkEditField = 0; // 0: minBlinkDuration, 1: consecutiveGap
String blinkEditBuffer = "";

// Add new variables for edit mode
String editBuffer = "";
int editField = -1; // 0: Name, 1: PASS, 2: Valid Blink, 3: Consecutive Gap
String editHeading = "";
bool inEditScreen = false;
int selectedT9Key = -1;
int selectedPopupLetter = -1;

// T9 Keyboard Layout
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
  "SAVE", "0 _<", "#"
};

bool popupActive = false;
unsigned long popupStartTime = 0;
int activePopupIndex = -1;
int popupCount = 0;
String lastPopupChars[6];
int popupWidth = 50;
int popupSpacing = 5;
int popupXPositions[6];

// ===== EEPROM HELPERS =====
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

void saveWiFiToEEPROM() {
  currentSSID = typedSSID;
  currentPassword = typedPassword;
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

// Helper to get heading for edit field
String getEditHeading(int field) {
  switch (field) {
    case 0: return "WIFI-NAME";
    case 1: return "WIFI-PASSWORD";
    case 2: return "Valid Blink";
    case 3: return "Consecutive Gap";
    default: return "";
  }
}

// --- Flicker-free T9 input interface ---
void drawValueBar() {
  tft.setTextColor(TFT_WHITE);
  tft.drawRect(15, 50, 290, 40, TFT_WHITE);
  tft.fillRect(16, 51, 288, 38, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  if (editField == 0 || editField == 1) tft.setCursor(-100, 30); // Name or Password  x,y for text in second interface
  else tft.setCursor(23, 70); // Valid Blink or Consecutive Gap
  tft.print(editBuffer);
}

void drawT9Grid() {
  int t9Y = 160; // Move T9 grid lower to make space for popup bar
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

void drawPopupBar() {
  if (selectedT9Key == -1) return;
  String label = labels[selectedT9Key];
  int popupCount = 0;
  String popupChars[6];
  for (int i = 0; i < label.length(); i++) {
    if (label[i] != ' ') popupChars[popupCount++] = String(label[i]);
  }
  // Place popup bar just below the value bar (value bar y=50, h=40, so popupBarY=100)
  int popupBarY = 100;
  int popupWidth = 50;
  int popupSpacing = 5;
  int totalWidth = popupCount * popupWidth + (popupCount - 1) * popupSpacing;
  int popupX = (320 - totalWidth) / 2;
  for (int i = 0; i < popupCount; i++) {
    int px = popupX + i * (popupWidth + popupSpacing);
    uint16_t popupColor = (selectedPopupLetter == i) ? TFT_GREEN : TFT_BLACK;
    tft.fillRect(px, popupBarY, popupWidth, 40, popupColor);
    tft.drawRect(px, popupBarY, popupWidth, 40, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    int tw = tft.textWidth(popupChars[i]);
    tft.setCursor(px + (popupWidth - tw) / 2, popupBarY + 12);
    tft.print(popupChars[i]);
  }
}

void drawEditScreenStatic() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(15, 10); // x,y  for heading in setting interface
  tft.print(editHeading);
  drawValueBar();
  drawT9Grid();
  if (selectedT9Key != -1) drawPopupBar();
}

void drawEditScreen() {
  drawEditScreenStatic();
}

void enterEditScreen(int field) {
  editField = field;
  editHeading = getEditHeading(field);
  selectedT9Key = -1;
  selectedPopupLetter = -1;
  inEditScreen = true;
  switch (field) {
    case 0: editBuffer = typedSSID; break;
    case 1: editBuffer = typedPassword; break;
    case 2: editBuffer = String(minBlinkDuration); break;
    case 3: editBuffer = String(consecutiveGap); break;
  }
  drawEditScreen();
}

void saveEditBuffer() {
  switch (editField) {
    case 0: typedSSID = editBuffer; saveWiFiToEEPROM(); break;
    case 1: typedPassword = editBuffer; saveWiFiToEEPROM(); break;
    case 2: minBlinkDuration = editBuffer.toInt(); saveConfigToEEPROM(); break;
    case 3: consecutiveGap = editBuffer.toInt(); saveConfigToEEPROM(); break;
  }
}

void drawMainUI() {
  inKeyboardMode = false;
  inBlinkEditMode = false;
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);

  // Divide screen into two large bordered sections (320x480, setRotation(2))
  // Upper: WiFi Settings (top half)
  tft.drawRect(0, 0, 320, 240, TFT_WHITE); // WiFi block border
  tft.setCursor(15, 10);
  tft.setTextColor(TFT_YELLOW);
  tft.print("WiFi Settings");

  // Name label and bar
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(15, 50);
  tft.print("Name");
  tft.drawRect(15, 80, 290, 40, TFT_WHITE); // Full-width bar
  tft.fillRect(16, 81, 288, 38, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(-100, 64); // x=-100, y=64
  tft.print(typedSSID);

  // Password label and bar
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(15, 140);
  tft.print("Password");
  tft.drawRect(15, 170, 290, 40, TFT_WHITE); // Full-width bar
  tft.fillRect(16, 171, 288, 38, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(-100, 154); // x=-100, y=154
  tft.print(typedPassword);

  // Lower: Blink Settings (bottom half)
  tft.drawRect(0, 240, 320, 240, TFT_WHITE); // Blink block border
  tft.setCursor(15, 250);
  tft.setTextColor(TFT_YELLOW);
  tft.print("Blink Settings");

  // Valid Blink label and bar
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(15, 290);
  tft.print("Valid Blink");
  tft.drawRect(15, 320, 290, 40, TFT_WHITE); // Full-width bar
  tft.fillRect(16, 321, 288, 38, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(20, 335); // x=20, y=335
  tft.print(String(minBlinkDuration));

  // Consecutive Gap label and bar
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(15, 380);
  tft.print("Consecutive Gap");
  tft.drawRect(15, 410, 290, 40, TFT_WHITE); // Full-width bar
  tft.fillRect(16, 411, 288, 38, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(20, 425); // x=20, y=425
  tft.print(String(consecutiveGap));
}

void drawT9Keyboard() {
  inKeyboardMode = true;
  for (int i = 0; i < 12; i++) {
    int col = i % 3;
    int row = i / 3;
    int x = xOffset + col * (keyWidth + spacing);
    int y = yOffset + row * (keyHeight + spacing);
    tft.fillRect(x, y, keyWidth, keyHeight, TFT_DARKGREY);
    tft.drawRect(x, y, keyWidth, keyHeight, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    int textWidth = tft.textWidth(labels[i]);
    tft.setCursor(x + (keyWidth - textWidth) / 2, y + keyHeight / 2 - 6);
    tft.print(labels[i]);
  }
}

void drawBlinkEditUI() {
  inBlinkEditMode = true;
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 30);
  if (blinkEditField == 0) {
    tft.print("Edit Valid Blink Gap:");
  } else {
    tft.print("Edit Consecutive Gap:");
  }
  tft.drawRect(10, 80, 300, 40, TFT_WHITE);
  tft.fillRect(11, 81, 298, 38, TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(20, 90);
  tft.print(blinkEditBuffer);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 140);
  tft.print("Type value, then tap SAVE");
  drawT9Keyboard();
}

int getButtonIndexFromTouch(int x, int y) {
  for (int i = 0; i < 12; i++) {
    int col = i % 3;
    int row = i / 3;
    int bx = xOffset + col * (keyWidth + spacing);
    int by = yOffset + row * (keyHeight + spacing);
    if (x >= bx && x <= bx + keyWidth && y >= by && y <= by + keyHeight) return i;
  }
  return -1;
}

void drawPopup(int index) {
  popupCount = 0;
  String label = labels[index];
  for (int i = 0; i < label.length(); i++) {
    if (label[i] != ' ') lastPopupChars[popupCount++] = String(label[i]);
  }
  int totalWidth = popupCount * popupWidth + (popupCount - 1) * popupSpacing;
  int popupX = (320 - totalWidth) / 2;
  for (int i = 0; i < popupCount; i++) {
    int px = popupX + i * (popupWidth + popupSpacing);
    popupXPositions[i] = px;
    tft.fillRect(px, popupBarY, popupWidth, popupBarHeight, TFT_BLACK);
    tft.drawRect(px, popupBarY, popupWidth, popupBarHeight, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    int tw = tft.textWidth(lastPopupChars[i]);
    tft.setCursor(px + (popupWidth - tw) / 2, popupBarY + 6);
    tft.print(lastPopupChars[i]);
  }
}

int getPopupLetterIndexFromTouch(int x, int y) {
  for (int i = 0; i < popupCount; i++) {
    int px = popupXPositions[i];
    if (x >= px && x <= px + popupWidth && y >= popupBarY && y <= popupBarY + popupBarHeight) return i;
  }
  return -1;
}

// --- Update handleTouch for flicker-free T9 ---
void handleTouch() {
  uint16_t tx, ty;
  if (!inEditScreen) {
    if (tft.getTouch(&tx, &ty)) {
      // Name bar
      if (tx >= 15 && tx <= 305 && ty >= 80 && ty <= 120) {
        enterEditScreen(0);
      } else if (tx >= 15 && tx <= 305 && ty >= 170 && ty <= 210) {
        enterEditScreen(1);
      } else if (tx >= 15 && tx <= 305 && ty >= 320 && ty <= 360) {
        enterEditScreen(2);
      } else if (tx >= 15 && tx <= 305 && ty >= 410 && ty <= 450) {
        enterEditScreen(3);
      }
    }
    return;
  }

  // In edit screen
  if (tft.getTouch(&tx, &ty)) {
    // If popup bar is active
    if (selectedT9Key != -1) {
      // Find which popup letter is touched
      String label = labels[selectedT9Key];
      int popupCount = 0;
      String popupChars[6];
      for (int i = 0; i < label.length(); i++) {
        if (label[i] != ' ') popupChars[popupCount++] = String(label[i]);
      }
      // Place popup bar just below the value bar (value bar y=50, h=40, so popupBarY=100)
      int popupBarY = 100;
      int popupWidth = 50;
      int popupSpacing = 5;
      int totalWidth = popupCount * popupWidth + (popupCount - 1) * popupSpacing;
      int popupX = (320 - totalWidth) / 2;
      for (int i = 0; i < popupCount; i++) {
        int px = popupX + i * (popupWidth + popupSpacing);
        if (tx >= px && tx <= px + popupWidth && ty >= popupBarY && ty <= popupBarY + 40) {
          selectedPopupLetter = i;
          drawPopupBar(); // Only redraw popup bar for highlight
          delay(120); // Visual feedback
          // Add letter to buffer
          String sel = popupChars[i];
          if (sel == "<") {
            if (editBuffer.length()) editBuffer.remove(editBuffer.length() - 1);
          } else if (sel == "_") {
            editBuffer += " ";
          } else {
            editBuffer += sel;
          }
          selectedT9Key = -1;
          selectedPopupLetter = -1;
          drawValueBar(); // Only redraw value bar
          drawT9Grid();   // Only redraw T9 grid
          return;
        }
      }
      // If popup bar is up but not touched, ignore
      return;
    }
    // T9 keyboard
    int t9Y = 160;
    for (int i = 0; i < 12; i++) {
      int col = i % 3;
      int row = i / 3;
      int x = 15 + col * (90 + 10);
      int y = t9Y + row * (60 + 10);
      if (tx >= x && tx <= x + 90 && ty >= y && ty <= y + 60) {
        selectedT9Key = i;
        drawT9Grid(); // Only redraw T9 grid for highlight
        drawPopupBar(); // Only draw popup bar
        delay(120); // Visual feedback
        // If SAVE key (index 9)
        if (i == 9) {
          saveEditBuffer();
          inEditScreen = false;
          drawMainUI();
          return;
        }
        // Show popup bar for this key
        selectedPopupLetter = -1;
        drawPopupBar();
        return;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  tft.init();
  tft.setRotation(2);
  uint16_t calData[5] = { 471, 2859, 366, 3388, 2 };
  tft.setTouch(calData);
  loadConfigFromEEPROM();
  typedSSID = currentSSID;
  typedPassword = currentPassword;
  drawMainUI();
}

void loop() {
  handleTouch();
}
