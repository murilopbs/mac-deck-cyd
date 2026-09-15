#ifndef SPOTIFY_AUTH_H
#define SPOTIFY_AUTH_H

#include <Arduino.h>

class SpotifyAuth {
public:
  SpotifyAuth();
  void begin();
  void update();

  // Força ou renova o access_token usando o refresh_token
  bool refreshToken();

  // Troca o authorization_code temporario pelo refresh_token definitivo
  bool exchangeCode(const String &code, const String &redirectUri);

  // Retorna um access_token valido (se estiver expirando, renova automaticamente)
  String getValidAccessToken();

  bool isConfigured() const;
  bool isAuthenticated() const;
  uint32_t getSecondsUntilExpiration() const;
  String getLastError() const { return lastError; }

private:
  String accessToken;
  unsigned long tokenAcquiredAt;
  uint32_t expiresInSeconds;
  String lastError;
  bool isRefreshing;

  bool sendTokenRequest(const String &postData);
  String generateBasicAuth(const String &clientId, const String &clientSecret);
};

extern SpotifyAuth spotifyAuth;

#endif // SPOTIFY_AUTH_H
