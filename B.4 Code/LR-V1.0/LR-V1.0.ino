/*
@author: Justin Chin Cheong (34140) and Abhinav Kothari (33349)
@desc: Complete Script for the Talking Mailbox Project to read all sensors and send data via LoRaWAN to the TTN
@version: V1.0 - fully operational prototype w/ sensors, LoRaWAN 1.0.4, deep sleep, battery monitoring, mail categorization
*/

// ############################### Libraries ######################################
#include "LoRaConfig.h"
#include "SensorManager.h"

// ############################### Setup ######################################
void setup()
{
  Serial.begin(115200);

  // Initialising all the Sensors
  sensorManager_init();

  // Radio Setup
  Serial.println(F("\nSetup... "));

  Serial.println(F("Initialise the radio"));
  int16_t state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

  // SX1262 rf switch order: setRfSwitchPins(rxEn, txEn);
  radio.setRfSwitchPins(38, RADIOLIB_NC);

  // Activating the node for LoRaWAN
  state = lwActivate();
  Serial.println(F("Ready!\n")); // Ready to begin uploading

  // Reading Sensors and Building in Payload
  uplinkPayloadLen = detectMail(uplinkPayload);

  // Uploading the Payload
  state = lwUplink(uplinkPayload);

  delay(5000);

  // Sleep
  radio.sleep();
  goToSleep();
}
// ############################### Main Loop ######################################
void loop()
{
  // Empty since sleep interrupt operates as the loop
}