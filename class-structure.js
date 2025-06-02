// This is a JavaScript representation of how the C++ code could be organized into classes

// Class for console logging system
class ConsoleLogger {
  constructor(maxLogs = 20) {
    this.logs = Array(maxLogs).fill().map(() => ({
      timestamp: 0,
      operation: "",
      status: "",
      message: ""
    }));
    this.logIndex = 0;
    this.logCount = 0;
  }

  addLog(operation, status, message) {
    this.logs[this.logIndex] = {
      timestamp: Date.now(), // In real code: millis()
      operation,
      status,
      message
    };
    
    this.logIndex = (this.logIndex + 1) % this.logs.length;
    if (this.logCount < this.logs.length) this.logCount++;
    
    // Print to Serial if UART is active
    console.log(`[${Math.floor(Date.now()/1000)}s] ${operation} (${status}): ${message}`);
  }

  getLogsAsJson() {
    const logs = [];
    let currentIndex = this.logIndex;
    
    for (let i = 0; i < this.logCount; i++) {
      currentIndex = (currentIndex - 1 + this.logs.length) % this.logs.length;
      const log = this.logs[currentIndex];
      
      const seconds = Math.floor(log.timestamp / 1000);
      const minutes = Math.floor(seconds / 60);
      const hours = Math.floor(minutes / 60);
      
      const timeStr = `${String(hours % 24).padStart(2, '0')}:${String(minutes % 60).padStart(2, '0')}:${String(seconds % 60).padStart(2, '0')}`;
      
      logs.push({
        timestamp: timeStr,
        operation: log.operation,
        status: log.status,
        message: log.message
      });
    }
    
    return { logs };
  }
}

// Class for configuration management
class ConfigManager {
  constructor(eeprom) {
    this.eeprom = eeprom;
    
    // Default values
    this.czasPoSyg = 30;          // seconds
    this.progNapiecia = 11.5;     // V
    this.audioThreshold = 1.000;  // V
    this.tempMin = 35.0;          // C
    this.tempPrzegrzania = 60.0;  // C
    this.tempMax = 50.0;          // C
    this.delayRelaySwitch = 4000; // ms
    this.tempSave = 45.0;         // C
    
    // EEPROM addresses
    this.EEPROM_ADR_CZAS = 0;
    this.EEPROM_ADR_NAPIECIE = 4;
    this.EEPROM_ADR_AUDIO = 8;
    this.EEPROM_ADR_TMIN = 12;
    this.EEPROM_ADR_TPRZEGRZ = 16;
    this.EEPROM_ADR_TMAX = 20;
    this.EEPROM_ADR_DELAY_RELAY = 24;
    this.EEPROM_ADR_SAVETEMP = 28;
  }
  
  loadSettings(logger) {
    // In real code: EEPROM.get() calls
    this.czasPoSyg = this.eeprom.get(this.EEPROM_ADR_CZAS) || 30;
    this.progNapiecia = this.eeprom.get(this.EEPROM_ADR_NAPIECIE) || 11.5;
    this.audioThreshold = this.eeprom.get(this.EEPROM_ADR_AUDIO) || 1.0;
    this.tempMin = this.eeprom.get(this.EEPROM_ADR_TMIN) || 35.0;
    this.tempPrzegrzania = this.eeprom.get(this.EEPROM_ADR_TPRZEGRZ) || 60.0;
    this.tempMax = this.eeprom.get(this.EEPROM_ADR_TMAX) || 50.0;
    this.tempSave = this.eeprom.get(this.EEPROM_ADR_SAVETEMP) || 45.0;
    this.delayRelaySwitch = this.eeprom.get(this.EEPROM_ADR_DELAY_RELAY) || 4000;
    
    // Validate values
    if (this.czasPoSyg === 0xFFFFFFFF || this.czasPoSyg < 5 || this.czasPoSyg > 600) this.czasPoSyg = 30;
    if (this.progNapiecia < 11.0 || this.progNapiecia > 15.0) this.progNapiecia = 11.5;
    if (this.audioThreshold < 0.1 || this.audioThreshold > 3.0) this.audioThreshold = 1.0;
    if (isNaN(this.tempMin) || this.tempMin < 30.0 || this.tempMin > 70.0) this.tempMin = 35.0;
    if (isNaN(this.tempPrzegrzania) || this.tempPrzegrzania < 40.0 || this.tempPrzegrzania > 85.0) this.tempPrzegrzania = 60.0;
    if (isNaN(this.tempMax) || this.tempMax < 50.0 || this.tempMax > 100.0) this.tempMax = 80.0;
    if (isNaN(this.tempSave) || this.tempSave < 30.0 || this.tempSave > 70.0) this.tempSave = 45.0;
    if (this.delayRelaySwitch < 100 || this.delayRelaySwitch > 10000) this.delayRelaySwitch = 4000;
    
    logger.addLog("CONFIG", "success", "Ustawienia wczytane z EEPROM");
  }
  
