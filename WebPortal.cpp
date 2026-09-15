#include "WebPortal.h"
#include "WebPortalData.h"
#include "WiFiManager.h"
#include "BleManager.h"
#include "SpotifyAuth.h"
#include <ESPmDNS.h>
#include <ArduinoJson.h>

WebPortal webPortal;

WebPortal::WebPortal() : server(80), serverActive(false), buttonCallback(nullptr) {}

void WebPortal::begin() {
  setupRoutes();
  server.begin();
  serverActive = true;

  if (MDNS.begin("macdeck")) {
    Serial.println("[WebPortal] mDNS responder iniciado: http://macdeck.local");
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("[WebPortal] Erro ao iniciar mDNS responder.");
  }
}

void WebPortal::update() {
  if (serverActive) {
    server.handleClient();
  }
}

void WebPortal::setupRoutes() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server.on("/api/wifi/scan", HTTP_GET, [this]() { handleScanWifi(); });
  server.on("/api/wifi", HTTP_POST, [this]() { handleSaveWifi(); });
  server.on("/api/spotify", HTTP_POST, [this]() { handleSaveSpotify(); });
  server.on("/api/spotify/status", HTTP_GET, [this]() { handleSpotifyStatus(); });
  server.on("/api/spotify/refresh", HTTP_POST, [this]() { handleSpotifyRefresh(); });
  server.on("/callback", HTTP_GET, [this]() { handleCallback(); });
  server.on("/api/action", HTTP_POST, [this]() { handleAction(); });
  server.onNotFound([this]() { handleNotFound(); });
}

void WebPortal::handleRoot() {
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", "max-age=86400");
  server.send_P(200, "text/html", (const char *)WEB_PORTAL_HTML_GZ, WEB_PORTAL_HTML_SIZE);
}

void WebPortal::handleStatus() {
  JsonDocument doc;
  doc["ble_connected"] = bleMgr.isConnected();
  doc["wifi_ssid"] = wifiManager.getSSID();
  doc["ip"] = wifiManager.getIP();
  doc["rssi"] = wifiManager.getRSSI();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;
  doc["spot_has_creds"] = (wifiManager.getSpotifyClientId().length() > 0);

  String response;
  serializeJson(doc, response);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

void WebPortal::handleScanWifi() {
  int count = wifiManager.scanNetworks();
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  for (int i = 0; i < count; i++) {
    ScannedWifi net = wifiManager.getScannedNetwork(i);
    JsonObject obj = array.add<JsonObject>();
    obj["ssid"] = net.ssid;
    obj["rssi"] = net.rssi;
    obj["open"] = net.isOpen;
  }

  String response;
  serializeJson(doc, response);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

void WebPortal::handleSaveWifi() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String ssid = doc["ssid"].as<String>();
  String pass = doc["pass"].as<String>();

  if (ssid.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"SSID cannot be empty\"}");
    return;
  }

  server.send(200, "application/json", "{\"success\":true}");
  delay(100);
  wifiManager.saveCredentials(ssid, pass);
}

void WebPortal::handleSaveSpotify() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String clientId = doc["client_id"].as<String>();
  String clientSecret = doc["client_secret"].as<String>();
  String refreshToken = doc["refresh_token"].as<String>();

  wifiManager.saveSpotifyCredentials(clientId, clientSecret, refreshToken);
  server.send(200, "application/json", "{\"success\":true}");
  
  // Tenta renovar o token imediatamente com as novas credenciais
  if (clientId.length() > 0 && clientSecret.length() > 0 && refreshToken.length() > 0) {
    spotifyAuth.refreshToken();
  }
}

void WebPortal::handleSpotifyStatus() {
  JsonDocument doc;
  doc["configured"] = spotifyAuth.isConfigured();
  doc["authenticated"] = spotifyAuth.isAuthenticated();
  doc["expires_in"] = spotifyAuth.getSecondsUntilExpiration();
  doc["error"] = spotifyAuth.getLastError();

  String response;
  serializeJson(doc, response);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

void WebPortal::handleSpotifyRefresh() {
  bool ok = spotifyAuth.refreshToken();
  JsonDocument doc;
  doc["success"] = ok;
  doc["authenticated"] = spotifyAuth.isAuthenticated();
  doc["expires_in"] = spotifyAuth.getSecondsUntilExpiration();
  doc["error"] = spotifyAuth.getLastError();

  String response;
  serializeJson(doc, response);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(ok ? 200 : 400, "application/json", response);
}

void WebPortal::handleCallback() {
  if (!server.hasArg("code")) {
    server.send(400, "text/html", "<h2>Erro</h2><p>Codigo de autorizacao ausente.</p><a href='/'>Voltar</a>");
    return;
  }

  String code = server.arg("code");
  Serial.printf("[WebPortal] Codigo OAuth recebido: %s... Trocando por tokens...\n", code.substring(0, 10).c_str());

  bool ok = spotifyAuth.exchangeCode(code, "http://macdeck.local/callback");
  if (ok) {
    String html = "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='UTF-8'><meta http-equiv='refresh' content='3;url=/'><style>"
                  "body{font-family:sans-serif;background:#0d1117;color:#f0f6fc;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;margin:0;}"
                  ".card{background:#161b22;border:1px solid #1DB954;border-radius:14px;padding:32px;text-align:center;max-width:420px;box-shadow:0 8px 30px rgba(0,0,0,0.5);}"
                  "h1{color:#1DB954;font-size:1.5rem;margin-bottom:12px;}p{color:#8b949e;font-size:0.9rem;margin-bottom:20px;line-height:1.5;}"
                  "a{display:inline-block;background:#1DB954;color:#000;font-weight:bold;text-decoration:none;padding:12px 24px;border-radius:8px;}"
                  "</style></head><body><div class='card'>"
                  "<h1>🎉 Conectado ao Spotify!</h1>"
                  "<p>O MacDeck CYD obteve os tokens de acesso com sucesso. Redirecionando para o painel em 3 segundos...</p>"
                  "<a href='/'>Ir para o MacDeck Agora</a>"
                  "</div></body></html>";
    server.send(200, "text/html", html);
  } else {
    String html = "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='UTF-8'><style>"
                  "body{font-family:sans-serif;background:#0d1117;color:#f0f6fc;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;margin:0;}"
                  ".card{background:#161b22;border:1px solid #f85149;border-radius:14px;padding:32px;text-align:center;max-width:420px;}"
                  "h1{color:#f85149;font-size:1.4rem;margin-bottom:12px;}p{color:#8b949e;font-size:0.9rem;margin-bottom:20px;}"
                  "a{display:inline-block;background:#388bfd;color:#fff;font-weight:bold;text-decoration:none;padding:10px 20px;border-radius:8px;}"
                  "</style></head><body><div class='card'>"
                  "<h1>Falha na Autorizacao</h1>"
                  "<p>" + spotifyAuth.getLastError() + "</p>"
                  "<a href='/'>Voltar e tentar novamente</a>"
                  "</div></body></html>";
    server.send(500, "text/html", html);
  }
}

void WebPortal::handleAction() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String action = doc["action"].as<String>();
  if (action == "press_button") {
    int buttonId = doc["id"] | -1;
    if (buttonId >= 0 && buttonCallback != nullptr) {
      buttonCallback(buttonId);
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
  } else if (action == "reboot") {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Rebooting\"}");
    delay(500);
    ESP.restart();
    return;
  }

  server.send(400, "application/json", "{\"error\":\"Unknown action\"}");
}

void WebPortal::handleNotFound() {
  // Redireciona qualquer rota nao encontrada para a raiz (Captive Portal behavior)
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}
