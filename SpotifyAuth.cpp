#include "SpotifyAuth.h"
#include "WiFiManager.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <mbedtls/base64.h>

SpotifyAuth spotifyAuth;

SpotifyAuth::SpotifyAuth()
    : tokenAcquiredAt(0), expiresInSeconds(0), lastError("Não autenticado"), isRefreshing(false) {}

void SpotifyAuth::begin() {
  tokenAcquiredAt = 0;
  expiresInSeconds = 0;
  lastError = "Aguardando credenciais";
  isRefreshing = false;

  if (isConfigured()) {
    Serial.println("[SpotifyAuth] Credenciais encontradas na Flash. Pronto para renovar token.");
    if (wifiManager.isConnected()) {
      refreshToken();
    }
  }
}

void SpotifyAuth::update() {
  if (!isConfigured()) return;

  // Se tem credenciais mas ainda nao obteve o token e o Wi-Fi acabou de conectar
  if (accessToken.length() == 0 && wifiManager.isConnected() && !isRefreshing) {
    refreshToken();
    return;
  }

  // Renova preventivamente se faltar menos de 5 minutos (300s)
  if (isAuthenticated() && getSecondsUntilExpiration() < 300 && !isRefreshing) {
    Serial.println("[SpotifyAuth] Token proximo de expirar. Renovando automaticamente em background...");
    refreshToken();
  }
}

bool SpotifyAuth::isConfigured() const {
  return (wifiManager.getSpotifyClientId().length() > 0 &&
          wifiManager.getSpotifyClientSecret().length() > 0 &&
          wifiManager.getSpotifyRefreshToken().length() > 0);
}

bool SpotifyAuth::isAuthenticated() const {
  if (accessToken.length() == 0 || tokenAcquiredAt == 0) return false;
  return (getSecondsUntilExpiration() > 0);
}

uint32_t SpotifyAuth::getSecondsUntilExpiration() const {
  if (tokenAcquiredAt == 0 || expiresInSeconds == 0) return 0;
  unsigned long elapsedSec = (millis() - tokenAcquiredAt) / 1000;
  if (elapsedSec >= expiresInSeconds) return 0;
  return (expiresInSeconds - elapsedSec);
}

String SpotifyAuth::getValidAccessToken() {
  if (isAuthenticated()) {
    return accessToken;
  }
  if (isConfigured() && wifiManager.isConnected()) {
    if (refreshToken()) {
      return accessToken;
    }
  }
  return "";
}

String SpotifyAuth::generateBasicAuth(const String &clientId, const String &clientSecret) {
  String raw = clientId + ":" + clientSecret;
  size_t outputLen = 0;
  mbedtls_base64_encode(nullptr, 0, &outputLen, (const unsigned char *)raw.c_str(), raw.length());

  std::vector<unsigned char> encoded(outputLen + 1, 0);
  mbedtls_base64_encode(encoded.data(), encoded.size(), &outputLen, (const unsigned char *)raw.c_str(), raw.length());
  return String((char *)encoded.data());
}

bool SpotifyAuth::refreshToken() {
  if (!wifiManager.isConnected()) {
    lastError = "Sem conexao Wi-Fi";
    return false;
  }

  String refreshTokenStr = wifiManager.getSpotifyRefreshToken();
  if (refreshTokenStr.length() == 0) {
    lastError = "Refresh token ausente";
    return false;
  }

  String postData = "grant_type=refresh_token&refresh_token=" + refreshTokenStr;
  return sendTokenRequest(postData);
}

bool SpotifyAuth::exchangeCode(const String &code, const String &redirectUri) {
  if (!wifiManager.isConnected()) {
    lastError = "Sem conexao Wi-Fi";
    return false;
  }

  String postData = "grant_type=authorization_code&code=" + code +
                    "&redirect_uri=" + redirectUri;
  return sendTokenRequest(postData);
}

bool SpotifyAuth::sendTokenRequest(const String &postData) {
  isRefreshing = true;
  String clientId = wifiManager.getSpotifyClientId();
  String clientSecret = wifiManager.getSpotifyClientSecret();

  if (clientId.length() == 0 || clientSecret.length() == 0) {
    lastError = "Client ID ou Secret nao configurados";
    isRefreshing = false;
    return false;
  }

  Serial.println("[SpotifyAuth] Enviando requisicao OAuth para accounts.spotify.com...");

  WiFiClientSecure client;
  client.setInsecure(); // Economiza memoria RAM sem alocacao pesada de certificados CA

  HTTPClient http;
  http.begin(client, "https://accounts.spotify.com/api/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", "Basic " + generateBasicAuth(clientId, clientSecret));
  http.setTimeout(8000);

  int httpCode = http.POST(postData);

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (!err && doc["access_token"]) {
      accessToken = doc["access_token"].as<String>();
      expiresInSeconds = doc["expires_in"] | 3600;
      tokenAcquiredAt = millis();
      lastError = "OK";

      // Se a resposta trouxer um novo refresh_token (comum em exchangeCode), salva
      if (doc["refresh_token"]) {
        String newRefresh = doc["refresh_token"].as<String>();
        if (newRefresh.length() > 0) {
          wifiManager.saveSpotifyCredentials(clientId, clientSecret, newRefresh);
        }
      }

      Serial.printf("[SpotifyAuth] Token obtido com sucesso! Validade: %u segundos.\n", expiresInSeconds);
      http.end();
      isRefreshing = false;
      return true;
    } else {
      lastError = "Erro ao processar JSON da resposta";
    }
  } else {
    String body = http.getString();
    lastError = "HTTP " + String(httpCode) + ": " + body;
    Serial.printf("[SpotifyAuth] Falha na autenticacao: %s\n", lastError.c_str());
  }

  http.end();
  isRefreshing = false;
  return false;
}
