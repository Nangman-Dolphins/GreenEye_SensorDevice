// include library for I2C communication
#include <Wire.h>

// I2C Address for ADS1015
const byte ADS1015_ADDRESS = 0x48;

// I2C pins for your ESP32-CAM
const int I2C_SDA_PIN = 14;
const int I2C_SCL_PIN = 2;

// EC Probe control pins
#define EC_PIN1 12
#define EC_PIN2 13

// ADC channel for EC measurement
#define EC_READ_CHANNEL 3 // AIN3

// Fixed resistor value (R2 = 1kΩ)
#define R1 1000.0

// helper function to read a single-ended channel from the ADS1015
int16_t readSingleEnded(byte channel) {
  if (channel > 3) {
    return 0;
  }
  uint16_t config = 0;
  config |= (1 << 15); // OS: Start a single conversion
  config |= ((4 + channel) << 12); // MUX: Set channel (AIN0-3 vs GND)
  config |= (1 << 9);  // PGA: Gain 1, for +/- 4.096V range
  config |= (1 << 8);  // MODE: Single-shot mode
  config |= (4 << 5);  // DR: 1600 Samples per second (default)
  config |= 3;         // COMP: Comparator disabled
  Wire.beginTransmission(ADS1015_ADDRESS);
  Wire.write(0x01);
  Wire.write((config >> 8) & 0xFF);
  Wire.write(config & 0xFF);
  Wire.endTransmission();
  delay(2);
  Wire.beginTransmission(ADS1015_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(ADS1015_ADDRESS, 2);
  if (Wire.available() == 2) {
    int16_t result = (Wire.read() << 8) | Wire.read();
    return result;
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nSoil EC Sensor Test with ADS1015");
  
  // start I2C with specific pins
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
}

void loop() {
  // --- First measurement: EC_PIN1 -> EC_PIN2 ---
  pinMode(EC_PIN1, OUTPUT);
  pinMode(EC_PIN2, OUTPUT);
  delay(10);

  digitalWrite(EC_PIN1, HIGH);
  digitalWrite(EC_PIN2, LOW);
  delay(15); // allow voltage to stabilize
  
  // READ from AIN3
  int16_t reading1_16bit = readSingleEnded(EC_READ_CHANNEL);
  int reading1 = reading1_16bit >> 4; // convert to 12-bit

  digitalWrite(EC_PIN1, LOW);
  digitalWrite(EC_PIN2, LOW);
  delay(15); // stabilize
  
  // --- Second measurement: EC_PIN1 <- EC_PIN2 ---
  digitalWrite(EC_PIN1, LOW);
  digitalWrite(EC_PIN2, HIGH);
  delay(15); // allow voltage to stabilize
  
  // READ from AIN3 again
  int16_t reading2_16bit = readSingleEnded(EC_READ_CHANNEL);
  int reading2 = reading2_16bit >> 4; // convert to 12-bit

  // --- Turn probes off ---
  digitalWrite(EC_PIN1, LOW);
  digitalWrite(EC_PIN2, LOW);
  pinMode(EC_PIN1, INPUT);
  pinMode(EC_PIN2, INPUT);
  delay(10);

  // --- Calculations ---
  // Calculate AVG reading, compensating for 12-bit range (0-2047)
  float avg_reading = (reading1 + (2047 - reading2)) / 2.0;

  // Calculate the resistance of soil (R2) using 12-bit range
  float soil_resistance = (R1 * avg_reading) / (2047.0 - avg_reading);

  // ECC = 1/R, convert to microSiemens (uS)
  float ec = 1000000.0 / soil_resistance;

  // --- Print results ---
  Serial.print("ADC Avg: ");
  Serial.print(avg_reading);
  Serial.print(", ");

  Serial.print("Soil RES: ");
  Serial.print(soil_resistance);
  Serial.print(" Ohm, ");
  
  Serial.print("Relative EC: ");
  Serial.print(ec);
  Serial.println(" uS/cm");

  delay(2000);
}