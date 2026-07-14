#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_system.h>
#include <esp_wifi.h>

namespace {

constexpr char FIRMWARE_NAME[] = "testcode1";
constexpr char FIRMWARE_VERSION[] = "pump-control-v4";

constexpr uint8_t PIN_MOISTURE_ADC = 1;
constexpr uint8_t PIN_PUMP_GATE = 4;
constexpr uint8_t PIN_RESERVOIR_SW = 13;
constexpr uint8_t PIN_FLOW_PULSE = 14;
constexpr uint8_t PIN_ERROR_LED = 15;
constexpr uint8_t PIN_STATUS_LED = 16;

constexpr bool LED_ON = HIGH;
constexpr bool LED_OFF = LOW;
constexpr bool PUMP_OFF = LOW;
constexpr bool PUMP_ON = HIGH;

const char *AP_SSID = "FlowerPot-testcode1";
const char *AP_PASSWORD = "flowerpot1";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);
constexpr uint16_t DNS_PORT = 53;

constexpr uint32_t MOISTURE_SAMPLE_INTERVAL_MS = 250;
constexpr uint8_t MOISTURE_AVG_WINDOW = 16;
constexpr uint16_t ADC_MAX_VALUE = 4095;

constexpr uint32_t PUMP_MIN_RUNTIME_MS = 100;
constexpr uint32_t PUMP_MAX_RUNTIME_MS = 10000;
constexpr char PUMP_CONFIRM_TOKEN[] = "pump-test";
constexpr uint32_t USB_UI_ANNOUNCE_INTERVAL_MS = 5000;
constexpr uint32_t USB_UI_FAST_ANNOUNCE_UNTIL_MS = 30000;
constexpr uint32_t HISTORY_SAMPLE_INTERVAL_MS = 10000;
constexpr uint16_t HISTORY_CAPACITY = 360;

WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

String savedStaSsid = "";
String savedStaPassword = "";
uint32_t staConnectStartedMs = 0;

volatile uint32_t flowPulseCount = 0;
volatile uint32_t apConnectEvents = 0;
volatile uint32_t apDisconnectEvents = 0;

uint16_t moistureWindow[MOISTURE_AVG_WINDOW] = {};
uint8_t moistureWindowIndex = 0;
uint8_t moistureWindowCount = 0;
uint32_t moistureWindowTotal = 0;
uint32_t moistureSampleCount = 0;
uint16_t moistureRaw = 0;
uint16_t moistureMin = ADC_MAX_VALUE;
uint16_t moistureMax = 0;
uint32_t lastMoistureSampleMs = 0;

uint32_t ledTestUntilMs = 0;
String ledTestMode = "";
uint32_t reservoirLowStartedMs = 0;
uint32_t ledFlashCount = 0;
uint32_t redLedFlashCount = 0;
uint32_t greenLedFlashCount = 0;
uint32_t bothLedFlashCount = 0;

bool pumpRunning = false;
uint32_t pumpStartedMs = 0;
uint32_t pumpUntilMs = 0;
uint32_t pumpPulseCount = 0;
uint32_t pumpAttemptCount = 0;
uint32_t pumpRejectedAuthCount = 0;
uint32_t pumpRejectedBusyCount = 0;
uint32_t pumpCompletedCount = 0;
uint32_t lastPumpRequestedMs = 0;
uint32_t lastPumpActualMs = 0;
String lastPumpStopReason = "never_started";
uint32_t exportCount = 0;
uint8_t maxApClients = 0;

bool preferencesReady = false;
uint32_t bootCount = 0;
esp_reset_reason_t bootResetReason = ESP_RST_UNKNOWN;
bool previousBootPumpActive = false;
uint32_t pumpInterruptedBootCount = 0;
uint32_t previousPumpRequestedMs = 0;
uint32_t previousPumpBootCount = 0;
uint32_t loopCount = 0;
uint32_t maxLoopGapMs = 0;
uint32_t lastLoopStartedMs = 0;

struct HistoryPoint {
  uint32_t uptimeMs;
  uint16_t raw;
  uint16_t avg;
  uint8_t clients;
  bool pump;
};

HistoryPoint history[HISTORY_CAPACITY] = {};
uint16_t historyWriteIndex = 0;
uint16_t historyCount = 0;
uint32_t lastHistorySampleMs = 0;

String serialLine = "";
bool serialWasConnected = false;
uint32_t lastSerialUiAnnounceMs = 0;

String statusJson();
String historyJson();
String exportJson();
String diagnosticsJson();
void addHistoryPoint();

void IRAM_ATTR onFlowPulse() {
  flowPulseCount++;
}

void markPumpNvsActive(uint32_t requestedMs) {
  if (!preferencesReady) {
    return;
  }
  preferences.putBool("pump_active", true);
  preferences.putUInt("pump_req_ms", requestedMs);
  preferences.putUInt("pump_boot", bootCount);
}

void clearPumpNvsActive() {
  if (!preferencesReady) {
    return;
  }
  preferences.putBool("pump_active", false);
}

void setPumpOff(const String &reason = "forced_off") {
  const bool wasRunning = pumpRunning;
  const uint32_t now = millis();
  if (wasRunning) {
    lastPumpActualMs = now - pumpStartedMs;
    lastPumpStopReason = reason;
    if (reason == "runtime_elapsed") {
      pumpCompletedCount++;
    }
  }
  pumpRunning = false;
  pumpStartedMs = 0;
  pumpUntilMs = 0;
  digitalWrite(PIN_PUMP_GATE, PUMP_OFF);
  if (wasRunning) {
    clearPumpNvsActive();
    addHistoryPoint();
  }
}

void servicePumpSafety() {
  const uint32_t now = millis();
  if (pumpRunning && static_cast<int32_t>(now - pumpUntilMs) >= 0) {
    setPumpOff("runtime_elapsed");
    return;
  }
  digitalWrite(PIN_PUMP_GATE, pumpRunning ? PUMP_ON : PUMP_OFF);
}

uint32_t pumpRemainingMs() {
  if (!pumpRunning) {
    return 0;
  }
  const uint32_t now = millis();
  if (static_cast<int32_t>(pumpUntilMs - now) <= 0) {
    return 0;
  }
  return pumpUntilMs - now;
}

bool startPumpPulse(uint32_t requestedMs) {
  servicePumpSafety();
  if (pumpRunning) {
    pumpRejectedBusyCount++;
    return false;
  }
  if (requestedMs > PUMP_MAX_RUNTIME_MS) {
    requestedMs = PUMP_MAX_RUNTIME_MS;
  } else if (requestedMs < PUMP_MIN_RUNTIME_MS) {
    requestedMs = PUMP_MIN_RUNTIME_MS;
  }

  const uint32_t now = millis();
  lastPumpRequestedMs = requestedMs;
  lastPumpActualMs = 0;
  lastPumpStopReason = "running";
  markPumpNvsActive(requestedMs);
  pumpRunning = true;
  pumpStartedMs = now;
  pumpUntilMs = now + requestedMs;
  pumpPulseCount++;
  digitalWrite(PIN_PUMP_GATE, PUMP_ON);
  addHistoryPoint();
  return true;
}

uint32_t readFlowPulseCount() {
  noInterrupts();
  const uint32_t count = flowPulseCount;
  interrupts();
  return count;
}

void resetFlowPulseCount() {
  noInterrupts();
  flowPulseCount = 0;
  interrupts();
}

uint32_t readApConnectEvents() {
  noInterrupts();
  const uint32_t count = apConnectEvents;
  interrupts();
  return count;
}

uint32_t readApDisconnectEvents() {
  noInterrupts();
  const uint32_t count = apDisconnectEvents;
  interrupts();
  return count;
}

uint8_t currentApClients() {
  return WiFi.softAPgetStationNum();
}

void updateMaxApClients() {
  const uint8_t clients = currentApClients();
  if (clients > maxApClients) {
    maxApClients = clients;
  }
}

void onWiFiEvent(arduino_event_id_t event) {
  if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
    apConnectEvents++;
    updateMaxApClients();
  } else if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
    apDisconnectEvents++;
    updateMaxApClients();
  }
}

