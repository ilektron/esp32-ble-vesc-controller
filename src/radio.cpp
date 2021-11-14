#include "datatypes.h"
#include "packet.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <array>

// The remote Nordic UART service service we wish to connect to.
// This service exposes two characteristics: one for transmitting and one for receiving (as seen from the client).
static BLEUUID serviceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");

// The characteristics of the above service we are interested in.
// The client can send data to the server by writing to this characteristic.
static BLEUUID charUUID_RX("6e400002-b5a3-f393-e0a9-e50e24dcca9e"); // RX Characteristic

// If the client has enabled notifications for this characteristic,
// the server can send data to the client as notifications.
static BLEUUID charUUID_TX("6e400003-b5a3-f393-e0a9-e50e24dcca9e"); // TX Characteristic

static BLEAddress *pServerAddress;

// This is a messaging tool to let the init function know that a proper BLE device is connected
// Should change to a task notification for better performance?
static SemaphoreHandle_t xDoConnect;

static BLERemoteCharacteristic *pTXCharacteristic;
static BLERemoteCharacteristic *pRXCharacteristic;

// Our buffer for sending and receiving packets
constexpr const auto PACKET_QUEUE_SIZE = 5u;

QueueHandle_t rxPackets;
QueueHandle_t txPackets;

static vesc::payload<vesc::PACKET_MAX_PL_LEN> rx_buffer;
// This is the callback functions that gets data from the ble service, which in this case should be data that the vesc
// sends back to us. Is this called in an ISR?

// So, this should read in bytes from BLE and populate payloads to stick into the rxPackets queue
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length,
                           bool isNotify) {

  // Warning: This is a large object and we need to
  Serial.println("Notify callback for TX characteristic received. Data:");
  rx_buffer.append(pData, length);
  // For debug purposes for now, print out all the data
  for (int i = 0; i < length; i++) {
    // Serial.print((char)pData[i]);     // Print character to uart
    Serial.printf("0x%x", pData[i]); // print raw data to uart
    Serial.print(" ");
  }
  Serial.println();
}

bool connectToServer(BLEAddress pAddress) {
  Serial.print("Establishing a connection to device address: ");
  Serial.println(pAddress.toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  // Connect to the remove BLE Server.
  pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM);
  Serial.println(" - Connected to server");

  // Obtain a reference to the Nordic UART service on the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find Nordic UART service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return false;
  }
  Serial.println(" - Remote BLE service reference established");

  // Obtain a reference to the TX characteristic of the Nordic UART service on the remote BLE server.
  pTXCharacteristic = pRemoteService->getCharacteristic(charUUID_TX);
  if (pTXCharacteristic == nullptr) {
    Serial.print("Failed to find TX characteristic UUID: ");
    Serial.println(charUUID_TX.toString().c_str());
    return false;
  }
  Serial.println(" - Remote BLE TX characteristic reference established");

  // Read the value of the TX characteristic.
  std::string value = pTXCharacteristic->readValue();
  Serial.print("The characteristic value is currently: ");
  Serial.println(value.c_str());

  pTXCharacteristic->registerForNotify(notifyCallback);

  // Obtain a reference to the RX characteristic of the Nordic UART service on the remote BLE server.
  pRXCharacteristic = pRemoteService->getCharacteristic(charUUID_RX);
  if (pRXCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID_RX.toString().c_str());
    return false;
  }
  Serial.println(" - Remote BLE RX characteristic reference established");

  // TODO Send a simple packet for requesting firmware information
  vesc::payload<1u> p = {COMM_FW_VERSION};

  // Send the packet

  return true;
}

