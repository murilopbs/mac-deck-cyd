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
 * - Integração Spotify Web API: Faixa Atual + Próxima da Fila + Progresso
 * - Modo Focus Full-Screen ao tocar no card do Spotify
 * - Coexistência Wi-Fi + BLE sem interferência ou perda de pacotes
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

// Modos de Exibição da Tela
enum ScreenMode {
  MODE_DECK = 0,        // Grade com 6 botões + Card Spotify no topo
  MODE_SPOTIFY_FOCUS    // Player do Spotify expandido em tela cheia
};

ScreenMode currentScreenMode = MODE_DECK;

// Definição dos 6 Botões da Grade (2 linhas x 3 colunas)
// Ajustados para y = 74..146 (Linha 1) e y = 150..222 (Linha 2)
DeckButton buttons[6] = {
  // --- LINHA 1 (y = 74 a 146, altura 72px) ---
  {
    10, 74, 94, 72,
    "PLAY/PAUSE", "Spotify/Midia",
    ICON_PLAY_PAUSE, COLOR_ACCENT,
    ACT_MEDIA, MEDIA_PLAY_PAUSE, 0, 0,
    false
  },
  {
    113, 74, 94, 72,
    "PROXIMO", "Next Track",
    ICON_NEXT, COLOR_CYAN,
    ACT_MEDIA, MEDIA_NEXT_TRACK, 0, 0,
    false
  },
  {
    216, 74, 94, 72,
    "MUDO", "Audio Mute",
    ICON_MUTE, COLOR_RED,
    ACT_MEDIA, MEDIA_MUTE, 0, 0,
    false
  },

  // --- LINHA 2 (y = 150 a 222, altura 72px) ---
  {
    10, 150, 94, 72,
    "MIC MUTE", "Meet / Zoom",
    ICON_MIC_MUTE, COLOR_RED,
    ACT_MACRO, 0, (KEY_MOD_LGUI | KEY_MOD_LSHIFT), KEY_M,
    false
  },
  {
    113, 150, 94, 72,
    "CAPTURA", "Cmd+Shift+4",
    ICON_SCREENSHOT, COLOR_GREEN,
    ACT_MACRO, 0, (KEY_MOD_LGUI | KEY_MOD_LSHIFT), KEY_4,
    false
  },
  {
    216, 150, 94, 72,
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
String lastDisplayedSong = "";
bool lastPlayingState = false;

void redrawCurrentScreen() {
  if (currentScreenMode == MODE_DECK) {
    display.clear(COLOR_BG);
    display.drawHeader(bleMgr.isConnected(), wifiManager.isConnected(), wifiManager.isAPMode());
    display.drawSpotifyCard(spotifyClient.getData());
    display.drawAllButtons(buttons);
    if (wifiManager.isConnected()) {
      display.drawFooter("http://macdeck.local \x07 IP: " + wifiManager.getIP());
    } else if (wifiManager.isAPMode()) {
      display.drawFooter("AP: MacDeck-Setup \x07 192.168.4.1");
    } else {
      display.drawFooter("http://macdeck.local \x07 Toque no card para ampliar");
    }
  } else {
    display.drawHeader(bleMgr.isConnected(), wifiManager.isConnected(), wifiManager.isAPMode());
    display.drawSpotifyFullScreen(spotifyClient.getData());
  }
}

void executeDeckButton(int index) {
  if (index < 0 || index >= 6) return;
  DeckButton &btn = buttons[index];
  Serial.printf("[DECK] Botao %d acionado: %s\n", index + 1, btn.title);

  // Feedback visual de botão pressionado
  btn.isPressed = true;
  if (currentScreenMode == MODE_DECK) {
    display.drawButton(btn);
  }

  // Dispara o comando BLE para o Mac
  bleMgr.executeButton(btn);

  // Duração do efeito de clique
  delay(100);

  // Restaura aparência normal
  btn.isPressed = false;
  if (currentScreenMode == MODE_DECK) {
    display.drawButton(btn);
  }
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
  redrawCurrentScreen();

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

  // 3. Atualização de status do Spotify na tela
  if (spotifyClient.hasChanged()) {
    const SpotifyTrackData &track = spotifyClient.getData();

    if (currentScreenMode == MODE_SPOTIFY_FOCUS) {
      display.drawSpotifyFullScreen(track);
    } else {
      // Se a música ou estado de reprodução mudou, redesenha o card inteiro
      if (track.title != lastDisplayedSong || track.isPlaying != lastPlayingState) {
        lastDisplayedSong = track.title;
        lastPlayingState = track.isPlaying;
        display.drawSpotifyCard(track);
      } else {
        // Apenas o tempo avançou: atualiza a barra de progresso suavemente
        display.drawSpotifyProgressOnly(track);
      }
    }
    spotifyClient.clearChanged();
  }

  // 4. Verifica alterações de status de conexão de rede periodicamente
  if (millis() - lastHeaderRefresh > 1000) {
    lastHeaderRefresh = millis();
    bool curBle = bleMgr.isConnected();
    bool curWifi = wifiManager.isConnected();
    bool curAp = wifiManager.isAPMode();

    if (curBle != lastBleStatus || curWifi != lastWifiStatus || curAp != lastApStatus) {
      lastBleStatus = curBle;
      lastWifiStatus = curWifi;
      lastApStatus = curAp;
      display.drawHeader(curBle, curWifi, curAp);
    }
  }

  // 5. Processa toques na tela física (Touchscreen)
  int tx, ty;
  if (touch.getTouch(tx, ty)) {
    if (currentScreenMode == MODE_SPOTIFY_FOCUS) {
      // No modo foco, qualquer toque ou toque no botão de voltar retorna ao deck
      Serial.println("[UI] Saindo do Modo Focus... Retornando ao Stream Deck.");
      currentScreenMode = MODE_DECK;
      redrawCurrentScreen();
      delay(200);
    } else {
      // Modo Deck
      // Toque na área do Spotify Card (y = 26..72) expande para modo foco
      if (ty >= 26 && ty <= 72 && tx >= 10 && tx <= 310) {
        Serial.println("[UI] Toque no Spotify Card: abrindo Modo Focus Full-Screen...");
        currentScreenMode = MODE_SPOTIFY_FOCUS;
        redrawCurrentScreen();
        delay(200);
      } else {
        // Toque na grade dos 6 botões
        for (int i = 0; i < 6; i++) {
          DeckButton &btn = buttons[i];
          if (tx >= btn.x && tx <= (btn.x + btn.w) &&
              ty >= btn.y && ty <= (btn.y + btn.h)) {
            executeDeckButton(i);
            break;
          }
        }
      }
    }
  }

  delay(5);
}
