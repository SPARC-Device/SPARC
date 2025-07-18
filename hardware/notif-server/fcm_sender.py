import requests
import json

# Replace with your actual Firebase Project ID and Server Key
FIREBASE_PROJECT_ID = "sparc-8b7af"
FCM_ENDPOINT = f"https://fcm.googleapis.com/v1/projects/{FIREBASE_PROJECT_ID}/messages:send"

# Load the OAuth2 access token from a service account (better practice)
def get_access_token():
    from google.oauth2 import service_account
    import google.auth.transport.requests

    SCOPES = ["https://www.googleapis.com/auth/firebase.messaging"]
    SERVICE_ACCOUNT_FILE = "service-account.json"  # Downloaded from Firebase Console

    credentials = service_account.Credentials.from_service_account_file(
        SERVICE_ACCOUNT_FILE, scopes=SCOPES
    )
    request = google.auth.transport.requests.Request()
    credentials.refresh(request)

    return credentials.token

def send_fcm_notification(notif_type, topic):
    body_map = {
        "FOOD": "Meal notification triggered by user.",
        "RESTROOM": "Restroom notification triggered by user.",
        "DOCTOR_CALL": "Call notification triggered by user.",
        "EMERGENCY": "EMERGENCY notification triggered by user. Assisstance needed.."
    }

    body = body_map.get(notif_type, "Notification triggered by user.")

    headers = {
        "Authorization": f"Bearer {get_access_token()}",
        "Content-Type": "application/json; UTF-8",
    }

    payload = {
        "message": {
            "notification": {
                "title": "User Request received",
                "body": body
            },
            "data": {
                "type": notif_type
            },
            "topic": topic
        }
    }

    response = requests.post(FCM_ENDPOINT, headers=headers, json=payload)
    return response.status_code, response.text