String jsonEscape(const String &value) {
  String out;
  out.reserve(value.length() + 4);
  for (size_t i = 0; i < value.length(); i++) {
    const char c = value[i];
    if (c == '\\') out += F("\\\\");
    else if (c == '"') out += F("\\\"");
    else if (c == '\n') out += F("\\n");
    else if (c == '\r') out += F("\\r");
    else out += c;
  }
  return out;
}

String resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:
      return "power_on";
    case ESP_RST_EXT:
      return "external_reset";
    case ESP_RST_SW:
      return "software_reset";
    case ESP_RST_PANIC:
      return "panic_exception";
    case ESP_RST_INT_WDT:
      return "interrupt_watchdog";
    case ESP_RST_TASK_WDT:
      return "task_watchdog";
    case ESP_RST_WDT:
      return "other_watchdog";
    case ESP_RST_DEEPSLEEP:
      return "deep_sleep";
    case ESP_RST_BROWNOUT:
      return "brownout";
    case ESP_RST_SDIO:
      return "sdio";
    default:
      return "unknown";
  }
}

String flashModeName(FlashMode_t mode) {
  switch (mode) {
    case FM_QIO:
      return "qio";
    case FM_QOUT:
      return "qout";
    case FM_DIO:
      return "dio";
    case FM_DOUT:
      return "dout";
    default:
      return "unknown";
  }
}

float safeTemperatureC() {
  const float temp = temperatureRead();
  if (isnan(temp) || temp < -80.0f || temp > 160.0f) {
    return NAN;
  }
  return temp;
}

void resetMoistureStats() {
  moistureWindowIndex = 0;
  moistureWindowCount = 0;
  moistureWindowTotal = 0;
  moistureSampleCount = 0;
  moistureRaw = 0;
  moistureMin = ADC_MAX_VALUE;
  moistureMax = 0;
  for (uint8_t i = 0; i < MOISTURE_AVG_WINDOW; i++) {
    moistureWindow[i] = 0;
  }
}

void addMoistureSample(const uint16_t raw) {
  moistureRaw = raw;

  if (moistureWindowCount < MOISTURE_AVG_WINDOW) {
    moistureWindow[moistureWindowIndex] = raw;
    moistureWindowTotal += raw;
    moistureWindowCount++;
  } else {
    moistureWindowTotal -= moistureWindow[moistureWindowIndex];
    moistureWindow[moistureWindowIndex] = raw;
    moistureWindowTotal += raw;
  }

  moistureWindowIndex = (moistureWindowIndex + 1) % MOISTURE_AVG_WINDOW;
  moistureSampleCount++;

  if (raw < moistureMin) moistureMin = raw;
  if (raw > moistureMax) moistureMax = raw;
}

void sampleMoistureNow() {
  addMoistureSample(static_cast<uint16_t>(analogRead(PIN_MOISTURE_ADC)));
}

void updateMoistureSampler() {
  const uint32_t now = millis();
  if (now - lastMoistureSampleMs < MOISTURE_SAMPLE_INTERVAL_MS) {
    return;
  }
  lastMoistureSampleMs = now;
  sampleMoistureNow();
}

uint16_t moistureAverage() {
  if (moistureWindowCount == 0) {
    return 0;
  }
  return static_cast<uint16_t>((moistureWindowTotal + moistureWindowCount / 2) / moistureWindowCount);
}

uint16_t moistureMinValue() {
  return moistureSampleCount == 0 ? 0 : moistureMin;
}

uint16_t moistureMaxValue() {
  return moistureSampleCount == 0 ? 0 : moistureMax;
}

uint16_t moistureSpan() {
  if (moistureSampleCount == 0) {
    return 0;
  }
  return moistureMax - moistureMin;
}

void resetHistory() {
  historyWriteIndex = 0;
  historyCount = 0;
  lastHistorySampleMs = 0;
  for (uint16_t i = 0; i < HISTORY_CAPACITY; i++) {
    history[i] = {};
  }
}

void addHistoryPoint() {
  updateMaxApClients();
  history[historyWriteIndex] = {
      millis(),
      moistureRaw,
      moistureAverage(),
      currentApClients(),
      pumpRunning,
  };
  historyWriteIndex = (historyWriteIndex + 1) % HISTORY_CAPACITY;
  if (historyCount < HISTORY_CAPACITY) {
    historyCount++;
  }
}

void updateHistorySampler() {
  const uint32_t now = millis();
  if (now - lastHistorySampleMs < HISTORY_SAMPLE_INTERVAL_MS) {
    return;
  }
  lastHistorySampleMs = now;
  addHistoryPoint();
}

void appendHistoryPoints(String &json) {
  json += F("[");
  for (uint16_t i = 0; i < historyCount; i++) {
    const uint16_t index = historyCount == HISTORY_CAPACITY
                               ? (historyWriteIndex + i) % HISTORY_CAPACITY
                               : i;
    const HistoryPoint &point = history[index];
    if (i > 0) {
      json += F(",");
    }
    json += F("{\"t\":");
    json += String(point.uptimeMs);
    json += F(",\"raw\":");
    json += String(point.raw);
    json += F(",\"avg\":");
    json += String(point.avg);
    json += F(",\"clients\":");
    json += String(point.clients);
    json += F(",\"pump\":");
    json += String(point.pump ? "true" : "false");
    json += F("}");
  }
  json += F("]");
}

String moistureBand() {
  if (moistureSampleCount == 0) {
    return "starting";
  }

  const uint16_t avg = moistureAverage();
  if (avg > 3200) return "very dry";
  if (avg >= 2600) return "dry-ish";
  if (avg >= 2000) return "moist";
  return "wet";
}

String staStatusText() {
  switch (WiFi.status()) {
    case WL_CONNECTED:
      return "connected";
    case WL_NO_SSID_AVAIL:
      return "ssid_not_found";
    case WL_CONNECT_FAILED:
      return "connect_failed";
    case WL_CONNECTION_LOST:
      return "connection_lost";
    case WL_DISCONNECTED:
      return savedStaSsid.length() ? "disconnected" : "not_configured";
    case WL_IDLE_STATUS:
      return "connecting";
    default:
      return "unknown";
  }
}

String apUiUrl() {
  return "http://" + WiFi.softAPIP().toString();
}

String staUiUrl() {
  return WiFi.status() == WL_CONNECTED ? "http://" + WiFi.localIP().toString() : "";
}

String primaryUiUrl() {
  const String staUrl = staUiUrl();
  return staUrl.length() ? staUrl : apUiUrl();
}

void printUsbUiBanner() {
  const String staStatus = staStatusText();
  const String staUrl = staUiUrl();

  Serial.println();
  Serial.println("FLOWERPOT_UI_BEGIN");
  Serial.print("NAME=");
  Serial.println(FIRMWARE_NAME);
  Serial.print("VERSION=");
  Serial.println(FIRMWARE_VERSION);
  Serial.print("UI_URL=");
  Serial.println(primaryUiUrl());
  Serial.print("AP_SSID=");
  Serial.println(AP_SSID);
  Serial.print("AP_PASSWORD=");
  Serial.println(AP_PASSWORD);
  Serial.print("AP_URL=");
  Serial.println(apUiUrl());
  Serial.print("STA_CONFIGURED=");
  Serial.println(savedStaSsid.length() ? "true" : "false");
  Serial.print("STA_SSID=");
  Serial.println(savedStaSsid);
  Serial.print("STA_STATUS=");
  Serial.println(staStatus);
  Serial.print("STA_URL=");
  Serial.println(staUrl);
  Serial.print("PUMP_MAX_RUNTIME_MS=");
  Serial.println(PUMP_MAX_RUNTIME_MS);
  Serial.print("RESET_REASON=");
  Serial.println(resetReasonName(bootResetReason));
  Serial.print("BOOT_COUNT=");
  Serial.println(bootCount);
  Serial.print("PREVIOUS_BOOT_PUMP_ACTIVE=");
  Serial.println(previousBootPumpActive ? "true" : "false");
  Serial.println("COMMANDS=ui,status,diag,help");
  Serial.println("FLOWERPOT_UI_END");
}

void printSerialHelp() {
  Serial.println("FLOWERPOT_HELP_BEGIN");
  Serial.println("ui     - print browser UI URL hints");
  Serial.println("status - print /api/status JSON");
  Serial.println("diag   - print /api/diagnostics JSON");
  Serial.println("help   - print this command list");
  Serial.println("FLOWERPOT_HELP_END");
}

