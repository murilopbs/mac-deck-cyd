#include "DisplayDriver.h"

#include "SpotifyClient.h"

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

void DisplayDriver::drawHeader(bool isBleConnected, bool isWifiConnected, bool isApMode) {
  tft.fillRect(0, 0, SCREEN_WIDTH, 24, COLOR_HEADER_BG);
  tft.drawFastHLine(0, 24, SCREEN_WIDTH, COLOR_DIVIDER);

  // Título (Esquerda)
  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(10, 8);
  tft.print("MACDECK");

  // Indicador Wi-Fi (Centro-Esquerda)
  int wifiX = 66;
  int wifiY = 8;
  if (isWifiConnected) {
    tft.fillCircle(wifiX + 3, wifiY + 3, 3, COLOR_GREEN);
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(wifiX + 10, wifiY);
    tft.print("Wi-Fi");
  } else if (isApMode) {
    tft.fillCircle(wifiX + 3, wifiY + 3, 3, COLOR_ACCENT);
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(wifiX + 10, wifiY);
    tft.print("AP Setup");
  } else {
    tft.fillCircle(wifiX + 3, wifiY + 3, 3, COLOR_TEXT_MUTED);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(wifiX + 10, wifiY);
    tft.print("Wi-Fi Off");
  }

  // Status Bluetooth (Direita)
  int statusW = 100;
  int statusH = 16;
  int statusX = SCREEN_WIDTH - statusW - 8;
  int statusY = 4;

  uint16_t badgeBg = isBleConnected ? 0x0A85 : 0x098A;
  uint16_t badgeBorder = isBleConnected ? COLOR_GREEN : COLOR_BLUE;
  uint16_t badgeText = isBleConnected ? COLOR_GREEN : 0x9E3F;
  const char* label = isBleConnected ? "CONECTADO" : "PAREANDO...";

  tft.fillRoundRect(statusX, statusY, statusW, statusH, 6, badgeBg);
  tft.drawRoundRect(statusX, statusY, statusW, statusH, 6, badgeBorder);

  // Ponto colorido indicador
  tft.fillCircle(statusX + 8, statusY + 8, 3, isBleConnected ? COLOR_GREEN : COLOR_BLUE);

  tft.setTextSize(1);
  tft.setTextColor(badgeText);
  tft.setCursor(statusX + 18, statusY + 4);
  tft.print(label);
}

void DisplayDriver::drawSpotifyCard(const SpotifyTrackData &track) {
  int cardX = 10;
  int cardY = 27;
  int cardW = 300;
  int cardH = 43;

  tft.fillRoundRect(cardX, cardY, cardW, cardH, 6, COLOR_SPOTIFY_DARK);
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 6, track.hasTrack ? COLOR_SPOTIFY_GREEN : COLOR_DIVIDER);

  if (!track.hasTrack) {
    // Spotify Inativo
    tft.fillCircle(cardX + 16, cardY + 21, 6, COLOR_SPOTIFY_GREEN);
    tft.drawCircle(cardX + 16, cardY + 21, 3, COLOR_SPOTIFY_DARK);

    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(cardX + 30, cardY + 17);
    tft.print("Spotify inativo ou em pausa");
    return;
  }

  // Linha 1: Ícone Play/Pause + Título e Artista
  if (track.isPlaying) {
    tft.fillTriangle(cardX + 8, cardY + 6, cardX + 8, cardY + 14, cardX + 14, cardY + 10, COLOR_SPOTIFY_GREEN);
  } else {
    tft.fillRect(cardX + 8, cardY + 6, 2, 8, COLOR_ACCENT);
    tft.fillRect(cardX + 12, cardY + 6, 2, 8, COLOR_ACCENT);
  }

  String songLine = track.title;
  if (track.artist.length() > 0) {
    songLine += " - " + track.artist;
  }
  if (songLine.length() > 34) {
    songLine = songLine.substring(0, 31) + "...";
  }

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(cardX + 20, cardY + 7);
  tft.print(songLine);

  // Linha 2: Barra de Progresso + Tempo
  drawSpotifyProgressOnly(track);

  // Linha 3: Próxima Música (A Seguir)
  tft.setTextSize(1);
  tft.setCursor(cardX + 8, cardY + 31);
  if (track.nextTitle.length() > 0) {
    tft.setTextColor(COLOR_CYAN);
    tft.print(">> A Seguir: ");

    String nextLine = track.nextTitle;
    if (track.nextArtist.length() > 0) {
      nextLine += " - " + track.nextArtist;
    }
    if (nextLine.length() > 24) {
      nextLine = nextLine.substring(0, 22) + "...";
    }
    tft.setTextColor(COLOR_TEXT);
    tft.print(nextLine);
  } else {
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.print(">> Sem proxima faixa na fila");
  }
}

