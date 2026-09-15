#include "DisplayDriver.h"

DisplayDriver display;

DisplayDriver::DisplayDriver()
  : tftSPI(HSPI),
    tft(&tftSPI, TFT_CS, TFT_DC, TFT_RST) {
}

void DisplayDriver::begin() {
  tftSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.init(240, 320);
  tft.setRotation(DISPLAY_ROTATION);
  tft.fillScreen(COLOR_BG);

  // Inicializa Backlight
#if TFT_BL >= 0
  pinMode(TFT_BL, OUTPUT);
  setBacklight(85);
#endif
}

void DisplayDriver::setBacklight(uint8_t brightnessPct) {
#if TFT_BL >= 0
  if (brightnessPct > 100) brightnessPct = 100;
  uint32_t duty = (brightnessPct * 255) / 100;
  analogWrite(TFT_BL, duty);
#endif
}

void DisplayDriver::clear(uint16_t color) {
  tft.fillScreen(color);
}

void DisplayDriver::drawHeader(bool isConnected) {
  tft.fillRect(0, 0, SCREEN_WIDTH, 30, COLOR_HEADER_BG);
  tft.drawFastHLine(0, 30, SCREEN_WIDTH, COLOR_DIVIDER);

  // Título
  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(12, 11);
  tft.print("MACDECK BLE");

  // Status Bluetooth
  int statusX = 180;
  int statusY = 8;
  int statusW = 128;
  int statusH = 16;

  uint16_t badgeBg = isConnected ? 0x0A84 : 0x11E8;
  uint16_t badgeBorder = isConnected ? COLOR_GREEN : COLOR_BLUE;
  uint16_t badgeText = isConnected ? 0x97F2 : 0x9E3F;
  const char* label = isConnected ? "CONECTADO" : "PAREANDO...";

  tft.fillRoundRect(statusX, statusY, statusW, statusH, 8, badgeBg);
  tft.drawRoundRect(statusX, statusY, statusW, statusH, 8, badgeBorder);

  // Ponto colorido indicador
  tft.fillCircle(statusX + 10, statusY + 8, 3, isConnected ? COLOR_GREEN : COLOR_BLUE);

  tft.setTextSize(1);
  tft.setTextColor(badgeText);
  tft.setCursor(statusX + 22, statusY + 4);
  tft.print(label);
}

void DisplayDriver::drawFooter() {
  tft.drawFastHLine(10, 222, 300, COLOR_DIVIDER);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_MUTED);
  const char* foot = "Apple Silicon & macOS \x07 Touch Deck";
  int textW = strlen(foot) * 6;
  tft.setCursor((SCREEN_WIDTH - textW) / 2, 228);
  tft.print(foot);
}

void DisplayDriver::drawButton(const DeckButton &btn) {
  uint16_t bg = btn.isPressed ? BTN_BG_PRESSED : BTN_BG_DEFAULT;
  uint16_t border = btn.isPressed ? BTN_BORDER_PRESSED : BTN_BORDER_DEFAULT;

  tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 10, bg);
  tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 10, border);

  // Efeito de brilho duplo se pressionado
  if (btn.isPressed) {
    tft.drawRoundRect(btn.x + 1, btn.y + 1, btn.w - 2, btn.h - 2, 9, COLOR_ACCENT);
  }

  int cx = btn.x + btn.w / 2;
  int cy = btn.y + 26;

  drawIcon(cx, cy, btn.icon, btn.isPressed ? COLOR_ACCENT : btn.iconColor);

  // Título
  tft.setTextSize(1);
  tft.setTextColor(btn.isPressed ? COLOR_ACCENT : COLOR_TEXT);
  int titleW = strlen(btn.title) * 6;
  tft.setCursor(cx - titleW / 2, btn.y + 52);
  tft.print(btn.title);

  // Subtítulo
  tft.setTextColor(COLOR_TEXT_MUTED);
  int subW = strlen(btn.subtitle) * 6;
  tft.setCursor(cx - subW / 2, btn.y + 66);
  tft.print(btn.subtitle);
}

