/*
 * ============================================================================
 * PROJETO: MACDECK CYD - STREAM DECK BLUETOOTH BLE PARA MAC (APPLE SILICON)
 * ============================================================================
 * 
 * Hardware: ESP32-2432S028 (CYD - Cheap Yellow Display 2.8" Touch)
 * Compatibilidade: macOS Sonoma, Sequoia, Ventura, Monterey
 * Chips Suportados: Apple Silicon M1, M2, M3, M4 e Intel
 * 
 * Recursos:
 * - 6 Botões Touch com ícones vetoriais modernos e feedback tátil
 * - Bluetooth Low Energy nativo HID (sem precisar instalar nada no Mac)
 * - Atalhos de Mídia (Play/Pause, Próximo, Mudo)
 * - Atalhos de Produtividade e Reunião (Mic Mute, Print, Travar Mac)
 * - LED RGB traseiro para feedback de comando
 * 
 * ============================================================================
 */

#include "Config.h"
#include "DisplayDriver.h"
#include "TouchDriver.h"
#include "BleManager.h"
#include <BLEHIDKeys.h>
#include <BLEHIDMediaKeys.h>

// Definição dos 6 Botões da Grade (2 linhas x 3 colunas)
DeckButton buttons[6] = {
  // --- LINHA 1 (y = 36 a 122) ---
  {
    10, 36, 94, 86,
    "PLAY/PAUSE", "Spotify/Midia",
    ICON_PLAY_PAUSE, COLOR_ACCENT,
    ACT_MEDIA, MEDIA_PLAY_PAUSE, 0, 0,
    false
  },
  {
    113, 36, 94, 86,
    "PROXIMO", "Next Track",
    ICON_NEXT, COLOR_CYAN,
    ACT_MEDIA, MEDIA_NEXT_TRACK, 0, 0,
    false
  },
  {
    216, 36, 94, 86,
    "MUDO", "Audio Mute",
    ICON_MUTE, COLOR_RED,
    ACT_MEDIA, MEDIA_MUTE, 0, 0,
    false
  },

  // --- LINHA 2 (y = 130 a 216) ---
  {
    10, 130, 94, 86,
    "MIC MUTE", "Meet / Zoom",
    ICON_MIC_MUTE, COLOR_RED,
    ACT_MACRO, 0, (KEY_MOD_LGUI | KEY_MOD_LSHIFT), KEY_M,
    false
  },
  {
    113, 130, 94, 86,
    "CAPTURA", "Cmd+Shift+4",
    ICON_SCREENSHOT, COLOR_GREEN,
    ACT_MACRO, 0, (KEY_MOD_LGUI | KEY_MOD_LSHIFT), KEY_4,
    false
  },
  {
    216, 130, 94, 86,
    "TRAVAR MAC", "Lock Screen",
    ICON_LOCK, COLOR_PURPLE,
    ACT_MACRO, 0, (KEY_MOD_LGUI | KEY_MOD_LCTRL), KEY_Q,
    false
  }
};

bool lastBleStatus = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=======================================================");
  Serial.println("     MACDECK CYD - STREAM DECK BLUETOOTH BLE (ARM)");
  Serial.println("=======================================================\n");

  // 1. Inicializa Display ST7789
  display.begin();

  // 2. Inicializa Touchscreen XPT2046
  touch.begin();

  // 3. Inicializa Bluetooth BLE Keyboard
  bleMgr.begin();

  // 4. Renderiza Interface Completa
  display.clear(COLOR_BG);
  display.drawHeader(bleMgr.isConnected());
  display.drawAllButtons(buttons);
  display.drawFooter();

  Serial.println("[SYSTEM] Pronto! Abra Ajustes > Bluetooth no Mac e conecte.");
}

void loop() {
  // 1. Monitora estado da conexão Bluetooth
  bleMgr.update();
  bool currentBleStatus = bleMgr.isConnected();
  if (currentBleStatus != lastBleStatus) {
    lastBleStatus = currentBleStatus;
    display.drawHeader(currentBleStatus);
  }

  // 2. Processa toques na tela (Touchscreen)
  int tx, ty;
  if (touch.getTouch(tx, ty)) {
    for (int i = 0; i < 6; i++) {
      DeckButton &btn = buttons[i];
      if (tx >= btn.x && tx <= (btn.x + btn.w) &&
          ty >= btn.y && ty <= (btn.y + btn.h)) {

        Serial.printf("[DECK] Botao %d pressionado: %s\n", i + 1, btn.title);

        // Feedback visual de botão pressionado
        btn.isPressed = true;
        display.drawButton(btn);

        // Dispara o comando BLE para o Mac
        bleMgr.executeButton(btn);

        // Duração do efeito de clique
        delay(120);

        // Restaura aparência normal
        btn.isPressed = false;
        display.drawButton(btn);
        break;
      }
    }
  }

  delay(10);
}
