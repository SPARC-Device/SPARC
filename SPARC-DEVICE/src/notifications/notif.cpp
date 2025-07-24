#include "notif.h"

#include <Arduino.h>
#include <WiFi.h>


WiFiServer notificationServer(5000);
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

        tempClient.println("ESP32 received your connection.");
        delay(100);
        tempClient.stop();

        Serial.println("Closed initial connection.");
      }
    }
}


void sendNotificationRequest(const String& userId, const String& type) {
  if (!notificationServerIPCaptured) {
    Serial.println("No IP captured for notification server. Cannot send POST.");
    return;
  }

  WiFiClient client;
  const int port = 8080;

  Serial.print("Connecting to Python server at ");
  Serial.print(notificationServerIP);
  Serial.print(":");
  Serial.println(port);

  if (!client.connect(notificationServerIP, port)) {
    Serial.println("Failed to connect to Python server.");
    return;
  }

  String url = "/?topic=" + userId + "&type=" + type;
  String request =
    "POST " + url + " HTTP/1.1\r\n" +
    "Host: " + notificationServerIP.toString() + ":" + String(port) + "\r\n" +
    "Connection: close\r\n" +
    "Content-Length: 0\r\n\r\n";

  const char* requestCstr = request.c_str();
  size_t requestLength = request.length();

  client.write((const uint8_t*)requestCstr, requestLength);
  Serial.println("Sent POST:");
  Serial.println(request);

  client.stop();
  Serial.println("Notification POST request completed and connection closed.");
}

