#include "BleManager.h"

BleManager bleMgr;

BleManager::BleManager()
  : bleKeyboard("MacDeck CYD", "Apple", 100),
    wasConnected(false) {
}

void BleManager::begin() {
  Serial.println("[BLE] Inicializando MacDeck CYD Bluetooth...");
  bleKeyboard.begin();
  bleKeyboard.setTapDelay(30);
  bleKeyboard.setKeyGap(20);

  // Configura pinos do LED RGB traseiro para feedback visual
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);

  // Desliga todos os LEDs (ativo em LOW)
  digitalWrite(CYD_LED_RED, HIGH);
  digitalWrite(CYD_LED_GREEN, HIGH);
  digitalWrite(CYD_LED_BLUE, HIGH);

  Serial.println("[BLE] Anunciando como 'MacDeck CYD'. Emparelhe no MacBook!");
}

void BleManager::update() {
  bool connected = isConnected();
  if (connected != wasConnected) {
    wasConnected = connected;
    if (connected) {
      Serial.println("[BLE] MacBook conectado com sucesso!");
      // Breve piscar verde de confirmação
      digitalWrite(CYD_LED_GREEN, LOW);
      delay(120);
      digitalWrite(CYD_LED_GREEN, HIGH);
    } else {
      Serial.println("[BLE] Conexao Bluetooth perdida. Aguardando Mac...");
      // Piscar azul
      digitalWrite(CYD_LED_BLUE, LOW);
      delay(120);
      digitalWrite(CYD_LED_BLUE, HIGH);
    }
  }
}

bool BleManager::isConnected() {
  return bleKeyboard.isConnected();
}

bool BleManager::isPaired() {
  return bleKeyboard.isPaired();
}

void BleManager::executeButton(DeckButton &btn) {
  if (!isConnected()) {
    Serial.println("[BLE] Tentativa de comando com Mac desconectado.");
    // Pisca vermelho avisando que não está conectado
    digitalWrite(CYD_LED_RED, LOW);
    delay(60);
    digitalWrite(CYD_LED_RED, HIGH);
    return;
  }

  // Feedback luminoso de disparo no LED traseiro (Verde)
  digitalWrite(CYD_LED_GREEN, LOW);

  switch (btn.action) {
    case ACT_MEDIA:
      Serial.printf("[BLE] Enviando Tecla de Midia: 0x%04X (%s)\n", btn.mediaCode, btn.title);
      sendMedia(btn.mediaCode);
      break;

    case ACT_MACRO:
      Serial.printf("[BLE] Enviando Macro: Key 0x%02X + Mod 0x%02X (%s)\n", btn.keyCode, btn.keyModifier, btn.title);
      sendMacro(btn.keyCode, btn.keyModifier);
      break;

    case ACT_KEY:
      Serial.printf("[BLE] Enviando Tecla: 0x%02X (%s)\n", btn.keyCode, btn.title);
      sendKey(btn.keyCode);
      break;

    case ACT_NONE:
    default:
      break;
  }

  delay(40);
  digitalWrite(CYD_LED_GREEN, HIGH);
}

void BleManager::sendMedia(uint16_t mediaCode) {
  bleKeyboard.tap(mediaCode);
}

void BleManager::sendMacro(uint8_t keyCode, uint8_t modifiers) {
  bleKeyboard.tap(keyCode, modifiers);
}

void BleManager::sendKey(uint8_t keyCode) {
  bleKeyboard.tap(keyCode);
}

void BleManager::pulseLed(bool r, bool g, bool b, uint16_t durationMs) {
  // LED RGB onboard do CYD é ativo em LOW
  digitalWrite(CYD_LED_RED, r ? LOW : HIGH);
  digitalWrite(CYD_LED_GREEN, g ? LOW : HIGH);
  digitalWrite(CYD_LED_BLUE, b ? LOW : HIGH);
  delay(durationMs);
  digitalWrite(CYD_LED_RED, HIGH);
  digitalWrite(CYD_LED_GREEN, HIGH);
  digitalWrite(CYD_LED_BLUE, HIGH);
}
