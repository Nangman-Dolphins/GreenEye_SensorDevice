#include <Wire.h>

// AHT20 I2C address
const byte AHT20_ADDR = 0x38;

// I2C pins for ESP32-CAM
const int I2C_SDA_PIN = 14;
const int I2C_SCL_PIN = 2;

// constant for 2^20
const float AHT20_RESOLUTION = 1048576.0;

void setup() {
  // start serial communication
  Serial.begin(115200);
  Serial.println("\nESP32-CAM AHT20 Temp/Humidity Sensor Test (Optimized)");

  // start I2C with specific pins for ESP32-CAM
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // initialize the AHT20 sensor
  // based on the manual, send 0xBE command
  Wire.beginTransmission(AHT20_ADDR);
  Wire.write(0xBE);
  Wire.endTransmission();
  
  // wait a bit after initialization
  delay(100); 
  Serial.println("AHT20 Initialized.");
}

void loop() {
  // trigger measurement
  // send 0xAC command followed by 0x33 and 0x00
  Wire.beginTransmission(AHT20_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  Wire.endTransmission();

  // wait for measurement to complete (it takes 75ms)
  delay(80);

  // read 7 bytes of data (Status + 5 data bytes + CRC)
  Wire.requestFrom(AHT20_ADDR, 7);
  
  if (Wire.available() >= 7) {
    byte sensorData[7];
    for (int i=0; i < 7; i++) {
      sensorData[i] = Wire.read();
    }

    // parse the raw humidity and temperature data
    // based on the data structure table in the manual
    unsigned long rawHumidity = ((unsigned long)sensorData[1] << 16 | (unsigned long)sensorData[2] << 8 | sensorData[3]) >> 4;
    unsigned long rawTemp = (((unsigned long)sensorData[3] & 0x0F) << 16 | (unsigned long)sensorData[4] << 8 | sensorData[5]);
    
    // apply the formulas from the manual using the constant
    float humidity = ( (float)rawHumidity / AHT20_RESOLUTION ) * 100.0;
    float temperature = ( ( (float)rawTemp / AHT20_RESOLUTION ) * 200.0 ) - 50.0;

    Serial.print("Temperature: ");
    Serial.print(temperature, 2);
    Serial.print(" *C");
    
    Serial.print("\t Humidity: ");
    Serial.print(humidity, 2);
    Serial.println(" %");

  } else {
    Serial.println("Failed to read from AHT20 sensor!");
  }

  // wait 2 seconds before the next reading
  delay(2000);
}