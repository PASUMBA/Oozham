// Transmitter Code for Xiao nRF52840 with RYLR998 LoRa Module

#include <SoftwareSerial.h>

// Define pins for SoftwareSerial communication with the RYLR998 LoRa module
// LoRa RX pin (connects to LoRa module's TX pin)
#define LORA_RX_PIN 9
// LoRa TX pin (connects to LoRa module's RX pin)
#define LORA_TX_PIN 10

// Define the digital input pin on the Xiao nRF52840
// This pin will be read to determine the signal to send
#define INPUT_PIN 2

// Create a SoftwareSerial object for communication with the LoRa module
// The baud rate for RYLR998 is typically 115200 by default.
SoftwareSerial loraSerial(LORA_RX_PIN, LORA_TX_PIN);

// LoRa Module Configuration Parameters
// These must match on both the transmitter and receiver for communication
const int LORA_ADDRESS = 0;    // Module address (0-65535)
const int LORA_NETWORK_ID = 10; // Network ID (0-16)

// Variable to store the current state of the input pin
int currentInputState = LOW;

void setup() {
  // Initialize the hardware serial for debugging output (optional, but recommended)
  Serial.begin(115200);
  Serial.println("Transmitter (Xiao nRF52840) Initializing...");

  // Initialize the SoftwareSerial communication with the LoRa module
  loraSerial.begin(115200); // Default baud rate for RYLR998

  // Set the input pin mode with internal pull-down resistor
  // This ensures the pin is LOW by default unless actively pulled HIGH.
  pinMode(INPUT_PIN, INPUT_PULLDOWN);

  // --- LoRa Module Configuration ---
  // It's crucial to wait for the module to be ready after power-up.
  // A small delay usually helps, or you can implement a more robust AT command response check.
  delay(1000); // Give the LoRa module time to power up and stabilize

  // Set LoRa Module Address
  // Format: AT+ADDRESS=<address>
  Serial.print("Setting LoRa Address to ");
  Serial.print(LORA_ADDRESS);
  Serial.print("... ");
  loraSerial.print("AT+ADDRESS=");
  loraSerial.print(LORA_ADDRESS);
  loraSerial.print("\r\n"); // CR+LF is required for AT commands
  delay(500); // Wait for module to process command
  while (loraSerial.available()) {
    Serial.write(loraSerial.read());
  }
  Serial.println("Done.");

  // Set LoRa Module Network ID
  // Format: AT+NETWORKID=<ID>
  Serial.print("Setting LoRa Network ID to ");
  Serial.print(LORA_NETWORK_ID);
  Serial.print("... ");
  loraSerial.print("AT+NETWORKID=");
  loraSerial.print(LORA_NETWORK_ID);
  loraSerial.print("\r\n");
  delay(500);
  while (loraSerial.available()) {
    Serial.write(loraSerial.read());
  }
  Serial.println("Done.");

  Serial.println("Transmitter setup complete. Ready to send signals.");
}

void loop() {
  // Read the current state of the input pin
  currentInputState = digitalRead(INPUT_PIN);

  // Send a signal if the input pin is HIGH
  if (currentInputState == HIGH) {
    String dataToSend = "H"; // Send 'H' for HIGH
    Serial.println("Input is HIGH. Sending 'H'...");

    // Send data using AT+SEND command
    // Format: AT+SEND=<destination_address>,<data_length>,<data>
    loraSerial.print("AT+SEND=");
    loraSerial.print(LORA_ADDRESS); // Destination address (receiver's address)
    loraSerial.print(",");
    loraSerial.print(dataToSend.length()); // Length of the data string
    loraSerial.print(",");
    loraSerial.print(dataToSend); // The actual data
    loraSerial.print("\r\n"); // CR+LF to terminate the AT command

    // Increased delay to allow the module more time to process the send command and respond
    delay(200);

    // Read and print any response from the LoRa module (e.g., "OK" or "ERR")
    delay(50);
    while (loraSerial.available()) {
      Serial.write(loraSerial.read());
    }
    Serial.println();
  }

  // A small delay to debounce the input and prevent rapid transmissions
  delay(50);
}
