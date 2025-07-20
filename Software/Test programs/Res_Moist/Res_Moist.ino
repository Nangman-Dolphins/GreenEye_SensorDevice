const int soilMoisturePin = A0;

int moistureValue = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  // READ
  moistureValue = analogRead(soilMoisturePin);

  Serial.print("토양 수분 값: ");
  Serial.println(moistureValue);

  delay(1000);
}