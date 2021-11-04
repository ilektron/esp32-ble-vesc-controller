/**
   ESPNOW - Basic communication - Master
   Date: 26th September 2017
   Author: Arvind Ravulavaru <https://github.com/arvindr21>
   Purpose: ESPNow Communication between a Master ESP32 and a Slave ESP32
   Description: This sketch consists of the code for the Master module.
   Resources: (A bit outdated)
   a. https://espressif.com/sites/default/files/documentation/esp-now_user_guide_en.pdf
   b. http://www.esploradores.com/practica-6-conexion-esp-now/

   << This Device Master >>

   Flow: Master
   Step 1 : ESPNow Init on Master and set it in STA mode
   Step 2 : Start scanning for Slave ESP32 (we have added a prefix of `slave` to the SSID of slave for an easy setup)
   Step 3 : Once found, add Slave as peer
   Step 4 : Register for send callback
   Step 5 : Start Transmitting data from Master to Slave

   Flow: Slave
   Step 1 : ESPNow Init on Slave
   Step 2 : Update the SSID of Slave with a prefix of `slave`
   Step 3 : Set Slave in AP mode
   Step 4 : Register for receive callback and wait for data
   Step 5 : Once data arrives, print it in the serial monitor

   Note: Master and Slave have been defined to easily understand the setup.
         Based on the ESPNOW API, there is no concept of Master and Slave.
         Any devices can act as master or salve.
*/

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <array>

BluetoothSerial bts;
uint8_t address[6] = {0xD1, 0x9B, 0xFB, 0xDC, 0xAE, 0x6C};

// Set up our bluetooth connection
// TODO enable scanning for devices and selecting which bt device you want to connect to
void radio_init() {
  Serial.println("\n");
  if (!bts.begin("BTVESC", true))
  {
    Serial.println("Failed to initiate BT");
    for (;;) {}
  }
  Serial.println("Initialized BT");

  do {
    Serial.println("Connecting to vesc... ");
    delay(1000);
    if (bts.connect(address))
    {
      Serial.println("Connected to VESC");
    }
  } while (!bts.hasClient());
}

void radio_run() {
  
}