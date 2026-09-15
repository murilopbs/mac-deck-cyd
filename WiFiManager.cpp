#include "WiFiManager.h"
#include <Preferences.h>

static Preferences prefs;
static const char *PREF_NAMESPACE = "macdeck";
static const char *KEY_WIFI_SSID = "wifi_ssid";
static const char *KEY_WIFI_PASS = "wifi_pass";
static const char *KEY_SPOT_ID   = "spot_id";
static const char *KEY_SPOT_SEC  = "spot_sec";
static const char *KEY_SPOT_TOK  = "spot_tok";

WiFiManager wifiManager;

WiFiManager::WiFiManager()
    : apActive(false), lastConnectAttempt(0), apStartTimer(0) {}

void WiFiManager::begin() {
  prefs.begin(PREF_NAMESPACE, false);
  String savedSSID = prefs.getString(KEY_WIFI_SSID, "");
  String savedPass = prefs.getString(KEY_WIFI_PASS, "");

  WiFi.persistent(false);
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
  // Se estiver tentando conectar no STA e demorar mais de 15s sem sucesso, abre o AP de configuracao
  if (!apActive && !isConnected() && apStartTimer > 0) {
    if (millis() - apStartTimer > 15000) {
      Serial.println("[WiFi] Timeout na conexao STA. Ativando AP de contingencia...");
      startAP();
    }
  }

  // Se nao estiver em modo AP e perdeu conexao, tenta reconectar a cada 30 segundos
  if (!apActive && !isConnected()) {
    if (millis() - lastConnectAttempt > 30000) {
      lastConnectAttempt = millis();
      String savedSSID = getSavedSSID();
      String savedPass = getSavedPass();
      if (savedSSID.length() > 0) {
        Serial.printf("[WiFi] Tentando reconectar a %s...\n", savedSSID.c_str());
        WiFi.disconnect();
        WiFi.begin(savedSSID.c_str(), savedPass.c_str());
      }
    }
  }
}

void WiFiManager::startAP() {
  apActive = true;
  WiFi.mode(WIFI_AP_STA);
  // Cria rede Wi-Fi aberta MacDeck-Setup para facilitar conexao de primeira viagem
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
  prefs.putString(KEY_WIFI_SSID, ssid);
  prefs.putString(KEY_WIFI_PASS, pass);
  Serial.printf("[WiFi] Novas credenciais salvas para %s. Reiniciando conexao...\n", ssid.c_str());
  
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  apActive = false;
  apStartTimer = millis();
  lastConnectAttempt = millis();
}

String WiFiManager::getSavedSSID() {
  return prefs.getString(KEY_WIFI_SSID, "");
}

String WiFiManager::getSavedPass() {
  return prefs.getString(KEY_WIFI_PASS, "");
}

void WiFiManager::saveSpotifyCredentials(const String &clientId, const String &clientSecret, const String &refreshToken) {
  prefs.putString(KEY_SPOT_ID, clientId);
  prefs.putString(KEY_SPOT_SEC, clientSecret);
  prefs.putString(KEY_SPOT_TOK, refreshToken);
  Serial.println("[Spotify] Credenciais salvas com sucesso no NVS.");
}

String WiFiManager::getSpotifyClientId() {
  return prefs.getString(KEY_SPOT_ID, "");
}

String WiFiManager::getSpotifyClientSecret() {
  return prefs.getString(KEY_SPOT_SEC, "");
}

String WiFiManager::getSpotifyRefreshToken() {
  return prefs.getString(KEY_SPOT_TOK, "");
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
