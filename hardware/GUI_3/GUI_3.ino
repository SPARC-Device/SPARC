#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // TFT instance

String typedMessage = "";

// Popup variables
bool popupActive = false;
unsigned long popupStartTime = 0;
int activePopupIndex = -1;
int popupCount = 0;
String lastPopupChars[6];
int popupWidth = 50;
int popupSpacing = 5;
int popupXPositions[6];
const int popupBarY = 130;              // Y position of popup bar (adjusted to be slightly lower)
const int popupBarHeight = 30;

// cursor blinking 
bool cursorVisible = true;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 500;  // in milliseconds



// Layout Constants
const int keyWidth = 90;
const int keyHeight = 60;
const int spacing = 10;
const int xOffset = 15;
const int yOffset = 180;
const int messageBoxHeight = 100;

// Labels for 12 buttons (Note: Added _ and <- to key "0")
const char* labels[12] = {
  "1 ABC", "2 DEF", "3 GHI",
  "4 JKL", "5 MNO", "6 PQR",
  "7 STU", "8 VWX", "9 YZ",
  "*", "0 _<-", "#"
};

void setup() {
  Serial.begin(9600);
  tft.init();
  tft.setRotation(2);
  uint16_t calData[5] = { 471, 2859, 366, 3388, 2 };
  tft.setTouch(calData);
  tft.fillScreen(TFT_BLACK);
  drawMessageBox();
  drawT9Grid();
}

