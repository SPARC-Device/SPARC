"""
@file
@brief Implements the main Grid Widget
"""


from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QGridLayout, QPushButton,
    QLabel, QFrame
)
from PyQt5.QtCore import Qt, QTimer
from controller import Controller
from character_popup import CharacterPopup
import os


# TODO: Remap
# 1 for SOS

T9_KEYS = {
    "1": ["1", "A", "B", "C"],
    "2": ["2", "D", "E", "F"],
    "3": ["3", "G", "H", "I"],
    "4": ["4", "J", "K", "L"],
    "5": ["5", "M", "N", "O"],
    "6": ["6", "P", "Q", "R"],
    "7": ["7", "S", "T", "U"],
    "8": ["8", "V", "W", "X"],
    "9": ["9", "Y", "Z", "."],
    "*": ["*"],
    "0": ["0", " "],
    "#": ["⌫"]  # Backspace
}


class T9Window(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("T9 Typing GUI")
        self.setFocusPolicy(Qt.StrongFocus)

        self.T9_KEYS = T9_KEYS

        self.controller = Controller(self)

        self.popup = CharacterPopup(self)
        self.controller.set_popup(self.popup)

        self.init_ui()


    def init_ui(self):
        layout = QVBoxLayout()

        # Display typed text
        self.text_display = QLabel("")
        self.text_display.setObjectName("textDisplay")
        self.text_display.setAlignment(Qt.AlignLeft | Qt.AlignVCenter)
        self.text_display.setMinimumHeight(40)
        # Apply input box stylesheet
        with open(os.path.join(os.path.dirname(__file__), "input_box.qss"), "r") as f:
            self.text_display.setStyleSheet(f.read())
        layout.addWidget(self.text_display)

# T9 grid layout
        self.grid_widget = QWidget()
        self.grid = QGridLayout()
        self.grid.setSpacing(10)
        self.grid_widget.setLayout(self.grid)

        self.buttons = []
        keys = list(T9_KEYS.keys())
        for i in range(4):
            for j in range(3):
                idx = i * 3 + j
                key = keys[idx]
                btn = QPushButton(f"{key} {' '.join(T9_KEYS[key][1:])}")
                btn.setFocusPolicy(Qt.NoFocus)
                self.grid.addWidget(btn, i, j)
                self.buttons.append(btn)
        # Apply T9 grid stylesheet to the grid widget
        with open(os.path.join(os.path.dirname(__file__), "t9_grid.qss"), "r") as f:
            self.grid_widget.setStyleSheet(f.read())
        layout.addWidget(self.grid_widget)
        self.setLayout(layout)

        # Highlight first button
        self.controller.set_buttons(self.buttons)
        self.controller.highlight_button(0)


    def keyPressEvent(self, event):
        self.controller.handle_key(event)
        self.setFocus()

