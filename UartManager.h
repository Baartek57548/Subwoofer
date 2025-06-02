#ifndef UART_MANAGER_H
#define UART_MANAGER_H

#include <Arduino.h>
#include "ConfigManager.h"

class UartManager {
private:
  Stream* serial;
  bool active;
  unsigned long startTime;
  const unsigned long UART_TIMEOUT = 120000;  // 2 minuty

public:
  UartManager();
  void init(Stream* serial);
  void checkTimeout();
  void parseCommands(ConfigManager* config);
  void showCommands();
  bool isActive() { return active; }
  void activate();
};

#endif