  saveSettings(logger) {
    // In real code: EEPROM.put() calls
    this.eeprom.put(this.EEPROM_ADR_CZAS, this.czasPoSyg);
    this.eeprom.put(this.EEPROM_ADR_NAPIECIE, this.progNapiecia);
    this.eeprom.put(this.EEPROM_ADR_AUDIO, this.audioThreshold);
    this.eeprom.put(this.EEPROM_ADR_TMIN, this.tempMin);
    this.eeprom.put(this.EEPROM_ADR_TPRZEGRZ, this.tempPrzegrzania);
    this.eeprom.put(this.EEPROM_ADR_TMAX, this.tempMax);
    this.eeprom.put(this.EEPROM_ADR_DELAY_RELAY, this.delayRelaySwitch);
    this.eeprom.put(this.EEPROM_ADR_SAVETEMP, this.tempSave);
    this.eeprom.commit();
    
    console.log("Ustawienia zapisane do EEPROM.");
    logger.addLog("SAVE", "success", "Ustawienia zapisane do EEPROM");
  }
  
  resetToDefaults(logger) {
    this.czasPoSyg = 60;
    this.progNapiecia = 12.0;
    this.audioThreshold = 1.000;
    this.tempMin = 35.0;
    this.tempPrzegrzania = 60.0;
    this.tempMax = 50.0;
    this.delayRelaySwitch = 4000;
    this.tempSave = 45.0;
    
    console.log("Ustawiono wartości domyślne.");
    logger.addLog("FACTORY RESET", "warning", "Przywrócono ustawienia fabryczne");
  }
  
  showSettings() {
    console.log("Zapisane ustawienia:");
    console.log(`  czasPoSyg: ${this.czasPoSyg}s.`);
    console.log(`  progNapiecia: ${this.progNapiecia} V.`);
    console.log(`  audioThreshold: ${this.audioThreshold} V.`);
    console.log(`  delayRelaySwitch: ${this.delayRelaySwitch}ms.`);
    console.log();
    console.log(`  tempMin: ${this.tempMin} *C`);
    console.log(`  tempPrzegrzania: ${this.tempPrzegrzania} *C`);
    console.log(`  tempMax: ${this.tempMax} *C`);
    console.log(`  tempSave: ${this.tempSave} *C`);
    console.log();
  }
}

// Class for hardware sensors
class SensorManager {
  constructor(config) {
    this.config = config;
    this.filteredAudioValue = 0.0;
    this.alpha = 0.1; // Smoothing factor
  }
  
  readAudio(audioPin, logger) {
    // In real code: analogRead(AUDIO_SIG)
    const voltage = (Math.random() * 0.5) - 0.25; // Simulate audio reading
    this.filteredAudioValue = this.alpha * voltage + (1.0 - this.alpha) * this.filteredAudioValue;
    
    if (Math.abs(voltage) > Math.abs(this.config.audioThreshold)) {
      console.log(`${voltage.toFixed(3)}  <--- Wykryto sygnał audio`);
      logger.addLog("AUDIO", "info", `Wykryto sygnał audio: ${voltage.toFixed(3)}V`);
    }
    
    return (Math.abs(this.filteredAudioValue) >= Math.abs(this.config.audioThreshold));
  }
  
