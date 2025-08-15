#include <Wire.h>

// default I2C address for GY-30
const byte GY302_ADDR = 0x23;

// I2C pins for ESP32-CAM 
const int I2C_SDA_PIN = 14;
const int I2C_SCL_PIN = 2;

void setup() {
  // start serial communication
  Serial.begin(115200);
  Serial.println("\nESP32-CAM GY-302 Light Sensor Test");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
}

void loop() {
  // send the command for "One-Time High-Resolution 1" 
  Wire.beginTransmission(GY302_ADDR);
  Wire.write(0x20);
  Wire.endTransmission();

  // wait for the measurement to complete. (this takes 120ms)
  delay(130);

  // request 2 bytes of data from the sensor 
  Wire.requestFrom(GY302_ADDR, 2);

  // check if two bytes were received
  if (Wire.available() >= 2) {
    // read the high and low bytes 
    byte highByte = Wire.read();
    byte lowByte = Wire.read();

    // combine the two bytes into a single 16-bit raw value
    unsigned int rawValue = (highByte << 8) | lowByte;

    // calculate the illuminance in lux using the correction factor 1.2 
    float lux = rawValue / 1.2;

    // print the results to the Serial Monitor
    Serial.print("Raw Value: ");
    Serial.print(rawValue);
    Serial.print("\t Illuminance: ");
    Serial.print(lux, 2); // print with 2 decimal places
    Serial.println(" lux");
    
  } else {
    Serial.println("Error: Could not read data from GY-302 sensor.");
  }

  // wait 2 seconds before the next measurement
  delay(2000);
}