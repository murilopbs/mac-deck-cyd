#include "TouchDriver.h"

TouchDriver touch;

TouchDriver::TouchDriver()
  : touchSPI(VSPI),
    ts(XPT2046_CS, XPT2046_IRQ),
    lastTouchTime(0) {
}

void TouchDriver::begin() {
  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchSPI);
  ts.setRotation(1);
}

bool TouchDriver::isTouched() {
  return ts.touched();
}

bool TouchDriver::getTouch(int &screenX, int &screenY) {
  unsigned long now = millis();
  if (now - lastTouchTime < 140) { // Debounce ágil de 140ms para atalhos
    return false;
  }

  if (ts.touched()) {
    TS_Point p = ts.getPoint();

    // Filtra ruído
    if (p.z > 250) {
#if DISPLAY_ROTATION == 3
      int mapped_x = map(p.x, 200, 3700, 0, SCREEN_WIDTH);
      int mapped_y = map(p.y, 240, 3800, 0, SCREEN_HEIGHT);
#else
      int mapped_x = map(p.x, 200, 3700, SCREEN_WIDTH, 0);
      int mapped_y = map(p.y, 240, 3800, SCREEN_HEIGHT, 0);
#endif

      screenX = constrain(mapped_x, 0, SCREEN_WIDTH - 1);
      screenY = constrain(mapped_y, 0, SCREEN_HEIGHT - 1);

      Serial.printf("[TOUCH] Raw: (%d, %d, z=%d) -> Tela: (%d, %d)\n", p.x, p.y, p.z, screenX, screenY);
      lastTouchTime = now;
      return true;
    }
  }
  return false;
}
