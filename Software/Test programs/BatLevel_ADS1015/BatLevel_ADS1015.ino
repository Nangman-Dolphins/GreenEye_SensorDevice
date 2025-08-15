// include library for I2C communication
#include <Wire.h>

// I2C Address for ADS1015
const byte ADS1015_ADDRESS = 0x48;

// I2C pins for your ESP32-CAM from the schematic
const int I2C_SDA_PIN = 14;
const int I2C_SCL_PIN = 2;

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
  Serial.println("\nBattery Level Test with ADS1015");
  
  // start I2C with specific pins
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
}

void loop() {
  // set the channel to read (2 = AIN2 for BAT_LEVEL)
  const byte adc_channel = 2;

  // read the raw ADC value from the AIN2 channel
  int16_t adcValue16bit = readSingleEnded(adc_channel);

  // shift right by 4 to get the actual 12-bit value
  int16_t adcValue = adcValue16bit >> 4;

  // convert the raw 12-bit ADC value to the voltage at the divider
  float dividerVoltage = adcValue * 0.002;

  // calculate the actual battery voltage.
  // the voltage divider halves the voltage, so multiply by 2.
  float batteryVoltage = dividerVoltage * 2.0;

  // print the results to the Serial Monitor
  Serial.print("Reading Channel AIN");
  Serial.print(adc_channel);
  Serial.print(" -> Divider Voltage: ");
  Serial.print(dividerVoltage, 3);
  Serial.print(" V, ");
  Serial.print("Battery Voltage: ");
  Serial.print(batteryVoltage, 3);
  Serial.println(" V");

  delay(2000);
}