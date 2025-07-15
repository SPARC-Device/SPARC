import time
import socket

# Replace with your Arduino's IP and port
ARDUINO_IP = "192.168.1.184"  # Example: for ESP8266 AP mode
PORT = 23

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        print(f"Connecting to Arduino at {ARDUINO_IP}:{PORT}...")
        s.connect((ARDUINO_IP, PORT))
        print("Connected!\n")
        data = s.recv(1024)
        print(data.decode().strip())
        print("Receiving IR Data...")

        s.sendall(b"minblinkduration=600")

        while True:
            data = s.recv(1024)
            if not data:
                break
            print("Received IR data:", data.decode().strip())
    except Exception as e:
        print("Error:", e)
    finally:
        s.close()

if __name__ == "__main__":
    main()

