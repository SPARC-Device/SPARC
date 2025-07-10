"""
@file
@brief Implements the controller for T9 Grid
"""



from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtGui import QKeyEvent


class Controller:
    def __init__(self, window):
        self.window = window
        self.buttons = []
        self.current_index = 0
        self.mode = "grid"  # "grid" or "char_select"
        self.char_index = 0
        self.timer = QTimer()
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.reset_to_grid)

        self.current_chars = []
        self.last_selected_key = ""


    def set_buttons(self, buttons):
        self.buttons = buttons


    def set_popup(self, popup):
        self.popup = popup


    def highlight_button(self, index):
        for i, btn in enumerate(self.buttons):
            if i == index:
                btn.setProperty("selected", True)
            else:
                btn.setProperty("selected", False)
            btn.style().unpolish(btn)
            btn.style().polish(btn)


    def handle_key(self, event: QKeyEvent):
        print(f'Processing key:')

        if event.key() == Qt.Key_Right:
            print("Right", end=' ')

            if self.mode == "grid":
                print(f'in {self.mode}\n')
                self.current_index = (self.current_index + 1) % len(self.buttons)
                self.highlight_button(self.current_index)

            elif self.mode == "char_select":
                print(f'in {self.mode}\n')
                self.popup.next_char()          # Move highlight in popup
                # Restart timer on navigation to prevent premature timeout
                self.timer.start(1000)

            print(f'{self.mode=}\n')

        elif event.key() == Qt.Key_Return or event.key() == Qt.Key_Enter:
            print('Enter', end=' ')

            if self.mode == "grid":
                print(f'in {self.mode}\n')
                key_text = self.buttons[self.current_index].text().split()[0]
                self.current_chars = self.window.T9_KEYS.get(key_text, [key_text])
                self.last_selected_key = key_text
                
                print("Changed mode to `char_select`\n")
                self.mode = "char_select"

                self.popup.show_popup(self.current_chars, self.buttons[self.current_index])
                # press esc
                self.timer.start(2000)

            elif self.mode == "char_select":
                print(f'in {self.mode}\n')
                char = self.popup.get_selected_char()
                if char == "⌫":
                    text = self.window.text_display.text()
                    self.window.text_display.setText(text[:-1])
                else:
                    self.window.text_display.setText(self.window.text_display.text() + char)

                # self.popup.hide_popup()
                self.reset_to_grid()

        print('Processed Key.\n')


    def reset_to_grid(self):
        print('Changed mode to `grid`\n')

        self.mode = "grid"
        self.char_index = 0         # Optional: keep for compatibility
        self.highlight_button(self.current_index)
        if hasattr(self, "popup"):
            self.popup.hide_popup()  # Hide popup on reset


    def update_char_highlight(self):
        key = self.last_selected_key
        all_chars = self.window.T9_KEYS[key]
        char_display = " ".join([
            f"[{c}]" if i == self.char_index else c
            for i, c in enumerate(all_chars)
        ])
        btn = self.buttons[self.current_index]
        btn.setText(f"{key} {char_display}")

