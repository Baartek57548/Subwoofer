/*
 * SubwooferController.ino
 * ESP32C3 sterownik Subwoofera - pomiar audio, napięcia akumulatora, temperatury, przekaźników
 */

#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include "ConsoleLogger.h"
#include "ConfigManager.h"
#include "SensorManager.h"
#include "RelayController.h"
#include "SubwooferWebServer.h"
#include "UartManager.h"

// Piny
#define WENTYLATOR_PIN 0  // GPIO0
#define GLOSNIK_PIN 10    // GPIO10
#define ZASILANIE_PIN 9   // GPIO9
#define AUDIO_SIG A2      // GPIO1
#define BATT_SIG A3       // GPIO3
#define LED_PIN 8         // GPIO8
#define PRZYCISK_PIN 4    // GPIO4
#define ONE_WIRE_BUS 2    // GPIO2

// Czujnik temperatury
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Instancje klas
ConsoleLogger logger;
ConfigManager configManager;
SensorManager sensorManager;
RelayController relayController;
SubwooferWebServer webServer;
UartManager uartManager;

// Zmienne globalne
unsigned long lastAudioDetected = 0;
bool przyciskTrzymany = false;
unsigned long przyciskStart = 0;

void setup() {
  // Konfiguracja pinów
  pinMode(LED_PIN, OUTPUT);
  pinMode(WENTYLATOR_PIN, OUTPUT);
  pinMode(GLOSNIK_PIN, OUTPUT);
  pinMode(ZASILANIE_PIN, OUTPUT);
  pinMode(BATT_SIG, INPUT);
  pinMode(AUDIO_SIG, INPUT);
  pinMode(PRZYCISK_PIN, INPUT_PULLUP);

  digitalWrite(ZASILANIE_PIN, LOW);
  digitalWrite(GLOSNIK_PIN, LOW);

  // Inicjalizacja UART
  Serial.begin(115200);
  uartManager.init(&Serial);

  // Inicjalizacja loggera
  logger.init();
  logger.addLog("SYSTEM", "info", "Inicjalizacja systemu...");

  // Inicjalizacja czujników
  sensors.begin();
  delay(500);
  sensors.requestTemperatures();
  sensorManager.init(&sensors, AUDIO_SIG, BATT_SIG);

  // Inicjalizacja EEPROM
  EEPROM.begin(64);
  configManager.init(&EEPROM, &logger);
  configManager.loadSettings();

  // Inicjalizacja kontrolera przekaźników
  relayController.init(ZASILANIE_PIN, GLOSNIK_PIN, &configManager, &logger);

  // Inicjalizacja serwera WWW - przekazujemy pin baterii
  webServer.init(&configManager, &logger, &relayController, &sensorManager, BATT_SIG);

  delay(500);

  // Wyświetl informacje startowe
  Serial.println("\n\n\n\n");
  Serial.println("ESP32C3 sterownik Subwoofera - pomiar audio, napiecia akumulatora, temperatury, przekaźników.");
  Serial.print("Nazwa sieci: ");
  Serial.print(webServer.getNazwaWifi());
  Serial.print("\t");
  Serial.print("Hasło: ");
  Serial.print(webServer.getHasloWifi());
  Serial.print("\t");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  uartManager.showCommands();
  
  logger.addLog("SYSTEM", "success", "System gotowy do pracy");

  // Inicjalizacja PWM dla wentylatora
  ledcAttach(LED_PIN, 5000, 8);
  ledcWrite(LED_PIN, 200);
}

