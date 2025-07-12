"""
@file
@brief Implements the Settings Widget
"""

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QPushButton,
    QLabel, QLineEdit, QSpinBox, QGroupBox
)
from PyQt5.QtCore import Qt
import os
import json


class SettingsWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Settings")
        self.setStyleSheet("background-color: #232936; color: #ECEFF4;")
        
        self.settings_file = os.path.join(os.path.dirname(__file__), "settings.json")
        self.init_ui()
        self.load_settings()
    
    def init_ui(self):
        layout = QVBoxLayout()
        
        # Title
        title = QLabel("Settings")
        title.setStyleSheet("font-size: 24px; font-weight: bold; margin: 10px;")
        title.setAlignment(Qt.AlignCenter)
        layout.addWidget(title)
        
        # WiFi Settings
        wifi_group = QGroupBox("WiFi Configuration")
        wifi_group.setStyleSheet("""
            QGroupBox {
                font-weight: bold;
                border: 2px solid #88C0D0;
                border-radius: 8px;
                margin-top: 10px;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
        """)
        wifi_layout = QVBoxLayout()
        
        # WiFi Name
        wifi_layout.addWidget(QLabel("WiFi Name:"))
        self.wifi_name_input = QLineEdit()
        self.wifi_name_input.setPlaceholderText("Enter WiFi network name")
        self.wifi_name_input.setStyleSheet("""
            QLineEdit {
                background-color: #3B4252;
                border: 2px solid #4C566A;
                border-radius: 6px;
                padding: 8px;
                color: #ECEFF4;
                font-size: 14px;
            }
            QLineEdit:focus {
                border: 2px solid #88C0D0;
            }
        """)
        wifi_layout.addWidget(self.wifi_name_input)
        
        # WiFi Password
        wifi_layout.addWidget(QLabel("WiFi Password:"))
        self.wifi_password_input = QLineEdit()
        self.wifi_password_input.setPlaceholderText("Enter WiFi password")
        self.wifi_password_input.setEchoMode(QLineEdit.Password)
        self.wifi_password_input.setStyleSheet("""
            QLineEdit {
                background-color: #3B4252;
                border: 2px solid #4C566A;
                border-radius: 6px;
                padding: 8px;
                color: #ECEFF4;
                font-size: 14px;
            }
            QLineEdit:focus {
                border: 2px solid #88C0D0;
            }
        """)
        wifi_layout.addWidget(self.wifi_password_input)
        
        wifi_group.setLayout(wifi_layout)
        layout.addWidget(wifi_group)
        
        # Natural Blink Duration Settings
        blink_group = QGroupBox("Natural Blink Duration")
        blink_group.setStyleSheet("""
            QGroupBox {
                font-weight: bold;
                border: 2px solid #88C0D0;
                border-radius: 8px;
                margin-top: 10px;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
        """)
        blink_layout = QVBoxLayout()
        
        # Current value display
        blink_layout.addWidget(QLabel("Current Duration:"))
        self.blink_duration_label = QLabel("1000 ms")
        self.blink_duration_label.setStyleSheet("""
            QLabel {
                background-color: #3B4252;
                border: 2px solid #4C566A;
                border-radius: 6px;
                padding: 8px;
                font-size: 16px;
                font-weight: bold;
                text-align: center;
            }
        """)
        blink_layout.addWidget(self.blink_duration_label)
        
        # +/- buttons
        button_layout = QHBoxLayout()
        
        self.decrease_btn = QPushButton("-")
        self.decrease_btn.setStyleSheet("""
            QPushButton {
                background-color: #BF616A;
                color: #ECEFF4;
                border: 2px solid #D08770;
                border-radius: 6px;
                padding: 10px 15px;
                font-weight: bold;
                font-size: 16px;
            }
            QPushButton:hover {
                background-color: #D08770;
            }
        """)
        self.decrease_btn.clicked.connect(self.decrease_blink_duration)
        
        self.increase_btn = QPushButton("+")
        self.increase_btn.setStyleSheet("""
            QPushButton {
                background-color: #A3BE8C;
                color: #ECEFF4;
                border: 2px solid #B9CA4A;
                border-radius: 6px;
                padding: 10px 15px;
                font-weight: bold;
                font-size: 16px;
            }
            QPushButton:hover {
                background-color: #B9CA4A;
            }
        """)
        self.increase_btn.clicked.connect(self.increase_blink_duration)
        
        button_layout.addWidget(self.decrease_btn)
        button_layout.addWidget(self.increase_btn)
        blink_layout.addLayout(button_layout)
        
        blink_group.setLayout(blink_layout)
        layout.addWidget(blink_group)
        
        # Buttons
        bottom_button_layout = QHBoxLayout()
        
        self.save_btn = QPushButton("Save")
        self.save_btn.setStyleSheet("""
            QPushButton {
                background-color: #5E81AC;
                color: #ECEFF4;
                border: 2px solid #81A1C1;
                border-radius: 6px;
                padding: 10px 20px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #81A1C1;
            }
        """)
        self.save_btn.clicked.connect(self.save_settings)
        
        self.cancel_btn = QPushButton("Cancel")
        self.cancel_btn.setStyleSheet("""
            QPushButton {
                background-color: #BF616A;
                color: #ECEFF4;
                border: 2px solid #D08770;
                border-radius: 6px;
                padding: 10px 20px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #D08770;
            }
        """)
        self.cancel_btn.clicked.connect(self.go_back)
        
        bottom_button_layout.addWidget(self.save_btn)
        bottom_button_layout.addWidget(self.cancel_btn)
        layout.addLayout(bottom_button_layout)
        
        self.setLayout(layout)
        
        # Initialize blink duration
        self.blink_duration = 1000  # Default 1000ms
    
    def load_settings(self):
        """Load settings from JSON file"""
        try:
            if os.path.exists(self.settings_file):
                with open(self.settings_file, 'r') as f:
                    settings = json.load(f)
                    
                    # Load WiFi settings
                    self.wifi_name_input.setText(settings.get('wifi_name', ''))
                    self.wifi_password_input.setText(settings.get('wifi_password', ''))
                    
                    # Load blink duration
                    self.blink_duration = settings.get('blink_duration', 1000)
                    self.update_blink_duration_label()
                    
        except Exception as e:
            print(f"Error loading settings: {e}")
    
    def save_settings(self):
        """Save settings to JSON file"""
        try:
            settings = {
                'wifi_name': self.wifi_name_input.text(),
                'wifi_password': self.wifi_password_input.text(),
                'blink_duration': self.blink_duration
            }
            
            with open(self.settings_file, 'w') as f:
                json.dump(settings, f, indent=2)
            
            print("Settings saved successfully!")
            
            # Update the parent's timer if it exists
            # Find the main T9Window by traversing up the widget hierarchy
            current_widget = self
            while current_widget.parent():
                current_widget = current_widget.parent()
                if hasattr(current_widget, 'controller') and hasattr(current_widget.controller, 'timer'):
                    current_widget.controller.timer.setInterval(self.blink_duration)
                    break

            self.go_back()

        except Exception as e:
            print(f"Error saving settings: {e}")
    
    def update_blink_duration_label(self):
        """Update the blink duration display label"""
        self.blink_duration_label.setText(f"{self.blink_duration} ms")
    
    def increase_blink_duration(self):
        """Increase blink duration by 50ms"""
        self.blink_duration += 50
        if self.blink_duration > 5000:  # Max 5 seconds
            self.blink_duration = 5000
        self.update_blink_duration_label()
    
    def decrease_blink_duration(self):
        """Decrease blink duration by 50ms"""
        self.blink_duration -= 50
        if self.blink_duration < 500:  # Min 500ms
            self.blink_duration = 500
        self.update_blink_duration_label()
    
    def go_back(self):
        """Return to main T9 interface"""
        # Find the main T9Window by traversing up the widget hierarchy
        current_widget = self
        while current_widget.parent():
            current_widget = current_widget.parent()
            if hasattr(current_widget, 'show_main_interface'):
                current_widget.show_main_interface()
                return
