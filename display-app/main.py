"""
@file
@brief Entry Point for the application
"""


import sys

from PyQt5.QtWidgets import QApplication

from main_widget import MainWidget



if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWidget()
    window.show()
    sys.exit(app.exec_())
