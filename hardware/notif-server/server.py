import http.server
import socketserver
import json
from urllib.parse import parse_qs
from fcm_sender import send_fcm_notification

VALID_TYPES = {"FOOD", "DOCTOR_CALL", "RESTROOM", "EMERGENCY"}

class RequestHandler(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length).decode()
        params = parse_qs(post_data)

        type_val = params.get("type", [None])[0]
        topic_val = params.get("topic", [None])[0]

        response = {}
        if type_val not in VALID_TYPES:
            self.send_response(400)
            response['error'] = "Invalid 'type' value."
        elif topic_val is None or len(topic_val) != 5:
            self.send_response(400)
            response['error'] = "Invalid 'topic' value. Must be 5 characters."
        else:
            self.send_response(200)
            response['status'] = "Success"
            response['type'] = type_val
            response['topic'] = topic_val

            # Send FCM notification
            status, text = send_fcm_notification(type_val, topic_val)
            response['fcm_status'] = status
            response['fcm_response'] = text

        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(response).encode())

# Custom TCPServer class with SO_REUSEADDR enabled
class ReusableTCPServer(socketserver.TCPServer):
    allow_reuse_address = True  # This enables SO_REUSEADDR

def run_server(ip="0.0.0.0", port=8080):
    server = None
    try:
        server = ReusableTCPServer((ip, port), RequestHandler)
        print(f"Server running at http://{ip}:{port}/")
        server.serve_forever()
    except Exception as e:
        print(f"[!] Server encountered an error: {e}")
    finally:
        if server:
            server.server_close()
            print("[*] Server closed cleanly.")

if __name__ == "__main__":
    run_server()

