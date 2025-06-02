#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DallasTemperature.h>
#include "ConsoleLogger.h"

class SensorManager {
private:
  DallasTemperature* sensors;
  int audioPin;
  int batteryPin;
  float filteredValue;
  float alpha;  // Współczynnik wygładzania

public:
  SensorManager();
  void init(DallasTemperature* sensors, int audioPin, int batteryPin);
  bool readAudio(float threshold, ConsoleLogger* logger, bool uartActive);
  bool readBattery(float threshold);
  float getFilteredAudio() { return filteredValue; }
};

#endif
