# ESP32c3 supermini Subwoofer Controller

Zrefaktoryzowany kod sterownika subwoofera dla ESP32C3 z podziałem na klasy.

## Struktura projektu

\`\`\`
SubwooferController/
├── SubwooferController.ino       // Główny plik szkicu
├── ConsoleLogger.h               // Klasa do logowania
├── ConsoleLogger.cpp
├── ConfigManager.h               // Klasa zarządzania konfiguracją
├── ConfigManager.cpp
├── SensorManager.h               // Klasa obsługi czujników
├── SensorManager.cpp
├── RelayController.h             // Klasa kontroli przekaźników
├── RelayController.cpp
├── SubwooferWebServer.h          // Klasa serwera WWW
├── SubwooferWebServer.cpp
├── UartManager.h                 // Klasa obsługi UART
└── UartManager.cpp
\`\`\`

## Wymagane biblioteki

- ArduinoJson (wersja 6.x)
- OneWire
- DallasTemperature

## Instalacja

1. Pobierz wszystkie pliki z tego folderu
2. Utwórz nowy projekt w Arduino IDE o nazwie "SubwooferController"
3. Skopiuj wszystkie pliki do folderu projektu
4. Zainstaluj wymagane biblioteki przez Library Manager
5. Skompiluj i wgraj na ESP32C3

## Funkcje

- **Monitoring audio** - detekcja sygnału audio z regulowanym progiem
- **Kontrola napięcia** - monitoring napięcia akumulatora
- **Zarządzanie temperaturą** - kontrola wentylatora i ochrona przed przegrzaniem
- **Sterowanie przekaźnikami** - sekwencyjne włączanie/wyłączanie z opóźnieniem
- **Interfejs WWW** - nowoczesny interfejs mobilny z real-time monitoring
- **Konfiguracja UART** - komendy tekstowe do konfiguracji
- **System logowania** - śledzenie wszystkich operacji systemu

## Konfiguracja pinów

- GPIO0: Wentylator (PWM)
- GPIO1: Sygnał audio (ADC)
- GPIO2: Czujnik temperatury (OneWire)
- GPIO3: Napięcie akumulatora (ADC)
- GPIO4: Przycisk
- GPIO8: LED
- GPIO9: Przekaźnik zasilania
- GPIO10: Przekaźnik głośnika

## WiFi Access Point

- Nazwa sieci: "Subwoofer"
- Hasło: "Subwoofer321"
- IP: 192.168.4.1

https://v0.dev/chat/plik1-do-pliku2-UXZPKH8bm6a
