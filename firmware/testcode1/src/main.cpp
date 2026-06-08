#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_wifi.h>

namespace {

constexpr char FIRMWARE_NAME[] = "testcode1";
constexpr char FIRMWARE_VERSION[] = "sensor-ui-v2";

constexpr uint8_t PIN_MOISTURE_ADC = 1;
constexpr uint8_t PIN_PUMP_GATE = 4;
constexpr uint8_t PIN_RESERVOIR_SW = 13;
constexpr uint8_t PIN_FLOW_PULSE = 14;
constexpr uint8_t PIN_ERROR_LED = 15;
constexpr uint8_t PIN_STATUS_LED = 16;

constexpr bool LED_ON = HIGH;
constexpr bool LED_OFF = LOW;
constexpr bool PUMP_DISABLED = LOW;

const char *AP_SSID = "FlowerPot-testcode1";
const char *AP_PASSWORD = "flowerpot1";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);
constexpr uint16_t DNS_PORT = 53;

constexpr uint32_t MOISTURE_SAMPLE_INTERVAL_MS = 250;
constexpr uint8_t MOISTURE_AVG_WINDOW = 16;
constexpr uint16_t ADC_MAX_VALUE = 4095;

WebServer server(80);
DNSServer dnsServer;

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

void IRAM_ATTR onFlowPulse() {
  flowPulseCount++;
}

