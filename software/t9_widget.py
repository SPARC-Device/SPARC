"""
@file
@brief Implements the main Grid Widget
"""


from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QGridLayout, QPushButton,
    QLabel, QFrame, QStackedWidget
)
from PyQt5.QtCore import Qt, QTimer
from controller import Controller
from character_popup import CharacterPopup
from settings_widget import SettingsWidget
import os


T9_KEYS = {
    "1": ["A", "B", "C", "1"],
    "2": ["D", "E", "F", "2"],
    "3": ["G", "H", "I", "3"],
    "4": ["J", "K", "L", "4"],
    "5": ["M", "N", "O", "5"],
    "6": ["P", "Q", "R", "6"],
    "7": ["S", "T", "U", "7"],
    "8": ["V", "W", "X", "8"],
    "9": ["Y", "Z", "9"],
    "📞": ["🍞", "🚽", "📞"],
    "0": ["␣", "←", "0"],
    "⚙": [""]
}


class T9Window(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("T9 Typing GUI")
        self.setFocusPolicy(Qt.StrongFocus)
        # Set dark background for the main window
        self.setStyleSheet("background-color: #232936;")

        self.T9_KEYS = T9_KEYS

        self.controller = Controller(self)

        self.popup = CharacterPopup(self)
        self.controller.set_popup(self.popup)

        # Create stacked widget for interface switching
        self.stacked_widget = QStackedWidget()
        
        # Create main T9 interface
        self.main_widget = QWidget()
        self.init_main_ui()
        
        # Create settings interface
        self.settings_widget = SettingsWidget(self)  # self is T9Window
        
        # Add widgets to stacked widget
        self.stacked_widget.addWidget(self.main_widget)
        self.stacked_widget.addWidget(self.settings_widget)
        
        # Set main layout
        main_layout = QVBoxLayout()
        main_layout.addWidget(self.stacked_widget)
        self.setLayout(main_layout)

        # Highlight first button
        self.controller.set_buttons(self.buttons)
        self.controller.highlight_button(0)

    def init_main_ui(self):
        """Initialize the main T9 interface"""
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
                # Create T9-style button text with letters prominent and digit as subscript
                if len(T9_KEYS[key]) > 2:
                    # For keys with letters, show letters prominently
                    letters = ' '.join(T9_KEYS[key][:-1])  # All elements except the last (digit)
                    btn_text = f"{letters}\n{key}"
                else:
                    btn_text = f"{key} {' '.join(T9_KEYS[key][1:])}"

                btn = QPushButton()
                btn.setText(btn_text)
                btn.setProperty("t9_key", key)  # Store the actual key for reference
                btn.setFocusPolicy(Qt.NoFocus)
                self.grid.addWidget(btn, i, j)
                self.buttons.append(btn)
        # Apply T9 grid stylesheet to the grid widget
        with open(os.path.join(os.path.dirname(__file__), "t9_grid.qss"), "r") as f:
            self.grid_widget.setStyleSheet(f.read())
        layout.addWidget(self.grid_widget)
        self.main_widget.setLayout(layout)

    def show_settings(self):
        """Switch to settings interface"""
        self.stacked_widget.setCurrentWidget(self.settings_widget)
        self.settings_widget.setFocus()

    def show_main_interface(self):
        """Switch back to main T9 interface"""
        self.stacked_widget.setCurrentWidget(self.main_widget)
        self.setFocus()

    def keyPressEvent(self, event):
        self.controller.handle_key(event)
        self.setFocus()

