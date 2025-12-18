#include <Preferences.h>

Preferences store;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Clearing LoRaWAN stored data...");

  store.begin("radiolib", false);

  // Remove specific keys
  store.remove("nonces");
  store.remove("session");

  // OR wipe the entire namespace (safe if you only use it for RadioLib)
  // store.clear();

  store.end();

  Serial.println("Done. Power cycle and flash normal firmware.");
  while (true) delay(1000);
}

void loop() {}
