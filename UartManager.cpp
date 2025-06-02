#include "UartManager.h"

UartManager::UartManager() : active(true), startTime(0) {
}

void UartManager::init(Stream* serial) {
  this->serial = serial;
  this->startTime = millis();
}

void UartManager::activate() {
  active = true;
  Serial.begin(115200);
  startTime = millis();
}

void UartManager::checkTimeout() {
  if (active && (millis() - startTime > UART_TIMEOUT)) {
    // For ESP32, we'll just disable UART functionality after timeout
    // without trying to check serial connection status
    active = false;
    // Note: We don't call serial->end() as Stream doesn't have this method
  }
}

void UartManager::parseCommands(ConfigManager* config) {
  static String linia = "";
  while (serial->available()) {
    char c = serial->read();
    if (c == '\n' || c == '\r') {
      linia.trim();
      if (linia.startsWith("czas=")) {
        config->setCzasPoSyg(linia.substring(5).toInt());
        config->showSettings();
      } else if (linia.startsWith("napiecie=")) {
        config->setProgNapiecia(linia.substring(9).toFloat());
        config->showSettings();
      } else if (linia.startsWith("audio=")) {
        config->setAudioThreshold(linia.substring(6).toFloat());
        config->showSettings();
      } else if (linia.startsWith("tmin=")) {
        config->setTempMin(linia.substring(5).toFloat());
        config->showSettings();
      } else if (linia.startsWith("tprzegrz=")) {
        config->setTempPrzegrzania(linia.substring(10).toFloat());
        config->showSettings();
      } else if (linia.startsWith("tmax=")) {
        config->setTempMax(linia.substring(5).toFloat());
        config->showSettings();
      } else if (linia.startsWith("delayrelay=")) {
        config->setDelayRelaySwitch(linia.substring(11).toInt());
        config->showSettings();
      } else if (linia.startsWith("savetemp=")) {
        config->setTempSave(linia.substring(9).toFloat());
        config->showSettings();
      } else if (linia.equalsIgnoreCase("SAVE")) {
        config->saveSettings();
      } else if (linia.equalsIgnoreCase("SHOW")) {
        config->showSettings();
      } else if (linia.equalsIgnoreCase("HELP")) {
        config->showSettings();
      } else if (linia.equalsIgnoreCase("RETURN FABRIC")) {
        config->resetToDefaults();
      } else if (linia.equalsIgnoreCase("RESTART")) {
        ESP.restart();
      }
      serial->println();
      delay(4000);
      showCommands();
      linia = "";
    } else linia += c;
  }
}

void UartManager::showCommands() {
  serial->println();
  serial->println("DOSTEPNE KOMENDY:");
  serial->println("  czas=XX               - czas podtrzymania [s] po sygnale audio");
  serial->println("  napiecie=XX.X         - minimalne napięcie akumulatora [V]");
  serial->println("  audio=X.XXX           - próg detekcji sygnału audio");
  serial->println("  delayrelay=XXXX       - opóźnienie przekaźników [ms]");
  serial->println();
  serial->println("  tmin=XX.X             - temperatura startu wentylatora [C]");
  serial->println("  tprzegrz=XX.X         - temperatura ostrzegawcza [C]");
  serial->println("  tmax=XX.X             - temperatura krytyczna [C]");
  serial->println("  savetemp=XX.X         - temperatura zakończenia chłodzenia [C]");
  serial->println();
  serial->println("  SAVE                  - zapisuje ustawienia do EEPROM");
  serial->println("  SHOW/HELP             - pokazuje zapisane ustawienia");
  serial->println("  RETURN FABRIC         - wczytuje domyślne ustawienia");
  serial->println("  RESTART               - restartuje urządzenie");
  serial->println();
}
