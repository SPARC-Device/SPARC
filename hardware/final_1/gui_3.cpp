#include "gui_3.h"
#include "emoji_arrays.h"
#include "emoji_arrays3.h"
#include <TFT_eSPI.h>
#include "notif.h" // Add this include
#include "setting_2.h" // For userId

extern void openSettingsInterface();
// --- Static variables for T9 state and UI ---
static TFT_eSPI tft = TFT_eSPI();
static String typedMessage = "";
static bool cursorVisible = true;
static unsigned long lastCursorBlink = 0;
static const unsigned long cursorBlinkInterval = 500; // ms
static int cursorX = 0; // Global cursor X position

// T9 layout and state
static const char* labels[12] = {
  "1 ABC", "2 DEF", "3 GHI",
  "4 JKL", "5 MNO", "6 PQR",
  "7 STU", "8 VWX", "9 YZ.",
  "", "0 _<-", ""
};
static int selectedCell = 0; // 0-11
static bool popupActive = false;
static bool popupSelecting = false; // New: true when navigating popup
static int popupIndex = 0; // index in popup
static int popupCount = 0;
static String lastPopupChars[6];
static int popupXPositions[6];
static int popupWidth = 50;
static int popupSpacing = 5;
static const int popupBarY = 130;
static const int popupBarHeight = 30;
static unsigned long popupStartTime = 0;
static const unsigned long popupTimeout = 5000; // 5 seconds

// --- Forward declarations for static helper functions ---
static void drawMessageBox();
static void drawT9Grid();
static void highlightCell(int index);
static void drawButton(int index, bool highlightYellow, bool highlightGreen);
static void setupPopup(int index);
static void drawPopup();
static void drawPopupSelection(int idx);
static void clearPopupText();

// --- Setup ---
void gui3Setup() {
    Serial.begin(115200);
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
    // Handle cursor blinking
    if (millis() - lastCursorBlink > cursorBlinkInterval) {
        cursorVisible = !cursorVisible;
        int cursorY = 25;
        int cursorHeight = 24;
        if (cursorVisible) {
            tft.drawLine(cursorX, cursorY, cursorX, cursorY + cursorHeight, TFT_WHITE);
        } else {
            // Erase the cursor by overdrawing with background color
            tft.drawLine(cursorX, cursorY, cursorX, cursorY + cursorHeight, TFT_NAVY);
        }
        lastCursorBlink = millis();
    }
    // --- Touch handling for settings cell (index 11) ---
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
        // Settings cell (index 11) position
        int col = 2;
        int row = 3;
        int cellX = 15 + col * (90 + 10);
        int cellY = 180 + row * (60 + 10);
        int cellW = 90;
        int cellH = 60;
        if (x >= cellX && x < cellX + cellW && y >= cellY && y < cellY + cellH) {
            openSettingsInterface();
        }
    }
}

// --- Blink event: single blink ---
void gui3OnSingleBlink() {
    if (popupActive && popupSelecting) {
        // Move to next popup button (cyclic)
        int prevPopup = popupIndex;
        popupIndex = (popupIndex + 1) % popupCount;
        drawPopup(); // redraw all, only one is navy blue
        popupStartTime = millis(); // reset timer
    } else if (!popupActive) {
        // Move to next cell (cyclic)
        int prevCell = selectedCell;
        selectedCell = (selectedCell + 1) % 12;
        drawButton(prevCell, false, false); // white border
        highlightCell(selectedCell); // yellow border
    }
}

// --- Blink event: double blink ---
void gui3OnDoubleBlink() {
    if (!popupActive) {
        // Select current cell, show popup, turn cell yellow
        drawButton(selectedCell, false, true); // green border
        setupPopup(selectedCell);
        popupActive = true;
        popupSelecting = true; // Now in popup selection mode
        popupIndex = 0;
        drawPopup();
        popupStartTime = millis();
    } else if (popupActive && popupSelecting) {
        // Double blink in popup: select current popup button, add to message bar, clear popup
        drawPopupSelection(popupIndex); // green highlight
        delay(150); // brief visual feedback
        String sel = lastPopupChars[popupIndex];
        if (sel == "<") {
            if (typedMessage.length()) typedMessage.remove(typedMessage.length() - 1);
        } else if (sel == "_") {
            typedMessage += " ";
        } else if (sel == "toilet" || sel == "food" || sel == "doctor") {
            // Send notification request based on selection
            String type;
            if (sel == "toilet") type = "RESTROOM";
            else if (sel == "doctor") type = "DOCTOR_CALL";
            else if (sel == "food") type = "FOOD";
            sendNotificationRequest(userId, type);
        }  else if (sel=="."){ 
            typedMessage = "";
          } else {
            typedMessage += sel;
        }
        drawMessageBox();
        clearPopupText();
        drawButton(selectedCell, false, false); // white border
        highlightCell(selectedCell);
        popupActive = false;
        popupSelecting = false;
    }
}

