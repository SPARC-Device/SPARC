
import requests
import json
import time
from datetime import datetime, timedelta

# Replace with your actual Firebase Project ID and Server Key
FIREBASE_PROJECT_ID = "sparc-8b7af"
FCM_ENDPOINT = f"https://fcm.googleapis.com/v1/projects/{FIREBASE_PROJECT_ID}/messages:send"

# Token caching to avoid refreshing on every request
_cached_token = None
_token_expiry = None

def get_access_token():
    """Get cached access token or refresh if expired"""
    global _cached_token, _token_expiry
    
    try:
        # Check if we have a valid cached token
        if _cached_token and _token_expiry and datetime.now() < _token_expiry:
            print(f"🔑 Using cached access token (expires in {(_token_expiry - datetime.now()).total_seconds():.0f}s)")
            return _cached_token
        
        print(f"🔑 Refreshing access token...")
        from google.oauth2 import service_account
        import google.auth.transport.requests

        SCOPES = ["https://www.googleapis.com/auth/firebase.messaging"]
        SERVICE_ACCOUNT_FILE = "service-account.json"  # Downloaded from Firebase Console

        credentials = service_account.Credentials.from_service_account_file(
            SERVICE_ACCOUNT_FILE, scopes=SCOPES
        )
        
        # Set shorter timeout for token refresh
        request = google.auth.transport.requests.Request()
        
        # Refresh with timeout
        credentials.refresh(request)
        
        # Cache the token (expires in ~1 hour, we'll refresh after 50 minutes)
        _cached_token = credentials.token
        _token_expiry = datetime.now() + timedelta(minutes=50)
        
        print(f"✅ Access token refreshed successfully")
        return _cached_token
        
    except Exception as e:
        print(f"❌ Failed to get access token: {e}")
        # Return cached token if available, even if expired
        if _cached_token:
            print(f"🔄 Using expired cached token as fallback")
            return _cached_token
        raise

def send_fcm_notification(notif_type, topic, max_retries=2):
    """Send FCM notification with retries and better error handling"""
    body_map = {
        "FOOD": "Meal notification triggered by user.",
        "RESTROOM": "Restroom notification triggered by user.",
        "DOCTOR_CALL": "Call notification triggered by user.",
        "EMERGENCY": "EMERGENCY notification triggered by user. Assistance needed."
    }

    body = body_map.get(notif_type, "Notification triggered by user.")

    for attempt in range(max_retries + 1):
        try:
            print(f"🚀 FCM Attempt {attempt + 1}/{max_retries + 1}")
            
            # Get access token
            token = get_access_token()
            
            headers = {
                "Authorization": f"Bearer {token}",
                "Content-Type": "application/json; UTF-8",
            }

            payload = {
                "message": {
                    "notification": {
                        "title": "User Request received",
                        "body": body
                    },
                    "data": {
                        "type": notif_type,
                        "timestamp": str(int(time.time()))
                    },
                    "topic": topic
                }
            }

            print(f"📤 Sending to topic '{topic}' with type '{notif_type}'")
            
            # Send with timeout
            response = requests.post(
                FCM_ENDPOINT, 
                headers=headers, 
                json=payload, 
                timeout=15  # 15 second timeout
            )
            
            print(f"📨 FCM Response: Status {response.status_code}")
            
            if response.status_code == 200:
                print(f"✅ FCM notification sent successfully!")
                return response.status_code, response.text
            elif response.status_code == 401:
                print(f"🔑 Token expired, clearing cache and retrying...")
                # Clear cached token on auth error
                global _cached_token, _token_expiry
                _cached_token = None
                _token_expiry = None
                continue
            else:
                print(f"⚠️ FCM failed with status {response.status_code}: {response.text}")
                if attempt == max_retries:
                    return response.status_code, response.text
                continue
                
        except requests.exceptions.Timeout:
            error_msg = f"FCM request timeout (attempt {attempt + 1})"
            print(f"⏰ {error_msg}")
            if attempt == max_retries:
                return "timeout", error_msg
            time.sleep(1)  # Brief delay before retry
            
        except requests.exceptions.ConnectionError as e:
            error_msg = f"FCM connection error: {str(e)}"
            print(f"🌐 {error_msg}")
            if attempt == max_retries:
                return "connection_error", error_msg
            time.sleep(2)  # Longer delay for connection issues
            
        except Exception as e:
            error_msg = f"FCM unexpected error: {str(e)}"
            print(f"❌ {error_msg}")
            if attempt == max_retries:
                return "error", error_msg
            time.sleep(1)
    
    return "failed", f"All {max_retries + 1} attempts failed"

# Removed the test_connection function as it was causing misleading 404 errors.
# The get_access_token function implicitly tests connectivity to Google's auth servers.

