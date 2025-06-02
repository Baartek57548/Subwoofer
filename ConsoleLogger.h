#ifndef CONSOLE_LOGGER_H
#define CONSOLE_LOGGER_H

#include <Arduino.h>

#define MAX_LOGS 20

struct ConsoleLog {
  unsigned long timestamp;
  String operation;
  String status;  // "success", "warning", "error", "info"
  String message;
};

class ConsoleLogger {
private:
  ConsoleLog logs[MAX_LOGS];
  int logIndex;
  int logCount;

public:
  ConsoleLogger();
  void init();
  void addLog(String operation, String status, String message);
  String getLogsAsJson();
  int getLogCount() { return logCount; }
  ConsoleLog* getLogs() { return logs; }
};

#endif
