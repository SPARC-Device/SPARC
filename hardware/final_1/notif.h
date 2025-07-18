#ifndef NOTIF_H
#define NOTIF_H
#include <Arduino.h>

void notificationServerSetup();
void notificationServerLoop();
void sendNotificationRequest(const String& userId, const String& type);

#endif // NOTIF_H