  readBattery(batteryPin) {
    // In real code: analogRead(BATT_SIG)
    const adc = Math.random() * 4095; // Simulate battery reading
    const voltage = ((adc) / 4095.0) * 3.3 * (47 + 12) / 12;
    
    return voltage >= this.config.progNapiecia;
  }
  
  readTemperature(sensors) {
    // In real code: sensors.requestTemperatures() and sensors.getTempCByIndex(0)
    return 40 + Math.random() * 10; // Simulate temperature between 40-50°C
  }
}

// Class for relay control
class RelayController {
  constructor(config, logger) {
    this.config = config;
    this.logger = logger;
    this.relaysActive = false;
    this.currentSequence = "SEQUENCE_IDLE";
    this.sequenceStartTime = 0;
    
    // Pin definitions (would be used in real code)
    this.ZASILANIE_PIN = 9;
    this.GLOSNIK_PIN = 10;
  }
  
  startupSequence() {
    if (this.currentSequence === "SEQUENCE_IDLE") {
      this.logger.addLog("STARTUP", "info", "Włączanie przetwornicy...");
      console.log(`Startup: Włączanie przetwornicy, a po ${this.config.delayRelaySwitch / 1000}s głośnika.`);
      
      // In real code: digitalWrite(ZASILANIE_PIN, HIGH)
      this.currentSequence = "SEQUENCE_STARTUP_POWER";
      this.sequenceStartTime = Date.now();
    }
  }
  
  shutdownSequence() {
    if (this.currentSequence === "SEQUENCE_IDLE" && this.relaysActive) {
      this.logger.addLog("SHUTDOWN", "info", "Rozpoczęcie sekwencji wyłączania...");
      console.log(`Shutdown: Wyłączanie głośnika, a po ${this.config.delayRelaySwitch / 1000}s przetwornicy.`);
      
      // In real code: digitalWrite(GLOSNIK_PIN, LOW)
      this.currentSequence = "SEQUENCE_SHUTDOWN_SPEAKER";
      this.sequenceStartTime = Date.now();
    }
  }
  
  handleSequences() {
    if (this.currentSequence === "SEQUENCE_IDLE") {
      return;
    }
    
    const currentTime = Date.now();
    const elapsedTime = currentTime - this.sequenceStartTime;
    
    switch (this.currentSequence) {
      case "SEQUENCE_STARTUP_POWER":
        if (elapsedTime >= this.config.delayRelaySwitch) {
          // In real code: digitalWrite(GLOSNIK_PIN, HIGH)
          this.relaysActive = true;
          this.logger.addLog("STARTUP", "success", "Sekwencja uruchomienia zakończona - system aktywny");
          this.currentSequence = "SEQUENCE_IDLE";
        }
        break;
        
      case "SEQUENCE_SHUTDOWN_SPEAKER":
        if (elapsedTime >= this.config.delayRelaySwitch) {
          // In real code: digitalWrite(ZASILANIE_PIN, LOW)
          this.relaysActive = false;
          this.logger.addLog("SHUTDOWN", "success", "System wyłączony - przekaźniki nieaktywne");
          this.currentSequence = "SEQUENCE_IDLE";
        }
        break;
        
      default:
        this.currentSequence = "SEQUENCE_IDLE";
        break;
    }
  }
}

// Class for web server
class WebServer {
  constructor(config, logger, relayController) {
    this.config = config;
    this.logger = logger;
    this.relayController = relayController;
    this.active = true;
    this.startTime = Date.now();
    this.connectedClients = 0;
    this.WIFI_TIMEOUT = 120000; // 2 minutes
  }
  
  start() {
    // In real code: WiFi.softAP and server.begin()
    this.logger.addLog("WIFI", "success", "Access Point aktywny - IP: 192.168.4.1");
    console.log("Web server started");
  }
  
  handleClient() {
    // In real code: server.handleClient()
    
    // Check for timeout
    if (this.connectedClients === 0 && (Date.now() - this.startTime > this.WIFI_TIMEOUT)) {
      console.log("Web server timeout – brak klientów, wyłączam AP.");
      this.logger.addLog("WIFI", "warning", "Timeout - wyłączanie Access Point");
      // In real code: server.stop() and WiFi.softAPdisconnect(true)
      this.active = false;
    }
  }
}

