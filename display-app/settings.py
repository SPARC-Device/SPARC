"""
@file
@brief Implements the Settings Widget (renamed to Settings)
"""

import os
import json

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QPushButton,
    QLabel, QLineEdit, QGroupBox
)
from PyQt5.QtCore import Qt


class Settings(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Settings")
        self.setObjectName("SettingsWindow")
        with open(os.path.join(os.path.dirname(__file__), "settings.qss"), "r") as f:
            self.setStyleSheet(f.read())
        self.settings_file = os.path.join(os.path.dirname(__file__), "settings.json")
        self.init_ui()
        self.load_settings()

    def init_ui(self):
        layout = QVBoxLayout()
        
        title = QLabel("Settings")
        title.setAlignment(Qt.AlignCenter)
        
        layout.addWidget(title)
        
        wifi_group = QGroupBox("WiFi Configuration")
        wifi_layout = QVBoxLayout()
        wifi_layout.addWidget(QLabel("WiFi Name:"))
        
        self.wifi_name_input = QLineEdit()
        self.wifi_name_input.setPlaceholderText("Enter WiFi network name")
        
        wifi_layout.addWidget(self.wifi_name_input)
        wifi_layout.addWidget(QLabel("WiFi Password:"))
        
        self.wifi_password_input = QLineEdit()
        self.wifi_password_input.setPlaceholderText("Enter WiFi password")
        self.wifi_password_input.setEchoMode(QLineEdit.Password)
        
        wifi_layout.addWidget(self.wifi_password_input)
        
        wifi_group.setLayout(wifi_layout)
        
        layout.addWidget(wifi_group)
        
        blink_group = QGroupBox("Natural Blink Duration")
        blink_layout = QVBoxLayout()
        blink_layout.addWidget(QLabel("Current Duration:"))
        
        self.blink_duration_label = QLabel("1000 ms")
        self.blink_duration_label.setProperty("class", "blink-duration")
        
        blink_layout.addWidget(self.blink_duration_label)
        button_layout = QHBoxLayout()
        
        self.decrease_btn = QPushButton("-")
        self.decrease_btn.setProperty("class", "dec")
        self.decrease_btn.clicked.connect(self.decrease_blink_duration)

        self.increase_btn = QPushButton("+")
        self.increase_btn.setProperty("class", "inc")
        self.increase_btn.clicked.connect(self.increase_blink_duration)

        button_layout.addWidget(self.decrease_btn)
        button_layout.addWidget(self.increase_btn)

        blink_layout.addLayout(button_layout)
        blink_group.setLayout(blink_layout)

        layout.addWidget(blink_group)

        bottom_button_layout = QHBoxLayout()

        self.save_btn = QPushButton("Save")
        self.save_btn.setProperty("class", "save")
        self.save_btn.clicked.connect(self.save_settings)

        self.cancel_btn = QPushButton("Cancel")
        self.cancel_btn.setProperty("class", "cancel")
        self.cancel_btn.clicked.connect(lambda: self.go_back(play_cancel=True))

        bottom_button_layout.addWidget(self.save_btn)
        bottom_button_layout.addWidget(self.cancel_btn)

        layout.addLayout(bottom_button_layout)

        self.setLayout(layout)
        self.blink_duration = 1000

    def load_settings(self):
        try:
            if os.path.exists(self.settings_file):
                with open(self.settings_file, 'r') as f:
                    settings = json.load(f)
                    self.wifi_name_input.setText(settings.get('wifi_name', ''))
                    self.wifi_password_input.setText(settings.get('wifi_password', ''))
                    self.blink_duration = settings.get('blink_duration', 1000)
                    self.update_blink_duration_label()
        except Exception as e:
            print(f"Error loading settings: {e}")

    def save_settings(self):
        try:
            # Play save sound
            self._play_audio('play_save')
            settings = {
                'wifi_name': self.wifi_name_input.text(),
                'wifi_password': self.wifi_password_input.text(),
                'blink_duration': self.blink_duration
            }
            with open(self.settings_file, 'w') as f:
                json.dump(settings, f, indent=2)
            print("Settings saved successfully!")
            current_widget = self
            while current_widget.parent():
                current_widget = current_widget.parent()
                if hasattr(current_widget, 'controller') and hasattr(current_widget.controller, 'timer'):
                    current_widget.controller.timer.setInterval(self.blink_duration)
                    break
            self.go_back(play_cancel=False)
        except Exception as e:
            print(f"Error saving settings: {e}")

    def update_blink_duration_label(self):
        self.blink_duration_label.setText(f"{self.blink_duration} ms")

    def increase_blink_duration(self):
        self._play_audio('play_plus')
        self.blink_duration += 50
        if self.blink_duration > 5000:
            self.blink_duration = 5000
        self.update_blink_duration_label()

    def decrease_blink_duration(self):
        self._play_audio('play_minus')
        self.blink_duration -= 50
        if self.blink_duration < 500:
            self.blink_duration = 500
        self.update_blink_duration_label()

    def go_back(self, play_cancel=True):
        if play_cancel:
            self._play_audio('play_cancel')
        current_widget = self
        while current_widget.parent():
            current_widget = current_widget.parent()
            if hasattr(current_widget, 'show_main_interface'):
                current_widget.show_main_interface()
                return

    def _play_audio(self, method):
        # Traverse up to find the main widget and play the audio
        current_widget = self
        while current_widget.parent():
            current_widget = current_widget.parent()
            if hasattr(current_widget, 'controller') and hasattr(current_widget.controller, 'audio_manager'):
                audio_manager = current_widget.controller.audio_manager
                if hasattr(audio_manager, method):
                    getattr(audio_manager, method)()
                break
