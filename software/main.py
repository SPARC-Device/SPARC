"""
@file
@brief Entry Point for the application
"""


import sys
from PyQt5.QtWidgets import QApplication
from t9_widget import T9Window



if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = T9Window()
    window.show()
    sys.exit(app.exec_())

