#ifndef BLINK_WIFI_H
#define BLINK_WIFI_H

void blinkWifiSetup();
void blinkWifiLoop();
bool blinkWifiCheckSingleBlink();
bool blinkWifiCheckDoubleBlink();
void reconnectWiFi();
int getBlinks();
void blinkWifiResetFlags();

#endif // BLINK_WIFI_H 