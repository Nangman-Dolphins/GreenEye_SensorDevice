#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "SensorsIO.h" // include the sensor io class

// === 1. Global Variables for Sensor Data ===
int   battery_level    = 0;    // holds the battery level percentage
float ambient_temp     = 0.0;  // holds ambient temperature
float ambient_humidity = 0.0;  // holds ambient humidity
float light_intensity  = 0.0;  // holds light intensity
float soil_temp        = 0.0;  // holds soil temperature
float soil_moisture    = 0.0;  // holds soil moisture
float soil_ec          = 0.0;  // holds soil electrical conductivity

// === 2. Create SensorIO Object ===
// pass the addresses of the global variables to the constructor
SensorIO sensors(
    &battery_level,
    &ambient_temp,
    &ambient_humidity,
    &light_intensity,
    &soil_temp,
    &soil_moisture,
    &soil_ec,
    true
);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

  Serial.begin(115200); // initialize serial communication at 115200 baud
  sensors.begin();      // initialize all sensors

  delay(2000); // wait a second for everything to stabilize
}

void loop() {
  // read all sensor values every 2 seconds
  sensors.readAllSensors();

  // --- [MODIFIED] Print data with labels for the Serial Plotter ---
  
  Serial.print("\n=====\nAmbientTemp:");
  Serial.print(ambient_temp);
  Serial.print(", ");
  
  Serial.print("Humidity:");
  Serial.print(ambient_humidity);
  Serial.print(",");

  Serial.print("Light:");
  Serial.print(light_intensity);
  Serial.print("\n");

  Serial.print("SoilTemp:");
  Serial.print(soil_temp);
  Serial.print(", ");

  Serial.print("SoilMoisture:");
  Serial.print(soil_moisture);
  Serial.print(", ");

  Serial.print("EC:");
  Serial.print(soil_ec);
  Serial.print(", ");

  Serial.print("Battery:");
  Serial.print(battery_level); // use println for the last value
  Serial.print("\n=====\n");

  delay(1000); // wait 2 seconds before the next reading
}