void DisplayDriver::drawSpotifyProgressOnly(const SpotifyTrackData &track) {
  if (!track.hasTrack) return;

  int barX = 18;
  int barY = 46;
  int barW = 196;
  int barH = 3;

  // Barra de fundo
  tft.fillRect(barX, barY, barW, barH, COLOR_SPOTIFY_BAR);

  // Progresso preenchido
  if (track.durationMs > 0) {
    uint32_t prog = track.progressMs;
    if (prog > track.durationMs) prog = track.durationMs;
    int filledW = (prog * barW) / track.durationMs;
    if (filledW > barW) filledW = barW;
    if (filledW > 0) {
      tft.fillRect(barX, barY, filledW, barH, COLOR_SPOTIFY_GREEN);
    }
  }

  // Texto de Tempo (00:00 / 00:00)
  tft.fillRect(220, 44, 86, 9, COLOR_SPOTIFY_DARK);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(222, 45);
  String timeStr = SpotifyClient::formatTime(track.progressMs) + "/" + SpotifyClient::formatTime(track.durationMs);
  tft.print(timeStr);
}

void DisplayDriver::drawSpotifyFullScreen(const SpotifyTrackData &track) {
  tft.fillRect(0, 25, SCREEN_WIDTH, SCREEN_HEIGHT - 25, COLOR_SPOTIFY_DARK);

  // Botão Superior Direito para retornar ao Deck
  int backW = 76;
  int backH = 20;
  int backX = SCREEN_WIDTH - backW - 12;
  int backY = 32;
  tft.fillRoundRect(backX, backY, backW, backH, 6, BTN_BG_DEFAULT);
  tft.drawRoundRect(backX, backY, backW, backH, 6, COLOR_ACCENT);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(backX + 10, backY + 6);
  tft.print("< DECK");

  if (!track.hasTrack) {
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(40, 90);
    tft.print("Spotify Inativo");
    tft.setTextSize(1);
    tft.setCursor(40, 120);
    tft.print("Inicie uma musica no seu Mac ou celular");

    tft.fillRoundRect(80, 160, 160, 36, 8, BTN_BG_DEFAULT);
    tft.drawRoundRect(80, 160, 160, 36, 8, COLOR_SPOTIFY_GREEN);
    tft.setTextColor(COLOR_SPOTIFY_GREEN);
    tft.setCursor(95, 173);
    tft.print("< Voltar ao Deck");
    return;
  }

  // Título da música em tamanho 2
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TEXT);
  String title = track.title;
  if (title.length() > 16) title = title.substring(0, 14) + "...";
  tft.setCursor(16, 36);
  tft.print(title);

  // Artista e Álbum
  tft.setTextSize(1);
  tft.setTextColor(COLOR_SPOTIFY_GREEN);
  String artistAlbum = track.artist;
  if (track.album.length() > 0) artistAlbum += " \x07 " + track.album;
  if (artistAlbum.length() > 36) artistAlbum = artistAlbum.substring(0, 34) + "...";
  tft.setCursor(16, 58);
  tft.print(artistAlbum);

  // Status Badge
  int badgeW = 90;
  int badgeH = 18;
  int badgeX = 16;
  int badgeY = 74;
  if (track.isPlaying) {
    tft.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 6, 0x0A85);
    tft.drawRoundRect(badgeX, badgeY, badgeW, badgeH, 6, COLOR_SPOTIFY_GREEN);
    tft.setTextColor(COLOR_SPOTIFY_GREEN);
    tft.setCursor(badgeX + 8, badgeY + 5);
    tft.print("\x10 TOCANDO");
  } else {
    tft.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 6, 0x3180);
    tft.drawRoundRect(badgeX, badgeY, badgeW, badgeH, 6, COLOR_ACCENT);
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(badgeX + 8, badgeY + 5);
    tft.print("❚❚ PAUSADO");
  }

  // Barra de Progresso Grande
  int bX = 16;
  int bY = 104;
  int bW = 288;
  int bH = 6;
  tft.fillRect(bX, bY, bW, bH, COLOR_SPOTIFY_BAR);
  if (track.durationMs > 0) {
    int fW = (track.progressMs * bW) / track.durationMs;
    if (fW > bW) fW = bW;
    if (fW > 0) tft.fillRect(bX, bY, fW, bH, COLOR_SPOTIFY_GREEN);
  }

  // Tempos abaixo da barra
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(bX, bY + 10);
  tft.print(SpotifyClient::formatTime(track.progressMs));

  String totalStr = SpotifyClient::formatTime(track.durationMs);
  tft.setCursor(bX + bW - (totalStr.length() * 6), bY + 10);
  tft.print(totalStr);

  // Card Próxima Música (A Seguir)
  tft.fillRoundRect(16, 132, 288, 36, 8, 0x18E5);
  tft.drawRoundRect(16, 132, 288, 36, 8, COLOR_CYAN);
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(24, 140);
  tft.print(">> A Seguir na Fila:");
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(24, 153);
  String nextStr = track.nextTitle;
  if (track.nextArtist.length() > 0) nextStr += " - " + track.nextArtist;
  if (nextStr.length() > 36) nextStr = nextStr.substring(0, 34) + "...";
  if (nextStr.length() == 0) nextStr = "Fim da fila do Spotify";
  tft.print(nextStr);

  // 3 Botões de Controle na parte inferior
  // Botão 1: ANTERIOR (x = 20, y = 184, w = 84, h = 42)
  tft.fillRoundRect(20, 184, 84, 42, 8, BTN_BG_DEFAULT);
  tft.drawRoundRect(20, 184, 84, 42, 8, BTN_BORDER_DEFAULT);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(34, 201);
  tft.print("|<< PREV");

  // Botão 2: PLAY / PAUSE (x = 114, y = 184, w = 92, h = 42)
  uint16_t playBg = track.isPlaying ? 0x2126 : 0x0A85;
  uint16_t playBorder = track.isPlaying ? COLOR_ACCENT : COLOR_SPOTIFY_GREEN;
  tft.fillRoundRect(114, 184, 92, 42, 8, playBg);
  tft.drawRoundRect(114, 184, 92, 42, 8, playBorder);
  tft.setTextColor(track.isPlaying ? COLOR_ACCENT : COLOR_SPOTIFY_GREEN);
  if (track.isPlaying) {
    tft.setCursor(134, 201);
    tft.print("❚❚ PAUSE");
  } else {
    tft.setCursor(138, 201);
    tft.print("▶ PLAY");
  }

  // Botão 3: PRÓXIMO (x = 216, y = 184, w = 84, h = 42)
  tft.fillRoundRect(216, 184, 84, 42, 8, BTN_BG_DEFAULT);
  tft.drawRoundRect(216, 184, 84, 42, 8, BTN_BORDER_DEFAULT);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(232, 201);
  tft.print("NEXT >>");
}

