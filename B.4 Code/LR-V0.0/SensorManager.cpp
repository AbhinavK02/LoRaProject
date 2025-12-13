/*
  Set up and functions for reading and integrating sensors
*/

// Libraries
#include "SensorManager.h"
#include <HX711.h>

HX711 scale;
uint8_t weight_thres = 15;

void sensorManager_init() {
  pinMode(LIGHT_SENSOR_PIN, INPUT); // Input mode for light sensor
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); // initiliase weight sensor
  Serial.println("Sensor Manager Initialized.");
}

SensorReading readWeightSensor() {
  SensorReading reading;
  int weightState = 0;
  reading.id = ID_WEIGHT_SENSOR; 
  
  if (scale.is_ready()) {
    long weight = scale.read();
    if (weight > weight_thres) { // If weight is above our threshold
      weightState = 1;
    } else {
      weightState = 0;
    }
  } else {
    Serial.println("HX711 not found.");
  }

  reading.value = (uint8_t)weightState; //shifting the decimal point

  Serial.print("[Sensor 0x01] Weight State: ");
  Serial.println(weightState == HIGH ? "ON (1)" : "OFF (0)");
  
  return reading;
}

SensorReading readLightSensor() {
  SensorReading reading;
  reading.id = ID_LIGHT_SENSOR; 
  
  int lightState = digitalRead(LIGHT_SENSOR_PIN);
  reading.value = (uint8_t)lightState;

  Serial.print("[Sensor 0x02] Light State: ");
  Serial.println(lightState == HIGH ? "ON (1)" : "OFF (0)");
  
  return reading;
}

SensorReading readTiltSensor() {
  SensorReading reading;
  reading.id = ID_TILT_SENSOR; 
  
  int tiltState = digitalRead(LIGHT_SENSOR_PIN);
  reading.value = (uint8_t)tiltState;

  Serial.print("[Sensor 0x03] Tilt State: ");
  Serial.println(tiltState == HIGH ? "ON (1)" : "OFF (0)");
  
  return reading;
}

uint16_t collectAndEncodeData(uint8_t* payloadBuffer) { // uses pointer to affect global payload variable
  uint16_t len = 0;
  
  // Weight Sensor
  SensorReading weightReading = readWeightSensor();
  payloadBuffer[len++] = weightReading.id;     // 1 byte ID 
  payloadBuffer[len++] = weightReading.value;  // 1 byte Value (0 or 1)

  // Light Sensor
  SensorReading lightReading = readLightSensor();
  payloadBuffer[len++] = lightReading.id;     // 1 byte ID 
  payloadBuffer[len++] = lightReading.value;  // 1 byte Value (0 or 1)

  // Tilt Sensor
  SensorReading tiltReading = readTiltSensor();
  payloadBuffer[len++] = tiltReading.id;     // 1 byte ID 
  payloadBuffer[len++] = tiltReading.value;  // 1 byte Value (0 or 1)
  
  return len;
}