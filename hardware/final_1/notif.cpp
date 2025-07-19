#include "notif.h"
#include <WiFi.h>
#include <Arduino.h>



WiFiServer notificationServer(5000);
WiFiClient notificationClient;
bool notificationClientConnected = false;
IPAddress notificationServerIP;

bool notificationServerIPCaptured = false;

void notificationServerSetup() {
    notificationServer.begin();
}

void notificationServerLoop() {
    if (!notificationServerIPCaptured) {
      WiFiClient tempClient = notificationServer.available();
      if (tempClient) {
        notificationServerIP = tempClient.remoteIP();
        notificationServerIPCaptured = true;
  
        Serial.print("Captured notification server IP: ");
        Serial.println(notificationServerIP);
  
        // Send acknowledgment (optional)
        tempClient.println("ESP32 received your connection.");
        delay(100); // ensure data is sent before closing
        tempClient.stop();
  
        Serial.println("Closed initial connection.");
      }
    }
  }
  

void sendNotificationRequest(const String& userId, const String& type) {
    if (!notificationClientConnected) {
        Serial.println("Notification server not connected, cannot send notification.");
        return;
    }
    String url = "/?topic=" + userId + "&type=" + type;
    String request =
        "POST " + url + " HTTP/1.1\r\n"
        "Host: " + notificationServerIP.toString() + ":8080\r\n"
        "Connection: keep-alive\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    const char* requestCstr = request.c_str();
    size_t requestLength = request.length();

    notificationClient.write((const uint8_t*)requestCstr, requestLength);
    Serial.print("Sent notification POST request: ");
    Serial.println(request);
}