void loop() {
  uint16_t tx, ty;

  if (popupActive) {
    if (tft.getTouch(&tx, &ty)) {
      int index = getPopupLetterIndexFromTouch(tx, ty);
      if (index != -1) {
        int px = popupXPositions[index];

        // Highlight selected box green
        tft.fillRect(px, popupBarY, popupWidth, popupBarHeight, TFT_GREEN);
        tft.drawRect(px, popupBarY, popupWidth, popupBarHeight, TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.setTextSize(2);

        String boxText = lastPopupChars[index];
        int tw = tft.textWidth(boxText);
        int tx = px + (popupWidth - tw) / 2;
        int ty = popupBarY + (popupBarHeight / 2) - 6;

        tft.setCursor(tx, ty);
        tft.print(boxText);

        // Process selected character
        String sel = lastPopupChars[index];
        if (sel == "<") {
          if (typedMessage.length()) typedMessage.remove(typedMessage.length() - 1);
        } else if (sel == "_") {
          typedMessage += " ";
        } else {
          typedMessage += sel;
        }

        drawMessageBox();
        clearPopupText();
        drawButton(activePopupIndex);
        popupActive = false;
        activePopupIndex = -1;
        return;
      }
    }

      // Blink cursor every 500ms
     if (millis() - lastBlinkTime >= blinkInterval) {
             cursorVisible = !cursorVisible;
             drawMessageBox();  // Redraw message + cursor
             lastBlinkTime = millis();
       }



    // Clear popup after 3 seconds
    if (millis() - popupStartTime >= 3000) {
      clearPopupText();
      if (activePopupIndex != -1) drawButton(activePopupIndex);
      popupActive = false;
      activePopupIndex = -1;
    }
  } else {
    handleTouch();
  }
}

// Draw message box at top
void drawMessageBox() {
  tft.fillRect(10, 10, 300, messageBoxHeight, TFT_NAVY);  
  tft.drawRect(10, 10, 300, messageBoxHeight, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(3);
  tft.setCursor(15, 25);
  tft.print(typedMessage);

  // Draw cursor if visible
  if (cursorVisible) {
    int textWidth = tft.textWidth(typedMessage);
    int cursorX = 15 + textWidth;
    int cursorY = 25;
    int cursorHeight = 24;  // matches font size

    tft.drawLine(cursorX, cursorY, cursorX, cursorY + cursorHeight, TFT_WHITE);
  }
}


// Draw the full keypad
void drawT9Grid() {
  for (int i = 0; i < 12; i++) drawButton(i);
}

void drawButton(int index) {
  int col = index % 3;
  int row = index / 3;
  int x = xOffset + col * (keyWidth + spacing);
  int y = yOffset + row * (keyHeight + spacing);

  tft.fillRect(x, y, keyWidth, keyHeight, TFT_DARKGREY);
  tft.drawRect(x, y, keyWidth, keyHeight, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextSize(2);

  int textWidth = tft.textWidth(labels[index]);
  int textX = x + (keyWidth - textWidth) / 2;
  int textY = y + (keyHeight / 2) + 4;

  tft.setCursor(textX, textY);
  tft.print(labels[index]);
}

void handleTouch() {
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    int index = getButtonIndexFromTouch(x, y);
    if (index != -1) {
      highlightButton(index);
      drawPopup(index);
      popupActive = true;
      popupStartTime = millis();
      activePopupIndex = index;
    }
  }
}

int getButtonIndexFromTouch(int touchX, int touchY) {
  for (int i = 0; i < 12; i++) {
    int col = i % 3;
    int row = i / 3;
    int x = xOffset + col * (keyWidth + spacing);
    int y = yOffset + row * (keyHeight + spacing);

    if (touchX >= x && touchX <= x + keyWidth &&
        touchY >= y && touchY <= y + keyHeight) {
      return i;
    }
  }
  return -1;
}

void highlightButton(int index) {
  int col = index % 3;
  int row = index / 3;
  int x = xOffset + col * (keyWidth + spacing);
  int y = yOffset + row * (keyHeight + spacing);

  tft.fillRect(x, y, keyWidth, keyHeight, TFT_YELLOW);
  tft.drawRect(x, y, keyWidth, keyHeight, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_YELLOW);
  tft.setTextSize(2);

  int textWidth = tft.textWidth(labels[index]);
  int textX = x + (keyWidth - textWidth) / 2;
  int textY = y + (keyHeight / 2) + 4;

  tft.setCursor(textX, textY);
  tft.print(labels[index]);
}

// Draw the popup bar
void drawPopup(int index) {
  String label = labels[index];
  label.trim();
  popupCount = 0;

  if (index == 10) {  // "0 _<-"
    lastPopupChars[popupCount++] = "0";
    lastPopupChars[popupCount++] = "_";
    lastPopupChars[popupCount++] = "<";
  } else {
    for (int i = 0; i < label.length(); i++) {
      if (label[i] != ' ') {
        lastPopupChars[popupCount++] = String(label[i]);
      }
    }
  }

  int totalWidth = popupCount * popupWidth + (popupCount - 1) * popupSpacing;
  int popupX = (320 - totalWidth) / 2;

  for (int i = 0; i < popupCount; i++) {
    int px = popupX + i * (popupWidth + popupSpacing);
    popupXPositions[i] = px;

    tft.fillRect(px, popupBarY, popupWidth, popupBarHeight, TFT_BLACK);
    tft.drawRect(px, popupBarY, popupWidth, popupBarHeight, TFT_WHITE);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    String boxText = lastPopupChars[i];
    int tw = tft.textWidth(boxText);
    int tx = px + (popupWidth - tw) / 2;
    int ty = popupBarY + (popupBarHeight / 2) - 6;

    tft.setCursor(tx, ty);
    tft.print(boxText);
  }
}

void clearPopupText() {
  for (int i = 0; i < popupCount; i++) {
    int px = popupXPositions[i];
    tft.fillRect(px, popupBarY, popupWidth, popupBarHeight, TFT_BLACK);
  }
}

int getPopupLetterIndexFromTouch(int x, int y) {
  for (int i = 0; i < popupCount; i++) {
    int px = popupXPositions[i];
    if (x >= px && x <= px + popupWidth &&
        y >= popupBarY && y <= popupBarY + popupBarHeight) {
      return i;
    }
  }
  return -1;
}