void handleSerialCommand(String command) {
  command.trim();
  if (command.length() == 0) {
    return;
  }

  String lower = command;
  lower.toLowerCase();

  if (lower == "ui" || lower == "url" || lower == "open") {
    printUsbUiBanner();
  } else if (lower == "status") {
    Serial.println(statusJson());
  } else if (lower == "diag" || lower == "diagnostics") {
    Serial.println(diagnosticsJson());
  } else if (lower == "help" || lower == "?") {
    printSerialHelp();
  } else {
    Serial.print("ERR unknown command: ");
    Serial.println(command);
    printSerialHelp();
  }
}

void serviceSerialUi() {
  const bool serialConnected = Serial;
  const uint32_t now = millis();

  if (serialConnected && !serialWasConnected) {
    printUsbUiBanner();
    lastSerialUiAnnounceMs = now;
  }
  serialWasConnected = serialConnected;

  if (serialConnected && now < USB_UI_FAST_ANNOUNCE_UNTIL_MS &&
      now - lastSerialUiAnnounceMs >= USB_UI_ANNOUNCE_INTERVAL_MS) {
    printUsbUiBanner();
    lastSerialUiAnnounceMs = now;
  }

  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      handleSerialCommand(serialLine);
      serialLine = "";
    } else if (serialLine.length() < 96) {
      serialLine += c;
    } else {
      serialLine = "";
      Serial.println("ERR command too long");
    }
  }
}

void connectStaIfConfigured() {
  if (savedStaSsid.length() == 0) {
    return;
  }
  WiFi.begin(savedStaSsid.c_str(), savedStaPassword.c_str());
  staConnectStartedMs = millis();
}

void loadWifiSettings() {
  preferences.begin("flowerpot", false);
  preferencesReady = true;
  savedStaSsid = preferences.getString("sta_ssid", "");
  savedStaPassword = preferences.getString("sta_pass", "");
  bootCount = preferences.getUInt("boot_count", 0) + 1;
  preferences.putUInt("boot_count", bootCount);
  previousBootPumpActive = preferences.getBool("pump_active", false);
  previousPumpRequestedMs = preferences.getUInt("pump_req_ms", 0);
  previousPumpBootCount = preferences.getUInt("pump_boot", 0);
  pumpInterruptedBootCount = preferences.getUInt("pump_intr", 0);
  if (previousBootPumpActive) {
    pumpInterruptedBootCount++;
    preferences.putUInt("pump_intr", pumpInterruptedBootCount);
    preferences.putBool("pump_active", false);
  }
}

void updateStaReconnect() {
  if (savedStaSsid.length() == 0 || WiFi.status() == WL_CONNECTED) {
    return;
  }

  const uint32_t now = millis();
  if (staConnectStartedMs == 0 || now - staConnectStartedMs > 30000) {
    connectStaIfConfigured();
  }
}

String statusJson() {
  servicePumpSafety();
  const bool reservoirLow = digitalRead(PIN_RESERVOIR_SW) == LOW;
  const bool flowLow = digitalRead(PIN_FLOW_PULSE) == LOW;
  const uint32_t now = millis();
  const uint32_t flowCount = readFlowPulseCount();
  const uint8_t apClients = currentApClients();
  updateMaxApClients();
  const String band = moistureBand();
  const String staStatus = staStatusText();

  String json = "{";
  json += "\"name\":\"" + String(FIRMWARE_NAME) + "\",";
  json += "\"version\":\"" + String(FIRMWARE_VERSION) + "\",";
  json += "\"uptime_ms\":" + String(now) + ",";
  json += "\"boot_count\":" + String(bootCount) + ",";
  json += "\"reset_reason\":\"" + jsonEscape(resetReasonName(bootResetReason)) + "\",";
  json += "\"previous_boot_pump_active\":" + String(previousBootPumpActive ? "true" : "false") + ",";
  json += "\"pump_interrupted_boots\":" + String(pumpInterruptedBootCount) + ",";
  json += "\"mac\":\"" + WiFi.macAddress() + "\",";
  json += "\"ap_ssid\":\"" + String(AP_SSID) + "\",";
  json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"ap_clients\":" + String(apClients) + ",";
  json += "\"clients\":" + String(apClients) + ",";
  json += "\"ap_connect_events\":" + String(readApConnectEvents()) + ",";
  json += "\"ap_disconnect_events\":" + String(readApDisconnectEvents()) + ",";
  json += "\"ap_max_clients\":" + String(maxApClients) + ",";
  json += "\"sta_configured\":" + String(savedStaSsid.length() ? "true" : "false") + ",";
  json += "\"sta_ssid\":\"" + jsonEscape(savedStaSsid) + "\",";
  json += "\"sta_status\":\"" + jsonEscape(staStatus) + "\",";
  json += "\"sta_ip\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String("")) + "\",";
  json += "\"moisture_adc_raw\":" + String(moistureRaw) + ",";
  json += "\"moisture_adc_avg\":" + String(moistureAverage()) + ",";
  json += "\"moisture_adc_min\":" + String(moistureMinValue()) + ",";
  json += "\"moisture_adc_max\":" + String(moistureMaxValue()) + ",";
  json += "\"moisture_adc_span\":" + String(moistureSpan()) + ",";
  json += "\"moisture_band\":\"" + jsonEscape(band) + "\",";
  json += "\"samples\":" + String(moistureSampleCount) + ",";
  json += "\"reservoir_sw_low\":" + String(reservoirLow ? "true" : "false") + ",";
  json += "\"flow_input_low\":" + String(flowLow ? "true" : "false") + ",";
  json += "\"flow_pulses\":" + String(flowCount) + ",";
  json += "\"pump\":\"" + String(pumpRunning ? "running" : "ready_limited_10s") + "\",";
  json += "\"pump_running\":" + String(pumpRunning ? "true" : "false") + ",";
  json += "\"pump_remaining_ms\":" + String(pumpRemainingMs()) + ",";
  json += "\"pump_max_runtime_ms\":" + String(PUMP_MAX_RUNTIME_MS) + ",";
  json += "\"pump_pulses\":" + String(pumpPulseCount) + ",";
  json += "\"pump_attempts\":" + String(pumpAttemptCount) + ",";
  json += "\"pump_completed\":" + String(pumpCompletedCount) + ",";
  json += "\"pump_rejected_busy\":" + String(pumpRejectedBusyCount) + ",";
  json += "\"pump_rejected_auth\":" + String(pumpRejectedAuthCount) + ",";
  json += "\"last_pump_requested_ms\":" + String(lastPumpRequestedMs) + ",";
  json += "\"last_pump_actual_ms\":" + String(lastPumpActualMs) + ",";
  json += "\"last_pump_stop_reason\":\"" + jsonEscape(lastPumpStopReason) + "\",";
  json += "\"led_test\":\"" + jsonEscape(ledTestMode) + "\",";
  json += "\"led_test_remaining_ms\":" + String(ledTestUntilMs > now ? ledTestUntilMs - now : 0) + ",";
  json += "\"led_flash_count\":" + String(ledFlashCount) + ",";
  json += "\"led_flash_red\":" + String(redLedFlashCount) + ",";
  json += "\"led_flash_green\":" + String(greenLedFlashCount) + ",";
  json += "\"led_flash_both\":" + String(bothLedFlashCount) + ",";
  json += "\"history_count\":" + String(historyCount) + ",";
  json += "\"history_capacity\":" + String(HISTORY_CAPACITY) + ",";
  json += "\"history_interval_ms\":" + String(HISTORY_SAMPLE_INTERVAL_MS) + ",";
  json += "\"export_count\":" + String(exportCount);
  json += "}";
  return json;
}

String historyJson() {
  String json;
  json.reserve(256 + static_cast<uint32_t>(historyCount) * 72);
  json += F("{\"interval_ms\":");
  json += String(HISTORY_SAMPLE_INTERVAL_MS);
  json += F(",\"capacity\":");
  json += String(HISTORY_CAPACITY);
  json += F(",\"count\":");
  json += String(historyCount);
  json += F(",\"points\":");
  appendHistoryPoints(json);
  json += F("}");
  return json;
}

