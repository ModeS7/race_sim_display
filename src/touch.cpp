#include "touch.h"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

#define TOUCH_CLK  25
#define TOUCH_MISO 39
#define TOUCH_MOSI 32
#define TOUCH_CS   33
#define TOUCH_IRQ  36

static XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

static bool wasTouched = false;
static uint32_t lastTouchTime = 0;
#define TOUCH_DEBOUNCE_MS 400

void touchInit() {
    // Init default SPI (VSPI) with touch pins — display uses HSPI, no conflict
    SPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    ts.begin();
    ts.setRotation(1);
    Serial.println("Touch initialized");
}

bool touchGetXY(int& x, int& y) {
    bool touched = ts.touched();

    if (!touched) {
        wasTouched = false;
        return false;
    }

    if (wasTouched) return false;

    uint32_t now = millis();
    if (now - lastTouchTime < TOUCH_DEBOUNCE_MS) return false;

    wasTouched = true;
    lastTouchTime = now;

    TS_Point p = ts.getPoint();
    Serial.printf("Touch raw: x=%d y=%d z=%d\n", p.x, p.y, p.z);

    x = map(p.x, 200, 3800, 0, 319);
    y = map(p.y, 200, 3800, 0, 239);

    if (x < 0) x = 0; if (x > 319) x = 319;
    if (y < 0) y = 0; if (y > 239) y = 239;

    Serial.printf("Touch mapped: x=%d y=%d\n", x, y);
    return true;
}

TouchZone touchPoll() {
    int x, y;
    if (!touchGetXY(x, y)) return TOUCH_NONE;
    return (x < 160) ? TOUCH_LEFT : TOUCH_RIGHT;
}
