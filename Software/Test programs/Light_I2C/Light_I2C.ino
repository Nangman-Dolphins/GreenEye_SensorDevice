#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  
  /*
    BH1750 Initialize (Default I2C addr 0x23)
    - CONTINUOUS_HIGH_RES_MODE:   연속 측정, 고해상도 (1 lx), 측정 시간 120ms (Default)
    - CONTINUOUS_HIGH_RES_MODE_2: 연속 측정, 더 높은 해상도 (0.5 lx), 측정 시간 120ms
    - CONTINUOUS_LOW_RES_MODE:    연속 측정, 저해상도 (4 lx), 측정 시간 16ms
    - ONE_TIME_HIGH_RES_MODE:     한 번만 측정 후 절전 모드, 고해상도
    - ONE_TIME_LOW_RES_MODE:      한 번만 측정 후 절전 모드, 저해상도
  */
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 Initialize Complete."));
  } else {
    Serial.println(F("Cannot find BH1750"));
    while (1) delay(10); 
  }
}

void loop() {
  float lux = lightMeter.readLightLevel();

  Serial.print("광도: ");
  Serial.print(lux);
  Serial.println(" lx");

  delay(1000);
}