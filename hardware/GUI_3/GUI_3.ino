#include <TFT_eSPI.h>
#include <SPI.h>
#include "emoji_arrays.h"  // Include the header generated from your Python script

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
const int popupBarY = 130;
const int popupBarHeight = 30;

// cursor blinking 
bool cursorVisible = true;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 500;

// Layout Constants
const int keyWidth = 90;
const int keyHeight = 60;
const int spacing = 10;
const int xOffset = 15;
const int yOffset = 180;
const int messageBoxHeight = 100;

const char* labels[12] = {
  "1 ABC", "2 DEF", "3 GHI",
  "4 JKL", "5 MNO", "6 PQR",
  "7 STU", "8 VWX", "9 YZ",
  "", "0 _<-", "#"
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

        tft.fillRect(px, popupBarY, popupWidth, popupBarHeight, TFT_GREEN);
        tft.drawRect(px, popupBarY, popupWidth, popupBarHeight, TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.setTextSize(2);

        String sel = lastPopupChars[index];
        int tw = tft.textWidth(sel);
        int tx = px + (popupWidth - tw) / 2;
        int ty = popupBarY + (popupBarHeight / 2) - 6;
        tft.setCursor(tx, ty);
        tft.print(sel);

        if (sel == "<") {
          if (typedMessage.length()) typedMessage.remove(typedMessage.length() - 1);
        } else if (sel == "_") {
          typedMessage += " ";
        } else if (sel == "toilet" || sel == "food" || sel == "doctor") {
          typedMessage += sel;
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

    if (millis() - lastBlinkTime >= blinkInterval) {
      cursorVisible = !cursorVisible;
      drawMessageBox();
      lastBlinkTime = millis();
    }

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

void drawMessageBox() {
  tft.fillRect(10, 10, 300, messageBoxHeight, TFT_NAVY);  
  tft.drawRect(10, 10, 300, messageBoxHeight, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(3);
  tft.setCursor(15, 25);
  tft.print(typedMessage);

  if (cursorVisible) {
    int textWidth = tft.textWidth(typedMessage);
    int cursorX = 15 + textWidth;
    int cursorY = 25;
    int cursorHeight = 24;
    tft.drawLine(cursorX, cursorY, cursorX, cursorY + cursorHeight, TFT_WHITE);
  }
}

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

  if (index == 9) {
    tft.pushImage(x + 5, y + 25, 24, 24, emoji_toilet);
    tft.pushImage(x + 33, y + 25, 24, 24, emoji_food);
    tft.pushImage(x + 61, y + 25, 24, 24, emoji_doctor);
  } else {
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.setTextSize(2);
    int textWidth = tft.textWidth(labels[index]);
    int textX = x + (keyWidth - textWidth) / 2;
    int textY = y + (keyHeight / 2) + 4;
    tft.setCursor(textX, textY);
    tft.print(labels[index]);
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

void drawPopup(int index) {
  popupCount = 0;

  if (index == 9) {
    lastPopupChars[popupCount++] = "toilet";
    lastPopupChars[popupCount++] = "food";
    lastPopupChars[popupCount++] = "doctor";
    popupWidth = 80;
  } else if (index == 10) {
    lastPopupChars[popupCount++] = "0";
    lastPopupChars[popupCount++] = "_";
    lastPopupChars[popupCount++] = "<";
    popupWidth = 50;
  } else {
    String label = labels[index];
    for (int i = 0; i < label.length(); i++) {
      if (label[i] != ' ') lastPopupChars[popupCount++] = String(label[i]);
    }
    popupWidth = 50;
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
