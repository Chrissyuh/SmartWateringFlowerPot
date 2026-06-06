#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_wifi.h>

namespace {

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

WebServer server(80);
DNSServer dnsServer;

volatile uint32_t flowPulseCount = 0;
uint32_t ledTestUntilMs = 0;
String ledTestMode = "";
uint32_t reservoirLowStartedMs = 0;

void IRAM_ATTR onFlowPulse() {
  flowPulseCount++;
}

void forcePumpDisabled() {
  digitalWrite(PIN_PUMP_GATE, PUMP_DISABLED);
}

uint16_t readMoistureRaw() {
  uint32_t total = 0;
  constexpr uint8_t samples = 16;
  for (uint8_t i = 0; i < samples; i++) {
    total += analogRead(PIN_MOISTURE_ADC);
    delay(2);
  }
  return static_cast<uint16_t>(total / samples);
}

String htmlEscape(const String &value) {
  String out;
  out.reserve(value.length());
  for (size_t i = 0; i < value.length(); i++) {
    const char c = value[i];
    if (c == '&') out += F("&amp;");
    else if (c == '<') out += F("&lt;");
    else if (c == '>') out += F("&gt;");
    else if (c == '"') out += F("&quot;");
    else out += c;
  }
  return out;
}

String statusJson() {
  const uint16_t moisture = readMoistureRaw();
  const bool reservoirLow = digitalRead(PIN_RESERVOIR_SW) == LOW;
  const bool flowLow = digitalRead(PIN_FLOW_PULSE) == LOW;
  const uint32_t now = millis();

  String json = "{";
  json += "\"name\":\"testcode1\",";
  json += "\"uptime_ms\":" + String(now) + ",";
  json += "\"mac\":\"" + WiFi.macAddress() + "\",";
  json += "\"ap_ssid\":\"" + String(AP_SSID) + "\",";
  json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
  json += "\"moisture_adc_raw\":" + String(moisture) + ",";
  json += "\"reservoir_sw_low\":" + String(reservoirLow ? "true" : "false") + ",";
  json += "\"flow_input_low\":" + String(flowLow ? "true" : "false") + ",";
  json += "\"flow_pulses\":" + String(flowPulseCount) + ",";
  json += "\"pump\":\"disabled_in_testcode1\",";
  json += "\"led_test\":\"" + htmlEscape(ledTestMode) + "\",";
  json += "\"led_test_remaining_ms\":" + String(ledTestUntilMs > now ? ledTestUntilMs - now : 0);
  json += "}";
  return json;
}

String pageHtml() {
  const String mac = WiFi.macAddress();
  String html;
  html.reserve(7000);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Flower Pot testcode1</title>");
  html += F("<style>");
  html += F("body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;margin:0;background:#f7f7f2;color:#172018}");
  html += F("main{max-width:760px;margin:0 auto;padding:18px}h1{font-size:26px;margin:0 0 8px}");
  html += F(".sub{color:#526052;margin-bottom:16px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(185px,1fr));gap:10px}");
  html += F(".card{background:white;border:1px solid #d8ddd3;border-radius:8px;padding:12px}.k{font-size:12px;color:#66705f;text-transform:uppercase}.v{font-size:22px;font-weight:650;word-break:break-word}");
  html += F("button{appearance:none;border:0;border-radius:6px;padding:12px 14px;margin:5px 5px 5px 0;font-weight:650;background:#245b3b;color:white}");
  html += F("button.red{background:#9f2929}button.green{background:#27733e}button.gray{background:#555}");
  html += F(".warn{border-left:5px solid #b93a32;background:#fff;padding:12px;margin:14px 0}.ok{border-left:5px solid #27733e;background:#fff;padding:12px;margin:14px 0}");
  html += F("code{background:#ecefe8;padding:2px 5px;border-radius:4px}pre{white-space:pre-wrap;background:#172018;color:#e9f0e5;padding:12px;border-radius:8px;overflow:auto}");
  html += F("</style></head><body><main>");
  html += F("<h1>Flower Pot testcode1</h1>");
  html += F("<div class='sub'>Board A bring-up UI. Pump output is intentionally disabled.</div>");
  html += F("<div class='warn'><b>Pump disabled:</b> GPIO4 is held LOW continuously and this page has no pump control.</div>");
  html += F("<div><button class='red' onclick=\"flashLed('red')\">Flash red LED 5s</button>");
  html += F("<button class='green' onclick=\"flashLed('green')\">Flash green LED 5s</button>");
  html += F("<button onclick=\"flashLed('both')\">Flash both 5s</button>");
  html += F("<button class='gray' onclick='refreshStatus()'>Refresh</button></div>");
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
  html += F("<section class='grid'>");
  html += F("<div class='card'><div class='k'>TP6 to TP7</div><div class='v' id='tp6State'>...</div></div>");
  html += F("<div class='card'><div class='k'>TP10 to TP9</div><div class='v' id='tp10State'>...</div></div>");
  html += F("<div class='card'><div class='k'>Moisture ADC</div><div class='v' id='moistureState'>...</div></div>");
  html += F("<div class='card'><div class='k'>Flow pulses</div><div class='v' id='flowPulseState'>...</div></div>");
  html += F("</section>");
  html += F("<div class='ok'>Short <code>TP6</code> to <code>TP7/GND</code> for 3 seconds while running to trigger both LEDs and confirm the service-pad input. No pump action is tied to this.</div>");
  html += F("<h2>Status</h2><pre id='status'>Loading...</pre>");
  html += F("<script>");
  html += F("function yn(v){return v?'SHORTED':'open'}");
  html += F("async function refreshStatus(){const r=await fetch('/api/status');const s=await r.json();");
  html += F("document.getElementById('tp6State').textContent=yn(s.reservoir_sw_low);");
  html += F("document.getElementById('tp10State').textContent=yn(s.flow_input_low);");
  html += F("document.getElementById('moistureState').textContent=s.moisture_adc_raw;");
  html += F("document.getElementById('flowPulseState').textContent=s.flow_pulses;");
  html += F("document.getElementById('status').textContent=JSON.stringify(s,null,2)}");
  html += F("async function flashLed(which){await fetch('/api/flash?led='+encodeURIComponent(which),{method:'POST'});refreshStatus()}");
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

}

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

  Serial.begin(115200);
  delay(600);
  Serial.println();
  Serial.println("SmartWateringFlowerPot testcode1");
  Serial.println("Pump is disabled in firmware.");

  WiFi.mode(WIFI_AP_STA);
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

  Serial.println("Scanning nearby Wi-Fi from ESP32 radio...");
  const int networkCount = WiFi.scanNetworks();
  Serial.print("Networks found: ");
  Serial.println(networkCount);
  for (int i = 0; i < networkCount && i < 12; i++) {
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" RSSI=");
    Serial.print(WiFi.RSSI(i));
    Serial.print(" ch=");
    Serial.println(WiFi.channel(i));
  }
  WiFi.scanDelete();
}

void loop() {
  forcePumpDisabled();
  dnsServer.processNextRequest();
  server.handleClient();
  updateServicePad();
  updateLeds();
}