String exportJson() {
  updateMaxApClients();
  String json;
  json.reserve(1024 + static_cast<uint32_t>(historyCount) * 80);
  json += F("{\"name\":\"");
  json += FIRMWARE_NAME;
  json += F("\",\"version\":\"");
  json += FIRMWARE_VERSION;
  json += F("\",\"generated_uptime_ms\":");
  json += String(millis());
  json += F(",\"ap_ssid\":\"");
  json += AP_SSID;
  json += F("\",\"ap_ip\":\"");
  json += WiFi.softAPIP().toString();
  json += F("\",\"sta_configured\":");
  json += String(savedStaSsid.length() ? "true" : "false");
  json += F(",\"sta_ssid\":\"");
  json += jsonEscape(savedStaSsid);
  json += F("\",\"sta_status\":\"");
  json += jsonEscape(staStatusText());
  json += F("\",\"sta_ip\":\"");
  json += (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String(""));
  json += F("\",\"summary\":{");
  json += F("\"moisture_adc_raw\":");
  json += String(moistureRaw);
  json += F(",\"moisture_adc_avg\":");
  json += String(moistureAverage());
  json += F(",\"moisture_adc_min\":");
  json += String(moistureMinValue());
  json += F(",\"moisture_adc_max\":");
  json += String(moistureMaxValue());
  json += F(",\"moisture_adc_span\":");
  json += String(moistureSpan());
  json += F(",\"moisture_band\":\"");
  json += jsonEscape(moistureBand());
  json += F("\",\"samples\":");
  json += String(moistureSampleCount);
  json += F(",\"flow_pulses\":");
  json += String(readFlowPulseCount());
  json += F(",\"pump_pulses\":");
  json += String(pumpPulseCount);
  json += F(",\"pump_max_runtime_ms\":");
  json += String(PUMP_MAX_RUNTIME_MS);
  json += F(",\"led_flash_count\":");
  json += String(ledFlashCount);
  json += F(",\"led_flash_red\":");
  json += String(redLedFlashCount);
  json += F(",\"led_flash_green\":");
  json += String(greenLedFlashCount);
  json += F(",\"led_flash_both\":");
  json += String(bothLedFlashCount);
  json += F(",\"ap_connect_events\":");
  json += String(readApConnectEvents());
  json += F(",\"ap_disconnect_events\":");
  json += String(readApDisconnectEvents());
  json += F(",\"ap_max_clients\":");
  json += String(maxApClients);
  json += F(",\"export_count\":");
  json += String(exportCount);
  json += F("},\"history\":{\"interval_ms\":");
  json += String(HISTORY_SAMPLE_INTERVAL_MS);
  json += F(",\"capacity\":");
  json += String(HISTORY_CAPACITY);
  json += F(",\"count\":");
  json += String(historyCount);
  json += F(",\"points\":");
  appendHistoryPoints(json);
  json += F("}}");
  return json;
}

String diagnosticsJson() {
  servicePumpSafety();
  updateMaxApClients();
  const uint32_t now = millis();
  const float tempC = safeTemperatureC();
  const bool staConnected = WiFi.status() == WL_CONNECTED;

  String json;
  json.reserve(5200);
  json += F("{\"name\":\"");
  json += FIRMWARE_NAME;
  json += F("\",\"version\":\"");
  json += FIRMWARE_VERSION;
  json += F("\",\"uptime_ms\":");
  json += String(now);
  json += F(",\"boot\":{\"count\":");
  json += String(bootCount);
  json += F(",\"reset_reason_code\":");
  json += String(static_cast<int>(bootResetReason));
  json += F(",\"reset_reason\":\"");
  json += jsonEscape(resetReasonName(bootResetReason));
  json += F("\",\"previous_boot_pump_active\":");
  json += String(previousBootPumpActive ? "true" : "false");
  json += F(",\"pump_interrupted_boots\":");
  json += String(pumpInterruptedBootCount);
  json += F(",\"previous_pump_requested_ms\":");
  json += String(previousPumpRequestedMs);
  json += F(",\"previous_pump_boot_count\":");
  json += String(previousPumpBootCount);
  json += F("},\"power\":{\"rail_voltage_measurement\":\"not_available_on_this_pcb\",\"note\":\"Use TP1-TP2 for 5V and TP3-TP2 for 3V3 with a multimeter.\"}");
  json += F(",\"chip\":{\"model\":\"");
  json += ESP.getChipModel();
  json += F("\",\"revision\":");
  json += String(ESP.getChipRevision());
  json += F(",\"cores\":");
  json += String(ESP.getChipCores());
  json += F(",\"cpu_mhz\":");
  json += String(ESP.getCpuFreqMHz());
  json += F(",\"sdk\":\"");
  json += ESP.getSdkVersion();
  json += F("\",\"temperature_c\":");
  json += isnan(tempC) ? String("null") : String(tempC, 1);
  json += F("},\"memory\":{\"heap_size\":");
  json += String(ESP.getHeapSize());
  json += F(",\"free_heap\":");
  json += String(ESP.getFreeHeap());
  json += F(",\"min_free_heap\":");
  json += String(ESP.getMinFreeHeap());
  json += F(",\"max_alloc_heap\":");
  json += String(ESP.getMaxAllocHeap());
  json += F(",\"psram_size\":");
  json += String(ESP.getPsramSize());
  json += F(",\"free_psram\":");
  json += String(ESP.getFreePsram());
  json += F("},\"flash\":{\"chip_size\":");
  json += String(ESP.getFlashChipSize());
  json += F(",\"chip_speed\":");
  json += String(ESP.getFlashChipSpeed());
  json += F(",\"chip_mode\":\"");
  json += flashModeName(ESP.getFlashChipMode());
  json += F("\",\"sketch_size\":");
  json += String(ESP.getSketchSize());
  json += F(",\"free_sketch_space\":");
  json += String(ESP.getFreeSketchSpace());
  json += F("},\"wifi\":{\"ap_ssid\":\"");
  json += AP_SSID;
  json += F("\",\"ap_ip\":\"");
  json += WiFi.softAPIP().toString();
  json += F("\",\"ap_mac\":\"");
  json += WiFi.softAPmacAddress();
  json += F("\",\"ap_clients\":");
  json += String(currentApClients());
  json += F(",\"ap_max_clients\":");
  json += String(maxApClients);
  json += F(",\"ap_connect_events\":");
  json += String(readApConnectEvents());
  json += F(",\"ap_disconnect_events\":");
  json += String(readApDisconnectEvents());
  json += F(",\"sta_configured\":");
  json += String(savedStaSsid.length() ? "true" : "false");
  json += F(",\"sta_ssid\":\"");
  json += jsonEscape(savedStaSsid);
  json += F("\",\"sta_status\":\"");
  json += jsonEscape(staStatusText());
  json += F("\",\"sta_ip\":\"");
  json += staConnected ? WiFi.localIP().toString() : String("");
  json += F("\",\"sta_rssi\":");
  json += staConnected ? String(WiFi.RSSI()) : String("null");
  json += F(",\"channel\":");
  json += String(WiFi.channel());
  json += F("},\"gpio\":{\"pump_gate_gpio\":");
  json += String(PIN_PUMP_GATE);
  json += F(",\"pump_gate_level\":");
  json += String(digitalRead(PIN_PUMP_GATE));
  json += F(",\"reservoir_gpio\":");
  json += String(PIN_RESERVOIR_SW);
  json += F(",\"reservoir_low\":");
  json += String(digitalRead(PIN_RESERVOIR_SW) == LOW ? "true" : "false");
  json += F(",\"flow_gpio\":");
  json += String(PIN_FLOW_PULSE);
  json += F(",\"flow_low\":");
  json += String(digitalRead(PIN_FLOW_PULSE) == LOW ? "true" : "false");
  json += F(",\"moisture_adc_gpio\":");
  json += String(PIN_MOISTURE_ADC);
  json += F(",\"error_led_gpio\":");
  json += String(PIN_ERROR_LED);
  json += F(",\"status_led_gpio\":");
  json += String(PIN_STATUS_LED);
  json += F("},\"moisture\":{\"raw\":");
  json += String(moistureRaw);
  json += F(",\"avg\":");
  json += String(moistureAverage());
  json += F(",\"min\":");
  json += String(moistureMinValue());
  json += F(",\"max\":");
  json += String(moistureMaxValue());
  json += F(",\"span\":");
  json += String(moistureSpan());
  json += F(",\"band\":\"");
  json += jsonEscape(moistureBand());
  json += F("\",\"samples\":");
  json += String(moistureSampleCount);
  json += F(",\"sample_interval_ms\":");
  json += String(MOISTURE_SAMPLE_INTERVAL_MS);
  json += F("},\"pump\":{\"running\":");
  json += String(pumpRunning ? "true" : "false");
  json += F(",\"remaining_ms\":");
  json += String(pumpRemainingMs());
  json += F(",\"max_runtime_ms\":");
  json += String(PUMP_MAX_RUNTIME_MS);
  json += F(",\"pulses_started\":");
  json += String(pumpPulseCount);
  json += F(",\"attempts\":");
  json += String(pumpAttemptCount);
  json += F(",\"completed\":");
  json += String(pumpCompletedCount);
  json += F(",\"rejected_busy\":");
  json += String(pumpRejectedBusyCount);
  json += F(",\"rejected_auth\":");
  json += String(pumpRejectedAuthCount);
  json += F(",\"last_requested_ms\":");
  json += String(lastPumpRequestedMs);
  json += F(",\"last_actual_ms\":");
  json += String(lastPumpActualMs);
  json += F(",\"last_stop_reason\":\"");
  json += jsonEscape(lastPumpStopReason);
  json += F("\"},\"runtime\":{\"loop_count\":");
  json += String(loopCount);
  json += F(",\"max_loop_gap_ms\":");
  json += String(maxLoopGapMs);
  json += F(",\"history_count\":");
  json += String(historyCount);
  json += F(",\"history_capacity\":");
  json += String(HISTORY_CAPACITY);
  json += F(",\"export_count\":");
  json += String(exportCount);
  json += F("}}");
  return json;
}

