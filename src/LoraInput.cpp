#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <math.h>
#include <stdlib.h>
#include "SmartTankState.h"
#include "TankSettings.h"
#include "FeatureControl.h"

#define LORA_SCK  14
#define LORA_MISO 13
#define LORA_MOSI 15
#define LORA_CS   16
#define LORA_RST  17
#define LORA_DIO0 18
#define LORA_FREQUENCY 433E6
// #define REMOTE_TIMEOUT 7000

static SPIClass loraSPI(FSPI);

void initializeLoRa() {
  if (!featureLoraEnabled()) {
    loraInitialized = false;
    remoteConnected = false;
    sensorReadingValid = false;
    waterLevelValid = false;
    return;
  }
  loraSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setSPI(loraSPI);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("ERROR: LoRa initialization failed!");
    loraInitialized = false;
    return;
  }
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  loraInitialized = true;
  Serial.println("LoRa initialized successfully");
}

void receiveSensorData() {
  if (!featureLoraEnabled() || !loraInitialized) return;
  int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) return;

  String payload = "";
  while (LoRa.available()) payload += (char)LoRa.read();
  payload.trim();

  lastRSSI = LoRa.packetRssi();
  lastSNR  = LoRa.packetSnr();
  lastPacketMillis = millis();
  remoteConnected = true;
  packetsReceived++;
  Serial.print("Received: "); Serial.println(payload);

  if (payload == "DIST:ERR") {
    sensorReadingValid = false;
    waterLevelValid = false;
    sensorErrors++;
    return;
  }
  if (payload.startsWith("DIST:")) {
    String value = payload.substring(5);
    char* endPointer = nullptr;
    float newDistance = strtof(value.c_str(), &endPointer);
    if (endPointer == value.c_str() || *endPointer != '\0' ||
        !isfinite(newDistance) || newDistance <= 0.0f) {
      sensorReadingValid = false; waterLevelValid = false; invalidPackets++;
      return;
    }
    distanceCm = newDistance;
    sensorReadingValid = true;
    calculateWaterLevel(distanceCm);
    return;
  }
  invalidPackets++;
  sensorReadingValid = false;
  waterLevelValid = false;
}

void updateConnectionStatus() {
  remoteConnected = featureLoraEnabled() && lastPacketMillis != 0 &&
                    millis() - lastPacketMillis < REMOTE_TIMEOUT;
  if (!remoteConnected) { sensorReadingValid = false; waterLevelValid = false; }
}

void setLoraReceiverEnabled(bool enabled) {
  featureSetLoraEnabled(enabled);
  if (!enabled) {
    if (loraInitialized) LoRa.sleep();
    loraInitialized = false;
    remoteConnected = false;
    lastPacketMillis = 0;
    sensorReadingValid = false;
    waterLevelValid = false;
    Serial.println("LoRa receiver disabled from admin configuration");
    return;
  }
  initializeLoRa();
}
