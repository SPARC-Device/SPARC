import http.server
import socketserver
import json
from urllib.parse import parse_qs, urlparse
from fcm_sender import send_fcm_notification, get_access_token # Import get_access_token
import threading
import time
import asyncio
from concurrent.futures import ThreadPoolExecutor
import socket # Import socket for network connections

VALID_TYPES = {"FOOD", "DOCTOR_CALL", "RESTROOM", "EMERGENCY"}

# Thread pool for async FCM calls
executor = ThreadPoolExecutor(max_workers=3)

def send_fcm_async(type_val, topic_val):
    """
    Send FCM notification asynchronously with error handling.
    The timeout for the actual FCM request is handled within send_fcm_notification.
    Removed signal-based timeout as it's not compatible with non-main threads.
    """
    try:
        status, text = send_fcm_notification(type_val, topic_val)
        return status, text
    except Exception as e:
        return "error", f"Async FCM error: {str(e)}"

class RequestHandler(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        try:
            print(f"\n{'='*50}")
            print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Received POST request: {self.path}")
            print(f"Client: {self.client_address}")
            
            # Parse query parameters from the URL path
            parsed_url = urlparse(self.path)
            query_params = parse_qs(parsed_url.query)

            type_val = query_params.get("type", [None])[0]
            topic_val = query_params.get("topic", [None])[0]

            print(f"Parsed parameters - type: {type_val}, topic: {topic_val}")

            response = {}
            
            # Validation
            if type_val not in VALID_TYPES:
                print(f"❌ Invalid type: {type_val}")
                self.send_response(400)
                response['error'] = f"Invalid 'type' value: {type_val}. Valid types: {list(VALID_TYPES)}"
            elif topic_val is None or len(topic_val) != 5:
                print(f"❌ Invalid topic: {topic_val}")
                self.send_response(400)
                response['error'] = f"Invalid 'topic' value: {topic_val}. Must be 5 characters."
            else:
                print(f"✅ Valid request - processing...")
                self.send_response(200)
                response['status'] = "Success"
                response['type'] = type_val
                response['topic'] = topic_val

                # Send FCM notification asynchronously to avoid blocking
                print(f"📱 Sending FCM notification asynchronously...")
                
                # Submit FCM task to thread pool and don't wait for it
                future = executor.submit(send_fcm_async, type_val, topic_val)
                
                try:
                    # Wait for FCM result with a short timeout to avoid blocking too long
                    status, text = future.result(timeout=2.0)  # 2 second max wait
                    response['fcm_status'] = status
                    response['fcm_response'] = text
                    
                    if status == 200:
                        print(f"✅ FCM notification sent successfully")
                    else:
                        print(f"⚠️ FCM notification failed: {status} - {text}")
                        
                except TimeoutError:
                    print(f"⏰ FCM request taking too long, responding immediately")
                    response['fcm_status'] = "pending"
                    response['fcm_response'] = "FCM notification sent in background"
                    
                except Exception as fcm_error:
                    print(f"❌ FCM notification failed: {fcm_error}")
                    response['fcm_status'] = "error"
                    response['fcm_response'] = str(fcm_error)

            # Send response headers
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Connection", "close")  # Important: close connection after response
            self.end_headers()
            
            # Send response body
            response_json = json.dumps(response, indent=2)
            self.wfile.write(response_json.encode('utf-8'))
            self.wfile.flush()  # Ensure data is sent
            
            print(f"📤 Response sent:")
            print(response_json)
            print(f"{'='*50}\n")
            
        except Exception as e:
            print(f"❌ Error handling request: {e}")
            import traceback
            traceback.print_exc()
            
            try:
                self.send_response(500)
                self.send_header("Content-Type", "application/json")
                self.send_header("Connection", "close")
                self.end_headers()
                error_response = {'error': f'Server error: {str(e)}'}
                self.wfile.write(json.dumps(error_response).encode('utf-8'))
                self.wfile.flush()
            except:
                print("❌ Failed to send error response")

    def do_GET(self):
        """Handle GET requests for testing"""
        print(f"Received GET request: {self.path}")
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.end_headers()
        self.wfile.write(b"Server is running! Send POST requests with ?type=FOOD&topic=12345")

    def log_message(self, format, *args):
        """Override default logging to reduce noise"""
        pass  # We're using custom logging above

class ThreadedTCPServer(socketserver.ThreadingTCPServer):
    """Threaded server to handle multiple requests concurrently"""
    allow_reuse_address = True
    daemon_threads = True  # Don't wait for threads on shutdown

def test_fcm_function():
    """Test the FCM function independently by trying to get an access token."""
    print("🧪 Testing FCM function (attempting to get access token)...")
    try:
        # Attempt to get an access token. This implicitly tests connectivity to Google's auth servers.
        token = get_access_token()
        if token:
            print(f"✅ FCM test successful: Access token obtained.")
            return True
        else:
            print(f"⚠️ FCM test failed: Could not obtain access token.")
            return False
    except Exception as e:
        print(f"❌ FCM Test Failed: {e}")
        print(f"⚠️ Server will continue without FCM")
        return False

def test_hardware_connection(hardware_ip="192.168.120.13", hardware_port=5000, timeout=5):
    """
    Tests connectivity to the specified hardware IP and port using sockets.
    Returns True if connection is successful, False otherwise.
    """
    print(f"🔌 Testing connection to hardware at {hardware_ip}:{hardware_port} using sockets...")
    try:
        # Create a socket object
        # AF_INET for IPv4, SOCK_STREAM for TCP
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(timeout) # Set a timeout for the connection attempt
            s.connect((hardware_ip, hardware_port))
            time.sleep(1)
            s.send(b"Hello from Python server\n")
            s.close()
            print(f"✅ Successfully connected to hardware at {hardware_ip}:{hardware_port}")
            return True
    except socket.timeout:
        print(f"⏰ Timeout: Hardware at {hardware_ip}:{hardware_port} did not respond within {timeout} seconds.")
        return False
    except ConnectionRefusedError:
        print(f"❌ Connection refused: Hardware at {hardware_ip}:{hardware_port} actively refused the connection. Is it running?")
        return False
    except socket.error as e:
        print(f"❌ Socket error: Could not reach hardware at {hardware_ip}:{hardware_port}. Error: {e}")
        return False
    except Exception as e:
        print(f"❌ An unexpected error occurred while testing hardware connection: {e}")
        return False

def run_server(ip="0.0.0.0", port=8080):
    print(f"🚀 Starting server...")
    
    # First, test connection to the hardware
    # Note: I've changed the default port to 45454 to match your output
    if not test_hardware_connection(): 
        print("⚠️ Hardware connection test failed. Server will still start, but hardware communication might be impacted.")
        
    # Test FCM function next
    if not test_fcm_function():
        print("⚠️ FCM function test failed, but server will still start")
    
    server = None
    try:
        # Create threaded server for handling multiple concurrent requests
        server = ThreadedTCPServer((ip, port), RequestHandler)
        
        print(f"✅ Server running at http://{ip}:{port}/")
        print(f"📡 Waiting for POST requests...")
        print(f"📝 Valid request format: POST http://{ip}:{port}/?type=FOOD&topic=12345")
        print(f"🧪 Test with: curl -X POST \"http://{ip}:{port}/?type=FOOD&topic=12345\"")
        print(f"Press Ctrl+C to stop\n")
        
        # This will run indefinitely and handle multiple requests
        server.serve_forever()
        
    except KeyboardInterrupt:
        print("\n🛑 Server stopped by user (Ctrl+C)")
    except Exception as e:
        print(f"❌ Server encountered an error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        if server:
            print("🧹 Shutting down server...")
            server.shutdown()
            server.server_close()
            print("✅ Server closed cleanly.")

if __name__ == "__main__":
    run_server()

