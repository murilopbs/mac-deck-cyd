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
 * - Botão Play/Pause adaptativo dinâmico (inverte ícone instantaneamente)
 * - Controles de mídia híbridos: BLE HID instantâneo + API REST do Spotify
 * - Modo Focus Full-Screen com botões interativos (Prev, Play/Pause, Next)
 * - Feedback luminoso colorido no LED RGB traseiro
 * - Portal Web Local em http://macdeck.local
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
#include "ScreensaverDriver.h"
#include <BLEHIDKeys.h>
#include <BLEHIDMediaKeys.h>

// Modos de Exibição da Tela
enum ScreenMode {
  MODE_DECK = 0,        // Grade com 6 botões + Card Spotify no topo
  MODE_SPOTIFY_FOCUS,   // Player do Spotify expandido em tela cheia interativo
  MODE_SCREENSAVER      // Protetor de tela animado com GIF
};

ScreenMode currentScreenMode = MODE_DECK;

// Definição dos 6 Botões da Grade (2 linhas x 3 colunas)
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

void updateSpotifyButtonState() {
  bool isPlaying = spotifyClient.getData().isPlaying;
  if (isPlaying) {
    buttons[0].title = "PAUSE";
    buttons[0].subtitle = "Spotify";
    buttons[0].icon = ICON_PAUSE;
    buttons[0].iconColor = COLOR_ACCENT;
  } else {
    buttons[0].title = "PLAY";
    buttons[0].subtitle = "Spotify/Midia";
    buttons[0].icon = ICON_PLAY_PAUSE;
    buttons[0].iconColor = COLOR_SPOTIFY_GREEN;
  }
}

void redrawCurrentScreen() {
  updateSpotifyButtonState();

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
  if (index == 99) {
    Serial.println("[DECK] Acionamento do Screensaver via Web Portal!");
    currentScreenMode = MODE_SCREENSAVER;
    screensaver.start();
    return;
  }
  if (index < 0 || index >= 6) return;
  DeckButton &btn = buttons[index];
  Serial.printf("[DECK] Botao %d acionado: %s\n", index + 1, btn.title);

  // Tratamento especial para PLAY/PAUSE (Botão 0)
  if (index == 0) {
    bool nextState = !spotifyClient.getData().isPlaying;
    spotifyClient.setOptimisticPlaying(nextState);
    updateSpotifyButtonState();

    if (currentScreenMode == MODE_DECK) {
      btn.isPressed = true;
      display.drawButton(btn);
      display.drawSpotifyCard(spotifyClient.getData());
    }

    // Feedback luminoso no LED RGB: Verde se deu play, Âmbar se deu pause
    bleMgr.pulseLed(nextState ? false : true, true, false, 80);

    // Dispara via BLE HID para o Mac
    bleMgr.sendMedia(MEDIA_PLAY_PAUSE);

    // Se conectado ao Wi-Fi e com Spotify autenticado, chama também a API REST
    if (wifiManager.isConnected() && spotifyAuth.isAuthenticated()) {
      if (nextState) spotifyClient.play(); else spotifyClient.pause();
    }

    delay(100);
    btn.isPressed = false;
    if (currentScreenMode == MODE_DECK) {
      display.drawButton(btn);
    }
    return;
  }

  // Tratamento para PRÓXIMO (Botão 1)
  if (index == 1) {
    btn.isPressed = true;
    if (currentScreenMode == MODE_DECK) display.drawButton(btn);

    // Feedback ciano
    bleMgr.pulseLed(false, true, true, 80);
    bleMgr.sendMedia(MEDIA_NEXT_TRACK);

    if (wifiManager.isConnected() && spotifyAuth.isAuthenticated()) {
      spotifyClient.next();
    }

    delay(100);
    btn.isPressed = false;
    if (currentScreenMode == MODE_DECK) display.drawButton(btn);
    return;
  }

  // Tratamento para MUDO (Botão 2)
  if (index == 2) {
    btn.isPressed = true;
    if (currentScreenMode == MODE_DECK) display.drawButton(btn);

    // Feedback vermelho
    bleMgr.pulseLed(true, false, false, 100);
    bleMgr.sendMedia(MEDIA_MUTE);

    delay(100);
    btn.isPressed = false;
    if (currentScreenMode == MODE_DECK) display.drawButton(btn);
    return;
  }

  // Botões de macro padrão (Mic Mute, Print, Lock)
  btn.isPressed = true;
  if (currentScreenMode == MODE_DECK) display.drawButton(btn);

  bleMgr.executeButton(btn);

  delay(100);
  btn.isPressed = false;
  if (currentScreenMode == MODE_DECK) display.drawButton(btn);
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

  // 8. Inicializa Protetor de Tela (Screensaver)
  screensaver.begin();

  // 9. Renderiza Interface Completa
  redrawCurrentScreen();

  // 10. Cria Tarefa de Rede Dedicada no Core 0 (Wi-Fi, WebPortal, Spotify)
  xTaskCreatePinnedToCore(
    [](void *pvParameters) {
      Serial.printf("[SYSTEM] Tarefa de rede iniciada com sucesso no Core %d\n", xPortGetCoreID());
      for (;;) {
        wifiManager.update();
        webPortal.update();
        spotifyAuth.update();
        spotifyClient.update();
        vTaskDelay(pdMS_TO_TICKS(15)); // Cede tempo para o Task Watchdog e Idle Task do Core 0
      }
    },
    "NetworkTask",
    8192,
    NULL,
    1,
    NULL,
    0 // Fixado no Core 0!
  );

  Serial.println("[SYSTEM] Pronto! Acesse http://macdeck.local no navegador do Mac.");
}

