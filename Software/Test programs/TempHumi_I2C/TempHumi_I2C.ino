#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

void setup() {
  Serial.begin(9600);

  // AHT20 Initialize (Default I2C addr 0x38)
  if (!aht.begin()) {
    Serial.println("Cannot find AHT");
    while (1) delay(10);
  }

  // BMP280 Initialize (Default I2C addr 0x77)
  if (!bmp.begin()) {
    Serial.println("Cannot find BMP");
    while (1) delay(10);
  }

}

void loop() {
  sensors_event_t humidity, temp;
  
  aht.getEvent(&humidity, &temp);

  Serial.println("==================");

  Serial.print("온도: ");
  Serial.print(temp.temperature);
  Serial.println(" °C");

  Serial.print("습도: ");
  Serial.print(humidity.relative_humidity);
  Serial.println(" %");

  Serial.print("기압: ");
  Serial.print(bmp.readPressure() / 100.0F); // hPa 단위로 변환
  Serial.println(" hPa");

  delay(2000); // 2초마다 측정
}