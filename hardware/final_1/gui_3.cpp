#include "gui_3.h"
#include "emoji_arrays.h"
#include <TFT_eSPI.h>

// --- Static variables for T9 state and UI ---
static TFT_eSPI tft = TFT_eSPI();
static String typedMessage = "";

// T9 layout and state
static const char* labels[12] = {
  "1 ABC", "2 DEF", "3 GHI",
  "4 JKL", "5 MNO", "6 PQR",
  "7 STU", "8 VWX", "9 YZ.",
  "", "0 _<-", "#"
};
static int selectedCell = 0; // 0-11
static bool popupActive = false;
static int popupIndex = 0; // index in popup
static int popupCount = 0;
static String lastPopupChars[6];
static int popupXPositions[6];
static int popupWidth = 50;
static int popupSpacing = 5;
static const int popupBarY = 130;
static const int popupBarHeight = 30;
static unsigned long popupStartTime = 0;
static const unsigned long popupTimeout = 3000; // 3 seconds

// --- Forward declarations for static helper functions ---
static void drawMessageBox();
static void drawT9Grid();
static void highlightCell(int index);
static void drawButton(int index, bool highlightYellow);
static void setupPopup(int index);
static void drawPopup();
static void drawPopupSelection(int idx);
static void clearPopupText();

// --- Setup ---
void gui3Setup() {
    Serial.begin(9600);
    tft.init();
    tft.setRotation(2);
    uint16_t calData[5] = { 471, 2859, 366, 3388, 2 };
    tft.setTouch(calData);
    tft.fillScreen(TFT_BLACK);
    drawMessageBox();
    drawT9Grid();
    highlightCell(selectedCell);
}

// --- Main loop: handles periodic tasks (should be called in Arduino loop) ---
void gui3Loop() {
    gui3CheckPopupTimeout();
}

// --- Blink event: single blink ---
void gui3OnSingleBlink() {
    if (!popupActive) {
        // Move to next cell (cyclic)
        int prevCell = selectedCell;
        selectedCell = (selectedCell + 1) % 12;
        drawButton(prevCell, false); // revert previous to dark grey
        highlightCell(selectedCell); // highlight new cell
    } else {
        // Move to next popup button (cyclic)
        int prevPopup = popupIndex;
        popupIndex = (popupIndex + 1) % popupCount;
        drawPopup(); // redraw all, only one is navy blue
        popupStartTime = millis(); // reset timer
    }
}

// --- Blink event: double blink ---
void gui3OnDoubleBlink() {
    if (!popupActive) {
        // Select current cell, show popup, turn cell yellow
        drawButton(selectedCell, true); // yellow highlight
        setupPopup(selectedCell);
        popupActive = true;
        popupIndex = 0;
        drawPopup();
        popupStartTime = millis();
    } else {
        // Select current popup button, turn it green, add to message, clear popup
        drawPopupSelection(popupIndex); // green highlight
        delay(150); // brief visual feedback
        String sel = lastPopupChars[popupIndex];
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
        drawButton(selectedCell, false); // revert cell from yellow to navy blue
        highlightCell(selectedCell);
        popupActive = false;
    }
}

// --- Popup timeout handler (should be called periodically) ---
void gui3CheckPopupTimeout() {
    if (popupActive && (millis() - popupStartTime >= popupTimeout)) {
        clearPopupText();
        drawButton(selectedCell, false); // revert cell from yellow to navy blue
        highlightCell(selectedCell);
        popupActive = false;
    }
}

// --- Drawing and helper functions ---
static void drawMessageBox() {
    tft.fillRect(10, 10, 300, 100, TFT_NAVY);
    tft.drawRect(10, 10, 300, 100, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.setTextSize(3);
    tft.setCursor(15, 25);
    tft.print(typedMessage);
}

static void drawT9Grid() {
    for (int i = 0; i < 12; i++) drawButton(i, false);
}

static void highlightCell(int index) {
    int col = index % 3;
    int row = index / 3;
    int x = 15 + col * (90 + 10);
    int y = 180 + row * (60 + 10);
    tft.fillRect(x, y, 90, 60, TFT_NAVY);
    tft.drawRect(x, y, 90, 60, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.setTextSize(2);
    int textWidth = tft.textWidth(labels[index]);
    int textX = x + (90 - textWidth) / 2;
    int textY = y + (60 / 2) + 4;
    tft.setCursor(textX, textY);
    tft.print(labels[index]);
}

static void drawButton(int index, bool highlightYellow) {
    int col = index % 3;
    int row = index / 3;
    int x = 15 + col * (90 + 10);
    int y = 180 + row * (60 + 10);
    uint16_t fill = highlightYellow ? TFT_YELLOW : TFT_DARKGREY;
    uint16_t text = highlightYellow ? TFT_BLACK : TFT_WHITE;
    tft.fillRect(x, y, 90, 60, fill);
    tft.drawRect(x, y, 90, 60, TFT_WHITE);
    if (index == 9) {
        tft.pushImage(x + 5, y + 25, 24, 24, emoji_toilet);
        tft.pushImage(x + 33, y + 25, 24, 24, emoji_food);
        tft.pushImage(x + 61, y + 25, 24, 24, emoji_doctor);
    } else {
        tft.setTextColor(text, fill);
        tft.setTextSize(2);
        int textWidth = tft.textWidth(labels[index]);
        int textX = x + (90 - textWidth) / 2;
        int textY = y + (60 / 2) + 4;
        tft.setCursor(textX, textY);
        tft.print(labels[index]);
    }
}

static void setupPopup(int index) {
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
        popupXPositions[i] = popupX + i * (popupWidth + popupSpacing);
    }
}

static void drawPopup() {
    for (int i = 0; i < popupCount; i++) {
        int px = popupXPositions[i];
        uint16_t fill = (i == popupIndex) ? TFT_NAVY : TFT_BLACK;
        uint16_t text = (i == popupIndex) ? TFT_WHITE : TFT_WHITE;
        tft.fillRect(px, popupBarY, popupWidth, popupBarHeight, fill);
        tft.drawRect(px, popupBarY, popupWidth, popupBarHeight, TFT_WHITE);
        tft.setTextColor(text, fill);
        tft.setTextSize(2);
        String boxText = lastPopupChars[i];
        int tw = tft.textWidth(boxText);
        int tx = px + (popupWidth - tw) / 2;
        int ty = popupBarY + (popupBarHeight / 2) - 6;
        tft.setCursor(tx, ty);
        tft.print(boxText);
    }
}

static void drawPopupSelection(int idx) {
    int px = popupXPositions[idx];
    tft.fillRect(px, popupBarY, popupWidth, popupBarHeight, TFT_GREEN);
    tft.drawRect(px, popupBarY, popupWidth, popupBarHeight, TFT_WHITE);
    tft.setTextColor(TFT_BLACK, TFT_GREEN);
    tft.setTextSize(2);
    String sel = lastPopupChars[idx];
    int tw = tft.textWidth(sel);
    int tx = px + (popupWidth - tw) / 2;
    int ty = popupBarY + (popupBarHeight / 2) - 6;
    tft.setCursor(tx, ty);
    tft.print(sel);
}

static void clearPopupText() {
    for (int i = 0; i < popupCount; i++) {
        int px = popupXPositions[i];
        tft.fillRect(px, popupBarY, popupWidth, popupBarHeight, TFT_BLACK);
    }
} 