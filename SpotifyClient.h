#ifndef SPOTIFY_CLIENT_H
#define SPOTIFY_CLIENT_H

#include <Arduino.h>

struct SpotifyTrackData {
  bool hasTrack;
  bool isPlaying;
  String title;
  String artist;
  String album;
  uint32_t progressMs;
  uint32_t durationMs;
  String nextTitle;
  String nextArtist;
};

class SpotifyClient {
public:
  SpotifyClient();
  void begin();
  void update();

  // Força uma consulta imediata à API do Spotify
  bool fetchNow();

  const SpotifyTrackData& getData() const { return currentData; }
  bool hasChanged() const { return dataChanged; }
  void clearChanged() { dataChanged = false; }

  // Formata tempo em "MM:SS"
  static String formatTime(uint32_t ms);

private:
  SpotifyTrackData currentData;
  bool dataChanged;
  unsigned long lastPollTime;
  unsigned long lastInterpolationTime;
  unsigned long pollInterval;
  int consecutiveFailures;

  bool fetchPlaybackState();
  bool fetchQueue();
};

extern SpotifyClient spotifyClient;

#endif // SPOTIFY_CLIENT_H
