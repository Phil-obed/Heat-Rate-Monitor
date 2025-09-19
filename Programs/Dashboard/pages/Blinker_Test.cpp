void setup() {
  pinMode(2, OUTPUT);  // On many ESP8266 boards, GPIO2 is connected to onboard LED
}

void loop() {
  digitalWrite(2, LOW);
  delay(500);
  digitalWrite(2, HIGH);
  delay(500);
}
