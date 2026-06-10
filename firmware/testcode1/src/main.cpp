#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_wifi.h>

namespace {

constexpr char FIRMWARE_NAME[] = "testcode1";
constexpr char FIRMWARE_VERSION[] = "pump-test-v1";

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

constexpr uint32_t PUMP_MAX_RUNTIME_MS = 2000;
constexpr char PUMP_CONFIRM_TOKEN[] = "pump-test";
constexpr uint32_t USB_UI_ANNOUNCE_INTERVAL_MS = 5000;
constexpr uint32_t USB_UI_FAST_ANNOUNCE_UNTIL_MS = 30000;

WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

String savedStaSsid = "";
String savedStaPassword = "";
uint32_t staConnectStartedMs = 0;

volatile uint32_t flowPulseCount = 0;

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

bool pumpRunning = false;
uint32_t pumpStartedMs = 0;
uint32_t pumpUntilMs = 0;
uint32_t pumpPulseCount = 0;

String serialLine = "";
bool serialWasConnected = false;
uint32_t lastSerialUiAnnounceMs = 0;

String statusJson();

void IRAM_ATTR onFlowPulse() {
  flowPulseCount++;
}

void setPumpOff() {
  pumpRunning = false;
  pumpStartedMs = 0;
  pumpUntilMs = 0;
  digitalWrite(PIN_PUMP_GATE, PUMP_OFF);
}

void servicePumpSafety() {
  const uint32_t now = millis();
  if (pumpRunning && static_cast<int32_t>(now - pumpUntilMs) >= 0) {
    setPumpOff();
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
    return false;
  }
  if (requestedMs == 0 || requestedMs > PUMP_MAX_RUNTIME_MS) {
    requestedMs = PUMP_MAX_RUNTIME_MS;
  }

  const uint32_t now = millis();
  pumpRunning = true;
  pumpStartedMs = now;
  pumpUntilMs = now + requestedMs;
  pumpPulseCount++;
  digitalWrite(PIN_PUMP_GATE, PUMP_ON);
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
  Serial.println("COMMANDS=ui,status,help");
  Serial.println("FLOWERPOT_UI_END");
}

void printSerialHelp() {
  Serial.println("FLOWERPOT_HELP_BEGIN");
  Serial.println("ui     - print browser UI URL hints");
  Serial.println("status - print /api/status JSON");
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
  savedStaSsid = preferences.getString("sta_ssid", "");
  savedStaPassword = preferences.getString("sta_pass", "");
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
  const String band = moistureBand();
  const String staStatus = staStatusText();

  String json = "{";
  json += "\"name\":\"" + String(FIRMWARE_NAME) + "\",";
  json += "\"version\":\"" + String(FIRMWARE_VERSION) + "\",";
  json += "\"uptime_ms\":" + String(now) + ",";
  json += "\"mac\":\"" + WiFi.macAddress() + "\",";
  json += "\"ap_ssid\":\"" + String(AP_SSID) + "\",";
  json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"ap_clients\":" + String(WiFi.softAPgetStationNum()) + ",";
  json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
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
  json += "\"pump\":\"" + String(pumpRunning ? "running" : "ready_limited_2s") + "\",";
  json += "\"pump_running\":" + String(pumpRunning ? "true" : "false") + ",";
  json += "\"pump_remaining_ms\":" + String(pumpRemainingMs()) + ",";
  json += "\"pump_max_runtime_ms\":" + String(PUMP_MAX_RUNTIME_MS) + ",";
  json += "\"pump_pulses\":" + String(pumpPulseCount) + ",";
  json += "\"led_test\":\"" + jsonEscape(ledTestMode) + "\",";
  json += "\"led_test_remaining_ms\":" + String(ledTestUntilMs > now ? ledTestUntilMs - now : 0);
  json += "}";
  return json;
}

String pageHtml() {
  const String mac = WiFi.macAddress();
  String html;
  html.reserve(16000);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Flower Pot testcode1</title>");
  html += F("<style>");
  html += F(":root{color-scheme:light}body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;margin:0;background:#f6f7f2;color:#172018}");
  html += F("main{max-width:960px;margin:0 auto;padding:18px}h1{font-size:26px;margin:0 0 6px}h2{font-size:18px;margin:18px 0 8px}");
  html += F(".sub{color:#526052;margin-bottom:14px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(165px,1fr));gap:10px;margin:10px 0}");
  html += F(".card{background:white;border:1px solid #d7ddd2;border-radius:8px;padding:12px;min-height:72px}.k{font-size:12px;color:#66705f;text-transform:uppercase}.v{font-size:24px;font-weight:700;word-break:break-word}.small{font-size:14px;color:#53604f}");
  html += F("button{appearance:none;border:0;border-radius:6px;padding:12px 14px;margin:5px 5px 5px 0;font-weight:700;background:#245b3b;color:white}");
  html += F("button.red{background:#9f2929}button.green{background:#27733e}button.gray{background:#555}button.pump{background:#b33a22}button:disabled{opacity:.55}");
  html += F("input{box-sizing:border-box;width:100%;max-width:360px;border:1px solid #b8c1b3;border-radius:6px;padding:10px;margin:4px 0 8px;font:inherit}");
  html += F(".warn{border-left:5px solid #b93a32;background:#fff;padding:12px;margin:14px 0}.ok{border-left:5px solid #27733e;background:#fff;padding:12px;margin:14px 0}");
  html += F(".band{display:inline-block;border-radius:999px;padding:4px 10px;background:#e8ece1;font-size:18px}.wet{background:#cfe8ff}.moist{background:#d8efcf}.dry-ish{background:#fff0bf}.very-dry{background:#ffd7c7}");
  html += F("code{background:#ecefe8;padding:2px 5px;border-radius:4px}pre{white-space:pre-wrap;background:#172018;color:#e9f0e5;padding:12px;border-radius:8px;overflow:auto;font-size:13px}");
  html += F("</style></head><body><main>");
  html += F("<h1>Flower Pot testcode1</h1>");
  html += F("<div class='sub'>Pump-test firmware. AP always stays on. Pump is limited in firmware to a 2 second pulse.</div>");
  html += F("<div class='warn'><b>Pump unlocked:</b> use a dedicated 5 V supply or power bank, not a laptop USB port. Firmware caps each pump run at 2000 ms.</div>");

  html += F("<div><button class='pump' id='pumpBtn' onclick='runPump()'>Run pump 2s</button>");
  html += F("<button class='red' onclick=\"flashLed('red')\">Flash red LED 5s</button>");
  html += F("<button class='green' onclick=\"flashLed('green')\">Flash green LED 5s</button>");
  html += F("<button onclick=\"flashLed('both')\">Flash both 5s</button>");
  html += F("<button class='gray' onclick='resetStats()'>Reset stats</button>");
  html += F("<button class='gray' onclick='refreshStatus()'>Refresh</button></div>");

  html += F("<section class='grid'>");
  html += F("<div class='card'><div class='k'>Pump</div><div class='v' id='pumpState'>...</div><div class='small' id='pumpRemaining'>...</div></div>");
  html += F("<div class='card'><div class='k'>Moisture raw</div><div class='v' id='moistureRaw'>...</div></div>");
  html += F("<div class='card'><div class='k'>Moisture avg</div><div class='v' id='moistureAvg'>...</div></div>");
  html += F("<div class='card'><div class='k'>Moisture band</div><div class='v'><span class='band' id='moistureBand'>...</span></div></div>");
  html += F("<div class='card'><div class='k'>Min / max</div><div class='v' id='moistureRange'>...</div><div class='small' id='moistureSpan'>...</div></div>");
  html += F("</section>");

  html += F("<section class='grid'>");
  html += F("<div class='card'><div class='k'>TP6 to TP7</div><div class='v' id='tp6State'>...</div></div>");
  html += F("<div class='card'><div class='k'>TP10 to TP9</div><div class='v' id='tp10State'>...</div></div>");
  html += F("<div class='card'><div class='k'>Flow pulses</div><div class='v' id='flowPulseState'>...</div></div>");
  html += F("<div class='card'><div class='k'>Samples</div><div class='v' id='sampleCount'>...</div></div>");
  html += F("</section>");

  html += F("<h2>Network</h2><section class='grid'>");
  html += F("<div class='card'><div class='k'>AP SSID</div><div class='v'>");
  html += AP_SSID;
  html += F("</div><div class='small'>AP URL: http://192.168.4.1</div></div>");
  html += F("<div class='card'><div class='k'>Home Wi-Fi</div><div class='v' id='staStatus'>...</div><div class='small' id='staDetails'>...</div></div>");
  html += F("<div class='card'><div class='k'>MAC</div><div class='v'>");
  html += mac;
  html += F("</div></div>");
  html += F("</section>");

  html += F("<div class='card'><h2>Settings</h2><form id='wifiForm' onsubmit='saveWifi(event)'>");
  html += F("<label>Home Wi-Fi SSID<br><input name='ssid' id='ssidInput' autocomplete='off'></label><br>");
  html += F("<label>Password<br><input name='password' type='password' autocomplete='current-password'></label><br>");
  html += F("<button type='submit'>Connect ESP to home Wi-Fi</button>");
  html += F("<button type='button' class='gray' onclick='clearWifi()'>Clear saved Wi-Fi</button>");
  html += F("<div class='small' id='wifiMessage'></div></form></div>");

  html += F("<div class='ok'>Diagnostic bands are rough: higher ADC means drier. Use real soil readings before choosing watering thresholds.</div>");
  html += F("<h2>Status JSON</h2><pre id='status'>Loading...</pre>");
  html += F("<script>");
  html += F("function yn(v){return v?'SHORTED':'open'}");
  html += F("function bandClass(v){return String(v||'').replace(/ /g,'-')}");
  html += F("function ms(v){return Math.max(0,Math.ceil(Number(v||0)))+' ms'}");
  html += F("async function refreshStatus(){try{const r=await fetch('/api/status',{cache:'no-store'});const s=await r.json();");
  html += F("document.getElementById('pumpState').textContent=s.pump_running?'RUNNING':'ready';");
  html += F("document.getElementById('pumpRemaining').textContent=s.pump_running?('remaining '+ms(s.pump_remaining_ms)):('max '+ms(s.pump_max_runtime_ms));");
  html += F("document.getElementById('pumpBtn').disabled=!!s.pump_running;");
  html += F("document.getElementById('moistureRaw').textContent=s.moisture_adc_raw;");
  html += F("document.getElementById('moistureAvg').textContent=s.moisture_adc_avg;");
  html += F("const b=document.getElementById('moistureBand');b.textContent=s.moisture_band;b.className='band '+bandClass(s.moisture_band);");
  html += F("document.getElementById('moistureRange').textContent=s.moisture_adc_min+' / '+s.moisture_adc_max;");
  html += F("document.getElementById('moistureSpan').textContent='span '+s.moisture_adc_span;");
  html += F("document.getElementById('tp6State').textContent=yn(s.reservoir_sw_low);");
  html += F("document.getElementById('tp10State').textContent=yn(s.flow_input_low);");
  html += F("document.getElementById('flowPulseState').textContent=s.flow_pulses;");
  html += F("document.getElementById('sampleCount').textContent=s.samples;");
  html += F("document.getElementById('staStatus').textContent=s.sta_status;");
  html += F("document.getElementById('staDetails').textContent=s.sta_configured?(s.sta_ssid+(s.sta_ip?' at '+s.sta_ip:'')):'not configured';");
  html += F("if(!document.getElementById('ssidInput').value&&s.sta_ssid){document.getElementById('ssidInput').value=s.sta_ssid}");
  html += F("document.getElementById('status').textContent=JSON.stringify(s,null,2)}catch(e){document.getElementById('status').textContent='Status read failed: '+e}}");
  html += F("async function flashLed(which){await fetch('/api/flash?led='+encodeURIComponent(which),{method:'POST'});refreshStatus()}");
  html += F("async function resetStats(){await fetch('/api/reset-stats',{method:'POST'});refreshStatus()}");
  html += F("async function saveWifi(e){e.preventDefault();const fd=new FormData(e.target);const r=await fetch('/api/wifi',{method:'POST',body:new URLSearchParams(fd)});document.getElementById('wifiMessage').textContent=await r.text();setTimeout(refreshStatus,1000)}");
  html += F("async function clearWifi(){const r=await fetch('/api/wifi/clear',{method:'POST'});document.getElementById('wifiMessage').textContent=await r.text();refreshStatus()}");
  html += F("async function runPump(){if(!localStorage.getItem('pumpWarned')){const ok=confirm('First pump test warning: use a dedicated 5 V supply, keep water away from the board, confirm MOSFET/diode orientation, and expect only a 2 second pulse. Run pump now?');if(!ok)return;localStorage.setItem('pumpWarned','yes')}");
  html += F("const r=await fetch('/api/pump',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'duration_ms=2000&confirm=pump-test'});if(!r.ok){alert(await r.text())}refreshStatus()}");
  html += F("refreshStatus();setInterval(refreshStatus,1000);");
  html += F("</script></main></body></html>");
  return html;
}

void startLedTest(const String &mode) {
  if (mode != "red" && mode != "green" && mode != "both") {
    server.send(400, "application/json", "{\"error\":\"led must be red, green, or both\"}");
    return;
  }
  ledTestMode = mode;
  ledTestUntilMs = millis() + 5000;
  server.send(200, "application/json", "{\"ok\":true}");
}

void resetStatsEndpoint() {
  resetMoistureStats();
  sampleMoistureNow();
  lastMoistureSampleMs = millis();
  resetFlowPulseCount();
  server.send(200, "application/json", "{\"ok\":true}");
}

void pumpEndpoint() {
  if (server.arg("confirm") != PUMP_CONFIRM_TOKEN) {
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
  pinMode(PIN_PUMP_GATE, OUTPUT);
  setPumpOff();
  pinMode(PIN_ERROR_LED, OUTPUT);
  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_ERROR_LED, LED_OFF);
  digitalWrite(PIN_STATUS_LED, LED_OFF);

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
  Serial.println("Pump test is enabled with a 2000 ms firmware cap.");

  loadWifiSettings();
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  wifi_country_t country = {"US", 1, 11, WIFI_COUNTRY_POLICY_MANUAL};
  esp_wifi_set_country(&country);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  const bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false, 4);
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
  servicePumpSafety();
  updateMoistureSampler();
  updateStaReconnect();
  dnsServer.processNextRequest();
  server.handleClient();
  serviceSerialUi();
  updateServicePad();
  updateLeds();
  servicePumpSafety();
}
