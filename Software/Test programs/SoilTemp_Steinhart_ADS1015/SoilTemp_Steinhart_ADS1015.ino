// include library for I2C communication
#include <Wire.h>
// include library for log() function
#include <math.h>

// I2C Address for ADS1015
const byte ADS1015_ADDRESS = 0x48;

// I2C pins for ESP32-CAM
const int I2C_SDA_PIN = 14;
const int I2C_SCL_PIN = 2;


// VCC --- [ NTC ] --- (A0) --- [ Fixed Resistor ] --- GND
#define THERMISTOR_PIN 0         // ADS1015 channel 0 (AIN0)
#define FIXED_RESISTOR 10000.0   // Fixed Resistor (10KΩ)
#define NOMINAL_RESISTANCE 10000.0 // NTC's Ref Resistor (10KΩ at 25°C)
#define NOMINAL_TEMPERATURE 25.0 // NTC's Ref Temperature (25°C at 10KΩ)

// Correct B-Coefficient based on the MF58 datasheet
#define B_COEFFICIENT 3950.0     // NTC's B value (3650K in datasheet)

// [IMPORTANT] Set this to the voltage powering your thermistor circuit (e.g., 3.3 or 5.0)
#define SUPPLY_VOLTAGE 3.3

// helper function to read a single-ended channel from the ADS1015
int16_t readSingleEnded(byte channel) {
  if (channel > 3) {
    return 0;
  }

  // build the configuration word to start a measurement
  uint16_t config = 0;
  config |= (1 << 15); // OS: Start a single conversion
  config |= ((4 + channel) << 12); // MUX: Set channel (AIN0-3 vs GND)
  config |= (1 << 9);  // PGA: Gain 1, for +/- 4.096V range
  config |= (1 << 8);  // MODE: Single-shot mode
  config |= (4 << 5);  // DR: 1600 Samples per second (default)
  config |= 3;         // COMP: Comparator disabled

  // write the configuration to the ADS1015 to trigger the conversion
  Wire.beginTransmission(ADS1015_ADDRESS);
  Wire.write(0x01); // Point to the Config register
  Wire.write((config >> 8) & 0xFF); // Write high byte
  Wire.write(config & 0xFF);        // Write low byte
  Wire.endTransmission();

  // wait for the conversion to complete (1/1600s = 0.625ms)
  delay(2);

  // point to the Conversion register to read the result
  Wire.beginTransmission(ADS1015_ADDRESS);
  Wire.write(0x00); // Point to the Conversion register
  Wire.endTransmission();

  // request and read the 2-byte result
  Wire.requestFrom(ADS1015_ADDRESS, 2);
  if (Wire.available() == 2) {
    int16_t result = (Wire.read() << 8) | Wire.read();
    return result;
  }
  return 0;
}


void setup() {
  Serial.begin(115200);
  
  // start I2C with specific pins for ESP32-CAM
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
}

void loop() {
  // read the raw ADC value from the ADS1015 on the specified channel
  int16_t adcValue16bit = readSingleEnded(THERMISTOR_PIN);

  // the result is a 12-bit value left-aligned in a 16-bit integer.
  // shift right by 4 to get the actual 12-bit value.
  int16_t adcValue = adcValue16bit >> 4;

  // convert the raw 12-bit ADC value to a voltage
  // for +/- 4.096V range, the resolution is 2mV per bit.
  float voltage = adcValue * 0.002;

  // calculate the resistance of the thermistor using the REVERSED voltage divider formula
  // R_thermistor = R_fixed * (V_in - V_out) / V_out
  float resistance = FIXED_RESISTOR * ( (SUPPLY_VOLTAGE - voltage) / voltage );

  // Steinhart-Hart equation
  float steinhart;
  steinhart = resistance / NOMINAL_RESISTANCE;        // (R/R0)
  steinhart = log(steinhart);                         // ln(R/R0)
  steinhart /= B_COEFFICIENT;                         // 1/B * ln(R/R0)
  steinhart += 1.0 / (NOMINAL_TEMPERATURE + 273.15);  // + (1/T0)
  steinhart = 1.0 / steinhart;                        // reciprocal
  
  float tempK = steinhart;                            // Kelvin
  float tempC = tempK - 273.15;                       // Celsius

  Serial.print("ADC: ");
  Serial.print(adcValue);
  Serial.print(", Resistance: ");
  Serial.print(resistance);
  Serial.print(" Ohms, ");
  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.println(" °C");

  delay(1000);
}