void loop() {
  // Obsługa sekwencji przekaźników
  relayController.handleSequences();
  
  // Obsługa serwera WWW
  webServer.handleClient();
  
  // Obsługa UART
  uartManager.checkTimeout();
  if (uartManager.isActive()) {
    uartManager.parseCommands(&configManager);
  }

  // Odczyt czujników
  bool napiecieOk = sensorManager.readBattery(configManager.getProgNapiecia());
  bool audioDetected = sensorManager.readAudio(configManager.getAudioThreshold(), &logger, uartManager.isActive());

  unsigned long currentTime = millis();

  // Logika sterowania
  if (napiecieOk) {
    if (audioDetected) {
      lastAudioDetected = currentTime;
      if (!relayController.isActive() && relayController.isIdle()) {
        relayController.startupSequence();
      }
    }

    if (relayController.isActive() && relayController.isIdle() && 
        (currentTime - lastAudioDetected >= configManager.getCzasPoSyg() * 1000UL)) {
      if (uartManager.isActive()) Serial.println("Brak aktywności – wyłączanie.");
      logger.addLog("TIMEOUT", "info", "Brak aktywności przez " + String(configManager.getCzasPoSyg()) + "s");
      relayController.shutdownSequence();
      Serial.println();
      uartManager.showCommands();
    }

    if (relayController.isActive()) {
      // Obsługa temperatury
      sensors.requestTemperatures();
      float temp = sensors.getTempCByIndex(0);

      if (temp != DEVICE_DISCONNECTED_C) {
        if (uartManager.isActive()) {
          Serial.print("Temp: ");
          Serial.println(temp);
        }

        // Sterowanie wentylatorem
        int pwm = constrain(map(temp, configManager.getTempMin(), configManager.getTempMax(), 0, 255), 0, 255);
        ledcWrite(WENTYLATOR_PIN, pwm);

        // Ostrzeżenia temperaturowe
        if (temp >= configManager.getTempPrzegrzania() && temp < configManager.getTempMax()) {
          logger.addLog("TEMPERATURE", "warning", "Temperatura ostrzegawcza: " + String(temp, 1) + "°C");
        }

        // Temperatura krytyczna
        if (temp >= configManager.getTempMax()) {
          if (uartManager.isActive()) Serial.println("Temp krytyczna – chłodzenie");
          logger.addLog("TEMPERATURE", "error", "Temperatura krytyczna: " + String(temp, 1) + "°C - wymuszenie chłodzenia");
          relayController.shutdownSequence();
          
          // Chłodzenie awaryjne
          while (temp >= configManager.getTempSave()) {
            sensors.requestTemperatures();
            temp = sensors.getTempCByIndex(0);
            ledcWrite(WENTYLATOR_PIN, 255);
            delay(200);
          }
          logger.addLog("TEMPERATURE", "success", "Chłodzenie zakończone - temp: " + String(temp, 1) + "°C");
        }
      } else if (uartManager.isActive()) {
        static int i = 0;
        if (i == 0) {
          Serial.println("Błąd odczytu temp.");
          logger.addLog("TEMPERATURE", "error", "Błąd odczytu czujnika temperatury");
          i = 1;
        }
      }
    }
  } else {
    // Zbyt niskie napięcie
    if (relayController.isActive()) {
      if (uartManager.isActive()) Serial.println("Zbyt niskie napięcie – wyłączam.");
      relayController.shutdownSequence();
    }
  }

  // Obsługa krótkiego i długiego naciśnięcia przycisku
  bool przyciskAktualny = (digitalRead(PRZYCISK_PIN) == LOW);
  unsigned long teraz = millis();

  // Obsługa wciśnięcia przycisku
  if (przyciskAktualny && !przyciskTrzymany) {
    // Początek naciśnięcia przycisku
    przyciskStart = teraz;
    przyciskTrzymany = true;
  } 
  // Obsługa długiego przytrzymania (4 sekundy)
  else if (przyciskAktualny && przyciskTrzymany && (teraz - przyciskStart >= 4000)) {
    Serial.println("Przycisk przytrzymany 4s – ponowne uruchomienie UART i WiFi");
    logger.addLog("BUTTON", "info", "Przycisk przytrzymany 4s - restart serwisów");

    // Restart obsługi UART
    uartManager.activate();
    logger.addLog("UART", "success", "UART ponownie aktywowany");

    // Restart obsługi WiFi
    webServer.activate();
    logger.addLog("WIFI", "success", "WiFi AP ponownie aktywowany");

    for (int i = 0; i < 10; i++) {
      analogRead(AUDIO_SIG);
      delay(20);
    }

    // Reset stanu przycisku po wykonaniu akcji
    przyciskTrzymany = false;
  } 
  // Obsługa zwolnienia przycisku (krótkie naciśnięcie)
  else if (!przyciskAktualny && przyciskTrzymany) {
    // Przycisk został zwolniony
    if (teraz - przyciskStart < 1000) {  // Krótsze niż 1 sekunda = kliknięcie
      Serial.println("Przycisk kliknięty - uruchamiam sekwencję");
      logger.addLog("BUTTON", "info", "Przycisk kliknięty - uruchomienie sekwencji");
      
      // Uruchomienie sekwencji - restart timera podtrzymania
      lastAudioDetected = teraz;
      
      // Uruchomienie sekwencji jeśli system jest nieaktywny
      if (!relayController.isActive() && relayController.isIdle()) {
        relayController.startupSequence();
      }
    }
    // Reset stanu przycisku
    przyciskTrzymany = false;
  }
  
  delay(10);
}
