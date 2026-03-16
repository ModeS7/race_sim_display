#pragma once
#include <TFT_eSPI.h>

// Draw a 7-segment style digit at position (x, y) with given width/height
// x,y is top-left corner of the bounding box
// segW = thickness of each segment
void draw7Seg(TFT_eSPI& tft, int x, int y, int w, int h, int segW,
              char ch, uint16_t color, uint16_t bg);
