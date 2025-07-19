#ifndef GUI_3_H
#define GUI_3_H

#include <Arduino.h>

void gui3Setup();
void gui3Loop();
// Called when a single blink is detected
void gui3OnSingleBlink();
// Called when a double blink is detected
void gui3OnDoubleBlink();
// Should be called periodically to handle popup timeout
void gui3CheckPopupTimeout();
void gui3InitAudio(); 

#endif // GUI_3_H 