// Class for UART command parser
class UartManager {
  constructor(config, logger) {
    this.config = config;
    this.logger = logger;
    this.active = true;
    this.startTime = Date.now();
    this.UART_TIMEOUT = 120000; // 2 minutes
  }
  
  checkTimeout() {
    if (this.active && (Date.now() - this.startTime > this.UART_TIMEOUT)) {
      // In real code: check if Serial is connected
      const serialConnected = false;
      
      if (!serialConnected) {
        this.active = false;
        // In real code: Serial.end()
        this.logger.addLog("UART", "info", "UART wyłączony - timeout");
      }
    }
  }
  
  parseCommands() {
    // In real code: read from Serial
    // This would handle all the command parsing from the original code
  }
  
  showCommands() {
    console.log();
    console.log("DOSTEPNE KOMENDY:");
    console.log("  czas=XX               - czas podtrzymania [s] po sygnale audio");
    console.log("  napiecie=XX.X         - minimalne napięcie akumulatora [V]");
    console.log("  audio=X.XXX           - próg detekcji sygnału audio");
    console.log("  delayrelay=XXXX       - opóźnienie przekaźników [ms]");
    console.log();
    console.log("  tmin=XX.X             - temperatura startu wentylatora [C]");
    console.log("  tprzegrz=XX.X         - temperatura ostrzegawcza [C]");
    console.log("  tmax=XX.X             - temperatura krytyczna [C]");
    console.log("  savetemp=XX.X         - temperatura zakończenia chłodzenia [C]");
    console.log();
    console.log("  SAVE                  - zapisuje ustawienia do EEPROM");
    console.log("  SHOW/HELP             - pokazuje zapisane ustawienia");
    console.log("  RETURN FABRIC         - wczytuje domyślne ustawienia");
    console.log("  RESTART               - restartuje urządzenie");
    console.log();
  }
}

// Main controller class
class SubwooferController {
  constructor() {
    // Mock EEPROM for demonstration
    this.eeprom = {
      data: {},
      get: function(addr) { return this.data[addr]; },
      put: function(addr, val) { this.data[addr] = val; },
      commit: function() { console.log("EEPROM committed"); }
    };
    
    this.logger = new ConsoleLogger();
    this.config = new ConfigManager(this.eeprom);
    this.sensors = new SensorManager(this.config);
    this.relayController = new RelayController(this.config, this.logger);
    this.webServer = new WebServer(this.config, this.logger, this.relayController);
    this.uartManager = new UartManager(this.config, this.logger);
    
    this.lastAudioDetected = 0;
    this.buttonPressed = false;
    this.buttonHoldStart = 0;
    this.buttonHolding = false;
  }
  
  setup() {
    // Initialize pins
    // In real code: pinMode() and digitalWrite() calls
    
    // Initialize serial
    // In real code: Serial.begin(115200)
    
    this.logger.addLog("SYSTEM", "info", "Inicjalizacja systemu...");
    
    // Initialize temperature sensors
    // In real code: sensors.begin()
    
    // Initialize EEPROM
    // In real code: EEPROM.begin(64)
    
    // Start WiFi AP and web server
    this.webServer.start();
    
    // Load settings
    this.config.loadSettings(this.logger);
    
    this.logger.addLog("SYSTEM", "success", "System gotowy do pracy");
  }
  
