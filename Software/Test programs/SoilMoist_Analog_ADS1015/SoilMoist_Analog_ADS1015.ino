// include library for I2C communication
#include <Wire.h>

// I2C Address for ADS1015
const byte ADS1015_ADDRESS = 0x48;

// I2C pins for your ESP32-CAM
const int I2C_SDA_PIN = 14;
const int I2C_SCL_PIN = 2;

// Soil Moisture Sensor Enable Pin
const int SOIL_MOISTURE_EN_PIN = 15;

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
  Serial.println("\nADS1015 AIN1 Read Test (with Enable Pin Control)");
  
  // start I2C with specific pins for ESP32-CAM
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  
  // set the enable pin as an output
  pinMode(SOIL_MOISTURE_EN_PIN, OUTPUT);
  
  
  // wait 300ms for the sensor to initialize and stabilize
  digitalWrite(SOIL_MOISTURE_EN_PIN, HIGH);
  delay(5000);
}

void loop() {
  

  // read the analog value from AIN1
  const byte adc_channel = 1;
  int16_t adcValue16bit = readSingleEnded(adc_channel);

  
  // process and Print Data ---
  int16_t adcValue = adcValue16bit >> 4;
  float voltage = adcValue * 0.002;

  Serial.print("Reading Channel AIN");
  Serial.print(adc_channel);
  Serial.print(" -> ADC: ");
  Serial.print(adcValue);
  Serial.print(", Voltage: ");
  Serial.print(voltage, 4);
  Serial.println(" V");

  // wait before the next measurement cycle
  delay(500);
}