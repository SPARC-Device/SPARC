from google.oauth2 import service_account
import google.auth.transport.requests
import requests
import json

class FCMNotifier:
    def __init__(self, service_account_path, project_id):
        self.service_account_path = service_account_path
        self.project_id = project_id
        self._access_token = None
        self._creds = None
        self._refresh_token()

    def _refresh_token(self):
        self._creds = service_account.Credentials.from_service_account_file(
            self.service_account_path,
            scopes=['https://www.googleapis.com/auth/firebase.messaging']
        )
        request = google.auth.transport.requests.Request()
        self._creds.refresh(request)
        self._access_token = self._creds.token

    def send_push_notification(self, device_token, title, body, data=None):
        if not self._access_token or self._creds.expired:
            self._refresh_token()
        message = {
            "message": {
                "token": device_token,
                "notification": {
                    "title": title,
                    "body": body
                }
            }
        }
        if data:
            message["message"]["data"] = data
        headers = {
            "Authorization": f"Bearer {self._access_token}",
            "Content-Type": "application/json"
        }
        url = f"https://fcm.googleapis.com/v1/projects/{self.project_id}/messages:send"
        response = requests.post(
            url,
            headers=headers,
            data=json.dumps(message)
        )
        return response.status_code, response.text

# Example usage (remove or comment out in production):
# notifier = FCMNotifier('service-account.json', 'sparc-8b7af')
# status, resp = notifier.send_push_notification(
#     device_token="cchRRTQ6QruqmOvtWxErUI:APA91bH02wAaNRUNG0m8mf_jFJv_0pny5wnd2JdtePCK8kfYbfd8qkr12vOY1rP9gJu9U0X6cMQQ9nf7nTvG_P4nUnuU3Ub7DE808T_hVAxAd_sbTja8Pr4",
#     title="Hello",
#     body="This is a v1 API push"
# )
# print(status, resp)

