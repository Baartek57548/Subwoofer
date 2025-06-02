#include "SubwooferWebServer.h"
#include <ArduinoJson.h>

// Deklaracje zewnętrznych zmiennych
extern unsigned long lastAudioDetected;
extern DallasTemperature sensors;

SubwooferWebServer::SubwooferWebServer() : 
  server(80),
  active(true),
  startTime(0),
  connectedClients(0),
  batteryPin(0) {
}

void SubwooferWebServer::init(ConfigManager* config, ConsoleLogger* logger, RelayController* relayController, SensorManager* sensorManager, int batteryPin) {
  this->config = config;
  this->logger = logger;
  this->relayController = relayController;
  this->sensorManager = sensorManager;
  this->batteryPin = batteryPin;  // Zapisz pin baterii
  
  WiFi.softAP(nazwaWifi, hasloWifi);
  startTime = millis();
  
  setupRoutes();
  server.begin();
  
  logger->addLog("WEB SERVER", "success", "Serwer HTTP uruchomiony");
}

void SubwooferWebServer::activate() {
  WiFi.softAP(nazwaWifi, hasloWifi);
  startTime = millis();
  active = true;
  setupRoutes();
  server.begin();
  Serial.print("Ponownie aktywowano AP. IP: ");
  Serial.println(WiFi.softAPIP());
}

void SubwooferWebServer::handleClient() {
  if (!active) return;
  
  server.handleClient();
  connectedClients = WiFi.softAPgetStationNum();

  if (connectedClients == 0 && millis() - startTime > WIFI_TIMEOUT) {
    Serial.println("Web server timeout – brak klientów, wyłączam AP.");
    logger->addLog("WIFI", "warning", "Timeout - wyłączanie Access Point");
    server.stop();
    WiFi.softAPdisconnect(true);
    active = false;
  }
}

void SubwooferWebServer::setupRoutes() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/set", HTTP_GET, [this]() { handleSet(); });
  server.on("/trigger", HTTP_GET, [this]() { handleTrigger(); });
  server.on("/force-shutdown", HTTP_GET, [this]() { handleForceShutdown(); });
  server.on("/fastdata", HTTP_GET, [this]() { handleFastData(); });
  server.on("/data", HTTP_GET, [this]() { handleData(); });
  server.on("/logs", HTTP_GET, [this]() { handleLogs(); });
  server.on("/help", HTTP_GET, [this]() { handleHelp(); });
  server.on("/factory", HTTP_GET, [this]() { handleFactory(); });
  server.on("/restart", HTTP_GET, [this]() { handleRestart(); });
}

