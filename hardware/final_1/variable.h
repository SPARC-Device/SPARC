#ifndef VARIABLE_H
#define VARIABLE_H
#include <Arduino.h>
#include<WiFi.h>

extern String userId;

extern String ssid;
extern String password;

extern int blinkDuration;
extern int blinkGap;

extern bool clientConnected;
extern bool clientFound;
extern WiFiClient tempClient;
extern WiFiClient client;
extern WiFiServer server;

#include <IPAddress.h>

extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;

#endif