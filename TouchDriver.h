#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "Config.h"

class TouchDriver {
public:
  TouchDriver();
  void begin();
  bool isTouched();
  bool getTouch(int &screenX, int &screenY);

private:
  SPIClass touchSPI;
  XPT2046_Touchscreen ts;
  unsigned long lastTouchTime;
};

extern TouchDriver touch;

#endif // TOUCH_DRIVER_H
