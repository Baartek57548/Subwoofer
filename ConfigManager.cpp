#include "ConfigManager.h"

ConfigManager::ConfigManager() : 
  czasPoSyg(30),
  progNapiecia(11.5),
  audioThreshold(1.000),
  tempMin(35.0),
  tempPrzegrzania(60.0),
  tempMax(50.0),
  delayRelaySwitch(4000),
  tempSave(45.0) {
}

void ConfigManager::init(EEPROMClass* eeprom, ConsoleLogger* logger) {
  this->eeprom = eeprom;
  this->logger = logger;
}

void ConfigManager::loadSettings() {
  EEPROM.get(EEPROM_ADR_CZAS, czasPoSyg);
  EEPROM.get(EEPROM_ADR_NAPIECIE, progNapiecia);
  EEPROM.get(EEPROM_ADR_AUDIO, audioThreshold);
  EEPROM.get(EEPROM_ADR_TMIN, tempMin);
  EEPROM.get(EEPROM_ADR_TPRZEGRZ, tempPrzegrzania);
  EEPROM.get(EEPROM_ADR_TMAX, tempMax);
  EEPROM.get(EEPROM_ADR_DELAY_RELAY, delayRelaySwitch);
  EEPROM.get(EEPROM_ADR_SAVETEMP, tempSave);

  // Walidacja wartości
  if (czasPoSyg == 0xFFFFFFFF || czasPoSyg < 5 || czasPoSyg > 600) czasPoSyg = 30;
  if (progNapiecia < 11.0 || progNapiecia > 15.0) progNapiecia = 11.5;
  if (audioThreshold < 0.1 || audioThreshold > 3.0) audioThreshold = 1.0;
  if (isnan(tempMin) || tempMin < 30.0 || tempMin > 70.0) tempMin = 35.0;
  if (isnan(tempPrzegrzania) || tempPrzegrzania < 40.0 || tempPrzegrzania > 85.0) tempPrzegrzania = 60.0;
  if (isnan(tempMax) || tempMax < 50.0 || tempMax > 100.0) tempMax = 80.0;
  if (isnan(tempSave) || tempSave < 30.0 || tempSave > 70.0) tempSave = 45.0;
  if (delayRelaySwitch < 100 || delayRelaySwitch > 10000) delayRelaySwitch = 4000;
  
  logger->addLog("CONFIG", "success", "Ustawienia wczytane z EEPROM");
}

void ConfigManager::saveSettings() {
  EEPROM.put(EEPROM_ADR_CZAS, czasPoSyg);
  EEPROM.put(EEPROM_ADR_NAPIECIE, progNapiecia);
  EEPROM.put(EEPROM_ADR_AUDIO, audioThreshold);
  EEPROM.put(EEPROM_ADR_TMIN, tempMin);
  EEPROM.put(EEPROM_ADR_TPRZEGRZ, tempPrzegrzania);
  EEPROM.put(EEPROM_ADR_TMAX, tempMax);
  EEPROM.put(EEPROM_ADR_DELAY_RELAY, delayRelaySwitch);
  EEPROM.put(EEPROM_ADR_SAVETEMP, tempSave);
  EEPROM.commit();
  
  Serial.println("Ustawienia zapisane do EEPROM.");
  logger->addLog("SAVE", "success", "Ustawienia zapisane do EEPROM");
}

void ConfigManager::resetToDefaults() {
  czasPoSyg = 60;
  progNapiecia = 12.0;
  audioThreshold = 1.000;
  tempMin = 35.0;
  tempPrzegrzania = 60.0;
  tempMax = 50.0;
  delayRelaySwitch = 4000;
  tempSave = 45.0;
  
  Serial.println("Ustawiono wartości domyślne.");
  logger->addLog("FACTORY RESET", "warning", "Przywrócono ustawienia fabryczne");
}

void ConfigManager::showSettings() {
  Serial.println("Zapisane ustawienia:");
  Serial.print("  czasPoSyg: ");
  Serial.print(czasPoSyg);
  Serial.println("s.");
  Serial.print("  progNapiecia: ");
  Serial.print(progNapiecia);
  Serial.println(" V.");
  Serial.print("  audioThreshold: ");
  Serial.print(audioThreshold);
  Serial.println(" V.");
  Serial.print("  delayRelaySwitch: ");
  Serial.print(delayRelaySwitch);
  Serial.println("ms.");
  Serial.println();
  Serial.print("  tempMin: ");
  Serial.print(tempMin);
  Serial.println(" *C");
  Serial.print("  tempPrzegrzania: ");
  Serial.print(tempPrzegrzania);
  Serial.println(" *C");
  Serial.print("  tempMax: ");
  Serial.print(tempMax);
  Serial.println(" *C");
  Serial.print("  tempSave: ");
  Serial.print(tempSave);
  Serial.println(" *C");
  Serial.println();
}