String pageHtml() {
  const String mac = WiFi.macAddress();
  String html;
  html.reserve(30000);
  html += R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Flower Pot testcode1</title>
<style>
:root{color-scheme:light;--ink:#172018;--muted:#5d675b;--line:#d7ddd2;--panel:#fff;--bg:#f6f7f2;--green:#245b3b;--red:#9f2929;--amber:#b36a22;--blue:#28628f}
*{box-sizing:border-box}body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;margin:0;background:var(--bg);color:var(--ink)}main{max-width:1120px;margin:0 auto;padding:16px}
header{display:flex;align-items:flex-start;justify-content:space-between;gap:12px;margin-bottom:12px}h1{font-size:26px;line-height:1.1;margin:0 0 6px}h2{font-size:18px;margin:0 0 10px}.sub{color:var(--muted);font-size:14px}.pill{display:inline-flex;align-items:center;border:1px solid var(--line);border-radius:999px;background:#fff;padding:6px 10px;font-size:13px;color:var(--muted);white-space:nowrap}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;margin:10px 0}.card,.panel,.chart-box{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:12px}.card{min-height:82px}.k{font-size:12px;color:#66705f;text-transform:uppercase;letter-spacing:.02em}.v{font-size:24px;font-weight:750;word-break:break-word}.small{font-size:14px;color:#53604f}.tiny{font-size:12px;color:#66705f}
.panel{margin:12px 0}.warn{border-left:5px solid #b93a32;background:#fff;padding:12px;margin:12px 0}.ok{border-left:5px solid #27733e;background:#fff;padding:12px;margin:12px 0}
button{appearance:none;border:0;border-radius:6px;padding:12px 14px;margin:4px 5px 4px 0;font-weight:750;background:var(--green);color:white;min-height:42px}button.red{background:var(--red)}button.green{background:#27733e}button.gray{background:#555}button.pump{background:#b33a22}button.blue{background:var(--blue)}button:disabled{opacity:.55}.button-row{display:flex;flex-wrap:wrap;gap:4px;align-items:center}
label{font-weight:650}.control-grid{display:grid;grid-template-columns:minmax(220px,1fr) auto;gap:10px;align-items:end}.duration-row{display:grid;grid-template-columns:1fr 112px;gap:10px;align-items:center}input{box-sizing:border-box;width:100%;border:1px solid #b8c1b3;border-radius:6px;padding:10px;margin:4px 0 8px;font:inherit}input[type=range]{padding:0;accent-color:#245b3b}
.band{display:inline-block;border-radius:999px;padding:4px 10px;background:#e8ece1;font-size:18px}.wet{background:#cfe8ff}.moist{background:#d8efcf}.dry-ish{background:#fff0bf}.very-dry{background:#ffd7c7}.starting{background:#ecefe8}
details.panel{padding:0}summary{cursor:pointer;list-style:none;padding:12px;font-weight:800;display:flex;justify-content:space-between;gap:12px;align-items:center}summary::-webkit-details-marker{display:none}summary:after{content:"v";font-size:14px;color:var(--muted)}details[open]>summary:after{content:"^"}.details-body{padding:0 12px 12px}
.chart-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(270px,1fr));gap:10px}.chart-title{font-weight:800;margin-bottom:4px}canvas{display:block;width:100%;height:190px;border-radius:6px;background:#fff}pre{white-space:pre-wrap;background:#172018;color:#e9f0e5;padding:12px;border-radius:8px;overflow:auto;font-size:13px}code{background:#ecefe8;padding:2px 5px;border-radius:4px}
@media(max-width:620px){main{padding:12px}header{display:block}.pill{margin-top:6px}.v{font-size:21px}.control-grid,.duration-row{grid-template-columns:1fr}button{width:100%;margin-right:0}.button-row{display:block}.chart-grid{grid-template-columns:1fr}}
</style></head><body><main>
<header><div><h1>Flower Pot testcode1</h1><div class="sub">Pump-test firmware. AP always stays on. Manual pump runs are capped in firmware.</div></div><div class="pill" id="versionPill">...</div></header>
<div class="warn"><b>Pump unlocked:</b> use a dedicated 5 V supply or power bank, not a laptop USB port. Firmware caps each pump request at 10 seconds.</div>

<section class="panel">
<h2>Manual Controls</h2>
<div class="control-grid">
  <div>
    <label for="pumpDuration">Pump duration <span id="pumpDurationText">1000 ms</span></label>
    <div class="duration-row">
      <input id="pumpDuration" type="range" min="100" max="10000" step="100" value="1000">
      <input id="pumpDurationNumber" type="number" min="100" max="10000" step="100" value="1000">
    </div>
    <div class="tiny">Choose any pulse from 100 ms through 10 seconds. The firmware-enforced 10-second limit cannot be bypassed by the page.</div>
  </div>
  <div class="button-row"><button class="pump" id="pumpBtn" onclick="runPump()">Run pump</button></div>
</div>
<div class="button-row">
  <button class="red" onclick="flashLed('red')">Flash red LED 5s</button>
  <button class="green" onclick="flashLed('green')">Flash green LED 5s</button>
  <button onclick="flashLed('both')">Flash both 5s</button>
  <button class="gray" onclick="resetStats()">Reset stats</button>
  <button class="gray" onclick="refreshStatus()">Refresh</button>
</div>
</section>

<section class="grid">
  <div class="card"><div class="k">Pump</div><div class="v" id="pumpState">...</div><div class="small" id="pumpRemaining">...</div></div>
  <div class="card"><div class="k">Moisture raw</div><div class="v" id="moistureRaw">...</div></div>
  <div class="card"><div class="k">Moisture avg</div><div class="v" id="moistureAvg">...</div></div>
  <div class="card"><div class="k">Moisture band</div><div class="v"><span class="band starting" id="moistureBand">...</span></div></div>
  <div class="card"><div class="k">Min / max</div><div class="v" id="moistureRange">...</div><div class="small" id="moistureSpan">...</div></div>
  <div class="card"><div class="k">AP clients</div><div class="v" id="apClients">...</div><div class="small" id="apClientEvents">...</div></div>
</section>

<section class="grid">
  <div class="card"><div class="k">TP6 to TP7</div><div class="v" id="tp6State">...</div></div>
  <div class="card"><div class="k">TP10 to TP9</div><div class="v" id="tp10State">...</div></div>
  <div class="card"><div class="k">Flow pulses</div><div class="v" id="flowPulseState">...</div></div>
  <div class="card"><div class="k">Pump runs</div><div class="v" id="pumpPulseState">...</div></div>
  <div class="card"><div class="k">LED tests</div><div class="v" id="ledFlashState">...</div><div class="small" id="ledFlashDetails">...</div></div>
  <div class="card"><div class="k">Samples</div><div class="v" id="sampleCount">...</div></div>
</section>

<section class="grid">
  <div class="card"><div class="k">Reset reason</div><div class="v" id="resetReason">...</div><div class="small" id="bootState">...</div></div>
  <div class="card"><div class="k">Pump reset clue</div><div class="v" id="prevPumpState">...</div><div class="small" id="prevPumpDetails">...</div></div>
  <div class="card"><div class="k">Heap free</div><div class="v" id="heapState">...</div><div class="small" id="heapDetails">...</div></div>
  <div class="card"><div class="k">Chip temp</div><div class="v" id="tempState">...</div><div class="small" id="chipDetails">...</div></div>
  <div class="card"><div class="k">Loop health</div><div class="v" id="loopState">...</div><div class="small" id="loopDetails">...</div></div>
  <div class="card"><div class="k">Flash/sketch</div><div class="v" id="flashState">...</div><div class="small" id="flashDetails">...</div></div>
</section>

<details class="panel" open id="chartsPanel"><summary><span>Charts</span><span class="tiny" id="chartHint">loading history</span></summary>
<div class="details-body">
  <div class="chart-grid">
    <div class="chart-box"><div class="chart-title">Moisture ADC</div><canvas id="moistureChart"></canvas><div class="tiny">Higher is drier, lower is wetter. Green is rolling average.</div></div>
    <div class="chart-box"><div class="chart-title">AP Clients</div><canvas id="clientChart"></canvas><div class="tiny">Current clients connected to the board AP.</div></div>
    <div class="chart-box"><div class="chart-title">Session Events</div><canvas id="eventChart"></canvas><div class="tiny">Counters since boot or reset-stats.</div></div>
  </div>
</div></details>

<section class="panel">
<h2>Session Export</h2>
<div class="small">Downloads JSON with firmware version, Wi-Fi/client counters, pump/LED counters, moisture stats, and the device history buffer.</div>
<div class="button-row"><button class="blue" onclick="downloadExport()">Download session JSON</button><button class="gray" onclick="loadHistory()">Reload history</button></div>
</section>

<details class="panel" open><summary><span>Deep Diagnostics</span><span class="tiny">ESP/system dump</span></summary>
<div class="details-body">
  <div class="small">Firmware cannot directly measure the board rails on this PCB. Use TP1-TP2 for 5V and TP3-TP2 for 3V3.</div>
  <div class="button-row"><button class="gray" onclick="refreshDiagnostics()">Refresh diagnostics</button><button class="blue" onclick="downloadDiagnostics()">Download diagnostics JSON</button></div>
  <pre id="diagnostics">Loading...</pre>
</div></details>

<section class="grid">
  <div class="card"><div class="k">AP SSID</div><div class="v">)HTML";
  html += AP_SSID;
  html += R"HTML(</div><div class="small">AP URL: http://192.168.4.1</div></div>
  <div class="card"><div class="k">Home Wi-Fi</div><div class="v" id="staStatus">...</div><div class="small" id="staDetails">...</div></div>
  <div class="card"><div class="k">MAC</div><div class="v">)HTML";
  html += mac;
  html += R"HTML(</div></div>
</section>

<section class="panel"><h2>Settings</h2><form id="wifiForm" onsubmit="saveWifi(event)">
<label>Home Wi-Fi SSID<br><input name="ssid" id="ssidInput" autocomplete="off"></label><br>
<label>Password<br><input name="password" type="password" autocomplete="current-password"></label><br>
<div class="button-row"><button type="submit">Connect ESP to home Wi-Fi</button><button type="button" class="gray" onclick="clearWifi()">Clear saved Wi-Fi</button></div>
<div class="small" id="wifiMessage"></div></form></section>

<div class="ok">Diagnostic bands are rough: higher ADC means drier. Use real soil readings before choosing watering thresholds.</div>
<details class="panel"><summary><span>Status JSON</span><span class="tiny">raw API view</span></summary><div class="details-body"><pre id="status">Loading...</pre></div></details>

<script>
const MAX_POINTS=360;
let historyPoints=[];
let latestStatus=null;
let latestDiagnostics=null;
let lastStatusUptime=-1;
function $(id){return document.getElementById(id)}
function text(id,v){const e=$(id);if(e)e.textContent=v}
function yn(v){return v?'SHORTED':'open'}
function bandClass(v){return String(v||'starting').replace(/ /g,'-')}
function ms(v){return Math.max(0,Math.ceil(Number(v||0)))+' ms'}
function bytes(v){v=Number(v||0);if(v>1048576)return (v/1048576).toFixed(1)+' MB';if(v>1024)return (v/1024).toFixed(1)+' KB';return v+' B'}
function clamp(v,min,max){return Math.min(max,Math.max(min,Number.isFinite(v)?v:min))}
function pumpMax(){return Number(latestStatus&&latestStatus.pump_max_runtime_ms)||10000}
function selectedDuration(){const max=pumpMax();return Math.round(clamp(Number($('pumpDurationNumber').value),100,max))}
function syncPumpInputs(source){
  const max=pumpMax();
  const range=$('pumpDuration');
  const number=$('pumpDurationNumber');
  range.max=max;
  number.max=max;
  const raw=source==='range'?Number(range.value):Number(number.value);
  const value=clamp(raw,250,max);
  range.value=value;
  number.value=value;
  text('pumpDurationText',value+' ms');
}
function addLivePoint(s){
  const t=Number(s.uptime_ms||0);
  if(t===lastStatusUptime)return;
  lastStatusUptime=t;
  historyPoints.push({t:t,raw:Number(s.moisture_adc_raw||0),avg:Number(s.moisture_adc_avg||0),clients:Number(s.ap_clients||0),pump:!!s.pump_running});
  while(historyPoints.length>MAX_POINTS)historyPoints.shift();
}
async function loadHistory(){
  try{
    const r=await fetch('/api/history',{cache:'no-store'});
    const h=await r.json();
    if(Array.isArray(h.points)){
      historyPoints=h.points.slice(-MAX_POINTS);
      if(historyPoints.length){lastStatusUptime=historyPoints[historyPoints.length-1].t}
    }
    text('chartHint',historyPoints.length+' points');
    drawCharts();
  }catch(e){text('chartHint','history unavailable')}
}
async function refreshStatus(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    const s=await r.json();
    latestStatus=s;
    syncPumpInputs();
    text('versionPill',s.name+' '+s.version);
    text('pumpState',s.pump_running?'RUNNING':'ready');
    text('pumpRemaining',s.pump_running?('remaining '+ms(s.pump_remaining_ms)):('max '+ms(s.pump_max_runtime_ms)));
    $('pumpBtn').disabled=!!s.pump_running;
    text('moistureRaw',s.moisture_adc_raw);
    text('moistureAvg',s.moisture_adc_avg);
    const b=$('moistureBand');b.textContent=s.moisture_band;b.className='band '+bandClass(s.moisture_band);
    text('moistureRange',s.moisture_adc_min+' / '+s.moisture_adc_max);
    text('moistureSpan','span '+s.moisture_adc_span);
    text('apClients',s.ap_clients);
    text('apClientEvents','joins '+s.ap_connect_events+' / max '+s.ap_max_clients);
    text('tp6State',yn(s.reservoir_sw_low));
    text('tp10State',yn(s.flow_input_low));
    text('flowPulseState',s.flow_pulses);
    text('pumpPulseState',s.pump_pulses);
    text('ledFlashState',s.led_flash_count);
    text('ledFlashDetails','red '+s.led_flash_red+' / green '+s.led_flash_green+' / both '+s.led_flash_both);
    text('sampleCount',s.samples);
    text('staStatus',s.sta_status);
    text('staDetails',s.sta_configured?(s.sta_ssid+(s.sta_ip?' at '+s.sta_ip:'')):'not configured');
    if(!$('ssidInput').value&&s.sta_ssid){$('ssidInput').value=s.sta_ssid}
    text('status',JSON.stringify(s,null,2));
    addLivePoint(s);
    text('chartHint',historyPoints.length+' points');
    drawCharts();
  }catch(e){text('status','Status read failed: '+e)}
}
async function refreshDiagnostics(){
  try{
    const r=await fetch('/api/diagnostics',{cache:'no-store'});
    const d=await r.json();
    latestDiagnostics=d;
    text('resetReason',d.boot.reset_reason);
    text('bootState','boot '+d.boot.count+' / uptime '+Math.floor((d.uptime_ms||0)/1000)+'s');
    text('prevPumpState',d.boot.previous_boot_pump_active?'YES':'no');
    text('prevPumpDetails','interrupts '+d.boot.pump_interrupted_boots+' / prev req '+d.boot.previous_pump_requested_ms+' ms');
    text('heapState',bytes(d.memory.free_heap));
    text('heapDetails','min '+bytes(d.memory.min_free_heap)+' / max block '+bytes(d.memory.max_alloc_heap));
    text('tempState',d.chip.temperature_c===null?'n/a':(d.chip.temperature_c+' C'));
    text('chipDetails',d.chip.model+' rev '+d.chip.revision+' / '+d.chip.cpu_mhz+' MHz');
    text('loopState',d.runtime.max_loop_gap_ms+' ms');
    text('loopDetails','loops '+d.runtime.loop_count+' / history '+d.runtime.history_count+'/'+d.runtime.history_capacity);
    text('flashState',bytes(d.flash.sketch_size));
    text('flashDetails','free '+bytes(d.flash.free_sketch_space)+' / mode '+d.flash.chip_mode);
    text('diagnostics',JSON.stringify(d,null,2));
  }catch(e){text('diagnostics','Diagnostics read failed: '+e)}
}
async function flashLed(which){await fetch('/api/flash?led='+encodeURIComponent(which),{method:'POST'});refreshStatus()}
async function resetStats(){await fetch('/api/reset-stats',{method:'POST'});historyPoints=[];lastStatusUptime=-1;await loadHistory();refreshStatus()}
async function saveWifi(e){e.preventDefault();const fd=new FormData(e.target);const r=await fetch('/api/wifi',{method:'POST',body:new URLSearchParams(fd)});text('wifiMessage',await r.text());setTimeout(refreshStatus,1000)}
async function clearWifi(){const r=await fetch('/api/wifi/clear',{method:'POST'});text('wifiMessage',await r.text());refreshStatus()}
async function runPump(){
  const duration=selectedDuration();
  if(!localStorage.getItem('pumpWarned')){
    const ok=confirm('First pump test warning: use a dedicated 5 V supply, keep water away from the board, confirm MOSFET/diode orientation, and expect only the selected capped pulse. Run pump now?');
    if(!ok)return;
    localStorage.setItem('pumpWarned','yes');
  }
  const body=new URLSearchParams({duration_ms:String(duration),confirm:'pump-test'});
  const r=await fetch('/api/pump',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body});
  if(!r.ok){alert(await r.text())}
  refreshStatus();
}
function downloadExport(){window.location.href='/api/export';setTimeout(refreshStatus,1200)}
function downloadDiagnostics(){window.location.href='/api/diagnostics?download=1';setTimeout(refreshDiagnostics,1200)}
function prepCanvas(id){
  const canvas=$(id);if(!canvas)return null;
  const rect=canvas.getBoundingClientRect();
  const w=Math.max(260,Math.floor(rect.width||canvas.parentElement.clientWidth||300));
  const h=190;
  const dpr=window.devicePixelRatio||1;
  canvas.width=Math.floor(w*dpr);canvas.height=Math.floor(h*dpr);
  const ctx=canvas.getContext('2d');
  ctx.setTransform(dpr,0,0,dpr,0,0);
  ctx.clearRect(0,0,w,h);
  ctx.fillStyle='#fff';ctx.fillRect(0,0,w,h);
  ctx.strokeStyle='#d7ddd2';ctx.lineWidth=1;ctx.strokeRect(.5,.5,w-1,h-1);
  ctx.font='12px system-ui,-apple-system,Segoe UI,sans-serif';
  return {ctx:ctx,w:w,h:h,pad:30};
}
function noData(c){c.ctx.fillStyle='#66705f';c.ctx.fillText('Waiting for data',c.pad,c.h/2)}
function drawSeries(c,key,color,width,maxValue){
  const pts=historyPoints;if(!pts.length)return;
  const left=c.pad,right=c.w-10,top=18,bottom=c.h-24,span=Math.max(1,right-left);
  c.ctx.beginPath();c.ctx.strokeStyle=color;c.ctx.lineWidth=width;
  pts.forEach((p,i)=>{const x=left+(pts.length===1?0:(i/(pts.length-1))*span);const v=clamp(Number(p[key]||0),0,maxValue);const y=bottom-(v/maxValue)*(bottom-top);if(i===0)c.ctx.moveTo(x,y);else c.ctx.lineTo(x,y)});
  c.ctx.stroke();
}
function drawMoistureChart(){
  const c=prepCanvas('moistureChart');if(!c)return;
  c.ctx.fillStyle='#66705f';c.ctx.fillText('4095 dry',c.pad,14);c.ctx.fillText('0 wet',c.pad,c.h-8);
  c.ctx.strokeStyle='#eef1ec';for(let i=1;i<4;i++){const y=18+i*((c.h-42)/4);c.ctx.beginPath();c.ctx.moveTo(c.pad,y);c.ctx.lineTo(c.w-10,y);c.ctx.stroke()}
  if(!historyPoints.length){noData(c);return}
  drawSeries(c,'raw','#9aa59a',1,4095);
  drawSeries(c,'avg','#245b3b',2.5,4095);
}
function drawClientChart(){
  const c=prepCanvas('clientChart');if(!c)return;
  const maxValue=Math.max(4,...historyPoints.map(p=>Number(p.clients||0)));
  c.ctx.fillStyle='#66705f';c.ctx.fillText(maxValue+' clients',c.pad,14);c.ctx.fillText('0',c.pad,c.h-8);
  if(!historyPoints.length){noData(c);return}
  drawSeries(c,'clients','#28628f',2.5,maxValue);
}
function drawEventChart(){
  const c=prepCanvas('eventChart');if(!c)return;
  const s=latestStatus||{};
  const bars=[
    ['Pump',Number(s.pump_pulses||0),'#b33a22'],
    ['LED',Number(s.led_flash_count||0),'#245b3b'],
    ['AP joins',Number(s.ap_connect_events||0),'#28628f'],
    ['Flow',Number(s.flow_pulses||0),'#555'],
    ['Exports',Number(s.export_count||0),'#7a4a16']
  ];
  const maxValue=Math.max(1,...bars.map(b=>b[1]));
  const left=16,top=20,barH=22,gap=12,usable=c.w-120;
  c.ctx.font='12px system-ui,-apple-system,Segoe UI,sans-serif';
  bars.forEach((b,i)=>{const y=top+i*(barH+gap);const w=(b[1]/maxValue)*usable;c.ctx.fillStyle=b[2];c.ctx.fillRect(left+72,y,Math.max(2,w),barH);c.ctx.fillStyle='#172018';c.ctx.fillText(b[0],left,y+16);c.ctx.fillText(String(b[1]),left+78+w,y+16)});
}
function drawCharts(){
  if(!$('chartsPanel').open)return;
  drawMoistureChart();drawClientChart();drawEventChart();
}
$('pumpDuration').addEventListener('input',()=>syncPumpInputs('range'));
$('pumpDurationNumber').addEventListener('input',()=>syncPumpInputs('number'));
$('chartsPanel').addEventListener('toggle',drawCharts);
window.addEventListener('resize',drawCharts);
syncPumpInputs();
loadHistory();
refreshStatus();
refreshDiagnostics();
setInterval(refreshStatus,1000);
setInterval(refreshDiagnostics,3000);
</script></main></body></html>)HTML";
  return html;
}

