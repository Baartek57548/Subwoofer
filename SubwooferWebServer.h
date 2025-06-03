#ifndef SUBWOOFER_WEB_SERVER_H
#define SUBWOOFER_WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "ConfigManager.h"
#include "ConsoleLogger.h"
#include "RelayController.h"
#include "SensorManager.h"

class SubwooferWebServer {
private:
  WebServer server;
  ConfigManager* config;
  ConsoleLogger* logger;
  RelayController* relayController;
  SensorManager* sensorManager;
  bool active;
  unsigned long startTime;
  int connectedClients;
  int batteryPin;  // Dodane pole dla pinu baterii
  const unsigned long WIFI_TIMEOUT = 120000;  // 2 minuty
  const char* nazwaWifi = "Subwoofer";
  const char* hasloWifi = "Subwoofer321";
  
  void setupRoutes();
  void handleRoot();
  void handleSet();
  void handleTrigger();
  void handleForceShutdown();
  void handleFastData();
  void handleData();
  void handleLogs();
  void handleHelp();
  void handleFactory();
  void handleRestart();
  
  // Dodane metody dla lepszego zarządzania statusem przekaźników
  String getRelayStatusText();
  String getRelayStatusClass();

public:
  SubwooferWebServer();
  void init(ConfigManager* config, ConsoleLogger* logger, RelayController* relayController, SensorManager* sensorManager, int batteryPin);
  void handleClient();
  bool isActive() { return active; }
  void activate();
  const char* getNazwaWifi() { return nazwaWifi; }
  const char* getHasloWifi() { return hasloWifi; }
};

#endif
