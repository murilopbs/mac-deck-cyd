#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <HijelHID_BLEKeyboard.h>
#include "Config.h"

class BleManager {
public:
  BleManager();
  void begin();
  void update();

  bool isConnected();
  bool isPaired();

  // Executa uma ação de botão do MacDeck
  void executeButton(DeckButton &btn);

  // Envio de comandos diretos
  void sendMedia(uint16_t mediaCode);
  void sendMacro(uint8_t keyCode, uint8_t modifiers);
  void sendKey(uint8_t keyCode);

  // Feedback luminoso no LED RGB onboard (Ativo em LOW)
  void pulseLed(bool r, bool g, bool b, uint16_t durationMs = 80);

private:
  HijelHID_BLEKeyboard bleKeyboard;
  bool wasConnected;
};

extern BleManager bleMgr;

#endif // BLE_MANAGER_H
