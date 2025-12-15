/*
@author: Justin Chin Cheong (34140) and Abhinav Kothari (33349)
@desc: Sketch to test integration of all sensors at once
*/
// ############################### Libraries ######################################
#include <Arduino.h>
#include <HX711.h>

// ############################### Configuration and Definitions ##################
// Pins
#define LIGHT_SENSOR_PIN 2
#define LOADCELL_DOUT_PIN 43
#define LOADCELL_SCK_PIN 6
#define TILT_SENSOR_PIN 44

// Define IDs for payload encoding 
#define ID_WEIGHT_SENSOR 0x01
#define ID_LIGHT_SENSOR 0x02
#define ID_TILT_SENSOR 0x03   

// ############################### Data Structure ################################
// A simple struct to hold one sensor's reading
struct SensorReading
{
    uint8_t id;    // Unique for each sensor ID 
    uint8_t value; // The actual value 
};
// ############################### Variables ######################################
// Variables for Weight Sensor
HX711 scale;
int weightState = 0;
long weight = 0;
long weight_thres = 15.0; // Threshold [g] at which sensor alerts system

// ############################### Functions ######################################
void sensorManager_init() {
  // Weight Sensor
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); // initiliase weight sensor
  scale.set_scale(940.f); // Set the default (calibrated against scale)
  scale.tare();

  // Light Sensor
  pinMode(LIGHT_SENSOR_PIN, INPUT); // Input mode for light sensor

  // Tilt Sensor
  pinMode(TILT_SENSOR_PIN, INPUT); // Input mode for light sensor

  // End Intialisation
  Serial.println("Sensor Manager Initialized.");
}

SensorReading readWeightSensor() {
  SensorReading reading;
  reading.id = ID_WEIGHT_SENSOR; 

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

  reading.value = (uint8_t)weightState; //storing state value

  Serial.print("[Sensor 0x01] Weight: ");
  Serial.print(weight);
  Serial.print(" Weight State: ");
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
  
  int tiltState = digitalRead(TILT_SENSOR_PIN);
  reading.value = (uint8_t)tiltState;

  Serial.print("[Sensor 0x03] Tilt State: ");
  Serial.println(tiltState == HIGH ? "ON (1)" : "OFF (0)");
  
  return reading;
}
// ############################### Setup ######################################
void setup() {
  Serial.begin(115200);
  sensorManager_init();
}
// ############################### Main Loop ######################################
void loop() {
  // Weight Sensor
  SensorReading weightReading = readWeightSensor();
  // Light Sensor
  SensorReading lightReading = readLightSensor();
  // Tilt Sensor
  SensorReading tiltReading = readTiltSensor();
  delay(1000);
}
