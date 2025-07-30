// 5V --- [ Fixed Resistor ] --- (A0) --- [ NTC ] --- GND
#define THERMISTOR_PIN A0
#define FIXED_RESISTOR 10000.0 // Fixed Resistor (10KΩ)
#define NOMINAL_RESISTANCE 10000.0 // NTC's Ref Resistor (10KΩ at 25°C)
#define NOMINAL_TEMPERATURE 25.0 // NTC's Ref Temperature (25°C at 10KΩ)
#define B_COEFFICIENT 3950.0 // NTC's B value (3950K in datasheet)

void setup() {
  Serial.begin(9600);
}

void loop() {
  int adcValue = analogRead(THERMISTOR_PIN);

  // calculate R_thermistor = R_fixed * (ADC_max / ADC_read - 1)
  float resistance = FIXED_RESISTOR * (1023.0 / adcValue - 1.0);

  // Steinhart-Hart equation
  float steinhart;
  steinhart = resistance / NOMINAL_RESISTANCE;     // (R/R0)
  steinhart = log(steinhart);                      // ln(R/R0)
  steinhart /= B_COEFFICIENT;                      // 1/B * ln(R/R0)
  steinhart += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/T0)
  steinhart = 1.0 / steinhart;                     // reciprocal
  
  float tempK = steinhart;                         // Kelvin
  float tempC = tempK - 273.15;                    // Celsius

  Serial.print("온도: ");
  Serial.print(tempC);
  Serial.print(" °C / ");

  delay(1000); // 1초 대기
}