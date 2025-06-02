#include "ConsoleLogger.h"
#include <ArduinoJson.h>

ConsoleLogger::ConsoleLogger() : logIndex(0), logCount(0) {
}

void ConsoleLogger::init() {
  for (int i = 0; i < MAX_LOGS; i++) {
    logs[i].timestamp = 0;
    logs[i].operation = "";
    logs[i].status = "";
    logs[i].message = "";
  }
}

void ConsoleLogger::addLog(String operation, String status, String message) {
  logs[logIndex].timestamp = millis();
  logs[logIndex].operation = operation;
  logs[logIndex].status = status;
  logs[logIndex].message = message;
  
  logIndex = (logIndex + 1) % MAX_LOGS;
  if (logCount < MAX_LOGS) logCount++;
  
  // Also print to Serial if active
  Serial.print("[");
  Serial.print(millis() / 1000);
  Serial.print("s] ");
  Serial.print(operation);
  Serial.print(" (");
  Serial.print(status);
  Serial.print("): ");
  Serial.println(message);
}

String ConsoleLogger::getLogsAsJson() {
  DynamicJsonDocument doc(4096);
  JsonArray logsArray = doc.createNestedArray("logs");
  
  int currentIndex = logIndex;
  for (int i = 0; i < logCount; i++) {
    currentIndex = (currentIndex - 1 + MAX_LOGS) % MAX_LOGS;
    JsonObject log = logsArray.createNestedObject();
    
    unsigned long seconds = logs[currentIndex].timestamp / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    char timeStr[10];
    sprintf(timeStr, "%02lu:%02lu:%02lu", hours % 24, minutes % 60, seconds % 60);
    
    log["timestamp"] = timeStr;
    log["operation"] = logs[currentIndex].operation;
    log["status"] = logs[currentIndex].status;
    log["message"] = logs[currentIndex].message;
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}
