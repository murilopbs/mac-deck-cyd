#include "WebPortal.h"
#include "WebPortalData.h"
#include "WiFiManager.h"
#include "BleManager.h"
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
