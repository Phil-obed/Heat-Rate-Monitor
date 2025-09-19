#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"   // <-- Real algorithm from Maxim

#define SDA_PIN 0  // GPIO0 = D3
#define SCL_PIN 2  // GPIO2 = D4

MAX30105 particleSensor;

const char *apSSID = "ESP_HealthMonitor";
const char *apPASS = "12345678";

AsyncWebServer server(80);
WebSocketsServer webSocket(81);

// Simple web page
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head><title>Heart & SpO2 Monitor</title></head>
  <body>
    <h2>Live Data</h2>
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

// Algorithm buffers
#define BUFFER_SIZE 100
uint32_t irBuffer[BUFFER_SIZE]; 
uint32_t redBuffer[BUFFER_SIZE];

int32_t spo2;              // SpO2 value
int8_t validSPO2;          // Indicator to show if SpO2 calculation is valid
int32_t heartRate;         // Heart rate value
int8_t validHeartRate;     // Indicator to show if HR calculation is valid

void notifyClients(String message) {
  webSocket.broadcastTXT(message);
}

void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {}

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 not found!");
    while (1);
  }

  particleSensor.setup(); // Default settings
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0); // Disable Green LED

  // --- Force Access Point Mode ---
  WiFi.disconnect(true);       // Clear old configs
  delay(500);
  WiFi.mode(WIFI_AP);          // Force AP only
  WiFi.softAP(apSSID, apPASS); // Start AP with custom SSID/PASS
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Serve test HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", htmlPage);
  });
  server.begin();

  webSocket.begin();
  webSocket.onEvent(handleWebSocketEvent);
}

void loop() {
  webSocket.loop();

  // Read BUFFER_SIZE samples (~4s of data at 25Hz)
  for (byte i = 0; i < BUFFER_SIZE; i++) {
    while (!particleSensor.available()) particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();
    particleSensor.nextSample();

    delay(40); // ~25Hz sampling
  }

  // Run Maxim algorithm on collected samples
  maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer, 
                                         &spo2, &validSPO2, &heartRate, &validHeartRate);

  // Send results
  if (validHeartRate && validSPO2) {
    String jsonData = "{ \"heartRate\": " + String(heartRate) +
                      ", \"spo2\": " + String(spo2) + " }";
    notifyClients(jsonData);
    Serial.println(jsonData);
  } else {
    String msg = "{ \"status\": \"No finger / invalid data\" }";
    notifyClients(msg);
    Serial.println(msg);
  }
}
