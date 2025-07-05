from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel
from PyQt5.QtCore import QTimer
from serial_listener import SerialBlinkListener


class BlinkGUI(QWidget):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Blink Interface")
        self.label = QLabel("Waiting for blink...")
        
        layout = QVBoxLayout()
        layout.addWidget(self.label)
        self.setLayout(layout)

        # Serial listener setup
        self.listener = SerialBlinkListener()
        self.listener.start()

        # Check for new data every 100ms
        self.timer = QTimer()
        self.timer.timeout.connect(self.check_serial)
        self.timer.start(100)


    def check_serial(self):
        blink = self.listener.get_blink()
        if blink is not None:
            self.label.setText(f"Received blink: {blink}")
            # TODO: map blink to action


if __name__ == '__main__':
    import sys
    app = QApplication(sys.argv)
    win = BlinkGUI()
    win.show()
    sys.exit(app.exec_())

