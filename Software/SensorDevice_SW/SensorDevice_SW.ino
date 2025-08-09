#include "Dashboard.h"

Dashboard dashboard;

void setup() {
    Serial.begin(115200);
  dashboard.begin();
}

void loop() {
  dashboard.loop();

  if (dashboard.isConnected()) {
    // 
  }
}