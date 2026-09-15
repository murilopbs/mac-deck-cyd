#include "ScreensaverDriver.h"
#include "ScreensaverData.h"
#include "DisplayDriver.h"

ScreensaverDriver screensaver;

static int gifXOffset = 0;
static int gifYOffset = 0;

void ScreensaverDriver::GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > 320) iWidth = 320;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y + gifYOffset;

  if (y < 0 || y >= 240) return; // Não desenha fora dos limites da tela

  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) { // Restaurar cor de fundo
    for (x = 0; x < iWidth; x++) {
      if (s[x] == pDraw->ucTransparent) s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  int destX = pDraw->iX + gifXOffset;

  // Se houver transparência
  if (pDraw->ucHasTransparency) {
    uint8_t c, ucTransparent = pDraw->ucTransparent;
    int iCount = 0;
    x = 0;
    while (x < iWidth) {
      c = ucTransparent - 1;
      d = usTemp;
      while (c != ucTransparent && s[x] == ucTransparent && x < iWidth) {
        x++;
      }
      iCount = 0;
      while (s[x] != ucTransparent && x < iWidth) {
        c = s[x];
        d[iCount++] = usPalette[c];
        x++;
      }
      if (iCount && (destX + x - iCount < 320)) {
        display.getTft().drawRGBBitmap(destX + x - iCount, y, usTemp, iCount, 1);
      }
    }
  } else {
    // Linha opaca: converte a paleta de cores para RGB565 e envia para o display
    for (x = 0; x < iWidth; x++) {
      usTemp[x] = usPalette[*s++];
    }
    if (destX + iWidth <= 320) {
      display.getTft().drawRGBBitmap(destX, y, usTemp, iWidth, 1);
    }
  }
}

ScreensaverDriver::ScreensaverDriver()
  : active(false), nextFrameTime(0) {}

void ScreensaverDriver::begin() {
  gif.begin(LITTLE_ENDIAN_PIXELS);
}

void ScreensaverDriver::start() {
  if (active) return;
  Serial.println("[Screensaver] Ativando modo protetor de tela...");
  display.clear(0x0000); // Tela preta total

  if (gif.open((uint8_t *)screensaver_gif, sizeof(screensaver_gif), GIFDraw)) {
    int gw = gif.getCanvasWidth();
    int gh = gif.getCanvasHeight();
    gifXOffset = (320 - gw) / 2;
    gifYOffset = (240 - gh) / 2;
    if (gifXOffset < 0) gifXOffset = 0;
    if (gifYOffset < 0) gifYOffset = 0;

    Serial.printf("[Screensaver] GIF aberto com sucesso: %dx%d (Offset: %d, %d)\n",
                  gw, gh, gifXOffset, gifYOffset);
    active = true;
    nextFrameTime = millis();
  } else {
    Serial.println("[Screensaver] Falha ao abrir GIF animado.");
    active = false;
  }
}

void ScreensaverDriver::update() {
  if (!active) return;

  if (millis() >= nextFrameTime) {
    int delayMs = 0;
    int result = gif.playFrame(true, &delayMs);
    if (!result) {
      // Fim dos frames: reinicia para loop contínuo
      gif.reset();
      result = gif.playFrame(true, &delayMs);
    }

    if (delayMs < 30) delayMs = 60; // Limite saudável de ~16-20 fps
    nextFrameTime = millis() + delayMs;
  }
}

void ScreensaverDriver::stop() {
  if (!active) return;
  Serial.println("[Screensaver] Desativando protetor de tela...");
  gif.close();
  active = false;
}