void SubwooferWebServer::handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html><html lang='pl'><head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'>
  <title>ESP32 Subwoofer Controller</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { 
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; 
      background: #0a0a0a; color: #fff; 
      line-height: 1.4; overflow-x: hidden;
    }
    
    .container { 
      max-width: 100vw; padding: 10px; 
      margin: 0 auto; min-height: 100vh;
    }
    
    .header {
      text-align: center; padding: 15px 0; 
      background: linear-gradient(135deg, #1a1a2e, #16213e);
      border-radius: 12px; margin-bottom: 15px;
      box-shadow: 0 4px 15px rgba(0,0,0,0.3);
    }
    .header h1 { 
      font-size: clamp(1.5rem, 5vw, 2.2rem); 
      color: #00d4ff; margin-bottom: 5px;
      text-shadow: 0 2px 10px rgba(0,212,255,0.3);
    }
    .header p { 
      font-size: clamp(0.8rem, 3vw, 1rem); 
      color: #aaa; opacity: 0.8;
    }
    
    .status-grid {
      display: grid; 
      grid-template-columns: repeat(auto-fit, minmax(130px, 1fr));
      gap: 10px; margin-bottom: 15px;
    }
    
    .status-card {
      background: linear-gradient(135deg, #1e1e1e, #2a2a2a);
      border: 1px solid #333; border-radius: 12px;
      padding: 12px; text-align: center;
      box-shadow: 0 2px 10px rgba(0,0,0,0.2);
      transition: transform 0.2s, box-shadow 0.2s;
    }
    .status-card:active { transform: scale(0.98); }
    
    .status-icon { font-size: 1.6rem; margin-bottom: 6px; }
    .status-label { 
      font-size: 0.8rem; color: #bbb; 
      margin-bottom: 4px; font-weight: 500;
    }
    .status-value { 
      font-size: 1rem; font-weight: bold; 
      padding: 3px 6px; border-radius: 6px;
    }
    
    .value-success { background: #1a4d1a; color: #4ade80; }
    .value-warning { background: #4d3d1a; color: #fbbf24; }
    .value-error { background: #4d1a1a; color: #f87171; }
    .value-info { background: #1a2d4d; color: #60a5fa; }
    .value-inactive { background: #2a2a2a; color: #888; }
    
    .timer-card {
      background: linear-gradient(135deg, #1a2d1a, #2a4d2a);
      border: 1px solid #4ade80;
    }
    .timer-card.warning {
      background: linear-gradient(135deg, #4d3d1a, #5f4e1a);
      border: 1px solid #fbbf24;
    }
    .timer-card.critical {
      background: linear-gradient(135deg, #4d1a1a, #5f1a1a);
      border: 1px solid #f87171;
      animation: pulse 2s infinite;
    }
    
    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.7; }
    }
    
    .section {
      background: linear-gradient(135deg, #1a1a1a, #252525);
      border: 1px solid #333; border-radius: 12px;
      margin-bottom: 15px; overflow: hidden;
      box-shadow: 0 4px 15px rgba(0,0,0,0.2);
    }
    
    .section-header {
      background: linear-gradient(135deg, #2a2a2a, #333);
      padding: 15px; border-bottom: 1px solid #444;
      display: flex; justify-content: space-between; align-items: center;
      cursor: pointer; user-select: none;
      transition: background 0.2s;
    }
    .section-header:hover {
      background: linear-gradient(135deg, #333, #3a3a3a);
    }
    .section-header:active {
      transform: scale(0.99);
    }
    .section-title { 
      font-size: 1.1rem; font-weight: 600; 
      color: #00d4ff; display: flex; align-items: center; gap: 8px;
    }
    .toggle-icon { 
      font-size: 1.2rem; transition: transform 0.3s ease;
      color: #888;
    }
    .section.collapsed .toggle-icon { 
      transform: rotate(-90deg); 
    }
    
    .section-content {
      max-height: 1000px;
      padding: 15px; 
      transition: max-height 0.3s ease, padding 0.3s ease, opacity 0.3s ease;
      overflow: hidden;
      opacity: 1;
    }
    .section.collapsed .section-content { 
      max-height: 0; 
      padding: 0 15px; 
      opacity: 0;
    }
    
    .console-logs {
      height: 250px; overflow-y: auto; 
      background: #0a0a0a; border: 1px solid #333; 
      border-radius: 8px; padding: 8px;
      font-family: 'Courier New', monospace;
    }
    .log-entry {
      display: flex; gap: 8px; margin: 2px 0; 
      font-size: 0.8rem; padding: 4px 6px; 
      border-radius: 4px; align-items: center;
      transition: background-color 0.2s;
      animation: fadeIn 0.3s ease-in;
    }
    .log-entry:hover { background: #1a1a1a; }
    .log-time { color: #666; min-width: 60px; font-size: 0.75rem; }
    .log-operation { 
      min-width: 80px; font-weight: bold; 
      padding: 2px 6px; border-radius: 4px; font-size: 0.7rem;
      text-align: center;
    }
    .log-message { flex: 1; color: #ccc; font-size: 0.75rem; }
    
    .form-grid {
      display: grid; 
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 12px;
    }
    .form-group label {
      display: block; font-size: 0.85rem; 
      color: #bbb; margin-bottom: 5px; font-weight: 500;
    }
    .form-group input {
      width: 100%; padding: 12px; 
      background: #1a1a1a; border: 1px solid #444; 
      border-radius: 8px; color: #fff; font-size: 1rem;
      transition: border-color 0.2s, box-shadow 0.2s;
    }
    .form-group input:focus {
      outline: none; border-color: #00d4ff;
      box-shadow: 0 0 0 2px rgba(0,212,255,0.2);
    }
    
    .btn-grid {
      display: grid; 
      grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
      gap: 10px; margin-top: 15px;
    }
    .btn {
      padding: 14px 16px; border: none; border-radius: 8px;
      font-size: 0.9rem; font-weight: 600; cursor: pointer;
      transition: all 0.2s; text-decoration: none;
      display: flex; align-items: center; justify-content: center; gap: 6px;
      min-height: 48px;
      position: relative;
      overflow: hidden;
    }
    .btn:active { 
      transform: scale(0.96); 
    }
    .btn:hover {
      box-shadow: 0 4px 12px rgba(0,0,0,0.3);
    }
    
    .btn-primary { background: linear-gradient(135deg, #2563eb, #1d4ed8); color: white; }
    .btn-primary:hover { background: linear-gradient(135deg, #1d4ed8, #1e40af); }
    
    .btn-success { background: linear-gradient(135deg, #059669, #047857); color: white; }
    .btn-success:hover { background: linear-gradient(135deg, #047857, #065f46); }
    
    .btn-warning { background: linear-gradient(135deg, #d97706, #b45309); color: white; }
    .btn-warning:hover { background: linear-gradient(135deg, #b45309, #92400e); }
    
    .btn-danger { background: linear-gradient(135deg, #dc2626, #b91c1c); color: white; }
    .btn-danger:hover { background: linear-gradient(135deg, #b91c1c, #991b1b); }
    
    .btn-full {
      grid-column: 1 / -1;
      font-size: 1rem;
      padding: 16px;
      min-height: 56px;
    }
    
    /* Ripple effect for buttons */
    .btn::before {
      content: '';
      position: absolute;
      top: 50%;
      left: 50%;
      width: 0;
      height: 0;
      border-radius: 50%;
      background: rgba(255,255,255,0.3);
      transition: width 0.6s, height 0.6s;
      transform: translate(-50%, -50%);
      z-index: 0;
    }
    .btn:active::before {
      width: 300px;
      height: 300px;
    }
    .btn > * {
      position: relative;
      z-index: 1;
    }
    
    .footer {
      text-align: center; padding: 20px; 
      color: #666; font-size: 0.8rem;
      border-top: 1px solid #333; margin-top: 20px;
    }
    
    /* Mobile optimizations */
    @media (max-width: 768px) {
      .container { padding: 8px; }
      .status-grid { grid-template-columns: repeat(2, 1fr); }
      .form-grid { grid-template-columns: 1fr; }
      .btn-grid { grid-template-columns: 1fr; }
      .console-logs { height: 200px; }
      .section-header { padding: 12px; }
      .section-content { padding: 12px; }
    }
    
    @media (max-width: 480px) {
      .status-grid { grid-template-columns: repeat(2, 1fr); }
      .status-card { padding: 10px; }
      .btn { padding: 12px; font-size: 0.85rem; }
    }
    
    /* Scrollbar styling */
    .console-logs::-webkit-scrollbar { width: 6px; }
    .console-logs::-webkit-scrollbar-track { background: #1a1a1a; }
    .console-logs::-webkit-scrollbar-thumb { 
      background: #444; border-radius: 3px; 
    }
    .console-logs::-webkit-scrollbar-thumb:hover { background: #555; }

    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(-10px); }
      to { opacity: 1; transform: translateY(0); }
    }

    /* Touch feedback for mobile */
    @media (hover: none) and (pointer: coarse) {
      .btn:active {
        background: rgba(255,255,255,0.1) !important;
      }
      .section-header:active {
        background: rgba(255,255,255,0.1) !important;
      }
    }

/* Theme Toggle Switch */
.theme-toggle {
  position: fixed;
  top: 15px;
  right: 15px;
  z-index: 1000;
  display: flex;
  align-items: center;
  gap: 10px;
  background: rgba(26, 26, 26, 0.95);
  padding: 10px 15px;
  border-radius: 25px;
  border: 1px solid #444;
  backdrop-filter: blur(10px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
  transition: all 0.3s ease;
}

.theme-toggle:hover {
  transform: translateY(-1px);
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.4);
}

.theme-toggle-label {
  font-size: 0.9rem;
  color: #ccc;
  user-select: none;
  transition: color 0.3s ease;
}

.switch {
  position: relative;
  display: inline-block;
  width: 54px;
  height: 28px;
}

.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #374151;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  border-radius: 28px;
  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.2);
}

.slider:before {
  position: absolute;
  content: "";
  height: 22px;
  width: 22px;
  left: 3px;
  bottom: 3px;
  background-color: #9ca3af;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  border-radius: 50%;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
}

input:checked + .slider {
  background-color: #3b82f6;
  box-shadow: inset 0 2px 4px rgba(59, 130, 246, 0.3);
}

input:checked + .slider:before {
  transform: translateX(26px);
  background-color: white;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
}

.slider:hover {
  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.3), 0 0 0 2px rgba(59, 130, 246, 0.2);
}

/* Mobile adjustments for theme toggle */
@media (max-width: 768px) {
  .theme-toggle {
    top: 10px;
    right: 10px;
    padding: 8px 12px;
    gap: 8px;
  }
  
  .switch {
    width: 48px;
    height: 24px;
  }
  
  .slider:before {
    height: 18px;
    width: 18px;
  }
  
  input:checked + .slider:before {
    transform: translateX(24px);
  }
  
  .theme-toggle-label {
    font-size: 0.8rem;
  }
}

/* Light theme styles */
body.light-theme {
  background: #ffffff;
  color: #1f2937;
}

body.light-theme .container {
  background: #ffffff;
}

body.light-theme .header {
  background: linear-gradient(135deg, #3b82f6, #1d4ed8);
  color: white;
}

body.light-theme .header h1 {
  color: white;
  text-shadow: 0 2px 10px rgba(0, 0, 0, 0.2);
}

body.light-theme .header p {
  color: rgba(255, 255, 255, 0.9);
}

body.light-theme .status-card {
  background: linear-gradient(135deg, #f8fafc, #ffffff);
  border: 2px solid #e5e7eb;
  color: #374151;
  box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
}

body.light-theme .status-card:nth-child(1) {
  border-color: #f59e0b;
  background: linear-gradient(135deg, #fef3c7, #ffffff);
}

body.light-theme .status-card:nth-child(2) {
  border-color: #10b981;
  background: linear-gradient(135deg, #d1fae5, #ffffff);
}

body.light-theme .status-card:nth-child(3) {
  border-color: #8b5cf6;
  background: linear-gradient(135deg, #ede9fe, #ffffff);
}

body.light-theme .status-card:nth-child(4) {
  border-color: #ef4444;
  background: linear-gradient(135deg, #fee2e2, #ffffff);
}

body.light-theme .value-success { 
  background: #dcfce7; 
  color: #166534; 
  border: 1px solid #bbf7d0;
}

body.light-theme .value-warning { 
  background: #fef3c7; 
  color: #92400e; 
  border: 1px solid #fde68a;
}

body.light-theme .value-error { 
  background: #fee2e2; 
  color: #dc2626; 
  border: 1px solid #fecaca;
}

body.light-theme .value-info { 
  background: #dbeafe; 
  color: #1d4ed8; 
  border: 1px solid #bfdbfe;
}

body.light-theme .value-inactive { 
  background: #f3f4f6; 
  color: #6b7280; 
  border: 1px solid #d1d5db;
}

body.light-theme .section {
  background: #ffffff;
  border: 2px solid #e5e7eb;
  box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
}

body.light-theme .section-header {
  background: linear-gradient(135deg, #f9fafb, #f3f4f6);
  border-bottom: 2px solid #e5e7eb;
}

body.light-theme .section-header:hover {
  background: linear-gradient(135deg, #f3f4f6, #e5e7eb);
}

body.light-theme .section-title {
  color: #1f2937;
  font-weight: 700;
}

body.light-theme .toggle-icon {
  color: #6b7280;
}

body.light-theme .console-logs {
  background: #f9fafb;
  border: 2px solid #e5e7eb;
  color: #374151;
}

body.light-theme .log-entry {
  color: #374151;
}

body.light-theme .log-entry:hover {
  background: #f3f4f6;
}

body.light-theme .log-time {
  color: #9ca3af;
}

body.light-theme .log-message {
  color: #4b5563;
}

body.light-theme .form-group input {
  background: #ffffff;
  border: 2px solid #d1d5db;
  color: #374151;
}

body.light-theme .form-group input:focus {
  border-color: #3b82f6;
  box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
}

body.light-theme .form-group label {
  color: #374151;
  font-weight: 600;
}

body.light-theme .footer {
  color: #6b7280;
  border-top: 2px solid #e5e7eb;
}

body.light-theme .theme-toggle {
  background: rgba(255, 255, 255, 0.95);
  border: 2px solid #e5e7eb;
  box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
}

body.light-theme .theme-toggle-label {
  color: #4b5563;
}

body.light-theme .slider {
  background-color: #d1d5db;
}

body.light-theme .slider:before {
  background-color: #6b7280;
}

body.light-theme input:checked + .slider {
  background-color: #3b82f6;
}

body.light-theme input:checked + .slider:before {
  background-color: white;
}

body.light-theme .btn-primary { 
  background: linear-gradient(135deg, #3b82f6, #2563eb); 
}

body.light-theme .btn-success { 
  background: linear-gradient(135deg, #10b981, #059669); 
}

body.light-theme .btn-warning { 
  background: linear-gradient(135deg, #f59e0b, #d97706); 
}

body.light-theme .btn-danger { 
  background: linear-gradient(135deg, #ef4444, #dc2626); 
}

.timer-bar {
  width: 100%;
  background: linear-gradient(135deg, #1e1e1e, #2a2a2a);
  border: 1px solid #333;
  border-radius: 12px;
  margin-bottom: 15px;
  overflow: hidden;
  max-height: 0;
  opacity: 0;
  transition: max-height 0.4s ease, opacity 0.4s ease, padding 0.4s ease;
  box-shadow: 0 2px 10px rgba(0,0,0,0.2);
}

.timer-bar.active {
  max-height: 80px;
  opacity: 1;
  padding: 15px;
}

.timer-content {
  display: flex;
  align-items: center;
  gap: 15px;
  height: 50px;
}

.timer-icon {
  font-size: 1.8rem;
  min-width: 40px;
  text-align: center;
}

.timer-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 5px;
}

.timer-label {
  font-size: 0.9rem;
  color: #bbb;
  font-weight: 500;
}

.timer-progress-container {
  position: relative;
  height: 8px;
  background: #333;
  border-radius: 4px;
  overflow: hidden;
}

.timer-progress-bar {
  height: 100%;
  background: linear-gradient(90deg, #4ade80, #22c55e);
  border-radius: 4px;
  transition: width 0.1s ease, background 0.3s ease;
  width: 100%;
}

.timer-progress-bar.warning {
  background: linear-gradient(90deg, #fbbf24, #f59e0b);
}

.timer-progress-bar.critical {
  background: linear-gradient(90deg, #f87171, #ef4444);
  animation: pulse-progress 1s infinite;
}

.timer-time {
  font-size: 1.2rem;
  font-weight: bold;
  color: #4ade80;
  min-width: 80px;
  text-align: right;
}

.timer-time.warning {
  color: #fbbf24;
}

.timer-time.critical {
  color: #f87171;
}

@keyframes pulse-progress {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.7; }
}

/* Light theme styles for timer */
body.light-theme .timer-bar {
  background: linear-gradient(135deg, #ffffff, #f8fafc);
  border: 2px solid #e5e7eb;
  box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
}

body.light-theme .timer-label {
  color: #4b5563;
}

body.light-theme .timer-progress-container {
  background: #e5e7eb;
}

body.light-theme .timer-time {
  color: #059669;
}

body.light-theme .timer-time.warning {
  color: #d97706;
}

body.light-theme .timer-time.critical {
  color: #dc2626;
}

/* Toast Notification */
.toast-notification {
  position: fixed;
  top: 0; /* Na samej górze */
  left: 50%;
  transform: translateX(-50%) translateY(-100%); /* Ukryty powyżej ekranu */
  background: linear-gradient(135deg, #3b82f6, #1d4ed8);
  color: white;
  padding: 12px 20px;
  border-radius: 0 0 15px 15px; /* Zaokrąglone tylko dolne rogi */
  font-weight: 600;
  font-size: 0.9rem;
  z-index: 999;
  opacity: 0;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 4px 12px rgba(59, 130, 246, 0.3);
  backdrop-filter: blur(10px);
  border: 1px solid rgba(255, 255, 255, 0.2);
  max-width: 90%;
  text-align: center;
  white-space: nowrap;
}

.toast-notification.show {
  opacity: 1;
  transform: translateX(-50%) translateY(0); /* Wysuwa się w dół */
}

.toast-notification.hide {
  opacity: 0;
  transform: translateX(-50%) translateY(-100%); /* Chowa się do góry */
}

/* Mobile adjustments for toast */
@media (max-width: 768px) {
  .toast-notification {
    font-size: 0.85rem;
    padding: 10px 16px;
    max-width: 85%;
    border-radius: 0 0 12px 12px;
  }
}

/* Light theme styles for toast */
body.light-theme .toast-notification {
  background: linear-gradient(135deg, #1e40af, #1d4ed8);
  border: 1px solid rgba(255, 255, 255, 0.3);
  box-shadow: 0 4px 12px rgba(30, 64, 175, 0.3);
}

body.light-theme .toast-notification.success {
  background: linear-gradient(135deg, #059669, #047857);
  box-shadow: 0 4px 12px rgba(5, 150, 105, 0.3);
}

body.light-theme .toast-notification.warning {
  background: linear-gradient(135deg, #d97706, #b45309);
  box-shadow: 0 4px 12px rgba(217, 119, 6, 0.3);
}

body.light-theme .toast-notification.danger {
  background: linear-gradient(135deg, #dc2626, #b91c1c);
  box-shadow: 0 4px 12px rgba(220, 38, 38, 0.3);
}
  </style>
</head>
<body>
  <div class='theme-toggle'>
    <span class='theme-toggle-label'>🌙</span>
    <label class='switch'>
      <input type='checkbox' id='themeToggle'>
      <span class='slider'></span>
    </label>
    <span class='theme-toggle-label'>☀️</span>
  </div>
  <div class='toast-notification' id='toastNotification'></div>
  <div class='container'>
    <div class='header'>
      <h1>🎵 Subwoofer Controller</h1>
      <p>ESP32C3 Audio System Management</p>
    </div>

    <div class='status-grid'>
      <div class='status-card'>
        <div class='status-icon'>🌡️</div>
        <div class='status-label'>Temperature</div>
        <div class='status-value value-info' id='temp'>--°C</div>
      </div>
      <div class='status-card'>
        <div class='status-icon'>🔋</div>
        <div class='status-label'>Battery</div>
        <div class='status-value value-success' id='batt'>--V</div>
      </div>
      <div class='status-card'>
        <div class='status-icon'>🔊</div>
        <div class='status-label'>Audio</div>
        <div class='status-value value-info' id='audio'>--V</div>
      </div>
      <div class='status-card'>
        <div class='status-icon'>⚡</div>
        <div class='status-label'>Relays</div>
        <div class='status-value' id='relays'>--</div>
      </div>
    </div>

    <div class='timer-bar' id='timerBar'>
      <div class='timer-content'>
        <div class='timer-icon'>⏱️</div>
        <div class='timer-info'>
          <div class='timer-label'>Shutdown in</div>
          <div class='timer-progress-container'>
            <div class='timer-progress-bar' id='timerProgressBar'></div>
          </div>
        </div>
        <div class='timer-time' id='timerTime'>--</div>
      </div>
    </div>

    <div class='section'>
      <div class='section-header' onclick='toggleSection(this)'>
        <div class='section-title'>
          📟 Console Logs
        </div>
        <div class='toggle-icon'>▼</div>
      </div>
      <div class='section-content'>
        <div class='console-logs' id='consoleLogs'>
          <!-- Logi będą załadowane automatycznie -->
        </div>
      </div>
    </div>

    <div class='section collapsed'>
      <div class='section-header' onclick='toggleSection(this)'>
        <div class='section-title'>
          ⚙️ Configuration
        </div>
        <div class='toggle-icon'>▼</div>
      </div>
      <div class='section-content'>
        <form id='configForm' action='/set' method='get'>
          <div class='form-grid'>
            <div class='form-group'>
              <label>Hold time [s]</label>
              <input name='czas' type='number' value=')rawliteral" + String(config->getCzasPoSyg()) + R"rawliteral(' min='5' max='600'>
            </div>
            <div class='form-group'>
              <label>Min voltage [V]</label>
              <input name='napiecie' type='number' step='0.1' value=')rawliteral" + String(config->getProgNapiecia(), 1) + R"rawliteral(' min='11' max='15'>
            </div>
            <div class='form-group'>
              <label>Audio threshold [V]</label>
              <input name='audio' type='number' step='0.001' value=')rawliteral" + String(config->getAudioThreshold(), 3) + R"rawliteral(' min='0.1' max='3'>
            </div>
            <div class='form-group'>
              <label>Fan start [°C]</label>
              <input name='tmin' type='number' step='0.1' value=')rawliteral" + String(config->getTempMin(), 1) + R"rawliteral(' min='30' max='70'>
            </div>
            <div class='form-group'>
              <label>Warning temp [°C]</label>
              <input name='tprzegrz' type='number' step='0.1' value=')rawliteral" + String(config->getTempPrzegrzania(), 1) + R"rawliteral(' min='40' max='85'>
            </div>
            <div class='form-group'>
              <label>Critical temp [°C]</label>
              <input name='tmax' type='number' step='0.1' value=')rawliteral" + String(config->getTempMax(), 1) + R"rawliteral(' min='50' max='100'>
            </div>
            <div class='form-group'>
              <label>Cool stop [°C]</label>
              <input name='savetemp' type='number' step='0.1' value=')rawliteral" + String(config->getTempSave(), 1) + R"rawliteral(' min='30' max='70'>
            </div>
            <div class='form-group'>
              <label>Relay delay [ms]</label>
              <input name='delayrelay' type='number' value=')rawliteral" + String(config->getDelayRelaySwitch()) + R"rawliteral(' min='100' max='10000'>
            </div>
          </div>
        </form>
        <div style='margin-top: 20px; display: grid; grid-template-columns: 1fr 2fr; gap: 10px;'>
          <button class='btn btn-warning' type='button' onclick='factoryReset()' style='padding: 16px; font-size: 0.9rem;'>🏭 Fabric Settings</button>
          <button class='btn btn-success' type='button' onclick='saveConfig()' style='padding: 16px; font-size: 1rem;'>💾 Save Configuration</button>
        </div>
      </div>
    </div>

    <!-- Przyciski poniżej kontenera Configuration -->
    <div class='btn-grid'>
      <button class='btn btn-danger' type='button' onclick='restart()'>🔄 Restart</button>
      <button class='btn btn-primary' type='button' onclick="location.href='/help'">❓ Help</button>
      <button class='btn btn-primary' type='button' id='triggerBtn'>
        <span id='triggerText'>🚀 Hold to Start</span>
        <div id='holdProgress' style='display: none; width: 100%; height: 3px; background: #333; border-radius: 2px; margin-top: 4px;'>
          <div id='progressBar' style='height: 100%; background: #f87171; border-radius: 2px; width: 0%; transition: width 0.1s;'></div>
        </div>
      </button>
    </div>

    <div class='footer'>
      ESP32C3 Subwoofer Controller © 2025
    </div>
  </div>

<script>
let currentHoldTime = )rawliteral" + String(config->getCzasPoSyg()) + R"rawliteral(;
let holdTimer = null;
let holdProgress = 0;
let holdInterval = null;
let isHolding = false;
let isSystemActive = false;
let activateServicesMode = false;
let forceShutdownMode = false;
let holdDuration = 0;
let lastServerTime = 0;
let lastServerTimeRemaining = 0;
let localCountdownInterval = null;

// Optymalizacja responsywności
let fastDataCache = {};
let tempDataCache = {};
let logsCache = {};
let isLogsVisible = false;
let updateIntervals = {
  fast: null,
  temp: null,
  logs: null
};
let requestQueue = new Set();
let lastUpdateTimes = {
  fast: 0,
  temp: 0,
  logs: 0
};

// Debouncing dla requestów
function debounce(func, wait) {
  let timeout;
  return function executedFunction(...args) {
    const later = () => {
      clearTimeout(timeout);
      func(...args);
    };
    clearTimeout(timeout);
    timeout = setTimeout(later, wait);
  };
}

// Toast Notification System
let toastTimeout = null;

function showToast(message, type = 'info', duration = 2000) {
  const toast = document.getElementById('toastNotification');
  if (!toast) return;
  
  // Clear existing timeout
  if (toastTimeout) {
    clearTimeout(toastTimeout);
  }
  
  // Remove hide class if exists
  toast.classList.remove('hide');
  
  // Set message and type
  toast.textContent = message;
  toast.className = `toast-notification ${type}`;
  
  // Show toast
  requestAnimationFrame(() => {
    toast.classList.add('show');
  });
  
  // Add vibration feedback if available
  if (navigator.vibrate) {
    if (type === 'danger') {
      navigator.vibrate([100, 50, 100, 50, 100]);
    } else if (type === 'warning') {
      navigator.vibrate([100, 50, 100]);
    } else if (type === 'success') {
      navigator.vibrate([50, 30, 50]);
    } else {
      navigator.vibrate(50);
    }
  }
  
  // Hide toast after duration
  toastTimeout = setTimeout(() => {
    hideToast();
  }, duration);
}

function hideToast() {
  const toast = document.getElementById('toastNotification');
  if (toast) {
    toast.classList.add('hide');
    toast.classList.remove('show');
  }
  if (toastTimeout) {
    clearTimeout(toastTimeout);
    toastTimeout = null;
  }
}

// Optymalizowane fetch z cache i error handling
async function optimizedFetch(url, cacheKey, maxAge = 1000) {
  const now = Date.now();
  const cached = window[cacheKey + 'Cache'];
  
  if (cached && cached.timestamp && (now - cached.timestamp < maxAge)) {
    return cached.data;
  }
  
  if (requestQueue.has(url)) {
    return null; // Unikaj duplikowania requestów
  }
  
  requestQueue.add(url);
  
  try {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 2000); // 2s timeout
    
    const response = await fetch(url, { 
      signal: controller.signal,
      cache: 'no-cache'
    });
    
    clearTimeout(timeoutId);
    
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    
    const data = await response.json();
    window[cacheKey + 'Cache'] = { data, timestamp: now };
    
    return data;
  } catch (error) {
    console.warn(`Fetch error for ${url}:`, error.message);
    return cached ? cached.data : null;
  } finally {
    requestQueue.delete(url);
  }
}

function toggleSection(header) {
  const section = header.parentElement;
  if (!section) {
    console.warn('toggleSection: section not found');
    return;
  }
  
  const wasCollapsed = section.classList.contains('collapsed');
  section.classList.toggle('collapsed');
  
  console.log('Section toggled:', !wasCollapsed ? 'collapsed' : 'expanded');
  
  // Lazy loading logów
  const logsContainer = section.querySelector('#consoleLogs');
  if (logsContainer) {
    if (!wasCollapsed) {
      // Sekcja została zwinięta
      isLogsVisible = false;
      if (updateIntervals.logs) {
        clearInterval(updateIntervals.logs);
        updateIntervals.logs = null;
      }
      console.log('Logs section collapsed, stopped updates');
    } else {
      // Sekcja została rozwinięta
      isLogsVisible = true;
      updateLogs(); // Natychmiastowe załadowanie
      startLogsUpdates();
      console.log('Logs section expanded, started updates');
    }
  }
}

// Zoptymalizowane aktualizacje z inteligentnym interwałem
async function updateReadings() {
  const data = await optimizedFetch('/fastdata', 'fastData', 300); // Cache na 300ms
  if (!data) return;
  
  // Batch DOM updates
  const updates = [];
  
  if (data.batt !== fastDataCache.batt) {
    updates.push(() => document.getElementById('batt').textContent = data.batt + 'V');
    fastDataCache.batt = data.batt;
  }
  
  if (data.audio !== fastDataCache.audio) {
    updates.push(() => document.getElementById('audio').textContent = data.audio + 'V');
    fastDataCache.audio = data.audio;
  }
  
  const relaysBadge = document.getElementById('relays');
  const triggerText = document.getElementById('triggerText');
  
  const wasSystemActive = isSystemActive;
  isSystemActive = data.relays;
  
  if (wasSystemActive !== isSystemActive && !isHolding) {
    updates.push(() => {
      if (isSystemActive) {
        triggerText.textContent = '🔄 Tap to Reset / Hold 5s to Stop';
      } else {
        triggerText.textContent = '🚀 Hold 3s to Start';
      }
    });
  }
  
  if (data.relays !== fastDataCache.relays) {
    updates.push(() => {
      const timerBar = document.getElementById('timerBar');
      if (data.relays) {
        relaysBadge.textContent = data.relayStatus || 'ACTIVE';
        relaysBadge.className = 'status-value ' + (data.relayStatusClass || 'value-success');
        timerBar.classList.add('active');
      } else {
        relaysBadge.textContent = 'OFF';
        relaysBadge.className = 'status-value value-warning';
        timerBar.classList.remove('active');
        
        if (localCountdownInterval) {
          clearInterval(localCountdownInterval);
          localCountdownInterval = null;
        }
      }
    });
    fastDataCache.relays = data.relays;
  }
  
  // Wykonaj wszystkie aktualizacje DOM jednocześnie
  if (updates.length > 0) {
    requestAnimationFrame(() => {
      updates.forEach(update => update());
    });
  }
  
  // Timer handling
  if (data.relays && data.timeRemaining !== undefined) {
    lastServerTime = Date.now();
    lastServerTimeRemaining = Math.max(0, data.timeRemaining);
    
    if (!localCountdownInterval) {
      startLocalCountdown();
    }
    
    updateTimerDisplay(lastServerTimeRemaining);
  }
}

function startLocalCountdown() {
  if (localCountdownInterval) {
    clearInterval(localCountdownInterval);
  }
  
  localCountdownInterval = setInterval(() => {
    if (!isSystemActive) {
      clearInterval(localCountdownInterval);
      localCountdownInterval = null;
      return;
    }
    
    const timeSinceLastUpdate = (Date.now() - lastServerTime) / 1000;
    const currentTimeRemaining = Math.max(0, lastServerTimeRemaining - timeSinceLastUpdate);
    
    updateTimerDisplay(currentTimeRemaining);
    
    if (currentTimeRemaining <= 0) {
      clearInterval(localCountdownInterval);
      localCountdownInterval = null;
    }
  }, 100);
}

// Zoptymalizowane wyświetlanie timera
function updateTimerDisplay(timeRemaining) {
  const timerBar = document.getElementById('timerBar');
  const timerTime = document.getElementById('timerTime');
  const timerProgressBar = document.getElementById('timerProgressBar');
  
  if (timeRemaining > 0) {
    const minutes = Math.floor(timeRemaining / 60);
    const seconds = timeRemaining % 60;
    const displaySeconds = Math.floor(seconds * 10) / 10;
    
    const timeText = minutes > 0 ? 
      `${minutes}:${displaySeconds.toFixed(1).padStart(4, '0')}` : 
      `${displaySeconds.toFixed(1)}s`;
    
    if (timerTime.textContent !== timeText) {
      timerTime.textContent = timeText;
    }
    
    // Calculate progress percentage (assuming max time from config)
    const maxTime = currentHoldTime;
    const progressPercent = Math.max(0, (timeRemaining / maxTime) * 100);
    timerProgressBar.style.width = progressPercent + '%';
    
    // Update colors based on time remaining
    let timeClass = '';
    let progressClass = '';
    
    if (timeRemaining <= 10) {
      timeClass = 'critical';
      progressClass = 'critical';
    } else if (timeRemaining <= 30) {
      timeClass = 'warning';
      progressClass = 'warning';
    }
    
    timerTime.className = `timer-time ${timeClass}`;
    timerProgressBar.className = `timer-progress-bar ${progressClass}`;
  } else {
    timerTime.textContent = 'SHUTTING DOWN';
    timerTime.className = 'timer-time critical';
    timerProgressBar.style.width = '0%';
    timerProgressBar.className = 'timer-progress-bar critical';
  }
}

async function updateTemperature() {
  const data = await optimizedFetch('/data', 'tempData', 2000); // Cache na 2s
  if (!data) return;
  
  const tempElement = document.getElementById('temp');
  const newText = data.temp + '°C';
  
  if (tempElement.textContent !== newText) {
    tempElement.textContent = newText;
    
    const temp = parseFloat(data.temp);
    let className = 'status-value value-info';
    
    if (temp > 60) {
      className = 'status-value value-error';
    } else if (temp > 45) {
      className = 'status-value value-warning';
    }
    
    if (tempElement.className !== className) {
      tempElement.className = className;
    }
  }
}

async function updateLogs() {
  if (!isLogsVisible) return; // Nie ładuj jeśli niewidoczne
  
  const data = await optimizedFetch('/logs', 'logsData', 1500); // Cache na 1.5s
  if (!data || !data.logs) return;
  
  const logsContainer = document.getElementById('consoleLogs');
  if (!logsContainer) return;
  
  // Sprawdź czy logi się zmieniły
  const newLogsHash = JSON.stringify(data.logs).length;
  if (logsCache.hash === newLogsHash) return;
  
  logsCache.hash = newLogsHash;
  
  // Użyj DocumentFragment dla lepszej wydajności
  const fragment = document.createDocumentFragment();
  
  data.logs.forEach(log => {
    const logEntry = document.createElement('div');
    logEntry.className = 'log-entry';
    
    let badgeClass = 'value-info';
    if (log.status === 'success') badgeClass = 'value-success';
    else if (log.status === 'warning') badgeClass = 'value-warning';
    else if (log.status === 'error') badgeClass = 'value-error';
    
    logEntry.innerHTML = `
      <span class='log-time'>${log.timestamp}</span>
      <span class='log-operation ${badgeClass}'>${log.operation}</span>
      <span class='log-message'>${log.message}</span>
    `;
    fragment.appendChild(logEntry);
  });
  
  // Jedna operacja DOM
  logsContainer.innerHTML = '';
  logsContainer.appendChild(fragment);
}

function addLocalLog(operation, status, message) {
  const logsContainer = document.getElementById('consoleLogs');
  if (!logsContainer) return;
  
  // Automatycznie rozwiń sekcję logów jeśli jest zwinięta
  const logsSection = logsContainer.closest('.section');
  if (logsSection && logsSection.classList.contains('collapsed')) {
    logsSection.classList.remove('collapsed');
    isLogsVisible = true;
    startLogsUpdates();
  }
  
  // Utwórz nowy wpis
  const logEntry = document.createElement('div');
  logEntry.className = 'log-entry';
  logEntry.style.animation = 'fadeIn 0.3s ease-in';
  
  let badgeClass = 'value-info';
  if (status === 'success') badgeClass = 'value-success';
  else if (status === 'warning') badgeClass = 'value-warning';
  else if (status === 'error') badgeClass = 'value-error';
  
  // Aktualny czas
  const now = new Date();
  const timeStr = now.toTimeString().substr(0, 8);
  
  logEntry.innerHTML = `
    <span class='log-time'>${timeStr}</span>
    <span class='log-operation ${badgeClass}'>${operation}</span>
    <span class='log-message'>${message}</span>
  `;
  
  // Dodaj na początek listy
  logsContainer.insertBefore(logEntry, logsContainer.firstChild);
  
  // Usuń stare wpisy jeśli jest ich za dużo (zachowaj max 20)
  while (logsContainer.children.length > 20) {
    logsContainer.removeChild(logsContainer.lastChild);
  }
  
  // Przewiń do góry żeby pokazać nowy wpis
  logsContainer.scrollTop = 0;
}

// Inteligentne zarządzanie interwałami
function startUpdates() {
  // Krytyczne dane - często
  updateIntervals.fast = setInterval(updateReadings, 400);
  
  // Temperatura - rzadziej
  updateIntervals.temp = setInterval(updateTemperature, 3000);
  
  // Logi - tylko gdy widoczne
  if (isLogsVisible) {
    updateLogs(); // Załaduj od razu
    startLogsUpdates();
  }
}

function startLogsUpdates() {
  if (updateIntervals.logs) return; // Już uruchomione
  updateIntervals.logs = setInterval(updateLogs, 2000);
}

function stopUpdates() {
  Object.values(updateIntervals).forEach(interval => {
    if (interval) clearInterval(interval);
  });
  updateIntervals = { fast: null, temp: null, logs: null };
}

// Debounced funkcje dla interakcji użytkownika
const debouncedTriggerRelay = debounce(() => {
  fetch('/trigger').then(() => {
    setTimeout(updateLogs, 100);
  }).catch(e => console.warn('Trigger error:', e));
}, 100);

const debouncedForceShutdown = debounce(() => {
  fetch('/force-shutdown').then(() => {
    setTimeout(updateLogs, 100);
  }).catch(e => console.warn('Shutdown error:', e));
}, 100);

// Zmienione zmienne dla nowej logiki
let holdPhase = 'none'; // 'none', 'reset', 'warning', 'shutdown'

function startHold(event) {
  if (event) {
    event.preventDefault();
  }
  
  if (isHolding) {
    return;
  }
  
  console.log('Hold started, system active:', isSystemActive);
  
  isHolding = true;
  holdProgress = 0;
  holdDuration = 0;
  holdPhase = 'none';
  
  const triggerBtn = document.getElementById('triggerBtn');
  const triggerText = document.getElementById('triggerText');
  const holdProgressDiv = document.getElementById('holdProgress');
  const progressBar = document.getElementById('progressBar');
  
  // Zachowaj oryginalny tekst przycisku
  const originalText = triggerText.textContent;
  
  holdProgressDiv.style.display = 'block';
  progressBar.style.width = '0%';
  progressBar.style.background = '#4ade80';
  
  // Show initial toast
  if (isSystemActive) {
    showToast('⏱️ Przytrzymaj aby zresetować timer', 'info');
  } else {
    showToast('🚀 Przytrzymaj aby aktywować system', 'info');
  }
  
  holdInterval = setInterval(() => {
    if (!isHolding) {
      clearInterval(holdInterval);
      return;
    }
    
    holdDuration += 100;
    
    if (isSystemActive) {
      // System aktywny: 2s reset + 3s force shutdown = 5s total
      if (holdDuration <= 2000) {
        // Faza 1: Reset timer (0-2s)
        holdPhase = 'reset';
        holdProgress = (holdDuration / 2000) * 100;
        progressBar.style.width = holdProgress + '%';
        progressBar.style.background = '#4ade80';
        
        if (holdDuration === 1000) {
          showToast('🔄 Jeszcze 1s aby zresetować timer', 'success');
        }
        
        if (holdDuration >= 2000) {
          showToast('⏱️ Timer zresetowany! Przytrzymaj 3s aby wyłączyć', 'warning');
          progressBar.style.background = '#fbbf24';
          
          // Wyślij reset timera
          fetch('/trigger').then(() => {
            console.log('Timer reset sent');
            setTimeout(() => {
              if (updateLogs) updateLogs();
            }, 100);
          }).catch(e => console.warn('Reset error:', e));
        }
      } else if (holdDuration <= 5000) {
        // Faza 2: Force shutdown warning (2-5s)
        holdPhase = 'warning';
        const warningProgress = ((holdDuration - 2000) / 3000) * 100;
        holdProgress = 100 + warningProgress;
        progressBar.style.width = Math.min(100, warningProgress) + '%';
        progressBar.style.background = '#f87171';
        
        if (holdDuration === 3000) {
          showToast('⏹️ Jeszcze 2s aby wymusić wyłączenie', 'danger');
        }
        
        if (holdDuration >= 5000) {
          showToast('🛑 Wymuszone wyłączenie aktywowane!', 'danger');
          holdPhase = 'shutdown';
          endHold();
          forceShutdown();
          return;
        }
      }
    } else {
      // System nieaktywny: 3s activation
      holdPhase = 'activate';
      holdProgress = (holdDuration / 3000) * 100;
      progressBar.style.width = holdProgress + '%';
      progressBar.style.background = '#4ade80';
      
      if (holdDuration === 1000) {
        showToast('🚀 Jeszcze 2s aby aktywować', 'success');
      }
      
      if (holdDuration >= 3000) {
        showToast('✅ System aktywowany!', 'success');
        endHold();
        triggerRelay();
        return;
      }
    }
  }, 100);
  
  // Ustaw styl przycisku bez zmiany tekstu
  triggerBtn.style.background = 'linear-gradient(135deg, #374151, #4b5563)';
}

function endHold(event) {
  if (event) {
    event.preventDefault();
  }
  
  if (!isHolding) {
    return;
  }
  
  console.log('Hold ended, duration:', holdDuration, 'phase:', holdPhase);
  
  isHolding = false;
  
  // Toast pozostaje widoczny przez 2 sekundy po puszczeniu przycisku
  // Nie wywołujemy hideToast() - toast zniknie automatycznie po czasie
  
  if (holdInterval) {
    clearInterval(holdInterval);
    holdInterval = null;
  }
  
  // Logika dla krótkiego kliknięcia
  if (holdDuration < 1000) {
    if (isSystemActive) {
      console.log('Short click - resetting timer');
      showToast('⏱️ Timer zresetowany', 'success');
      setTimeout(() => {
        fetch('/trigger').then(() => {
          console.log('Quick timer reset sent');
          setTimeout(() => {
            if (updateLogs) updateLogs();
          }, 100);
        }).catch(e => console.warn('Quick reset error:', e));
      }, 50);
    }
  }
  
  resetTriggerButton();
}

function resetTriggerButton() {
  const triggerBtn = document.getElementById('triggerBtn');
  const triggerText = document.getElementById('triggerText');
  const holdProgressDiv = document.getElementById('holdProgress');
  const progressBar = document.getElementById('progressBar');
  
  if (isSystemActive) {
    triggerText.textContent = '🔄 Tap to Reset / Hold 5s to Stop';
  } else {
    triggerText.textContent = '🚀 Hold 3s to Start';
  }
  triggerBtn.style.background = 'linear-gradient(135deg, #2563eb, #1d4ed8)';
  holdProgressDiv.style.display = 'none';
  progressBar.style.width = '0%';
  progressBar.style.background = '#4ade80';
  holdProgress = 0;
  holdDuration = 0;
  holdPhase = 'none';
}

// Event listeners
function setupTriggerButton() {
  const triggerBtn = document.getElementById('triggerBtn');
  if (!triggerBtn) {
    console.warn('Trigger button not found!');
    return;
  }
  
  // Usuń stare event listenery jeśli istnieją
  triggerBtn.removeEventListener('mousedown', startHold);
  triggerBtn.removeEventListener('mouseup', endHold);
  triggerBtn.removeEventListener('mouseleave', endHold);
  triggerBtn.removeEventListener('touchstart', startHold);
  triggerBtn.removeEventListener('touchend', endHold);
  triggerBtn.removeEventListener('touchcancel', endHold);
  
  // Dodaj nowe event listenery
  triggerBtn.addEventListener('mousedown', startHold);
  triggerBtn.addEventListener('mouseup', endHold);
  triggerBtn.addEventListener('mouseleave', endHold);
  
  triggerBtn.addEventListener('touchstart', function(e) {
    e.preventDefault();
    startHold(e);
  });
  
  triggerBtn.addEventListener('touchend', function(e) {
    e.preventDefault();
    endHold(e);
  });
  
  triggerBtn.addEventListener('touchcancel', function(e) {
    e.preventDefault();
    endHold(e);
  });
  
  triggerBtn.addEventListener('contextmenu', function(e) {
    e.preventDefault();
  });
  
  console.log('Trigger button event listeners set up successfully');
}

// Theme toggle functionality
function initThemeToggle() {
  const themeToggle = document.getElementById('themeToggle');
  const body = document.body;
  
  // Load saved theme
  const savedTheme = localStorage.getItem('theme');
  if (savedTheme === 'light') {
    body.classList.add('light-theme');
    themeToggle.checked = true;
  }
  
  themeToggle.addEventListener('change', function() {
    if (this.checked) {
      body.classList.add('light-theme');
      localStorage.setItem('theme', 'light');
    } else {
      body.classList.remove('light-theme');
      localStorage.setItem('theme', 'dark');
    }
  });
}

// Initialize theme toggle when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
  console.log('DOM Content Loaded - setting up components');
  
  // Setup theme toggle
  initThemeToggle();
  
  // Setup trigger button
  setupTriggerButton();
  
  // Sprawdź czy logi są widoczne na starcie i załaduj je od razu
  const logsSection = document.querySelector('#consoleLogs');
  if (logsSection) {
    const section = logsSection.closest('.section');
    isLogsVisible = !section.classList.contains('collapsed');
    console.log('Initial logs visibility:', isLogsVisible);
    
    // Załaduj logi od razu jeśli są widoczne
    if (isLogsVisible) {
      updateLogs();
    }
  }
  
  console.log('All components set up successfully');
});

// Zarządzanie cyklem życia strony
startUpdates();
updateReadings();
updateTemperature();
if (isLogsVisible) updateLogs();

// Optymalizacja dla mobile
let lastTouchEnd = 0;
document.addEventListener('touchend', function (event) {
  const now = Date.now();
  if (now - lastTouchEnd <= 300) {
    event.preventDefault();
  }
  lastTouchEnd = now;
}, false);

// Zarządzanie widocznością karty
document.addEventListener('visibilitychange', function() {
  if (document.hidden) {
    if (isHolding) endHold();
    stopUpdates();
    if (localCountdownInterval) {
      clearInterval(localCountdownInterval);
      localCountdownInterval = null;
    }
  } else {
    startUpdates();
    if (isSystemActive && !localCountdownInterval) {
      startLocalCountdown();
    }
  }
});

// Cleanup przy zamknięciu
window.addEventListener('beforeunload', function() {
  stopUpdates();
  if (localCountdownInterval) {
    clearInterval(localCountdownInterval);
  }
});

function triggerRelay() {
  if (isSystemActive) {
    addLocalLog('TRIGGER', 'info', 'Reset timera - przedłużenie czasu pracy');
  } else {
    addLocalLog('TRIGGER', 'info', 'Aktywacja systemu...');
  }
  
  debouncedTriggerRelay();
}

function forceShutdown() {
  addLocalLog('FORCE SHUTDOWN', 'warning', 'Wymuszone wyłączenie systemu...');
  debouncedForceShutdown();
}

function factoryReset() {
  if (confirm('Przywrócić ustawienia fabryczne?')) {
    addLocalLog('FACTORY RESET', 'warning', 'Przywracanie ustawień fabrycznych...');
    
    fetch('/factory')
      .then(response => {
        if (response.ok) {
          addLocalLog('FACTORY RESET', 'success', 'Ustawienia fabryczne przywrócone');
          addLocalLog('SYSTEM', 'info', 'Przeładowywanie strony za 2 sekundy...');
          setTimeout(() => location.reload(), 2000);
        } else {
          addLocalLog('FACTORY RESET', 'error', 'Błąd podczas przywracania ustawień');
        }
      })
      .catch(e => {
        addLocalLog('FACTORY RESET', 'error', 'Błąd połączenia');
        console.warn('Factory reset error:', e);
      });
  }
}

function restart() {
  if (confirm('Zrestartować urządzenie?')) {
    addLocalLog('RESTART', 'warning', 'Inicjowanie restartu urządzenia...');
    
    fetch('/restart')
      .then(() => {
        addLocalLog('RESTART', 'info', 'Komenda restartu wysłana');
        addLocalLog('SYSTEM', 'warning', 'Urządzenie zostanie zrestartowane za chwilę...');
      })
      .catch(e => {
        addLocalLog('RESTART', 'error', 'Błąd podczas wysyłania komendy restartu');
        console.warn('Restart error:', e);
      });
  }
}

function saveConfig() {
  addLocalLog('WEB CONFIG', 'info', 'Zapisywanie ustawień...');
  
  const form = document.getElementById('configForm');
  if (form) {
    // Wyślij formularz
    fetch('/set?' + new URLSearchParams(new FormData(form)).toString())
      .then(response => {
        if (response.ok) {
          addLocalLog('SAVE', 'success', 'Ustawienia zapisane pomyślnie');
          setTimeout(() => {
            if (updateLogs) updateLogs();
          }, 500);
        } else {
          addLocalLog('SAVE', 'error', 'Błąd podczas zapisywania ustawień');
        }
      })
      .catch(e => {
        addLocalLog('SAVE', 'error', 'Błąd połączenia podczas zapisywania');
        console.warn('Save error:', e);
      });
  }
}
</script>
</body></html>
)rawliteral";

  server.send(200, "text/html", html);
}

void SubwooferWebServer::handleSet() {
  bool changed = false;
  if (server.hasArg("czas") && server.arg("czas") != "") {
    config->setCzasPoSyg(server.arg("czas").toInt());
    changed = true;
  }
  if (server.hasArg("napiecie") && server.arg("napiecie") != "") {
    config->setProgNapiecia(server.arg("napiecie").toFloat());
    changed = true;
  }
  if (server.hasArg("audio") && server.arg("audio") != "") {
    config->setAudioThreshold(server.arg("audio").toFloat());
    changed = true;
  }
  if (server.hasArg("tmin") && server.arg("tmin") != "") {
    config->setTempMin(server.arg("tmin").toFloat());
    changed = true;
  }
  if (server.hasArg("tprzegrz") && server.arg("tprzegrz") != "") {
    config->setTempPrzegrzania(server.arg("tprzegrz").toFloat());
    changed = true;
  }
  if (server.hasArg("tmax") && server.arg("tmax") != "") {
    config->setTempMax(server.arg("tmax").toFloat());
    changed = true;
  }
  if (server.hasArg("savetemp") && server.arg("savetemp") != "") {
    config->setTempSave(server.arg("savetemp").toFloat());
    changed = true;
  }
  if (server.hasArg("delayrelay") && server.arg("delayrelay") != "") {
    config->setDelayRelaySwitch(server.arg("delayrelay").toInt());
    changed = true;
  }
  
  if (changed) {
    logger->addLog("WEB CONFIG", "info", "Ustawienia zmienione przez interfejs web");
    config->saveSettings();
  }
  
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void SubwooferWebServer::handleTrigger() {
  lastAudioDetected = millis();
  
  if (!relayController->isActive() && relayController->isIdle()) {
    relayController->startupSequence();
  } else {
    logger->addLog("TRIGGER RELAYS", "info", "Przekaźniki już aktywne - przedłużono czas");
  }
  server.send(200, "text/plain", "OK");
}

void SubwooferWebServer::handleForceShutdown() {
  if (relayController->isActive()) {
    logger->addLog("FORCE SHUTDOWN", "warning", "Wymuszone wyłączenie przez interfejs web");
    relayController->shutdownSequence();
  } else {
    logger->addLog("FORCE SHUTDOWN", "info", "Próba wymuszonego wyłączenia - system już nieaktywny");
  }
  server.send(200, "text/plain", "OK");
}

void SubwooferWebServer::handleFastData() {
  float adc = analogRead(batteryPin);  // Używamy zapisanego pinu baterii
  float napiecie = ((adc) / 4095.0) * 3.3 * (47 + 12) / 12;
  
  DynamicJsonDocument doc(400);
  doc["batt"] = String(napiecie, 2);
  doc["audio"] = String(sensorManager->getFilteredAudio(), 3);
  doc["relays"] = relayController->isActive();
  doc["relayStatus"] = relayController->getStatusText();
  doc["relayStatusClass"] = relayController->getStatusClass();
  
  // Dodaj informację o czasie pozostałym do wyłączenia
  if (relayController->isActive()) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - lastAudioDetected) / 1000; // w sekundach
    long timeRemaining = (long)config->getCzasPoSyg() - (long)elapsedTime;
    doc["timeRemaining"] = max(0L, timeRemaining);
  }
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void SubwooferWebServer::handleData() {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  
  DynamicJsonDocument doc(100);
  doc["temp"] = String(temp, 1);
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void SubwooferWebServer::handleLogs() {
  server.send(200, "application/json", logger->getLogsAsJson());
}

void SubwooferWebServer::handleHelp() {
  String html = R"rawliteral(
<!DOCTYPE html><html><head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>Help – ESP32 Subwoofer Controller</title>
  <style>
    body { 
      background: #0a0a0a; color: #eee; 
      font-family: -apple-system, BlinkMacSystemFont, sans-serif; 
      padding: 15px; line-height: 1.6; margin: 0;
    }
    .container { max-width: 600px; margin: 0 auto; }
    h2 { color: #00d4ff; margin-bottom: 15px; }
    h3 { color: #fbbf24; margin: 20px 0 10px 0; }
    p, li { margin-bottom: 8px; }
    code { 
      background: #1a1a1a; padding: 2px 6px; 
      border-radius: 4px; color: #60a5fa; 
    }
    ul { padding-left: 20px; }
    .btn-back {
      display: inline-block; margin-top: 30px; 
      padding: 12px 20px; background: #2563eb; 
      color: white; text-decoration: none; 
      border-radius: 8px; font-weight: 600;
      transition: background 0.2s;
    }
    .btn-back:hover { background: #1d4ed8; }
    .section {
      background: #1a1a1a; padding: 15px; 
      border-radius: 8px; margin: 15px 0;
      border: 1px solid #333;
    }
  </style>
</head>
<body>
  <div class='container'>
    <h2>📖 Help – ESP32 Subwoofer Controller</h2>
    
    <div class='section'>
      <h3>🎯 System Overview</h3>
      <p>This controller manages a subwoofer system by monitoring audio signal, battery voltage and temperature. You can configure all parameters from the main interface.</p>
    </div>

    <div class='section'>
      <h3>📊 Live Readings</h3>
      <ul>
        <li><strong>Temperature</strong> – Current amplifier temperature</li>
        <li><strong>Battery</strong> – Current battery voltage</li>
        <li><strong>Audio</strong> – Audio signal level detection</li>
        <li><strong>Relays</strong> – Power relay status (ACTIVE/OFF)</li>
        <li><strong>Shutdown Timer</strong> – Countdown to automatic shutdown (only when relays are active)</li>
      </ul>
    </div>

    <div class='section'>
      <h3>⏱️ Shutdown Timer</h3>
      <p>When the system is active, a countdown timer shows time remaining until automatic shutdown:</p>
      <ul>
        <li><strong>Green</strong> – More than 30 seconds remaining</li>
        <li><strong>Yellow</strong> – 10-30 seconds remaining</li>
        <li><strong>Red (pulsing)</strong> – Less than 10 seconds remaining</li>
      </ul>
      <p>Timer resets when audio is detected or Trigger button is pressed.</p>
    </div>

    <div class='section'>
      <h3>⚙️ Configuration Parameters</h3>
      <ul>
        <li><code>Hold time</code> – Time (seconds) to keep power ON after audio signal disappears</li>
        <li><code>Min voltage</code> – Minimum battery voltage threshold for operation</li>
        <li><code>Audio threshold</code> – Audio signal detection sensitivity</li>
        <li><code>Fan start</code> – Temperature to start cooling fan</li>
        <li><code>Warning temp</code> – Temperature for overheat warning</li>
        <li><code>Critical temp</code> – Temperature triggering emergency shutdown</li>
        <li><code>Cool stop</code> – Temperature to stop post-shutdown cooling</li>
        <li><code>Relay delay</code> – Delay between power-on and amplifier enable</li>
      </ul>
    </div>

    <div class='section'>
      <h3>📟 Console Logs</h3>
      <p>Real-time system events monitoring:</p>
      <ul>
        <li><strong>STARTUP/SHUTDOWN</strong> – System power sequences</li>
        <li><strong>SAVE</strong> – Configuration changes saved</li>
        <li><strong>TRIGGER RELAYS</strong> – Manual relay activation</li>
        
        <li><strong>AUDIO</strong> – Audio signal detection events</li>
        <li><strong>TEMPERATURE</strong> – Temperature warnings and cooling</li>
        <li><strong>BATTERY</strong> – Low voltage warnings</li>
        <li><strong>BUTTON</strong> – Physical button interactions</li>
      </ul>
    </div>

    <div class='section'>
      <h3>🔧 Controls</h3>
      <ul>
        <li><strong>Trigger (Tap)</strong> – Manually activate relays and reset timer</li>
        <li><strong>Trigger (Hold 3s)</strong> – Activate relays when system is OFF</li>
        <li><strong>Trigger (Hold 5s)</strong> – Force immediate shutdown when system is ON</li>
        <li><strong>Save Configuration</strong> – Save all configuration changes</li>
        <li><strong>Reset to Factory</strong> – Restore factory default settings</li>
        <li><strong>Restart</strong> – Reboot the ESP32 device</li>
      </ul>
    </div>

    <div class='section'>
      <h3>📱 Mobile Features</h3>
      <ul>
        <li>Collapsible sections to save screen space</li>
        <li>Touch-optimized buttons and controls</li>
        <li>Responsive grid layout for all screen sizes</li>
        <li>Real-time status updates every 0.4 seconds</li>
        <li>Visual countdown timer with color coding</li>
        <li>Ripple effects on button presses</li>
      </ul>
    </div>

    <a class='btn-back' href='/'>← Back to Controller</a>
  </div>
</body></html>
)rawliteral";
  server.send(200, "text/html", html);
}

void SubwooferWebServer::handleFactory() {
  config->resetToDefaults();
  config->saveSettings();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void SubwooferWebServer::handleRestart() {
  logger->addLog("RESTART", "warning", "Restart zainicjowany przez interfejs web");
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html><html><head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <meta http-equiv='refresh' content='5;url=/' />
  <style>
    body { 
      background: #0a0a0a; color: #fff; 
      font-family: -apple-system, BlinkMacSystemFont, sans-serif;
      display: flex; align-items: center; justify-content: center;
      min-height: 100vh; margin: 0; text-align: center;
    }
    .restart-container {
      background: #1a1a1a; padding: 40px; 
      border-radius: 12px; border: 1px solid #333;
      max-width: 400px;
    }
    h2 { color: #00d4ff; margin-bottom: 15px; }
    .spinner {
      width: 40px; height: 40px; margin: 20px auto;
      border: 4px solid #333; border-top: 4px solid #00d4ff;
      border-radius: 50%; animation: spin 1s linear infinite;
    }
    @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
  </style>
</head>
<body>
  <div class='restart-container'>
    <h2>🔄 Restarting Device...</h2>
    <div class='spinner'></div>
    <p>Please wait while the ESP32 restarts.</p>
    <p>You will be redirected automatically in 5 seconds.</p>
  </div>
</body></html>
)rawliteral");
  delay(1000);
  ESP.restart();
}
