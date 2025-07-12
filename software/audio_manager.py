"""
@file
@brief Implements the Audio Manager for sound effects
"""

from PyQt5.QtMultimedia import QMediaPlayer, QMediaContent
from PyQt5.QtCore import QUrl
import os


class AudioManager:
    def __init__(self):
        self.move_player = QMediaPlayer()
        self.confirm_player = QMediaPlayer()
        self.audio_available = True
        
        # Load audio files
        assets_dir = os.path.join(os.path.dirname(__file__), "assets")
        
        # Load move sound
        move_path = os.path.join(assets_dir, "move.wav")
        if os.path.exists(move_path):
            try:
                self.move_player.setMedia(QMediaContent(QUrl.fromLocalFile(move_path)))
                # Check if media loaded successfully
                if self.move_player.mediaStatus() == QMediaPlayer.NoMedia:
                    print("Warning: Could not load move.wav - audio will be disabled")
                    self.audio_available = False
            except Exception as e:
                print(f"Error loading move.wav: {e}")
                self.audio_available = False
        else:
            print("Warning: move.wav not found - audio will be disabled")
            self.audio_available = False
        
        # Load confirm sound
        confirm_path = os.path.join(assets_dir, "confirm.wav")
        if os.path.exists(confirm_path):
            try:
                self.confirm_player.setMedia(QMediaContent(QUrl.fromLocalFile(confirm_path)))
                # Check if media loaded successfully
                if self.confirm_player.mediaStatus() == QMediaPlayer.NoMedia:
                    print("Warning: Could not load confirm.wav - audio will be disabled")
                    self.audio_available = False
            except Exception as e:
                print(f"Error loading confirm.wav: {e}")
                self.audio_available = False
        else:
            print("Warning: confirm.wav not found - audio will be disabled")
            self.audio_available = False
    
    def play_move_sound(self):
        """Play the move navigation sound"""
        if not self.audio_available:
            return
        try:
            if self.move_player.mediaStatus() == QMediaPlayer.LoadedMedia:
                self.move_player.setPosition(0)  # Reset to beginning
                self.move_player.play()
        except Exception as e:
            print(f"Error playing move sound: {e}")
    
    def play_confirm_sound(self):
        """Play the confirm selection sound"""
        if not self.audio_available:
            return
        try:
            if self.confirm_player.mediaStatus() == QMediaPlayer.LoadedMedia:
                self.confirm_player.setPosition(0)  # Reset to beginning
                self.confirm_player.play()
        except Exception as e:
            print(f"Error playing confirm sound: {e}")
    
    def set_volume(self, volume):
        """Set volume for both players (0-100)"""
        if not self.audio_available:
            return
        try:
            self.move_player.setVolume(volume)
            self.confirm_player.setVolume(volume)
        except Exception as e:
            print(f"Error setting volume: {e}")
    
    def is_audio_available(self):
        """Check if audio is available"""
        return self.audio_available
