#!/usr/bin/env python3

import socket
import time

def test_hardware_connection(hardware_ip="192.168.120.13", hardware_port=5000):
    """Test connection to ESP32 hardware to trigger IP capture"""
    print(f"Testing connection to ESP32 at {hardware_ip}:{hardware_port}")
    
    try:
        # Create socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10)  # 10 second timeout
        
        print("Connecting...")
        s.connect((hardware_ip, hardware_port))
        print("✅ Connected successfully!")
        
        # Send a simple message
        s.send(b"Hello from Python server\n")
        
        # Try to receive response
        try:
            response = s.recv(1024).decode('utf-8')
            print(f"Response from ESP32: {response.strip()}")
        except socket.timeout:
            print("No response received (this is normal)")
        
        # Keep connection open briefly
        time.sleep(1)
        
        s.close()
        print("✅ Connection closed successfully")
        print("The ESP32 should have captured this Python server's IP address")
        
        return True
        
    except socket.timeout:
        print("❌ Connection timeout")
        return False
    except ConnectionRefusedError:
        print("❌ Connection refused - make sure ESP32 is running and listening on port 5000")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

if __name__ == "__main__":
    print("ESP32 Hardware Connection Test")
    print("=" * 40)
    
    # Test the connection
    success = test_hardware_connection()
    
    if success:
        print("\n🎉 Test completed successfully!")
        print("Your ESP32 should now have captured this computer's IP address.")
        print("Check the ESP32 serial output to confirm.")
    else:
        print("\n❌ Test failed!")
        print("Make sure:")
        print("1. ESP32 is connected to WiFi")
        print("2. ESP32 is running the notification server on port 5000")
        print("3. Both devices are on the same network")
        print("4. Firewall is not blocking the connection")

