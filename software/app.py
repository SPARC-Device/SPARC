import sys
import threading
import serial
import pyttsx3
from PyQt5.QtWidgets import (
    QApplication, QWidget, QGridLayout, QPushButton,
    QTextEdit, QVBoxLayout, QLabel
)
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QColor, QPalette

# ---- CONFIGURATION ----
SERIAL_PORT = 'COM3'  # Change this for your platform
BAUD_RATE = 9600
TTS_ENABLED = True

# ---- T9 KEYS ----
KEYS = [
    "1 ABC", "2 DEF", "3 GHI",
    "4 JKL", "5 MNO", "6 PQR",
    "7 STU", "8 VWX", "9 YZ_",
    "SPACE", "0 SOS", "CLEAR"
]

class BlinkInterface(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Blink Communication Interface")
        self.cursor = 0

        self.output = QTextEdit()
        self.output.setReadOnly(True)
        self.output.setPlaceholderText("Output will appear here...")

        self.status = QLabel("Waiting for blink input...")

        self.grid = QGridLayout()
        self.buttons = []

        # Set up grid layout
        for i, label in enumerate(KEYS):
            btn = QPushButton(label)
            btn.setFixedSize(120, 60)
            row, col = divmod(i, 3)
            self.grid.addWidget(btn, row, col)
            self.buttons.append(btn)

        self.highlight_button(0)

        layout = QVBoxLayout()
        layout.addLayout(self.grid)
        layout.addWidget(self.output)
        layout.addWidget(self.status)
        self.setLayout(layout)

        # Text-to-speech
        self.engine = pyttsx3.init() if TTS_ENABLED else None

        # Start serial reading
        self.serial_thread = threading.Thread(target=self.read_serial, daemon=True)
        self.serial_thread.start()

    def highlight_button(self, index):
        for i, btn in enumerate(self.buttons):
            if i == index:
                btn.setStyleSheet("background-color: lightgreen; font-weight: bold;")
            else:
                btn.setStyleSheet("")

    def move_cursor(self):
        self.cursor = (self.cursor + 1) % len(self.buttons)
        self.highlight_button(self.cursor)
        self.status.setText(f"Cursor moved to: {KEYS[self.cursor]}")

    def select_key(self):
        key = KEYS[self.cursor]
        if key == "SPACE":
            self.output.insertPlainText(" ")
        elif key == "CLEAR":
            self.output.clear()
        elif key == "0 SOS":
            self.output.append("\n[SOS Triggered]")
            self.status.setText("⚠️ EMERGENCY: SOS Triggered")
            if self.engine:
                self.engine.say("Emergency. SOS triggered.")
                self.engine.runAndWait()
        else:
            self.output.insertPlainText(key[0])

        if self.engine and key not in ["CLEAR", "0 SOS"]:
            self.engine.say(key[0])
            self.engine.runAndWait()

    def handle_blink(self, count):
        if count == 1:
            self.move_cursor()
        elif count == 2:
            self.select_key()
        elif count == 4:
            self.output.append("\n⚠️ EMERGENCY MODE ACTIVATED")
            self.status.setText("⚠️ Emergency blink pattern detected")
            if self.engine:
                self.engine.say("Emergency activated.")
                self.engine.runAndWait()

    def read_serial(self):
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            while True:
                line = ser.readline().decode().strip()
                if line.startswith("BLINK:"):
                    try:
                        count = int(line.split(":")[1])
                        QTimer.singleShot(0, lambda c=count: self.handle_blink(c))
                    except ValueError:
                        pass
        except serial.SerialException as e:
            print("[Serial Error]", e)
            self.status.setText("Serial connection error")

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = BlinkInterface()
    window.show()
    sys.exit(app.exec_())

