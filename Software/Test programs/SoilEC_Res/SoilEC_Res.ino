#define EC_PIN1 7
#define EC_PIN2 8

#define EC_READ_PIN A0

#define R1 10000.0

// Probe 1 - EC_PIN1
// Probe 2 - EC_PIN8 - R1 - EC_READ_PIN

void setup() {
  Serial.begin(9600);
}

void loop() {
  // EC_PIN1 -> EC_PIN2
  pinMode(EC_PIN1, OUTPUT);
  pinMode(EC_PIN2, OUTPUT);
  digitalWrite(EC_PIN1, HIGH);
  digitalWrite(EC_PIN2, LOW);
  
  // READ
  int reading1 = analogRead(EC_READ_PIN);
  
  // EC_PIN1 <- EC_PIN2
  digitalWrite(EC_PIN1, LOW);
  digitalWrite(EC_PIN2, HIGH);
  
  // READ (Change in a symmetrical way)
  int reading2 = analogRead(EC_READ_PIN);

  // PIN OFF
  pinMode(EC_PIN1, INPUT);
  pinMode(EC_PIN2, INPUT);

  // Calculate AVG
  float avg_reading = (reading1 + (1023 - reading2)) / 2.0;

  // Calculate the resistance of soil (R2)
  // V_out = V_in * (R2 / (R1 + R2))
  // avg_reading/1023.0 = R2 / (R1 + R2)
  float R2 = (R1 * avg_reading) / (1023.0 - avg_reading);

  // ECC = 1/R
  float ec = 1000000.0 / R2; // uS

  Serial.print("토양 저항: ");
  Serial.print(R2);
  Serial.print(" Ohm, ");
  
  Serial.print("상대 전도도 값: ");
  Serial.println(ec);

  delay(2000);
}