#include "SpotifyClient.h"
#include "SpotifyAuth.h"
#include "WiFiManager.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

SpotifyClient spotifyClient;

SpotifyClient::SpotifyClient()
    : dataChanged(false), lastPollTime(0), lastInterpolationTime(0),
      pollInterval(3500), consecutiveFailures(0) {
  currentData.hasTrack = false;
  currentData.isPlaying = false;
  currentData.title = "";
  currentData.artist = "";
  currentData.album = "";
  currentData.progressMs = 0;
  currentData.durationMs = 0;
  currentData.nextTitle = "";
  currentData.nextArtist = "";
}

void SpotifyClient::begin() {
  lastPollTime = 0;
  lastInterpolationTime = millis();
}

void SpotifyClient::update() {
  if (!wifiManager.isConnected() || !spotifyAuth.isAuthenticated()) {
    return;
  }

  unsigned long now = millis();

  // 1. Interpolação suave de progresso a cada segundo
  if (now - lastInterpolationTime >= 1000) {
    unsigned long delta = now - lastInterpolationTime;
    lastInterpolationTime = now;

    if (currentData.hasTrack && currentData.isPlaying) {
      currentData.progressMs += delta;
      if (currentData.progressMs > currentData.durationMs) {
        currentData.progressMs = currentData.durationMs;
      }
      dataChanged = true;
    }
  }

  // 2. Intervalo dinâmico de polling (3.5s tocando, 10s pausado/inativo)
  unsigned long activeInterval = (currentData.hasTrack && currentData.isPlaying) ? 3500 : 10000;
  if (now - lastPollTime >= activeInterval) {
    lastPollTime = now;
    fetchPlaybackState();
  }
}

bool SpotifyClient::fetchNow() {
  lastPollTime = millis();
  return fetchPlaybackState();
}

bool SpotifyClient::fetchPlaybackState() {
  String token = spotifyAuth.getValidAccessToken();
  if (token.length() == 0) {
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Economiza RAM sem handshake CA pesado

  HTTPClient http;
  http.begin(client, "https://api.spotify.com/v1/me/player/currently-playing");
  http.addHeader("Authorization", "Bearer " + token);
  http.setTimeout(4000);

  int httpCode = http.GET();

  // 204: Nenhuma música tocando / Spotify inativo
  if (httpCode == 204) {
    if (currentData.hasTrack) {
      currentData.hasTrack = false;
      currentData.isPlaying = false;
      currentData.title = "";
      currentData.artist = "";
      currentData.nextTitle = "";
      currentData.nextArtist = "";
      dataChanged = true;
    }
    http.end();
    return true;
  }

  if (httpCode == 200) {
    // Filtragem em streaming com ArduinoJson para economizar RAM
    JsonDocument filter;
    filter["is_playing"] = true;
    filter["progress_ms"] = true;
    filter["item"]["name"] = true;
    filter["item"]["duration_ms"] = true;
    filter["item"]["artists"][0]["name"] = true;
    filter["item"]["album"]["name"] = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();

    if (!err && doc["item"]) {
      String newTitle = doc["item"]["name"].as<String>();
      String newArtist = doc["item"]["artists"][0]["name"] | "Desconhecido";
      String newAlbum = doc["item"]["album"]["name"] | "";
      bool newPlaying = doc["is_playing"] | false;
      uint32_t newProg = doc["progress_ms"] | 0;
      uint32_t newDur = doc["item"]["duration_ms"] | 0;

      bool trackSwitched = (newTitle != currentData.title);

      currentData.hasTrack = true;
      currentData.isPlaying = newPlaying;
      currentData.title = newTitle;
      currentData.artist = newArtist;
      currentData.album = newAlbum;
      currentData.progressMs = newProg;
      currentData.durationMs = newDur;
      dataChanged = true;
      consecutiveFailures = 0;

      // Se mudou de música ou ainda não tem a próxima da fila, busca a fila
      if (trackSwitched || currentData.nextTitle.length() == 0) {
        fetchQueue();
      }

      return true;
    }
  } else if (httpCode == 401) {
    Serial.println("[SpotifyClient] Token expirado (401). Forçando renovação...");
    spotifyAuth.refreshToken();
  } else {
    Serial.printf("[SpotifyClient] HTTP Error: %d\n", httpCode);
    consecutiveFailures++;
  }

  http.end();
  return false;
}

bool SpotifyClient::fetchQueue() {
  String token = spotifyAuth.getValidAccessToken();
  if (token.length() == 0) return false;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, "https://api.spotify.com/v1/me/player/queue");
  http.addHeader("Authorization", "Bearer " + token);
  http.setTimeout(4000);

  int httpCode = http.GET();

  if (httpCode == 200) {
    // Filtragem em streaming com ArduinoJson para extrair apenas a próxima música da fila (queue[0])
    JsonDocument filter;
    filter["queue"][0]["name"] = true;
    filter["queue"][0]["artists"][0]["name"] = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();

    if (!err && doc["queue"] && doc["queue"].size() > 0) {
      currentData.nextTitle = doc["queue"][0]["name"].as<String>();
      currentData.nextArtist = doc["queue"][0]["artists"][0]["name"] | "";
      dataChanged = true;
      Serial.printf("[SpotifyClient] Próxima da fila capturada: %s - %s\n",
                    currentData.nextTitle.c_str(), currentData.nextArtist.c_str());
      return true;
    }
  }

  http.end();
  return false;
}

String SpotifyClient::formatTime(uint32_t ms) {
  uint32_t totalSec = ms / 1000;
  uint32_t minutes = totalSec / 60;
  uint32_t seconds = totalSec % 60;
  char buf[16];
  snprintf(buf, sizeof(buf), "%02u:%02u", minutes, seconds);
  return String(buf);
}
