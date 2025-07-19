#ifndef VARIABLE_H
#define VARIABLE_H
#include <Arduino.h>

extern String userId;

extern String ssid;
extern String password;

extern int blinkDuration;
extern int blinkGap;

#include <IPAddress.h>

extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;


#endif