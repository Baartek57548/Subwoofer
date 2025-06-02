#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "ConsoleLogger.h"

// EEPROM adresy
#define EEPROM_ADR_CZAS 0
#define EEPROM_ADR_NAPIECIE 4
#define EEPROM_ADR_AUDIO 8
#define EEPROM_ADR_TMIN 12
#define EEPROM_ADR_TPRZEGRZ 16
#define EEPROM_ADR_TMAX 20
#define EEPROM_ADR_DELAY_RELAY 24
#define EEPROM_ADR_SAVETEMP 28

class ConfigManager {
private:
  EEPROMClass* eeprom;
  ConsoleLogger* logger;
  
  // Parametry konfigurowalne
  unsigned long czasPoSyg;          // sekundy
  float progNapiecia;               // V
  float audioThreshold;             // V
  float tempMin;                    // C
  float tempPrzegrzania;            // C
  float tempMax;                    // C
  unsigned int delayRelaySwitch;    // ms
  float tempSave;                   // C

public:
  ConfigManager();
  void init(EEPROMClass* eeprom, ConsoleLogger* logger);
  void loadSettings();
  void saveSettings();
  void resetToDefaults();
  void showSettings();
  
  // Gettery
  unsigned long getCzasPoSyg() { return czasPoSyg; }
  float getProgNapiecia() { return progNapiecia; }
  float getAudioThreshold() { return audioThreshold; }
  float getTempMin() { return tempMin; }
  float getTempPrzegrzania() { return tempPrzegrzania; }
  float getTempMax() { return tempMax; }
  unsigned int getDelayRelaySwitch() { return delayRelaySwitch; }
  float getTempSave() { return tempSave; }
  
  // Settery
  void setCzasPoSyg(unsigned long val) { czasPoSyg = val; }
  void setProgNapiecia(float val) { progNapiecia = val; }
  void setAudioThreshold(float val) { audioThreshold = val; }
  void setTempMin(float val) { tempMin = val; }
  void setTempPrzegrzania(float val) { tempPrzegrzania = val; }
  void setTempMax(float val) { tempMax = val; }
  void setDelayRelaySwitch(unsigned int val) { delayRelaySwitch = val; }
  void setTempSave(float val) { tempSave = val; }
};

#endif