// --- Popup timeout handler (should be called periodically) ---
void gui3CheckPopupTimeout() {
    if (popupActive && popupSelecting && (millis() - popupStartTime >= popupTimeout)) {
        clearPopupText();
        drawButton(selectedCell, false, false); // white border
        highlightCell(selectedCell);
        popupActive = false;
        popupSelecting = false;
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
    // Draw cursor (always on when message box is redrawn)
    int textWidth = tft.textWidth(typedMessage);
    cursorX = 15 + textWidth + 2; // Update global cursorX with offset
    int cursorY = 25;
    int cursorHeight = 24;
    tft.drawLine(cursorX, cursorY, cursorX, cursorY + cursorHeight, TFT_WHITE);
}

static void drawT9Grid() {
    for (int i = 0; i < 12; i++) drawButton(i, false, false);
}

static void highlightCell(int index) {
    int col = index % 3;
    int row = index / 3;
    int x = 15 + col * (90 + 10);
    int y = 180 + row * (60 + 10);
    tft.fillRect(x, y, 90, 60, TFT_BLACK);
    // Draw thick yellow border for highlight
    for (int t = 0; t < 3; ++t) tft.drawRect(x + t, y + t, 90 - 2 * t, 60 - 2 * t, TFT_YELLOW);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    int textWidth = tft.textWidth(labels[index]);
    int textX = x + (90 - textWidth) / 2;
    int textY = y + (60 / 2) + 4;
    tft.setCursor(textX, textY);
    tft.print(labels[index]);
    if (index == 9) {
        tft.pushImage(x + 5, y + 25, 24, 24, emoji_toilet);
        tft.pushImage(x + 33, y + 25, 24, 24, emoji_food);
        tft.pushImage(x + 61, y + 25, 24, 24, emoji_doctor);
    } else if (index == 11) {
        tft.pushImage(x + 20, y + 7, 48, 48, emoji_settings);
    }
}

static void drawButton(int index, bool highlightYellow, bool highlightGreen) {
    int col = index % 3;
    int row = index / 3;
    int x = 15 + col * (90 + 10);
    int y = 180 + row * (60 + 10);
    uint16_t fill = TFT_BLACK;
    uint16_t border = TFT_WHITE;
    int thickness = 1;
    if (highlightGreen) { border = TFT_GREEN; thickness = 3; }
    else if (highlightYellow) { border = TFT_YELLOW; thickness = 3; }
    tft.fillRect(x, y, 90, 60, fill);
    for (int t = 0; t < thickness; ++t) tft.drawRect(x + t, y + t, 90 - 2 * t, 60 - 2 * t, border);
    if (thickness == 1) tft.drawRect(x, y, 90, 60, border); // ensure thin border for unselected
    tft.setTextColor(TFT_WHITE, fill);
    tft.setTextSize(2);
    int textWidth = tft.textWidth(labels[index]);
    int textX = x + (90 - textWidth) / 2;
    int textY = y + (60 / 2) + 4;
    tft.setCursor(textX, textY);
    tft.print(labels[index]);
    if (index == 9) {
        tft.pushImage(x + 5, y + 25, 24, 24, emoji_toilet);
        tft.pushImage(x + 33, y + 25, 24, 24, emoji_food);
        tft.pushImage(x + 61, y + 25, 24, 24, emoji_doctor);
    } else if (index == 11) {
        tft.pushImage(x + 20, y + 7, 48, 48, emoji_settings);
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
        uint16_t fill = TFT_BLACK;
        uint16_t border = (i == popupIndex) ? TFT_YELLOW : TFT_WHITE;
        int thickness = (i == popupIndex) ? 3 : 1;
        tft.fillRect(px, popupBarY, popupWidth, popupBarHeight, fill);
        for (int t = 0; t < thickness; ++t) tft.drawRect(px + t, popupBarY + t, popupWidth - 2 * t, popupBarHeight - 2 * t, border);
        if (thickness == 1) tft.drawRect(px, popupBarY, popupWidth, popupBarHeight, border);
        tft.setTextColor(TFT_WHITE, fill);
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
    tft.fillRect(px, popupBarY, popupWidth, popupBarHeight, TFT_BLACK);
    for (int t = 0; t < 3; ++t) tft.drawRect(px + t, popupBarY + t, popupWidth - 2 * t, popupBarHeight - 2 * t, TFT_GREEN);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
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