#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "Config.h"

struct SpotifyTrackData;

class DisplayDriver {
public:
  DisplayDriver();
  void begin();

  void clear(uint16_t color = COLOR_BG);
  void drawHeader(bool isBleConnected, bool isWifiConnected, bool isApMode = false);
  void drawSpotifyCard(const SpotifyTrackData &track);
  void drawSpotifyProgressOnly(const SpotifyTrackData &track);
  void drawSpotifyFullScreen(const SpotifyTrackData &track);
  void drawFooter(const String &info = "");
  void drawButton(const DeckButton &btn);
  void drawAllButtons(const DeckButton buttons[6]);

  void setBacklight(uint8_t brightnessPct);
  uint8_t getBacklight() const { return currentBrightness; }
  Adafruit_ST7789 &getTft() { return tft; }

private:
  uint8_t currentBrightness;
  SPIClass tftSPI;
  Adafruit_ST7789 tft;

  void drawIcon(int cx, int cy, IconType icon, uint16_t color);
  void drawPlayPauseIcon(int cx, int cy, uint16_t color);
  void drawPauseIcon(int cx, int cy, uint16_t color);
  void drawNextIcon(int cx, int cy, uint16_t color);
  void drawMuteIcon(int cx, int cy, uint16_t color);
  void drawMicMuteIcon(int cx, int cy, uint16_t color);
  void drawScreenshotIcon(int cx, int cy, uint16_t color);
  void drawLockIcon(int cx, int cy, uint16_t color);
};

extern DisplayDriver display;

#endif // DISPLAY_DRIVER_H