  loop() {
    // Handle relay sequences
    this.relayController.handleSequences();
    
    // Handle web server
    if (this.webServer.active) {
      this.webServer.handleClient();
    }
    
    // Handle UART timeout
    this.uartManager.checkTimeout();
    
    // Handle UART commands
    if (this.uartManager.active) {
      this.uartManager.parseCommands();
    }
    
    // Read sensors
    const batteryOk = this.sensors.readBattery();
    const buttonPressed = false; // In real code: digitalRead(PRZYCISK_PIN) == LOW
    const audioDetected = this.sensors.readAudio();
    
    const currentTime = Date.now();
    
    if (batteryOk) {
      if (audioDetected || buttonPressed) {
        this.lastAudioDetected = currentTime;
        
        if (buttonPressed) {
          console.log("Czas podtrzymania zrestartowany");
          this.logger.addLog("BUTTON", "info", "Przycisk naciśnięty - restart timera");
        }
        
        if (!this.relayController.relaysActive && this.relayController.currentSequence === "SEQUENCE_IDLE") {
          this.relayController.startupSequence();
        }
      }
      
      if (this.relayController.relaysActive && 
          this.relayController.currentSequence === "SEQUENCE_IDLE" && 
          (currentTime - this.lastAudioDetected >= this.config.czasPoSyg * 1000)) {
        
        if (this.uartManager.active) console.log("Brak aktywności – wyłączanie.");
        this.logger.addLog("TIMEOUT", "info", `Brak aktywności przez ${this.config.czasPoSyg}s`);
        this.relayController.shutdownSequence();
      }
      
      if (this.relayController.relaysActive) {
        // Handle temperature monitoring and fan control
        const temp = this.sensors.readTemperature();
        
        if (temp !== null) {
          if (this.uartManager.active) {
            console.log(`Temp: ${temp}`);
          }
          
          // Fan PWM control
          const pwm = Math.max(0, Math.min(255, Math.floor((temp - this.config.tempMin) / 
                                                          (this.config.tempMax - this.config.tempMin) * 255)));
          // In real code: ledcWrite(0, pwm)
          
          if (temp >= this.config.tempPrzegrzania && temp < this.config.tempMax) {
            this.logger.addLog("TEMPERATURE", "warning", `Temperatura ostrzegawcza: ${temp.toFixed(1)}°C`);
          }
          
          if (temp >= this.config.tempMax) {
            if (this.uartManager.active) console.log("Temp krytyczna – chłodzenie");
            this.logger.addLog("TEMPERATURE", "error", `Temperatura krytyczna: ${temp.toFixed(1)}°C - wymuszenie chłodzenia`);
            this.relayController.shutdownSequence();
            
            // In real code: cooling loop
            this.logger.addLog("TEMPERATURE", "success", `Chłodzenie zakończone - temp: ${temp.toFixed(1)}°C`);
          }
        }
      }
    } else {
      if (this.relayController.relaysActive) {
        if (this.uartManager.active) console.log("Zbyt niskie napięcie – wyłączam.");
        this.relayController.shutdownSequence();
      }
    }
    
    // Handle button long press
    this.handleButtonLongPress();
  }
  
  handleButtonLongPress() {
    const buttonPressed = false; // In real code: digitalRead(PRZYCISK_PIN) == LOW
    const currentTime = Date.now();
    
    if (buttonPressed) {
      if (!this.buttonHolding) {
        this.buttonHoldStart = currentTime;
        this.buttonHolding = true;
      } else if (currentTime - this.buttonHoldStart >= 3000) {
        console.log("Przycisk przytrzymany 5s – aktywuję UART i WiFi");
        this.logger.addLog("BUTTON", "info", "Przycisk przytrzymany 5s - aktywacja serwisów");
        
        if (!this.uartManager.active) {
          this.uartManager.active = true;
          // In real code: Serial.begin(115200)
          this.uartManager.startTime = currentTime;
          this.logger.addLog("UART", "success", "UART ponownie aktywowany");
        }
        
        if (!this.webServer.active) {
          // In real code: WiFi.softAP and server setup
          this.webServer.startTime = currentTime;
          this.webServer.active = true;
          this.logger.addLog("WIFI", "success", "WiFi AP ponownie aktywowany");
        }
        
        this.buttonHolding = false;
      }
    } else {
      this.buttonHolding = false;
    }
  }
}

// Create and run the controller
const controller = new SubwooferController();
controller.setup();

// Simulate a few loop cycles
console.log("\n=== Running simulation of the controller ===\n");
for (let i = 0; i < 5; i++) {
  console.log(`\n--- Loop cycle ${i+1} ---`);
  controller.loop();
}

// Show the logs
console.log("\n=== Console Logs ===\n");
console.log(JSON.stringify(controller.logger.getLogsAsJson(), null, 2));
