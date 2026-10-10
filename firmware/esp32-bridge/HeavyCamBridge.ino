// HeavyCamBridge — ESP32 hosts its own WiFi AP (no home router needed).
// reCamera joins this WiFi at a fixed IP; ESP32 polls /api/vehicle/code
// and drives a pin -> Xiao nRF52840 D2 (LoRa transmitter INPUT_PIN).
// API replies "true" when HEAVY (truck/bus) is detected.
// Logic: "true" = HIGH, anything else/error = LOW (fail-safe).

#include <WiFi.h>
#include <HTTPClient.h>

// ---------- EDIT THESE ----------
const char* AP_SSID   = "HeavyCam_AP";     // WiFi the ESP32 creates
const char* AP_PASS   = "heavycam123";     // min 8 chars

const char* CAMERA_IP = "192.168.4.2";     // give the camera this fixed IP
const int   CAMERA_PORT = 1880;
// --------------------------------

// ESP32's own fixed IP as the AP host (default 192.168.4.1)
IPAddress AP_IP   (192, 168, 4, 1);
IPAddress AP_GW   (192, 168, 4, 1);
IPAddress AP_MASK (255, 255, 255, 0);

#define OUT_PIN      D0     // wire to Xiao D2 (if D0 fails to compile, use the GPIO number)
#define POLL_MS      200    // poll rate
#define HTTP_TIMEOUT 8000   // ms (prevents -11 errors)

unsigned long lastPoll = 0;

void setup() {
  Serial.begin(115200);
  pinMode(OUT_PIN, OUTPUT);
  digitalWrite(OUT_PIN, LOW);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GW, AP_MASK);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.print("AP up: ");
  Serial.print(AP_SSID);
  Serial.print("  ESP32 IP: ");
  Serial.println(WiFi.softAPIP());
}

void setOut(bool heavy) {
  digitalWrite(OUT_PIN, heavy ? HIGH : LOW);
}

void loop() {
  if (millis() - lastPoll < POLL_MS) return;
  lastPoll = millis();

  HTTPClient http;
  String url = String("http://") + CAMERA_IP + ":" + CAMERA_PORT + "/api/heavy";
  http.begin(url);
  http.useHTTP10(true);                       // HTTP/1.0 + Connection: close
  http.addHeader("Connection", "close");
  http.setTimeout(HTTP_TIMEOUT);

  int code = http.GET();
  if (code == 200) {
    String body = http.getString();
    body.trim();
    bool heavy = (body == "true");
    setOut(heavy);
    Serial.printf("API=%s -> pin %s\n", body.c_str(), heavy ? "HIGH" : "LOW");
  } else {
    setOut(false);            // fail-safe: camera unreachable = LOW
    Serial.printf("HTTP error %d -> pin LOW\n", code);
  }
  http.end();
}
