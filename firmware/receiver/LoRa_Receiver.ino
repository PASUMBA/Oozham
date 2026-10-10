// Receiver Code for Arduino Nano with RYLR998 LoRa Module
// Default (no signal): D2 BLINKS. While 'H' arrives: D2 solid HIGH.

#include <SoftwareSerial.h>

#define LORA_RX_PIN 4   // connects to LoRa TX
#define LORA_TX_PIN 5   // connects to LoRa RX
#define OUTPUT_PIN 2

SoftwareSerial loraSerial(LORA_RX_PIN, LORA_TX_PIN);

const int LORA_ADDRESS = 0;
const int LORA_NETWORK_ID = 10;

const unsigned long ACTIVE_TIMEOUT_MS = 700;  // 'H' must arrive within this to stay solid (editable)
const unsigned long BLINK_MS = 250;           // idle blink speed (editable)

unsigned long lastSignalTime = 0;
bool solidHigh = false;
unsigned long lastBlinkToggle = 0;
bool blinkState = false;

void sendAT(const String &cmd) {
  loraSerial.print(cmd);
  loraSerial.print("\r\n");
  delay(500);
  while (loraSerial.available()) Serial.write(loraSerial.read());
}

void setup() {
  Serial.begin(115200);
  Serial.println("Receiver (Arduino Nano) Initializing...");

  loraSerial.begin(115200);

  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);

  delay(1000);
  Serial.print("Setting LoRa Address... ");
  sendAT("AT+ADDRESS=" + String(LORA_ADDRESS));
  Serial.println("Done.");
  Serial.print("Setting LoRa Network ID... ");
  sendAT("AT+NETWORKID=" + String(LORA_NETWORK_ID));
  Serial.println("Done.");

  Serial.println("Receiver setup complete. Idle = blinking.");
}

void loop() {
  if (loraSerial.available()) {
    String receivedData = "";
    unsigned long lastByte = millis();
    while (millis() - lastByte < 50) {
      if (loraSerial.available()) {
        char c = loraSerial.read();
        lastByte = millis();
        if (c == '\n') break;
        receivedData += c;
      }
    }

    Serial.print("Received ("); Serial.print(receivedData.length()); Serial.print(" bytes): ");
    Serial.println(receivedData);

    if (receivedData.indexOf("+OK") != -1 || receivedData.indexOf("+ERR") != -1) {
      // module's own reply - ignore
    } else if (receivedData.indexOf(",L,") != -1) {
      lastSignalTime = 0;
      solidHigh = false;
      Serial.println("Got 'L' -> back to blink.");
    } else {
      // any other message = signal active -> solid HIGH
      lastSignalTime = millis();
      if (!solidHigh) {
        solidHigh = true;
        digitalWrite(OUTPUT_PIN, HIGH);
        Serial.println(">> HEAVY: pin solid HIGH");
      }
    }
  }

  // signal went quiet -> back to blinking
  if (solidHigh && (millis() - lastSignalTime >= ACTIVE_TIMEOUT_MS)) {
    solidHigh = false;
    Serial.println("Signal lost -> blinking (idle).");
  }

  // idle blink
  if (!solidHigh && (millis() - lastBlinkToggle >= BLINK_MS)) {
    blinkState = !blinkState;
    digitalWrite(OUTPUT_PIN, blinkState ? HIGH : LOW);
    lastBlinkToggle = millis();
  }
}
