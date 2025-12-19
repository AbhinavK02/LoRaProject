/*
  Set up and functions for reading and integrating sensors
*/

#include "SensorManager.h"

// ############################### Variables #####################################
// Variables for Weight Sensor
HX711 scale;
uint8_t weightState_curr = 0;
uint8_t weightState_prev = 0;

// ############################### Functions ######################################
// Initialising all the sensors
void sensorManager_init() {
  // Weight Sensor
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); // initiliase weight sensor
  scale.set_scale(955.f); // Set the default (calibrated against scale)
  scale.tare();
  scale.power_down(); // Put HX711 to sleep to save power
  delay(500);
  SensorReading weightReading = readWeightSensor();
  weightState_prev = weightReading.value; // initial weight state

  // Light Sensor
  pinMode(LIGHT_SENSOR_PIN, INPUT); // Input mode for light sensor

  // Tilt Sensor
  pinMode(TILT_SENSOR_PIN, INPUT); // Input mode for light sensor

  // Setting up ADC for Battery Monitor
  analogReadResolution(12);     // 0–4095
  analogSetAttenuation(ADC_11db); // Allows full 0–3.3V range

  // End Intialisation
  Serial.println("Sensor Manager Initialized.");
}
// To read the Weight Sensor
SensorReading readWeightSensor() {
  scale.power_up(); // Wake HX711 from sleep
  delay(500); // short delay to ensure it wakes in time
  // Some variables
  int weightState = 0;
  long weight = 0;
  long weight_thres = 5.0; // Threshold [g] at which sensor alerts system
  
  SensorReading reading;
  reading.id = ID_WEIGHT_SENSOR; 

  if (scale.is_ready()) {
    weight = scale.get_units(10); // Average of 10 readings normalised (based on calibration)
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

  scale.power_down(); // Put HX711 to sleep to save power
  delay(500);
  return reading;
}
// To read the Light Sensor
SensorReading readLightSensor() {
  SensorReading reading;
  reading.id = ID_LIGHT_SENSOR; 
  
  int lightState = !digitalRead(LIGHT_SENSOR_PIN);
  reading.value = (uint8_t)lightState;

  Serial.print("[Sensor 0x02] Light State: ");
  Serial.println(lightState == HIGH ? "ON (1)" : "OFF (0)");
  
  return reading;
}
// To read the Tilt Sensor
SensorReading readTiltSensor() {
  SensorReading reading;
  reading.id = ID_TILT_SENSOR; 
  
  int tiltState = !digitalRead(TILT_SENSOR_PIN);
  reading.value = (uint8_t)tiltState;

  Serial.print("[Sensor 0x03] Tilt State: ");
  Serial.println(tiltState == HIGH ? "ON (1)" : "OFF (0)");
  
  return reading;
}
SensorReading batteryMonitor() {
    SensorReading reading;
    uint8_t batPercentage;
    reading.id = ID_BAT_MONITOR;

    int adcValue = analogRead(BAT_MONITOR_PIN);
    float adcVoltage = (adcValue * 3.3) / 4095;
    float batVoltage = adcVoltage * 1.5;
    if (batVoltage >= 4.20) {
        reading.value = 64;
    } else if (batVoltage <= 3.10) {
        reading.value = 0;
    } else {
        batPercentage = (batVoltage - 3.10) * 64 / (1.1);
        Serial.print("Battery Percentage: ");
        Serial.println(batPercentage);
        reading.value = batPercentage;
    }
    
    return reading;         
}
// Put MCU to sleep
void goToSleep(){
  Serial.println("Going to deep sleep...");

  // Configure Wake Interrupt
  esp_sleep_enable_ext1_wakeup((1ULL << LIGHT_SENSOR_PIN) | (1ULL << TILT_SENSOR_PIN), ESP_EXT1_WAKEUP_ANY_LOW);
   
  // Sleep
  Serial.flush();
  esp_deep_sleep_start();
}
// To detect new mail and build payload
uint16_t detectMail(uint8_t* payloadBuffer){
  uint16_t len = 0;  
  payloadBuffer[len++] = ID_DEVICE; // Device ID
  delay(40000); // Wait for mail to be placed
  SensorReading weightReading = readWeightSensor(); // read the weight sensor value
  SensorReading batteryReading = batteryMonitor(); // read the battery voltage
  weightState_curr = weightReading.value;         // Set current state from reading
  if (weightState_curr == weightState_prev) { // if no change, then someone is peaking 👀
    Serial.println("Tampering Detecting");
    payloadBuffer[len++] = ID_TAMPERING; // Encode Tampering code
  } else { // If state changed, look at if mail added or removed 
    Serial.print("Mail was ");
    Serial.println(weightState_curr == HIGH ? "Added" : "Removed"); // Mail is there?
    weightState_curr == HIGH ? payloadBuffer[len++] = ID_NEW_MAIL : payloadBuffer[len++] = ID_NO_MAIL;
  }
  // Also add the battery volatage into the payload
  payloadBuffer[len++] = batteryReading.id;
  payloadBuffer[len++] = batteryReading.value;
  weightState_prev = weightState_curr; // update weight state
  Serial.println(); 
  return len;
}