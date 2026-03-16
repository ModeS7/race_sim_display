#include "seg7.h"

//  Segment layout:
//   _a_
//  |   |
//  f   b
//  |_g_|
//  |   |
//  e   c
//  |_d_|

// Segment map: which segments are on for each character
// bits: 0=a, 1=b, 2=c, 3=d, 4=e, 5=f, 6=g
static const uint8_t segMap[] = {
    0b0111111, // 0
    0b0000110, // 1
    0b1011011, // 2
    0b1001111, // 3
    0b1100110, // 4
    0b1101101, // 5
    0b1111101, // 6
    0b0000111, // 7
    0b1111111, // 8
    0b1101111, // 9
};

// R = segments f, e, a, b, g (top half + left side)
static const uint8_t segR = 0b1110111;

static void hSeg(TFT_eSPI& tft, int x, int y, int len, int thick, uint16_t col) {
    // Horizontal segment with pointed ends
    for (int t = 0; t < thick; t++) {
        int shrink = (t < thick / 2) ? (thick / 2 - t) : (t - thick / 2);
        tft.drawFastHLine(x + shrink, y + t, len - shrink * 2, col);
    }
}

static void vSeg(TFT_eSPI& tft, int x, int y, int len, int thick, uint16_t col) {
    // Vertical segment with pointed ends
    for (int t = 0; t < thick; t++) {
        int shrink = (t < thick / 2) ? (thick / 2 - t) : (t - thick / 2);
        tft.drawFastVLine(x + t, y + shrink, len - shrink * 2, col);
    }
}

void draw7Seg(TFT_eSPI& tft, int x, int y, int w, int h, int segW,
              char ch, uint16_t color, uint16_t bg) {
    // Clear the area
    tft.fillRect(x, y, w, h, bg);

    uint8_t segs;
    if (ch >= '0' && ch <= '9') {
        segs = segMap[ch - '0'];
    } else if (ch == 'R' || ch == 'r') {
        segs = segR;
    } else {
        return; // unsupported character
    }

    int halfH = h / 2;
    int gap = 2; // gap between segments

    // a: top horizontal
    uint16_t col = (segs & 0x01) ? color : bg;
    hSeg(tft, x + segW + gap, y, w - 2 * segW - 2 * gap, segW, col);

    // b: top-right vertical
    col = (segs & 0x02) ? color : bg;
    vSeg(tft, x + w - segW, y + segW + gap, halfH - segW - 2 * gap, segW, col);

    // c: bottom-right vertical
    col = (segs & 0x04) ? color : bg;
    vSeg(tft, x + w - segW, y + halfH + gap, halfH - segW - 2 * gap, segW, col);

    // d: bottom horizontal
    col = (segs & 0x08) ? color : bg;
    hSeg(tft, x + segW + gap, y + h - segW, w - 2 * segW - 2 * gap, segW, col);

    // e: bottom-left vertical
    col = (segs & 0x10) ? color : bg;
    vSeg(tft, x, y + halfH + gap, halfH - segW - 2 * gap, segW, col);

    // f: top-left vertical
    col = (segs & 0x20) ? color : bg;
    vSeg(tft, x, y + segW + gap, halfH - segW - 2 * gap, segW, col);

    // g: middle horizontal
    col = (segs & 0x40) ? color : bg;
    hSeg(tft, x + segW + gap, y + halfH - segW / 2, w - 2 * segW - 2 * gap, segW, col);
}
