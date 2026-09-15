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
 * - Portal Web Local embarcado em http://macdeck.local para celular e Mac
 * - Coexistência Wi-Fi + BLE sem interferência ou perda de pacotes
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
#include "WiFiManager.h"
#include "WebPortal.h"
#include "SpotifyAuth.h"
#include "SpotifyClient.h"
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
bool lastWifiStatus = false;
bool lastApStatus = false;
unsigned long lastHeaderRefresh = 0;

void executeDeckButton(int index) {
  if (index < 0 || index >= 6) return;
  DeckButton &btn = buttons[index];
  Serial.printf("[DECK] Botao %d acionado: %s\n", index + 1, btn.title);

  // Feedback visual de botão pressionado
  btn.isPressed = true;
  display.drawButton(btn);

  // Dispara o comando BLE para o Mac
  bleMgr.executeButton(btn);

  // Duração do efeito de clique
  delay(100);

  // Restaura aparência normal
  btn.isPressed = false;
  display.drawButton(btn);
}

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

  // 4. Inicializa Wi-Fi Manager (com suporte a fallback AP)
  wifiManager.begin();

  // 5. Inicializa Web Portal (http://macdeck.local)
  webPortal.setButtonCallback(executeDeckButton);
  webPortal.begin();

  // 6. Inicializa Motor de Autenticação Spotify OAuth
  spotifyAuth.begin();

  // 7. Inicializa Cliente da Fila do Spotify Web API
  spotifyClient.begin();

  // 8. Renderiza Interface Completa
  display.clear(COLOR_BG);
  display.drawHeader(bleMgr.isConnected(), wifiManager.isConnected(), wifiManager.isAPMode());
  display.drawAllButtons(buttons);
  
  if (wifiManager.isConnected()) {
    display.drawFooter("http://macdeck.local \x07 IP: " + wifiManager.getIP());
  } else if (wifiManager.isAPMode()) {
    display.drawFooter("AP: MacDeck-Setup \x07 192.168.4.1");
  } else {
    display.drawFooter("http://macdeck.local \x07 Conectando Wi-Fi...");
  }

  Serial.println("[SYSTEM] Pronto! Acesse http://macdeck.local no navegador do Mac.");
}

void loop() {
  // 1. Monitora estado do Bluetooth BLE
  bleMgr.update();

  // 2. Atualiza Wi-Fi, Web Portal, renovação de tokens e polling da fila do Spotify
  wifiManager.update();
  webPortal.update();
  spotifyAuth.update();
  spotifyClient.update();

  // 3. Verifica alterações de status de conexão periodicamente
  if (millis() - lastHeaderRefresh > 500) {
    lastHeaderRefresh = millis();
    bool curBle = bleMgr.isConnected();
    bool curWifi = wifiManager.isConnected();
    bool curAp = wifiManager.isAPMode();

    if (curBle != lastBleStatus || curWifi != lastWifiStatus || curAp != lastApStatus) {
      lastBleStatus = curBle;
      lastWifiStatus = curWifi;
      lastApStatus = curAp;

      display.drawHeader(curBle, curWifi, curAp);
      if (curWifi) {
        display.drawFooter("http://macdeck.local \x07 IP: " + wifiManager.getIP());
      } else if (curAp) {
        display.drawFooter("AP: MacDeck-Setup \x07 192.168.4.1");
      } else {
        display.drawFooter("http://macdeck.local \x07 Conectando Wi-Fi...");
      }
    }
  }

  // 4. Processa toques na tela física (Touchscreen)
  int tx, ty;
  if (touch.getTouch(tx, ty)) {
    for (int i = 0; i < 6; i++) {
      DeckButton &btn = buttons[i];
      if (tx >= btn.x && tx <= (btn.x + btn.w) &&
          ty >= btn.y && ty <= (btn.y + btn.h)) {
        executeDeckButton(i);
        break;
      }
    }
  }

  delay(5);
}
