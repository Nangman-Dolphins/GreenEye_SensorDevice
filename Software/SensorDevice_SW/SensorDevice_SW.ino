#include "Dashboard.h"

// Dummy datas
float ambient_temp = 25.3;
float ambient_humidity = 62.1;
float light_intensity = 15000;
float soil_temp = 22.8;
float soil_moisture = 55.4;
float soil_ec = 1250;

Dashboard dashboard(
    &ambient_temp, 
    &ambient_humidity, 
    &light_intensity, 
    &soil_temp, 
    &soil_moisture, 
    &soil_ec,
    "defaultPW", // AP's PW
    true         // activate debug mode
);

void setup() {
  dashboard.begin();
}

void loop() {
  dashboard.loop();
}