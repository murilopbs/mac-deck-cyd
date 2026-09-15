#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// --- HARDWARE: ESP32-2432S028 (CYD - Cheap Yellow Display 2.8") ---
// ============================================================================
// Barramento SPI do Display ST7789 (HSPI)
#define TFT_MISO       12
#define TFT_MOSI       13
#define TFT_SCLK       14
#define TFT_CS         15
#define TFT_DC          2
#define TFT_RST        -1
#define TFT_BL         21  // Backlight PWM

// Barramento SPI do Touch XPT2046 (VSPI)
#define XPT2046_IRQ    36
#define XPT2046_MOSI   32
#define XPT2046_MISO   39
#define XPT2046_CLK    25
#define XPT2046_CS     33

// LED RGB Traseiro (Ativo em nível BAIXO / LOW)
#define CYD_LED_RED     4
#define CYD_LED_GREEN  16
#define CYD_LED_BLUE   17

// Dimensões do Display
#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  240
#define DISPLAY_ROTATION 3  // Horizontal 180° sem ruído

// ============================================================================
// --- CORES RGB565 (DESIGN DARK COM DESTAQUES MODERNOS) ---
// ============================================================================
#define COLOR_BG            0x0842  // Fundo grafite profundo escuro
#define COLOR_HEADER_BG     0x10A4  // Barra superior
#define COLOR_DIVIDER       0x2126  // Linhas divisórias sutis
#define COLOR_TEXT          0xFFFF  // Branco puro
#define COLOR_TEXT_MUTED    0x8410  // Cinza médio
#define COLOR_ACCENT        0xFD20  // Ouro / Âmbar
#define COLOR_CYAN          0x3DDF  // Ciano neon
#define COLOR_GREEN         0x2E8B  // Verde esmeralda (Conectado)
#define COLOR_RED           0xF986  // Vermelho coral (Mudo / Desconectado)
#define COLOR_BLUE          0x2CD9  // Azul Bluetooth
#define COLOR_PURPLE        0x925A  // Roxo moderno

// Cores dos Botões da Grade
#define BTN_BG_DEFAULT      0x18E5  // Fundo do botão em repouso
#define BTN_BORDER_DEFAULT  0x2967  // Borda sutil
#define BTN_BG_PRESSED      0x39E9  // Fundo iluminado quando pressionado
#define BTN_BORDER_PRESSED  0xFD20  // Borda dourada brilhante ao tocar

// Cores Spotify
#define COLOR_SPOTIFY_GREEN 0x1DC9  // Verde neon Spotify (#1DB954)
#define COLOR_SPOTIFY_DARK  0x10A2  // Grafite Spotify escuro (#121212)
#define COLOR_SPOTIFY_CARD  0x18E3  // Fundo do card Spotify
#define COLOR_SPOTIFY_BAR   0x2965  // Fundo da barra de progresso

// ============================================================================
// --- TIPOS DE AÇÃO DO STREAM DECK ---
// ============================================================================
enum ActionType {
  ACT_NONE = 0,
  ACT_MEDIA,       // Tecla de mídia Consumer Control (Play, Next, Prev, Mute)
  ACT_KEY,         // Tecla simples (Return, Space, etc.)
  ACT_MACRO        // Combinação modificadora (Cmd + Shift + ..., Cmd + Ctrl + ...)
};

enum IconType {
  ICON_PLAY_PAUSE = 0,
  ICON_PAUSE,
  ICON_NEXT,
  ICON_MUTE,
  ICON_MIC_MUTE,
  ICON_SCREENSHOT,
  ICON_LOCK
};

struct DeckButton {
  int x, y, w, h;
  const char* title;
  const char* subtitle;
  IconType icon;
  uint16_t iconColor;
  ActionType action;
  uint32_t mediaCode;
  uint8_t keyModifier;
  uint8_t keyCode;
  bool isPressed;
};

#endif // CONFIG_H