void DisplayDriver::drawFooter(const String &info) {
  tft.fillRect(0, 226, SCREEN_WIDTH, 14, COLOR_BG);
  tft.drawFastHLine(10, 226, 300, COLOR_DIVIDER);
  tft.setTextSize(1);

  String textToShow;
  if (info.length() > 0) {
    textToShow = info;
    tft.setTextColor(COLOR_CYAN);
  } else {
    textToShow = "http://macdeck.local \x07 Apple Silicon Deck";
    tft.setTextColor(COLOR_TEXT_MUTED);
  }

  int textW = textToShow.length() * 6;
  tft.setCursor((SCREEN_WIDTH - textW) / 2, 228);
  tft.print(textToShow);
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
  tft.setCursor(cx - titleW / 2, btn.y + 50);
  tft.print(btn.title);

  // Subtítulo
  tft.setTextColor(COLOR_TEXT_MUTED);
  int subW = strlen(btn.subtitle) * 6;
  tft.setCursor(cx - subW / 2, btn.y + 62);
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
    case ICON_PAUSE:      drawPauseIcon(cx, cy, color); break;
    case ICON_NEXT:       drawNextIcon(cx, cy, color); break;
    case ICON_MUTE:       drawMuteIcon(cx, cy, color); break;
    case ICON_MIC_MUTE:   drawMicMuteIcon(cx, cy, color); break;
    case ICON_SCREENSHOT: drawScreenshotIcon(cx, cy, color); break;
    case ICON_LOCK:       drawLockIcon(cx, cy, color); break;
    default: break;
  }
}

void DisplayDriver::drawPauseIcon(int cx, int cy, uint16_t color) {
  tft.fillRect(cx - 7, cy - 8, 5, 16, color);
  tft.fillRect(cx + 2, cy - 8, 5, 16, color);
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
