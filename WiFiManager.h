#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

struct ScannedWifi {
  String ssid;
  int rssi;
  bool isOpen;
};

class WiFiManager {
public:
  WiFiManager();
  void begin();
  void update();

  bool isConnected();
  String getIP();
  String getSSID();
  int getRSSI();
  bool isAPMode() const { return apActive; }

  void saveCredentials(const String &ssid, const String &pass);
  String getSavedSSID();
  String getSavedPass();

  // Spotify credentials storage (base para US02)
  void saveSpotifyCredentials(const String &clientId, const String &clientSecret, const String &refreshToken);
  String getSpotifyClientId();
  String getSpotifyClientSecret();
  String getSpotifyRefreshToken();

  int scanNetworks();
  int getScannedCount();
  ScannedWifi getScannedNetwork(int index);

private:
  bool apActive;
  unsigned long lastConnectAttempt;
  unsigned long apStartTimer;
  std::vector<ScannedWifi> scannedList;

  void startAP();
  void tryConnectSTA();
};

extern WiFiManager wifiManager;

#endif // WIFI_MANAGER_H
