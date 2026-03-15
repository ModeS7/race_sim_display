#pragma once
#include <Arduino.h>

enum TouchZone { TOUCH_NONE, TOUCH_LEFT, TOUCH_RIGHT };

void touchInit();
TouchZone touchPoll();
bool touchGetXY(int& x, int& y);  // returns true if touched, with display coordinates