void startLedTest(const String &mode) {
  if (mode != "red" && mode != "green" && mode != "both") {
    server.send(400, "application/json", "{\"error\":\"led must be red, green, or both\"}");
    return;
  }
  ledTestMode = mode;
  ledTestUntilMs = millis() + 5000;
  ledFlashCount++;
  if (mode == "red") {
    redLedFlashCount++;
  } else if (mode == "green") {
    greenLedFlashCount++;
  } else {
    bothLedFlashCount++;
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void resetStatsEndpoint() {
  resetMoistureStats();
  sampleMoistureNow();
  lastMoistureSampleMs = millis();
  resetFlowPulseCount();
  noInterrupts();
  apConnectEvents = 0;
  apDisconnectEvents = 0;
  interrupts();
  ledFlashCount = 0;
  redLedFlashCount = 0;
  greenLedFlashCount = 0;
  bothLedFlashCount = 0;
  pumpPulseCount = 0;
  pumpAttemptCount = 0;
  pumpRejectedAuthCount = 0;
  pumpRejectedBusyCount = 0;
  pumpCompletedCount = 0;
  lastPumpRequestedMs = 0;
  lastPumpActualMs = 0;
  lastPumpStopReason = "stats_reset";
  exportCount = 0;
  maxApClients = currentApClients();
  resetHistory();
  addHistoryPoint();
  lastHistorySampleMs = millis();
  server.send(200, "application/json", "{\"ok\":true}");
}

void pumpEndpoint() {
  pumpAttemptCount++;
  if (server.arg("confirm") != PUMP_CONFIRM_TOKEN) {
    pumpRejectedAuthCount++;
    server.send(403, "text/plain", "Pump request rejected: missing confirmation token.");
    return;
  }

  uint32_t requestedMs = PUMP_MAX_RUNTIME_MS;
  if (server.hasArg("duration_ms")) {
    const long parsed = server.arg("duration_ms").toInt();
    if (parsed > 0) {
      requestedMs = static_cast<uint32_t>(parsed);
    }
  }
  if (requestedMs > PUMP_MAX_RUNTIME_MS) {
    requestedMs = PUMP_MAX_RUNTIME_MS;
  }

  if (!startPumpPulse(requestedMs)) {
    server.send(409, "text/plain", "Pump is already running.");
    return;
  }

  String json = "{\"ok\":true,\"duration_ms\":";
  json += String(requestedMs);
  json += ",\"max_runtime_ms\":";
  json += String(PUMP_MAX_RUNTIME_MS);
  json += "}";
  server.send(200, "application/json", json);
}

void saveWifiEndpoint() {
  const String ssid = server.arg("ssid");
  const String password = server.arg("password");
  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID is required.");
    return;
  }

  savedStaSsid = ssid;
  savedStaPassword = password;
  preferences.putString("sta_ssid", savedStaSsid);
  preferences.putString("sta_pass", savedStaPassword);
  connectStaIfConfigured();
  server.send(200, "text/plain", "Saved. ESP is trying to join " + savedStaSsid + " while keeping the AP online.");
}

void clearWifiEndpoint() {
  preferences.remove("sta_ssid");
  preferences.remove("sta_pass");
  savedStaSsid = "";
  savedStaPassword = "";
  staConnectStartedMs = 0;
  WiFi.disconnect(false, false);
  server.send(200, "text/plain", "Saved Wi-Fi cleared. AP remains online.");
}

void updateLeds() {
  const uint32_t now = millis();
  if (ledTestUntilMs > now) {
    const bool on = ((now / 150) % 2) == 0;
    digitalWrite(PIN_ERROR_LED, (ledTestMode == "red" || ledTestMode == "both") && on ? LED_ON : LED_OFF);
    digitalWrite(PIN_STATUS_LED, (ledTestMode == "green" || ledTestMode == "both") && on ? LED_ON : LED_OFF);
    return;
  }

  if (ledTestMode.length() != 0) {
    ledTestMode = "";
  }

  digitalWrite(PIN_ERROR_LED, LED_OFF);
  const bool heartbeat = (now % 2000) < 90;
  digitalWrite(PIN_STATUS_LED, heartbeat ? LED_ON : LED_OFF);
}

void updateServicePad() {
  const uint32_t now = millis();
  if (digitalRead(PIN_RESERVOIR_SW) == LOW) {
    if (reservoirLowStartedMs == 0) {
      reservoirLowStartedMs = now;
    } else if (now - reservoirLowStartedMs > 3000 && ledTestUntilMs <= now) {
      ledTestMode = "both";
      ledTestUntilMs = now + 5000;
      Serial.println("TP6 service pad held low: flashing both LEDs");
      reservoirLowStartedMs = now + 6000;
    }
  } else {
    reservoirLowStartedMs = 0;
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", pageHtml());
  });
  server.on("/api/status", HTTP_GET, []() {
    server.send(200, "application/json", statusJson());
  });
  server.on("/api/diagnostics", HTTP_GET, []() {
    if (server.hasArg("download")) {
      server.sendHeader("Content-Disposition", "attachment; filename=flowerpot-diagnostics.json");
    }
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", diagnosticsJson());
  });
  server.on("/api/history", HTTP_GET, []() {
    server.send(200, "application/json", historyJson());
  });
  server.on("/api/export", HTTP_GET, []() {
    exportCount++;
    server.sendHeader("Content-Disposition", "attachment; filename=flowerpot-session.json");
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", exportJson());
  });
  server.on("/api/reset-stats", HTTP_POST, []() {
    resetStatsEndpoint();
  });
  server.on("/api/flash", HTTP_POST, []() {
    startLedTest(server.arg("led"));
  });
  server.on("/api/flash", HTTP_GET, []() {
    startLedTest(server.arg("led"));
  });
  server.on("/api/pump", HTTP_POST, []() {
    pumpEndpoint();
  });
  server.on("/api/wifi", HTTP_POST, []() {
    saveWifiEndpoint();
  });
  server.on("/api/wifi/clear", HTTP_POST, []() {
    clearWifiEndpoint();
  });
  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + AP_IP.toString(), true);
    server.send(302, "text/plain", "");
  });
  server.begin();
}

}  // namespace

