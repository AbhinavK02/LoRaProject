/*
@author: Justin Chin Cheong (34140) and Abhinav Kothari (33349)
@desc: Complete Script for the Talking Mailbox Project to read all sensors and send data via LoRaWAN to the TTN
@version: V0.2 - operating w/ sensors, LoRaWAN 1.0.4
*/

// ############################### Libraries ######################################
#include "config.h"
#include "EEPROM.h"
#include "SensorManager.h"

// ############################### Variables and Configuration ######################################
#define UPLINK_PAYLOAD_MAX_LEN  256
uint8_t uplinkPayload[UPLINK_PAYLOAD_MAX_LEN] = {0};
uint16_t uplinkPayloadLen = 0;

// ############################### Setup ######################################
void setup() {
  Serial.begin(115200);

  sensorManager_init();

  // Radio Setup
  Serial.println(F("\nSetup... ")); 

  Serial.println(F("Initialise the radio"));
  int16_t state = radio.begin();
  debug(state!= RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

  // SX1262 rf switch order: setRfSwitchPins(rxEn, txEn);
  radio.setRfSwitchPins(38, RADIOLIB_NC);

  // Setup the OTAA session information
  state = node.beginOTAA(joinEUI, devEUI, NULL, appKey);
  debug(state != RADIOLIB_ERR_NONE, F("Initialise node failed"), state, true);
  
  // Restore Nonces from Flash
  store.begin("radiolib", false);

  if (store.isKey("nonces")) {
    Serial.println(F("Restoring LoRaWAN nonces"));
    store.getBytes("nonces", lwNonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
    state = node.setBufferNonces(lwNonces);
    debug(state != RADIOLIB_ERR_NONE, F("Restoring nonces failed"), state, false);
  } else {
    Serial.println(F("No stored nonces found"));
  }

  // Joining TTN 
  while (state != RADIOLIB_LORAWAN_NEW_SESSION) {
    Serial.println(F("Joining LoRaWAN..."));
    state = node.activateOTAA();

    if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
      Serial.println(F("Join successful, saving nonces"));

      uint8_t *persist = node.getBufferNonces();
      memcpy(lwNonces, persist, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
      store.putBytes("nonces", lwNonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
      break;
    }

    Serial.print(F("Join failed: "));
    Serial.println(state);
    delay(5000);
  }
  store.end();

  // Disable the ADR algorithm (on by default which is preferable)
  node.setADR(false);

  // Set a fixed datarate
  node.setDatarate(LORAWAN_UPLINK_DATA_RATE);

  // Manages uplink intervals to the TTN Fair Use Policy
  node.setDutyCycle(false);
  
  // Sending Ready Message to TTN
  // Serial.print("Attemping to send first message");
  // uplinkPayload[0] = ID_DEVICE;
  // uplinkPayload[1] = ID_READY;
  // state = node.sendReceive(uplinkPayload, 2);
  // while(state!= RADIOLIB_ERR_DOWNLINK_MALFORMED && state!= RADIOLIB_ERR_NONE) {
  //   Serial.print(".");
  //   delay(50);
  //   state = node.sendReceive(uplinkPayload, 2);
  //   delay(1000);
  // }
  // Serial.println();
  // Serial.println(F("First Uplink Successful"));

  // Ready to begin streaming
  Serial.println(F("Ready!\n"));

  // Reading Sensors and Building in Payload
  uplinkPayloadLen = detectMail(uplinkPayload);

  // Checking if payload is complete
  if (uplinkPayloadLen >= 2){ // if complete, upload
    // Printing upload info
    Serial.print("Uplink payload length: ");
    Serial.println(uplinkPayloadLen);
    // Output the uplink payload for debugging
    Serial.print("Uplink payload: ");
    for (int i = 0; i < uplinkPayloadLen; i++) {
      Serial.print(uplinkPayload[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // Checking LoRa State and Uploading
    int16_t state = node.sendReceive(uplinkPayload, uplinkPayloadLen);
    while(state!= RADIOLIB_ERR_DOWNLINK_MALFORMED && state!= RADIOLIB_ERR_NONE) {
      Serial.println("Error in sendReceive:");
      Serial.println(state);
      delay(50);
      state = node.sendReceive(uplinkPayload, uplinkPayloadLen);
      delay(1000);
    }
    Serial.println("Sending uplink successful!");
    Serial.println();
  }
  delay(1000);
  radio.sleep();
  goToSleep();
}
// ############################### Main Loop ######################################
void loop() {
  // Empty since sleep interrupt operates as the loop
}