// Scan for BLE servers and find the first one that advertises the Nordic UART service.
class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
      Called for each advertising BLE server.
  */
  void onResult(BLEAdvertisedDevice advertisedDevice) {

    // We have found a device, check to see if it contains the Nordic UART service.
    if (advertisedDevice.haveServiceUUID()) {
      // Devices can have multiple service UUIDs
      for (auto i = 0ul; i < advertisedDevice.getServiceUUIDCount(); i++) {
        if (advertisedDevice.getServiceUUID(i).equals(serviceUUID)) {
          Serial.print("BLE Advertised Device found - ");
          Serial.println(advertisedDevice.toString().c_str());

          Serial.println("Found a device with the desired ServiceUUID!");
          advertisedDevice.getScan()->stop();

          pServerAddress = new BLEAddress(advertisedDevice.getAddress());
          doConnect = true;
        }
      }
    }
  }
};

// Set up our bluetooth connection
void radio_init() {
  Serial.println("Starting Arduino BLE Central Mode (Client) Nordic UART Service");

  // Create the semaphore for allowing us to move forward once we're connected to a BLE device
  xDoConnect = xSemaphoreCreateBinary();

  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device. Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  // Start the scan. If we get a matching device, the scan stops
  // TODO might need a pairing mode that listens for all devices and then allows a user
  // to select the desired device
  pBLEScan->start(30);

  // Here we should block until we find a device and then unblock once we can move forward
  if (xSemaphoreTake(xDoConnect, 30000u / portTICK_PERIOD_MS) == pdTRUE) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server. Reading firmware information");
      Serial.println("Should we just reboot?");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
  }

  vSemaphoreDelete(xDoConnect);
}

// This is poorly structured and rather than using simple flags, a proper FreeRTOS blocking semaphore would be better to
// ensure that everything happens in the correct order and we just don't run if we're waiting for something

void radio_run() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  Serial.print("Running radio");

  // This is the main body of our loop to make sure that things are getting processed
  if (connected) {

    // Once we're connected we should check our txPackets for packets we need to send to the ble device and then check
    // to see if we've received data from the device. If we have a full packet, go ahead and process the packet and
    // notify anybody who needs that updated data is available

    // pRXCharacteristic->writeValue(timeSinceBoot.c_str(), timeSinceBoot.length());

    // Read any data that's available

    // Check if we have something to write out in our queue
    // pRXCharacteristic->writeValue()
  }
}

/* Central Mode (client) BLE UART for ESP32
 *
 * This sketch is a central mode (client) Nordic UART Service (NUS) that connects automatically to a peripheral (server)
 * Nordic UART Service. NUS is what most typical "blueart" servers emulate. This sketch will connect to your BLE uart
 * device in the same manner the nRF Connect app does.
 *
 * A brief explanation of BLE client/server actions and rolls:
 *
 * Central Mode (client) - Connects to a peripheral (server).
 *   -Scans for devices and reads service UUID.
 *   -Connects to a server's address with the desired service UUID.
 *   -Checks for and makes a reference to one or more characteristic UUID in the current service.
 *   -The client can send data to the server by writing to this RX Characteristic.
 *   -If the client has enabled notifications for the TX characteristic, the server can send data to the client as
 *   notifications to that characteristic. This will trigger the notifyCallback function.
 *
 * Peripheral (server) - Accepts connections from a central mode device (client).
 *   -Advertises a service UUID.
 *   -Creates one or more characteristic for the advertised service UUID
 *   -Accepts connections from a client.
 *   -The server can send data to the client by writing to this TX Characteristic.
 *   -If the server has enabled notifications for the RX characteristic, the client can send data to the server as
 *   notifications to that characteristic. This the default function on most "Nordic UART Service" BLE uart sketches.
 *
 *
 * Copyright <2018> <Josh Campbell>
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright
 * notice and this permission notice shall be included in all copies or substantial portions of the Software. THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Based on the "BLE_Client" example by Neil Kolban:
 * https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLETests/Arduino/BLE_client/BLE_client.ino
 * With help from an example by Andreas Spiess:
 * https://github.com/SensorsIot/Bluetooth-BLE-on-Arduino-IDE/blob/master/Polar_Receiver/Polar_Receiver.ino
 * Nordic UART Service info:
 * https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v14.0.0%2Fble_sdk_app_nus_eval.html
 *
 */