void loop() {
  // 1. Monitora estado do Bluetooth BLE no Core 1
  bleMgr.update();

  // (Todas as chamadas pesadas de Wi-Fi, WebServer e HTTPS do Spotify rodam no Core 0!)

  // Se estiver em modo Screensaver, atualiza animação do GIF e pula desenho da interface
  if (currentScreenMode == MODE_SCREENSAVER) {
    screensaver.update();
  } else {
    // 2. Atualização de status do Spotify na tela (thread-safe)
    if (spotifyClient.hasChanged()) {
      SpotifyTrackData track = spotifyClient.getData();
      updateSpotifyButtonState();

      if (currentScreenMode == MODE_SPOTIFY_FOCUS) {
        display.drawSpotifyFullScreen(track);
      } else {
        // Se a música ou estado de reprodução mudou, redesenha o card e o botão play/pause
        if (track.title != lastDisplayedSong || track.isPlaying != lastPlayingState) {
          lastDisplayedSong = track.title;
          lastPlayingState = track.isPlaying;
          display.drawSpotifyCard(track);
          display.drawButton(buttons[0]);
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
  }

  // 5. Processa toques na tela física (Touchscreen)
  int tx, ty;
  if (touch.getTouch(tx, ty)) {
    if (currentScreenMode == MODE_SCREENSAVER) {
      // Qualquer toque no screensaver acorda o MacDeck
      Serial.println("[UI] Toque na tela: saindo do Screensaver.");
      screensaver.stop();
      currentScreenMode = MODE_DECK;
      redrawCurrentScreen();
      delay(200);
    } else if (currentScreenMode == MODE_SPOTIFY_FOCUS) {
      // Modo Focus Full-Screen
      // Botão Voltar (< DECK) no canto superior direito
      if (tx >= 210 && ty >= 25 && ty <= 58) {
        Serial.println("[UI] Botao Voltar pressionado: retornando ao Deck.");
        currentScreenMode = MODE_DECK;
        redrawCurrentScreen();
        delay(150);
      }
      // Botão PREV (x = 20..104, y = 180..230)
      else if (tx >= 15 && tx <= 105 && ty >= 175 && ty <= 235) {
        Serial.println("[UI] Focus Player: Botao PREV acionado.");
        bleMgr.pulseLed(false, true, true, 80);
        bleMgr.sendMedia(MEDIA_PREV_TRACK);
        if (wifiManager.isConnected() && spotifyAuth.isAuthenticated()) {
          spotifyClient.previous();
        }
        delay(150);
      }
      // Botão PLAY/PAUSE (x = 110..210, y = 180..230)
      else if (tx >= 110 && tx <= 210 && ty >= 175 && ty <= 235) {
        Serial.println("[UI] Focus Player: Botao PLAY/PAUSE acionado.");
        bool nextState = !spotifyClient.getData().isPlaying;
        spotifyClient.setOptimisticPlaying(nextState);
        updateSpotifyButtonState();

        bleMgr.pulseLed(nextState ? false : true, true, false, 80);
        bleMgr.sendMedia(MEDIA_PLAY_PAUSE);

        if (wifiManager.isConnected() && spotifyAuth.isAuthenticated()) {
          if (nextState) spotifyClient.play(); else spotifyClient.pause();
        }
        display.drawSpotifyFullScreen(spotifyClient.getData());
        delay(150);
      }
      // Botão NEXT (x = 214..305, y = 180..230)
      else if (tx >= 214 && tx <= 305 && ty >= 175 && ty <= 235) {
        Serial.println("[UI] Focus Player: Botao NEXT acionado.");
        bleMgr.pulseLed(false, true, true, 80);
        bleMgr.sendMedia(MEDIA_NEXT_TRACK);
        if (wifiManager.isConnected() && spotifyAuth.isAuthenticated()) {
          spotifyClient.next();
        }
        delay(150);
      }
      // Toque em qualquer outra parte do player retorna ao Deck
      else if (ty >= 60 && ty <= 170) {
        currentScreenMode = MODE_DECK;
        redrawCurrentScreen();
        delay(150);
      }
    } else {
      // Modo Deck
      // Toque no botão seta '>' no cabeçalho (x = 275..320, y = 0..26) ativa o Screensaver
      if (tx >= 270 && ty <= 28) {
        Serial.println("[UI] Botao Seta acionado: iniciando Screensaver com GIF!");
        currentScreenMode = MODE_SCREENSAVER;
        screensaver.start();
        delay(200);
      }
      // Toque na área do Spotify Card (y = 26..72) expande para modo foco
      else if (ty >= 26 && ty <= 72 && tx >= 10 && tx <= 265) {
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
