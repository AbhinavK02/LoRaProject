#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 43;
const int LOADCELL_SCK_PIN = 6;
int weightState = 0;
long weight = 0;
long weight_thres = 15.0;
HX711 scale;

void setup() {
  Serial.begin(38400);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(940.f); // Set the default 
  scale.tare();
  // if (scale.is_ready()) {
  //   scale.set_scale(940.f); // Set the default 
  //   scale.tare();		
    
  // } else {
  //   Serial.println("HX711 not found.");
  // }
}

void loop() {

  if (scale.is_ready()) {
    weight = scale.get_units(10);
    if (weight > weight_thres) { // If weight is above our threshold
      weightState = 1;
    } else {
      weightState = 0;
    }
  } else {
    Serial.println("HX711 not found.");
  }
  
  Serial.print("[Sensor 0x01] Weight: ");
  Serial.print(weight);
  Serial.print(" Weight State: ");
  Serial.println(weightState == HIGH ? "ON (1)" : "OFF (0)");

  delay(1000);
  
}
