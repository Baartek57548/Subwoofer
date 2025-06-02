#include "RelayController.h"

RelayController::RelayController() : 
  currentSequence(SEQUENCE_IDLE),
  sequenceStartTime(0),
  relaysActive(false) {
}

void RelayController::init(int zasilaniePin, int glosnikPin, ConfigManager* config, ConsoleLogger* logger) {
  this->zasilaniePin = zasilaniePin;
  this->glosnikPin = glosnikPin;
  this->config = config;
  this->logger = logger;
}

void RelayController::startupSequence() {
  if (currentSequence == SEQUENCE_IDLE) {
    logger->addLog("STARTUP", "info", "Włączanie przetwornicy...");
    Serial.print("Startup: Włączanie przetwornicy, a po ");
    Serial.print(config->getDelayRelaySwitch() / 1000);
    Serial.println("s głośnika.");
    digitalWrite(zasilaniePin, HIGH);
    currentSequence = SEQUENCE_STARTUP_POWER;
    sequenceStartTime = millis();
  }
}

void RelayController::shutdownSequence() {
  if ((currentSequence == SEQUENCE_IDLE && relaysActive) || currentSequence == SEQUENCE_STARTUP_POWER) {
    logger->addLog("SHUTDOWN", "info", "Rozpoczęcie sekwencji wyłączania...");
    Serial.print("Shutdown: Wyłączanie głośnika, a po ");
    Serial.print(config->getDelayRelaySwitch() / 1000);
    Serial.println("s przetwornicy.");
    
    // Jeśli byliśmy w trakcie uruchamiania, wyłącz głośnik jeśli był włączony
    if (relaysActive) {
      digitalWrite(glosnikPin, LOW);
    }
    
    currentSequence = SEQUENCE_SHUTDOWN_SPEAKER;
    sequenceStartTime = millis();
  }
}

void RelayController::handleSequences() {
  if (currentSequence == SEQUENCE_IDLE) {
    return;
  }
  
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - sequenceStartTime;
  
  switch (currentSequence) {
    case SEQUENCE_STARTUP_POWER:
      if (elapsedTime >= config->getDelayRelaySwitch()) {
        digitalWrite(glosnikPin, HIGH);
        relaysActive = true;
        logger->addLog("STARTUP", "success", "Sekwencja uruchomienia zakończona - system aktywny");
        currentSequence = SEQUENCE_IDLE;
      }
      break;
      
    case SEQUENCE_SHUTDOWN_SPEAKER:
      if (elapsedTime >= config->getDelayRelaySwitch()) {
        digitalWrite(zasilaniePin, LOW);
        relaysActive = false;
        logger->addLog("SHUTDOWN", "success", "System wyłączony - przekaźniki nieaktywne");
        currentSequence = SEQUENCE_IDLE;
      }
      break;
      
    default:
      currentSequence = SEQUENCE_IDLE;
      break;
  }
}

String RelayController::getStatusText() {
  switch (currentSequence) {
    case SEQUENCE_STARTUP_POWER:
      return "STARTING";
    case SEQUENCE_SHUTDOWN_SPEAKER:
      return "STOPPING";
    case SEQUENCE_IDLE:
      return relaysActive ? "ACTIVE" : "OFF";
    default:
      return "OFF";
  }
}

String RelayController::getStatusClass() {
  switch (currentSequence) {
    case SEQUENCE_STARTUP_POWER:
      return "value-info";
    case SEQUENCE_SHUTDOWN_SPEAKER:
      return "value-warning";
    case SEQUENCE_IDLE:
      return relaysActive ? "value-success" : "value-warning";
    default:
      return "value-warning";
  }
}