void setup() {
  bootResetReason = esp_reset_reason();
  pinMode(PIN_PUMP_GATE, OUTPUT);
  setPumpOff();
  pinMode(PIN_ERROR_LED, OUTPUT);
  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_ERROR_LED, LED_OFF);
  digitalWrite(PIN_STATUS_LED, LED_OFF);

  // The rail is already gone during a brownout, so report it immediately on
  // the following boot while the pump remains forced off.
  if (bootResetReason == ESP_RST_BROWNOUT) {
    digitalWrite(PIN_ERROR_LED, LED_ON);
    delay(1000);
    digitalWrite(PIN_ERROR_LED, LED_OFF);
  }

  pinMode(PIN_RESERVOIR_SW, INPUT_PULLUP);
  pinMode(PIN_FLOW_PULSE, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_PULSE), onFlowPulse, FALLING);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_MOISTURE_ADC, ADC_11db);
  resetMoistureStats();
  sampleMoistureNow();
  lastMoistureSampleMs = millis();

  Serial.begin(115200);
  delay(600);
  Serial.println();
  Serial.print("SmartWateringFlowerPot ");
  Serial.print(FIRMWARE_NAME);
  Serial.print(" ");
  Serial.println(FIRMWARE_VERSION);
  Serial.println("Pump control is enabled with a 10000 ms firmware cap.");
  Serial.print("Reset reason: ");
  Serial.println(resetReasonName(bootResetReason));

  loadWifiSettings();
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.onEvent(onWiFiEvent, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  WiFi.onEvent(onWiFiEvent, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
  wifi_country_t country = {"US", 1, 11, WIFI_COUNTRY_POLICY_MANUAL};
  esp_wifi_set_country(&country);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  const bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false, 4);
  maxApClients = currentApClients();
  resetHistory();
  addHistoryPoint();
  lastHistorySampleMs = millis();
  dnsServer.start(DNS_PORT, "*", AP_IP);
  setupWebServer();
  connectStaIfConfigured();

  Serial.print("AP started: ");
  Serial.println(apStarted ? "yes" : "no");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP MAC: ");
  Serial.println(WiFi.softAPmacAddress());
  Serial.print("Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("URL: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("USB serial command: ui");
  if (savedStaSsid.length()) {
    Serial.print("Trying home Wi-Fi SSID: ");
    Serial.println(savedStaSsid);
  }
}

void loop() {
  const uint32_t loopStartedMs = millis();
  if (lastLoopStartedMs != 0) {
    const uint32_t gap = loopStartedMs - lastLoopStartedMs;
    if (gap > maxLoopGapMs) {
      maxLoopGapMs = gap;
    }
  }
  lastLoopStartedMs = loopStartedMs;
  loopCount++;
  servicePumpSafety();
  updateMoistureSampler();
  updateHistorySampler();
  updateStaReconnect();
  dnsServer.processNextRequest();
  server.handleClient();
  serviceSerialUi();
  updateServicePad();
  updateLeds();
  servicePumpSafety();
}
