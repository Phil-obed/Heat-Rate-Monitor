#include <SoftwareSerial.h>

#define INT_PIN 2     // Pin connected to MAX30102 interrupt
#define TX_PIN 3      // TX pin to ESP-01 RX
#define RX_PIN 4      // Unused, but required for SoftwareSerial

SoftwareSerial mySerial(RX_PIN, TX_PIN); // RX, TX

volatile bool dataReady = false;

void IRAM_ATTR handleInterrupt() {
  dataReady = true;
}

void setup() {
  pinMode(INT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), handleInterrupt, RISING);
  mySerial.begin(9600);
}

void loop() {
  if (dataReady) {
    dataReady = false;
    mySerial.println("1");  // Signal ESP-01 that data is ready
  }
}
