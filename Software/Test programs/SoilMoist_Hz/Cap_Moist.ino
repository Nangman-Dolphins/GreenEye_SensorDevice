// NE555's OUTPUT
const int sensorPin = 2;

// Signal's HIGH, LOW time
unsigned long highTime;
unsigned long lowTime;

// Signal's variable
float period;
float frequency;

void setup() {
  pinMode(sensorPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  // Masure HIGH/LOW Time
  highTime = pulseIn(sensorPin, HIGH);
  lowTime = pulseIn(sensorPin, LOW);

  // Caculate period
  period = highTime + lowTime; // us

  // Caculate freq (1,000,000 / period)
  frequency = 1000000.0 / period;

  Serial.print("주파수: ");
  Serial.print(frequency);
  Serial.println(" Hz");

  delay(1000);
}