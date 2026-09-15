#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>
#include <WebServer.h>

typedef void (*WebButtonCallback)(int buttonId);

class WebPortal {
public:
  WebPortal();
  void begin();
  void update();

  void setButtonCallback(WebButtonCallback cb) { buttonCallback = cb; }

private:
  WebServer server;
  bool serverActive;
  WebButtonCallback buttonCallback;

  void setupRoutes();
  void handleRoot();
  void handleStatus();
  void handleScanWifi();
  void handleSaveWifi();
  void handleSaveSpotify();
  void handleSpotifyStatus();
  void handleSpotifyRefresh();
  void handleCallback();
  void handleAction();
  void handleNotFound();
};

extern WebPortal webPortal;

#endif // WEB_PORTAL_H
