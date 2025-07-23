#define SAMPLE    10
#define SENS_TERM 50

const int sensorPin = A0;
int sensorValue = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  // read the value from the sensor:
  for(int i = 0; i < SAMPLE; i++){
    sensorValue += analogRead(sensorPin);
    delay(SENS_TERM);
  }

  // print
  Serial.print("ad:");
  Serial.println(sensorValue/10);

  sensorValue = 0; // reset

  delay(10);
}