void DisplayDriver::drawAllButtons(const DeckButton buttons[6]) {
  for (int i = 0; i < 6; i++) {
    drawButton(buttons[i]);
  }
}

void DisplayDriver::drawIcon(int cx, int cy, IconType icon, uint16_t color) {
  switch (icon) {
    case ICON_PLAY_PAUSE: drawPlayPauseIcon(cx, cy, color); break;
    case ICON_NEXT:       drawNextIcon(cx, cy, color); break;
    case ICON_MUTE:       drawMuteIcon(cx, cy, color); break;
    case ICON_MIC_MUTE:   drawMicMuteIcon(cx, cy, color); break;
    case ICON_SCREENSHOT: drawScreenshotIcon(cx, cy, color); break;
    case ICON_LOCK:       drawLockIcon(cx, cy, color); break;
    default: break;
  }
}

void DisplayDriver::drawPlayPauseIcon(int cx, int cy, uint16_t color) {
  // Triângulo Play à esquerda
  tft.fillTriangle(cx - 10, cy - 8, cx - 10, cy + 8, cx - 1, cy, color);
  // Duas barras Pause à direita
  tft.fillRect(cx + 4, cy - 7, 3, 14, color);
  tft.fillRect(cx + 9, cy - 7, 3, 14, color);
}

void DisplayDriver::drawNextIcon(int cx, int cy, uint16_t color) {
  // Dois triângulos avançar
  tft.fillTriangle(cx - 9, cy - 7, cx - 9, cy + 7, cx - 1, cy, color);
  tft.fillTriangle(cx - 1, cy - 7, cx - 1, cy + 7, cx + 7, cy, color);
  tft.drawFastVLine(cx + 8, cy - 7, 15, color);
}

void DisplayDriver::drawMuteIcon(int cx, int cy, uint16_t color) {
  // Alto falante
  tft.fillRect(cx - 9, cy - 4, 4, 8, color);
  tft.fillTriangle(cx - 6, cy - 4, cx - 6, cy + 4, cx - 1, cy + 7, color);
  tft.fillTriangle(cx - 6, cy - 4, cx - 1, cy - 7, cx - 1, cy + 7, color);
  // X de mudo
  tft.drawLine(cx + 3, cy - 5, cx + 9, cy + 5, COLOR_RED);
  tft.drawLine(cx + 9, cy - 5, cx + 3, cy + 5, COLOR_RED);
}

void DisplayDriver::drawMicMuteIcon(int cx, int cy, uint16_t color) {
  // Corpo do microfone
  tft.fillRoundRect(cx - 3, cy - 8, 7, 11, 3, color);
  // Suporte em U
  tft.drawCircle(cx, cy, 6, color);
  tft.fillRect(cx - 6, cy - 8, 13, 8, BTN_BG_DEFAULT); // Limpa topo do círculo
  tft.drawFastVLine(cx, cy + 6, 4, color);
  tft.drawFastHLine(cx - 4, cy + 10, 9, color);
  // Barra diagonal de mudo (Vermelha)
  tft.drawLine(cx - 7, cy + 8, cx + 7, cy - 8, COLOR_RED);
}

void DisplayDriver::drawScreenshotIcon(int cx, int cy, uint16_t color) {
  // Câmera / Captura
  tft.drawRoundRect(cx - 9, cy - 6, 19, 14, 2, color);
  tft.fillRect(cx - 3, cy - 8, 7, 3, color); // topo da câmera
  tft.drawCircle(cx, cy + 1, 4, color);     // lente central
  tft.fillCircle(cx + 5, cy - 3, 1, color); // flash
}

void DisplayDriver::drawLockIcon(int cx, int cy, uint16_t color) {
  // Corpo do cadeado
  tft.fillRoundRect(cx - 7, cy - 2, 15, 12, 2, color);
  // Arco superior
  tft.drawCircle(cx, cy - 2, 5, color);
  tft.fillRect(cx - 5, cy - 2, 11, 5, color); // fecha na base
  // Furo da fechadura
  tft.fillCircle(cx, cy + 3, 2, COLOR_BG);
  tft.drawFastVLine(cx, cy + 4, 3, COLOR_BG);
}
