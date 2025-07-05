import serial
import threading
from queue import Queue


SERIAL_PORT: str = 'COM3'
BAUD_RATE: int = 9600


class SerialBlinkListener:
    def __init__(self, callback=None):
        self.queue = Queue()
        self.callback = callback  # optional function to call on blink
        self.running = False


    def start(self):
        self.running = True
        threading.Thread(target=self.read_serial, daemon=True).start()


    def stop(self):
        self.running = False


    def read_serial(self):
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            while self.running:
                line = ser.readline().decode().strip()
                if line.startswith("BLINK:"):
                    try:
                        # TODO
                        # Change this according to the input

                        count = int(line.split(":")[1])
                        self.queue.put(count)
                        if self.callback:
                            self.callback(count)
                    except ValueError:
                        continue
        except serial.SerialException as e:
            print("[Serial Error]", e)


    def get_blink(self):
        if not self.queue.empty():
            return self.queue.get()
        return None

