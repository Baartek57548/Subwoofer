#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <Arduino.h>
#include "ConfigManager.h"
#include "ConsoleLogger.h"

// Stany sekwencji nieblokujących
enum SequenceState {
  SEQUENCE_IDLE,
  SEQUENCE_STARTUP_POWER,
  SEQUENCE_STARTUP_SPEAKER,
  SEQUENCE_SHUTDOWN_SPEAKER,
  SEQUENCE_SHUTDOWN_POWER
};

class RelayController {
private:
  int zasilaniePin;
  int glosnikPin;
  ConfigManager* config;
  ConsoleLogger* logger;
  SequenceState currentSequence;
  unsigned long sequenceStartTime;
  bool relaysActive;

public:
  RelayController();
  void init(int zasilaniePin, int glosnikPin, ConfigManager* config, ConsoleLogger* logger);
  void startupSequence();
  void shutdownSequence();
  void handleSequences();
  bool isActive() { return relaysActive || currentSequence == SEQUENCE_STARTUP_POWER; }
  bool isIdle() { return currentSequence == SEQUENCE_IDLE; }
  bool isStarting() { return currentSequence == SEQUENCE_STARTUP_POWER; }
  bool isStopping() { return currentSequence == SEQUENCE_SHUTDOWN_SPEAKER; }
  String getStatusText();
  String getStatusClass();
};

#endif
