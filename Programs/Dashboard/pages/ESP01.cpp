#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

#define SDA_PIN 0  // GPIO0 as I2C SDA
#define SCL_PIN 2  // GPIO2 as I2C SCL

MAX30105 particleSensor;

const char *apSSID = "ESP_HealthMonitor";
const char *apPASS = "12345678";

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Simple HTML page with WebSocket client for testing
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head><title>Heart Monitor</title></head>
  <body>
    <h2>Live Heart Data</h2>
    <pre id="output"></pre>
    <script>
      var ws = new WebSocket("ws://" + location.hostname + ":81/");
      ws.onmessage = function(event) {
        document.getElementById("output").textContent += event.data + "\\n";
      };
    </script>
  </body>
</html>
)rawliteral";

void notifyClients(String message) {
  webSocket.broadcastTXT(message);
}

void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  // Not handling incoming messages, only broadcasting data
}

void setup() {
  Serial.begin(9600); // UART from ATtiny85

  // Start I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Start MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 not found.");
    while (1);
  }
  particleSensor.setup(); // Default settings
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0); // Green LED off

  // Setup WiFi AP
  WiFi.softAP(apSSID, apPASS);
  Serial.println("Access Point Started");

  // Setup WebServer
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", htmlPage);
  });
  server.begin();

  // Setup WebSocket
  webSocket.begin();
  webSocket.onEvent(handleWebSocketEvent);
}

void loop() {
  webSocket.loop();

  if (Serial.available()) {
    char c = Serial.read();
    if (c == '1') {
      long irValue = particleSensor.getIR();
      long redValue = particleSensor.getRed();

      // Process HR & SpO2 (simplified for demo)
      int heartRate = random(60, 100); // Replace with real algorithm
      int spo2 = random(95, 99);       // Replace with real algorithm

      String jsonData = "{ \"heartRate\": " + String(heartRate) + ", \"spo2\": " + String(spo2) + " }";
      notifyClients(jsonData);

      Serial.println(jsonData); // Debug to serial monitor
    }
  }
}