void forcePumpDisabled() {
  digitalWrite(PIN_PUMP_GATE, PUMP_DISABLED);
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

String statusJson() {
  const bool reservoirLow = digitalRead(PIN_RESERVOIR_SW) == LOW;
  const bool flowLow = digitalRead(PIN_FLOW_PULSE) == LOW;
  const uint32_t now = millis();
  const uint32_t flowCount = readFlowPulseCount();
  const String band = moistureBand();

  String json = "{";
  json += "\"name\":\"" + String(FIRMWARE_NAME) + "\",";
  json += "\"version\":\"" + String(FIRMWARE_VERSION) + "\",";
  json += "\"uptime_ms\":" + String(now) + ",";
  json += "\"mac\":\"" + WiFi.macAddress() + "\",";
  json += "\"ap_ssid\":\"" + String(AP_SSID) + "\",";
  json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
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
  json += "\"pump\":\"disabled_in_testcode1\",";
  json += "\"led_test\":\"" + jsonEscape(ledTestMode) + "\",";
  json += "\"led_test_remaining_ms\":" + String(ledTestUntilMs > now ? ledTestUntilMs - now : 0);
  json += "}";
  return json;
}

String pageHtml() {
  const String mac = WiFi.macAddress();
  String html;
  html.reserve(11000);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Flower Pot testcode1</title>");
  html += F("<style>");
  html += F(":root{color-scheme:light}body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;margin:0;background:#f6f7f2;color:#172018}");
  html += F("main{max-width:900px;margin:0 auto;padding:18px}h1{font-size:26px;margin:0 0 6px}h2{font-size:18px;margin:18px 0 8px}");
  html += F(".sub{color:#526052;margin-bottom:14px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(165px,1fr));gap:10px;margin:10px 0}");
  html += F(".card{background:white;border:1px solid #d7ddd2;border-radius:8px;padding:12px;min-height:72px}.k{font-size:12px;color:#66705f;text-transform:uppercase}.v{font-size:24px;font-weight:700;word-break:break-word}.small{font-size:14px;color:#53604f}");
  html += F("button{appearance:none;border:0;border-radius:6px;padding:12px 14px;margin:5px 5px 5px 0;font-weight:700;background:#245b3b;color:white}");
  html += F("button.red{background:#9f2929}button.green{background:#27733e}button.gray{background:#555}");
  html += F(".warn{border-left:5px solid #b93a32;background:#fff;padding:12px;margin:14px 0}.ok{border-left:5px solid #27733e;background:#fff;padding:12px;margin:14px 0}");
  html += F(".band{display:inline-block;border-radius:999px;padding:4px 10px;background:#e8ece1;font-size:18px}.wet{background:#cfe8ff}.moist{background:#d8efcf}.dry-ish{background:#fff0bf}.very-dry{background:#ffd7c7}");
  html += F("code{background:#ecefe8;padding:2px 5px;border-radius:4px}pre{white-space:pre-wrap;background:#172018;color:#e9f0e5;padding:12px;border-radius:8px;overflow:auto;font-size:13px}");
  html += F("</style></head><body><main>");
  html += F("<h1>Flower Pot testcode1</h1>");
  html += F("<div class='sub'>Safe sensor/UI firmware. AP-only. Pump output is intentionally disabled.</div>");
  html += F("<div class='warn'><b>Pump disabled:</b> GPIO4 is held LOW continuously. This firmware has no pump control endpoint or button.</div>");

  html += F("<div><button class='red' onclick=\"flashLed('red')\">Flash red LED 5s</button>");
  html += F("<button class='green' onclick=\"flashLed('green')\">Flash green LED 5s</button>");
  html += F("<button onclick=\"flashLed('both')\">Flash both 5s</button>");
  html += F("<button class='gray' onclick='resetStats()'>Reset stats</button>");
  html += F("<button class='gray' onclick='refreshStatus()'>Refresh</button></div>");

  html += F("<section class='grid'>");
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

  html += F("<section class='grid'>");
  html += F("<div class='card'><div class='k'>AP SSID</div><div class='v'>");
  html += AP_SSID;
  html += F("</div></div>");
  html += F("<div class='card'><div class='k'>AP password</div><div class='v'>");
  html += AP_PASSWORD;
  html += F("</div></div>");
  html += F("<div class='card'><div class='k'>Address</div><div class='v'>192.168.4.1</div></div>");
  html += F("<div class='card'><div class='k'>MAC</div><div class='v'>");
  html += mac;
  html += F("</div></div>");
  html += F("</section>");

  html += F("<div class='ok'>Diagnostic bands are rough: higher ADC means drier. Use real soil readings before choosing watering thresholds.</div>");
  html += F("<h2>Status JSON</h2><pre id='status'>Loading...</pre>");
  html += F("<script>");
  html += F("function yn(v){return v?'SHORTED':'open'}");
  html += F("function bandClass(v){return String(v||'').replace(/ /g,'-')}");
  html += F("async function refreshStatus(){try{const r=await fetch('/api/status',{cache:'no-store'});const s=await r.json();");
  html += F("document.getElementById('moistureRaw').textContent=s.moisture_adc_raw;");
  html += F("document.getElementById('moistureAvg').textContent=s.moisture_adc_avg;");
  html += F("const b=document.getElementById('moistureBand');b.textContent=s.moisture_band;b.className='band '+bandClass(s.moisture_band);");
  html += F("document.getElementById('moistureRange').textContent=s.moisture_adc_min+' / '+s.moisture_adc_max;");
  html += F("document.getElementById('moistureSpan').textContent='span '+s.moisture_adc_span;");
  html += F("document.getElementById('tp6State').textContent=yn(s.reservoir_sw_low);");
  html += F("document.getElementById('tp10State').textContent=yn(s.flow_input_low);");
  html += F("document.getElementById('flowPulseState').textContent=s.flow_pulses;");
  html += F("document.getElementById('sampleCount').textContent=s.samples;");
  html += F("document.getElementById('status').textContent=JSON.stringify(s,null,2)}catch(e){document.getElementById('status').textContent='Status read failed: '+e}}");
  html += F("async function flashLed(which){await fetch('/api/flash?led='+encodeURIComponent(which),{method:'POST'});refreshStatus()}");
  html += F("async function resetStats(){await fetch('/api/reset-stats',{method:'POST'});refreshStatus()}");
  html += F("refreshStatus();setInterval(refreshStatus,2000);");
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
  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + AP_IP.toString(), true);
    server.send(302, "text/plain", "");
  });
  server.begin();
}

}  // namespace

void setup() {
  pinMode(PIN_PUMP_GATE, OUTPUT);
  forcePumpDisabled();
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
  Serial.println("Pump is disabled in firmware.");

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  wifi_country_t country = {"US", 1, 11, WIFI_COUNTRY_POLICY_MANUAL};
  esp_wifi_set_country(&country);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  const bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false, 4);
  dnsServer.start(DNS_PORT, "*", AP_IP);
  setupWebServer();

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
}

void loop() {
  forcePumpDisabled();
  updateMoistureSampler();
  dnsServer.processNextRequest();
  server.handleClient();
  updateServicePad();
  updateLeds();
}
