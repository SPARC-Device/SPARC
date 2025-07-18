#include "notif.h"
#include <WiFi.h>
#include <Arduino.h>



WiFiServer notificationServer(5000);
WiFiClient notificationClient;
bool notificationClientConnected = false;
IPAddress notificationServerIP;

void notificationServerSetup() {
    notificationServer.begin();
}

void notificationServerLoop() {
    if (!notificationClientConnected) {
        notificationClient = notificationServer.available();
        if (notificationClient) {
            notificationClientConnected = true;
            notificationServerIP = notificationClient.remoteIP();
            Serial.print("Notification server connected: ");
            Serial.println(notificationServerIP);
        }
    } else if (!notificationClient.connected()) {
        notificationClientConnected = false;
        notificationClient.stop();
        Serial.println("Notification server disconnected.");
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
    notificationClient.print(request);
    Serial.print("Sent notification POST request: ");
    Serial.println(request);
}
