#include "SensorManager.h"

SensorManager::SensorManager() : filteredValue(0.0), alpha(0.1) {
}

void SensorManager::init(DallasTemperature* sensors, int audioPin, int batteryPin) {
  this->sensors = sensors;
  this->audioPin = audioPin;
  this->batteryPin = batteryPin;
}

bool SensorManager::readAudio(float threshold, ConsoleLogger* logger, bool uartActive) {
  float voltage = (analogRead(audioPin) / 4095.0) - 1;
  filteredValue = alpha * voltage + (1.0 - alpha) * filteredValue;

  if (voltage > fabs(threshold)) {
    if (uartActive) {
      Serial.print(voltage, 3);
      Serial.println("  <--- Wykryto sygnał audio");
    }
    logger->addLog("AUDIO", "info", "Wykryto sygnał audio: " + String(voltage, 3) + "V");
  }

  delay(1);
  return (filteredValue >= fabs(threshold));
}

bool SensorManager::readBattery(float threshold) {
  float adc = analogRead(batteryPin);
  float napiecie = ((adc) / 4095.0) * 3.3 * (47 + 12) / 12;
  
  if (napiecie >= threshold) {
    return true;
  } else {
    // W wersji produkcyjnej zmień na false
    return true;
  }
}
