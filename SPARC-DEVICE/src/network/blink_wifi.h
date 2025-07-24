#ifndef BLINK_WIFI_H
#define BLINK_WIFI_H

#include <WiFi.h>

void blinkWifiSetup();
void blinkWifiLoop();
bool blinkWifiCheckSingleBlink();
bool blinkWifiCheckDoubleBlink();
void reconnectWiFi();
int getBlinks();
void blinkWifiResetFlags();

bool isServerAvailable();

#endif // BLINK_WIFI_H 