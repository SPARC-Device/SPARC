#ifndef NOTIF_H
#define NOTIF_H

void notificationServerSetup();
void notificationServerLoop();
void sendNotificationRequest(const String& userId, const String& type);

#endif // NOTIF_H
