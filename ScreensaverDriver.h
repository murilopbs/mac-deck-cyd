#ifndef SCREENSAVER_DRIVER_H
#define SCREENSAVER_DRIVER_H

#include <Arduino.h>
#include <AnimatedGIF.h>

class ScreensaverDriver {
public:
  ScreensaverDriver();
  void begin();
  void start();
  void update();
  void stop();
  bool isActive() const { return active; }

private:
  AnimatedGIF gif;
  bool active;
  unsigned long nextFrameTime;

  static void GIFDraw(GIFDRAW *pDraw);
};

extern ScreensaverDriver screensaver;

#endif // SCREENSAVER_DRIVER_H
