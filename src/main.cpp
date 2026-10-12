#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

// HP 82240A/B experimental infrared transmitter.
// Waveform has NOT yet been validated on a physical printer.
static constexpr uint8_t IR_PIN = D2;
static constexpr uint16_t HALF_US = 427;
ESP8266WebServer server(80);
String ssid, password;
uint32_t transmitted = 0;
bool printing = false;

void loadWifi() {
  File f = LittleFS.open("/wifi.txt", "r");
  if (!f) return;
  ssid = f.readStringUntil('\n'); ssid.trim();
  password = f.readStringUntil('\n'); password.trim();
  f.close();
}

void saveWifi(const String &s, const String &p) {
  File f = LittleFS.open("/wifi.txt", "w");
  if (!f) return;
  f.println(s); f.println(p); f.close();
  ssid = s; password = p;
}

void connectWifi() {
  WiFi.mode(WIFI_AP_STA);
  // WARNING: prototype recovery AP has no password.
  WiFi.softAP("HP82240-Setup");
  if (ssid.isEmpty()) return;
  WiFi.begin(ssid.c_str(), password.c_str());
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(200); yield();
  }
}

inline void burst() {
  // Approximate 32.768 kHz carrier with seven software-generated cycles.
  for (uint8_t i = 0; i < 7; ++i) {
    digitalWrite(IR_PIN, HIGH); delayMicroseconds(15);
    digitalWrite(IR_PIN, LOW); delayMicroseconds(15);
  }
}
inline void half(bool mark) {
  if (mark) { burst(); delayMicroseconds(217); }
  else delayMicroseconds(HALF_US);
}
void sendBit(bool value) { half(value); half(!value); }
uint8_t parity(uint8_t v, uint8_t mask) { return __builtin_parity((unsigned)(v & mask)); }
void sendByte(uint8_t value) {
  noInterrupts();
  half(true); half(true); half(true);
  const uint8_t masks[] = {0x78, 0xE6, 0xD5, 0x8B};
  for (uint8_t i = 0; i < 4; ++i) sendBit(parity(value, masks[i]));
  for (int i = 7; i >= 0; --i) sendBit((value >> i) & 1);
  half(false); half(false); half(false);
  interrupts();
  yield();
  ++transmitted;
}
void sendText(String text) {
  text.replace("\r\n", "\n"); text.replace("\r", "\n");
  for (size_t i = 0; i < text.length(); ++i) {
    uint8_t c = (uint8_t)text[i];
    if (c == '\n') sendByte(0x0A);
    else if (c >= 32 && c <= 126) sendByte(c);
    else if (c >= 128) {
      sendByte('?');
      while (i + 1 < text.length() && (((uint8_t)text[i+1] & 0xC0) == 0x80)) ++i;
    }
  }
}
void replyJson(const String &s, int code = 200) {
  server.send(code, "application/json; charset=utf-8", s);
}
String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\n", "\\n");
  value.replace("\r", "\\r");
  return value;
}
void setupRoutes() {
  server.on("/", HTTP_GET, []() {
    File f = LittleFS.open("/index.html", "r");
    if (!f) { server.send(500, "text/plain", "Upload LittleFS filesystem"); return; }
    server.streamFile(f, "text/html; charset=utf-8"); f.close();
  });
  server.on("/api/status", HTTP_GET, []() {
    String s = "{\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false")
      + ",\"ip\":\"" + WiFi.localIP().toString()
      + "\",\"ap\":\"" + WiFi.softAPIP().toString()
      + "\",\"ssid\":\"" + jsonEscape(ssid)
      + "\",\"sent\":" + String(transmitted)
      + ",\"busy\":" + String(printing ? "true" : "false") + "}";
    replyJson(s);
  });
  server.on("/api/wifi/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();
    String s = "[";
    for (int i = 0; i < n; ++i) {
      if (i) s += ",";
      s += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i))
        + "\",\"rssi\":" + String(WiFi.RSSI(i))
        + ",\"secure\":" + String(WiFi.encryptionType(i) == ENC_TYPE_NONE ? "false" : "true") + "}";
    }
    s += "]";
    WiFi.scanDelete();
    replyJson(s);
  });
  server.on("/api/wifi", HTTP_POST, []() {
    String s = server.arg("ssid"), p = server.arg("password");
    if (s.isEmpty() || s.length() > 32 || p.length() > 63) {
      replyJson("{\"error\":\"invalid credentials\"}", 400); return;
    }
    saveWifi(s, p);
    replyJson("{\"saved\":true,\"restart\":true}");
    delay(300); ESP.restart();
  });
  server.on("/api/print", HTTP_POST, []() {
    if (printing) { replyJson("{\"error\":\"busy\"}", 409); return; }
    String text = server.arg("plain");
    if (text.isEmpty() && server.hasArg("text")) text = server.arg("text");
    if (text.length() == 0 || text.length() > 2048) {
      replyJson("{\"error\":\"text must be 1..2048 bytes\"}", 400); return;
    }
    printing = true; sendText(text); printing = false;
    replyJson("{\"transmitted\":true,\"verified_on_printer\":false,\"bytes_total\":" + String(transmitted) + "}");
  });
  server.on("/api/ir/test", HTTP_POST, []() {
    if (printing) { replyJson("{\"error\":\"busy\"}", 409); return; }
    printing = true;
    sendText("HP 82240 IR TEST\n1234567890\n");
    printing = false;
    replyJson("{\"transmitted\":true,\"verified_on_printer\":false}");
  });
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
}
void setup() {
  Serial.begin(115200);
  pinMode(IR_PIN, OUTPUT);
  digitalWrite(IR_PIN, LOW);
  LittleFS.begin();
  loadWifi();
  connectWifi();
  setupRoutes();
  server.begin();
  Serial.printf("\nAP: %s STA: %s\n", WiFi.softAPIP().toString().c_str(), WiFi.localIP().toString().c_str());
}
void loop() { server.handleClient(); }
