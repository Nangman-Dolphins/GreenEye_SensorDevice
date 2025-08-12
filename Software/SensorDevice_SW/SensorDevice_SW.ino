#include "Dashboard.h"
#include "Camera.h"

// Dummy datas
int   battery_level = 100;
float ambient_temp = 25.3;
float ambient_humidity = 62.1;
float light_intensity = 15000;
float soil_temp = 22.8;
float soil_moisture = 55.4;
float soil_ec = 1250;
String ccu_address = "";

Camera camera;

Dashboard dashboard(
    &battery_level,
    &ambient_temp, 
    &ambient_humidity, 
    &light_intensity, 
    &soil_temp, 
    &soil_moisture, 
    &soil_ec,
    &camera,      // camera
    &ccu_address,
    "defaultPW",  // AP's PW
    true          // activate debug mode
);


void setup() {
  Serial.begin(115200);

  dashboard.begin();

  if(!camera.begin()){
    Serial.println("Failed to start camera");
    return;
  }
}

void loop() {
  dashboard.loop();
}