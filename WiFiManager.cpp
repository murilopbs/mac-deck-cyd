#include "WiFiManager.h"
#include <Preferences.h>

static const char *PREF_NAMESPACE = "macdeck";
static const char *KEY_WIFI_SSID = "wifi_ssid";
static const char *KEY_WIFI_PASS = "wifi_pass";
static const char *KEY_SPOT_ID   = "spot_id";
static const char *KEY_SPOT_SEC  = "spot_sec";
static const char *KEY_SPOT_TOK  = "spot_tok";
static const char *KEY_BRIGHTNESS = "brightness";

WiFiManager wifiManager;

WiFiManager::WiFiManager()
    : apActive(false), lastConnectAttempt(0), apStartTimer(0) {}

void WiFiManager::begin() {
  String savedSSID = getSavedSSID();
  String savedPass = getSavedPass();

  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);

  if (savedSSID.length() > 0) {
    Serial.printf("[WiFi] Conectando a rede salva: %s\n", savedSSID.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    lastConnectAttempt = millis();
    apStartTimer = millis();
  } else {
    Serial.println("[WiFi] Nenhuma rede configurada. Iniciando modo Access Point de configuracao...");
    startAP();
  }
}

void WiFiManager::update() {
  // Se conectou ao STA e o modo AP ainda estava ativo, desliga o AP automaticamente
  if (apActive && isConnected()) {
    Serial.printf("[WiFi] STA conectado com sucesso (%s)! Desativando SoftAP...\n", WiFi.localIP().toString().c_str());
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    apActive = false;
  }

  // Se estiver tentando conectar no STA no boot e demorar mais de 25s, ativa o AP de contingência
  if (!apActive && !isConnected() && apStartTimer > 0) {
    if (millis() - apStartTimer > 25000) {
      Serial.println("[WiFi] Timeout inicial na conexao STA. Ativando AP de contingencia...");
      startAP();
    }
  }

  // Se desconectado, tenta reconectar periodicamente a cada 15 segundos (mesmo com AP ativo)
  if (!isConnected()) {
    if (millis() - lastConnectAttempt > 15000) {
      lastConnectAttempt = millis();
      String savedSSID = getSavedSSID();
      String savedPass = getSavedPass();
      if (savedSSID.length() > 0) {
        Serial.printf("[WiFi] Tentando reconectar a %s...\n", savedSSID.c_str());
        WiFi.begin(savedSSID.c_str(), savedPass.c_str());
      }
    }
  }
}

void WiFiManager::startAP() {
  apActive = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("MacDeck-Setup");
  Serial.printf("[WiFi] Access Point ativo: 'MacDeck-Setup'. IP: %s\n",
                WiFi.softAPIP().toString().c_str());
}

bool WiFiManager::isConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::getIP() {
  if (isConnected()) {
    return WiFi.localIP().toString();
  }
  if (apActive) {
    return WiFi.softAPIP().toString();
  }
  return "0.0.0.0";
}

String WiFiManager::getSSID() {
  if (isConnected()) {
    return WiFi.SSID();
  }
  if (apActive) {
    return "MacDeck-Setup (AP)";
  }
  return "Desconectado";
}

int WiFiManager::getRSSI() {
  if (isConnected()) {
    return WiFi.RSSI();
  }
  return -100;
}

void WiFiManager::saveCredentials(const String &ssid, const String &pass) {
  Preferences p;
  if (p.begin(PREF_NAMESPACE, false)) {
    p.putString(KEY_WIFI_SSID, ssid);
    p.putString(KEY_WIFI_PASS, pass);
    p.end();
  }
  Serial.printf("[WiFi] Novas credenciais salvas para %s. Reiniciando conexao...\n", ssid.c_str());
  
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  apActive = false;
  apStartTimer = millis();
  lastConnectAttempt = millis();
}

String WiFiManager::getSavedSSID() {
  Preferences p;
  String val = "";
  if (p.begin(PREF_NAMESPACE, true)) {
    val = p.getString(KEY_WIFI_SSID, "");
    p.end();
  }
  return val;
}

String WiFiManager::getSavedPass() {
  Preferences p;
  String val = "";
  if (p.begin(PREF_NAMESPACE, true)) {
    val = p.getString(KEY_WIFI_PASS, "");
    p.end();
  }
  return val;
}

void WiFiManager::saveSpotifyCredentials(const String &clientId, const String &clientSecret, const String &refreshToken) {
  Preferences p;
  if (p.begin(PREF_NAMESPACE, false)) {
    p.putString(KEY_SPOT_ID, clientId);
    p.putString(KEY_SPOT_SEC, clientSecret);
    p.putString(KEY_SPOT_TOK, refreshToken);
    p.end();
  }
  Serial.println("[Spotify] Credenciais salvas com sucesso no NVS.");
}

String WiFiManager::getSpotifyClientId() {
  Preferences p;
  String val = "";
  if (p.begin(PREF_NAMESPACE, true)) {
    val = p.getString(KEY_SPOT_ID, "");
    p.end();
  }
  return val;
}

String WiFiManager::getSpotifyClientSecret() {
  Preferences p;
  String val = "";
  if (p.begin(PREF_NAMESPACE, true)) {
    val = p.getString(KEY_SPOT_SEC, "");
    p.end();
  }
  return val;
}

String WiFiManager::getSpotifyRefreshToken() {
  Preferences p;
  String val = "";
  if (p.begin(PREF_NAMESPACE, true)) {
    val = p.getString(KEY_SPOT_TOK, "");
    p.end();
  }
  return val;
}

void WiFiManager::saveBrightness(uint8_t pct) {
  if (pct < 10) pct = 10;
  if (pct > 100) pct = 100;
  Preferences p;
  if (p.begin(PREF_NAMESPACE, false)) {
    p.putUChar(KEY_BRIGHTNESS, pct);
    p.end();
  }
  Serial.printf("[System] Brilho salvo no NVS: %d%%\n", pct);
}

uint8_t WiFiManager::getSavedBrightness() {
  Preferences p;
  uint8_t val = 85; // Padrão confortável
  if (p.begin(PREF_NAMESPACE, true)) {
    val = p.getUChar(KEY_BRIGHTNESS, 85);
    p.end();
  }
  if (val < 10) val = 10;
  if (val > 100) val = 100;
  return val;
}

int WiFiManager::scanNetworks() {
  Serial.println("[WiFi] Escaneando redes sem fio...");
  scannedList.clear();
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    ScannedWifi net;
    net.ssid = WiFi.SSID(i);
    net.rssi = WiFi.RSSI(i);
    net.isOpen = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    if (net.ssid.length() > 0) {
      scannedList.push_back(net);
    }
  }
  WiFi.scanDelete();
  return (int)scannedList.size();
}

int WiFiManager::getScannedCount() {
  return (int)scannedList.size();
}

ScannedWifi WiFiManager::getScannedNetwork(int index) {
  if (index >= 0 && index < (int)scannedList.size()) {
    return scannedList[index];
  }
  ScannedWifi empty;
  empty.ssid = "";
  empty.rssi = 0;
  empty.isOpen = false;
  return empty;
}
