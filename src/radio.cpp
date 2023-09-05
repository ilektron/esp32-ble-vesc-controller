#include "radio.h"
#include "vesc.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <array>
#include <cmath>
#include <memory>

// The remote Nordic UART service service we wish to connect to.
// This service exposes two characteristics: one for transmitting and one for receiving (as seen from the client).
static BLEUUID serviceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");

// The characteristics of the above service we are interested in.
// The client can send data to the server by writing to this characteristic.
static BLEUUID charUUID_RX("6e400002-b5a3-f393-e0a9-e50e24dcca9e"); // RX Characteristic

// If the client has enabled notifications for this characteristic,
// the server can send data to the client as notifications.
static BLEUUID charUUID_TX("6e400003-b5a3-f393-e0a9-e50e24dcca9e"); // TX Characteristic

static std::unique_ptr<BLEAddress> pServerAddress;

// This is a messaging tool to let the init function know that a proper BLE device is connected
// Should change to a task notification for better performance?
static SemaphoreHandle_t xDoConnect;

static BLERemoteCharacteristic *pTXCharacteristic;
static BLERemoteCharacteristic *pRXCharacteristic;

// Our buffer for sending and receiving packets
constexpr const auto PACKET_QUEUE_SIZE = 5u;

QueueHandle_t rxPackets;
QueueHandle_t txPackets;
BLEState bleState;

vesc::controller controller;

static vesc::buffer<vesc::PACKET_MAX_PL_LEN> rx_buffer;

// Function prototypes
void ble_init();
void ble_reset();
void ble_scanning();

// Scan for BLE servers and find the first one that advertises the Nordic UART service.
// Should probably make a list of all the Nordic UART services and figure out if we
// have a preferred client to connect to
class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
      Called for each advertising BLE server.
  */
  void onResult(BLEAdvertisedDevice advertisedDevice) {

    // We have found a device, check to see if it contains the Nordic UART service.
    if (advertisedDevice.haveServiceUUID()) {
      Serial.println("Checking serviceid: ");
      Serial.printf("ID: %s\n", advertisedDevice.toString().c_str());
      // Devices can have multiple service UUIDs
      for (auto i = 0ul; i < advertisedDevice.getServiceUUIDCount(); i++) {
        // Should check the name as well to make sure that this is an appropriate device
        if (advertisedDevice.getServiceUUID(i).equals(serviceUUID)) {
          Serial.print("BLE Advertised Device found - ");
          Serial.println(advertisedDevice.toString().c_str());

          Serial.println("Found a device with the desired ServiceUUID!");
          advertisedDevice.getScan()->stop();

          // Use reset in case we are calling this again
          Serial.println("Reseting the server address");
          pServerAddress.reset(new BLEAddress(advertisedDevice.getAddress()));

          // Unblock the task waiting for us to discover the proper device
          Serial.println("Unblocking our scanning task");
          xSemaphoreGive(xDoConnect);
        }
      }
    }
  }
} xAdvertiseCallbacks;

class ClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient *client) {}
  void onDisconnect(BLEClient *client) { bleState = BLEState::DISCONNECTED; }
} xClientCallbacks;
// This is the callback functions that gets data from the ble service, which in this case should be data that the vesc
// sends back to us. Is this called in an ISR?

// So, this should read in bytes from BLE and populate payloads to stick into the rxPackets queue
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length,
                           bool isNotify) {

  // Warning: This is a large object and we need to
  Serial.printf("Notify callback for TX characteristic received. isNotify: %i Data:\n", isNotify);
  // rx_buffer.append(pData, length);
  // For debug purposes for now, print out all the data
  for (int i = 0; i < length; i++) {
    // Serial.print((char)pData[i]);     // Print character to uart
    Serial.printf("0x%x", pData[i]); // print raw data to uart
    Serial.print(" ");
  }
  Serial.println(" <<");

  // TODO this might need some additional error checking to clean up bad data
  static vesc::packet p;
  p.data().append(pData, length);

  Serial.printf("Current buffer len: %i\n", p.len());

  auto res = controller.parse_command(p);
  if (res == vesc::packet::VALIDATE_RESULT::VALID) {
    Serial.println(" We have a valid packet! ");
  } else if (res == vesc::packet::VALIDATE_RESULT::INCOMPLETE) {
    Serial.println("Incomplete packet");
  } else {
    Serial.printf("Bad packet: %i\n", static_cast<unsigned int>(res));
    p.data().reset();
  }
}

// Returns a reference to the newly created client that is connected to the server
// If the connection fails, returns a nullptr and an invalid client object
std::unique_ptr<BLEClient> connectToServer(BLEAddress pAddress) {
  Serial.print("Establishing a connection to device address: ");
  Serial.println(pAddress.toString().c_str());

  // TODO: Figure out where this client pointer should live
  // Can we delete this client object or do we need to hold it for a period of time?
  // We might need to return this object because the client is what we need to hold onto
  std::unique_ptr<BLEClient> client(BLEDevice::createClient());
  Serial.println(" - Created client");

  // Connect to the remove BLE Server.
  if (!client->connect(pAddress, BLE_ADDR_TYPE_RANDOM)) {
    Serial.println("Failed to connect to server");
    return nullptr;
  }
  Serial.println(" - Connected to server");

  // Obtain a reference to the Nordic UART service on the remote BLE server.
  BLERemoteService *pRemoteService = client->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find Nordic UART service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return nullptr;
  }
  Serial.println(" - Remote BLE service reference established");

  // Obtain a reference to the TX characteristic of the Nordic UART service on the remote BLE server.
  pTXCharacteristic = pRemoteService->getCharacteristic(charUUID_TX);
  if (pTXCharacteristic == nullptr) {
    Serial.print("Failed to find TX characteristic UUID: ");
    Serial.println(charUUID_TX.toString().c_str());
    return nullptr;
  }
  Serial.println(" - Remote BLE TX characteristic reference established");

  pTXCharacteristic->registerForNotify(notifyCallback);
  // const uint8_t v[] = {0x1, 0x0};
  // pTXCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t *)v, 2, true);
  Serial.println(" - Registered for callback");

  // Obtain a reference to the RX characteristic of the Nordic UART service on the remote BLE server.
  pRXCharacteristic = pRemoteService->getCharacteristic(charUUID_RX);
  if (pRXCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID_RX.toString().c_str());
    return nullptr;
  }

  controller.setTX(
      [&](uint8_t *data, std::size_t len, bool response) { pRXCharacteristic->writeValue(data, len, response); });
  Serial.println(" - Remote BLE RX characteristic reference established");

  return client;
}

void ble_init() {
  // Initialize our state machine
  bleState = BLEState::INIT;
  Serial.println("\nStarting Arduino BLE Central Mode (Client) Nordic UART Service");

  // Create the semaphore for allowing us to move forward once we're connected to a BLE device
  xDoConnect = xSemaphoreCreateBinary();

  // Init takes a name of the BLE device
  // TODO: Figure out if calling this multiple times is bad
  BLEDevice::init("MagicControl");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device. Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  // Signleton object, don't delete
  BLEScan *pBLEScan = BLEDevice::getScan();
  // Set our callbacks for the scan
  pBLEScan->setAdvertisedDeviceCallbacks(&xAdvertiseCallbacks);
  // Set state right before starting scan to avoid race condition
  bleState = BLEState::SCANNING;
  pBLEScan->setActiveScan(true);
  // Start the scan. If we get a matching device, the scan stops
  // TODO might need a pairing mode that listens for all devices and then allows a user
  // to select the desired device
  pBLEScan->start(30);
}

void ble_scanning() {
  // TODO: Create a state that assembles a list of viable devices and lets the user select the desired device to connect
  // to.
  // Here we should block until we find a device and then unblock once we can move forward
  if (xSemaphoreTake(xDoConnect, 30000u / portTICK_PERIOD_MS) == pdTRUE) {
    Serial.println("Found a device, let's connect to it");
    bleState = BLEState::FOUND_DEVICE;
  } else {
    Serial.println("Failed to discover appropriate BLE device, let's scan again");
    ble_reset();
  }

  Serial.println("Deleting semaphore");
  vSemaphoreDelete(xDoConnect);
}

void ble_found_device() {
  static std::unique_ptr<BLEClient> pClient{};

  Serial.printf("Connecting to server: %s\n", pServerAddress->toString().c_str());
  // Reset our client to the new one
  pClient = connectToServer(*pServerAddress);

  Serial.println("Setting client callbacks");
  pClient->setClientCallbacks(&xClientCallbacks);
  if (pClient) {
    bleState = BLEState::CONNECTED;
    Serial.println("We are now connected to the BLE Server");
  } else {
    Serial.println(
        "We have failed to connect to the server; there is nothin more we will do. Reseting and trying again");
    // Should re-init?
    ble_reset();
  }
}

void ble_connected() {
  // Yay! Let's get the device info
  bleState = BLEState::READING_DEVICE_INFO;
}

void ble_get_device_info() {
  static vesc::buffer<1u> fwp = {COMM_FW_VERSION};
  static vesc::packet fwpacket(fwp);

  controller.setCallback(COMM_FW_VERSION, [&](vesc::packet &p) {
    bleState = BLEState::PAIRED;
    Serial.println("Successfully read device info");
  });
  // TODO: Block for reply using semaphore. Release semaphore if not the reply that we're waiting for
  Serial.printf("Constructed packet, sending fw version request len:%i\n", fwpacket.len());
  pRXCharacteristic->writeValue(fwpacket, fwpacket.len(), true);

  // TODO: Should block and verify device information before continuing

  vTaskDelay(1000u / portTICK_PERIOD_MS);
}

void ble_paired(Joystick &j) {

  static auto cb = controller.setCallback(COMM_GET_VALUES, [&](vesc::packet &p) { Serial.println("+"); });

  if (cb) {
    // Read data from the controller for voltage and current

    // Read all the values.
    // TODO: Change this to only retrieve what we need
    controller.getValues();

    vTaskDelay(20u / portTICK_PERIOD_MS);

    // Control the motors
    auto x = j.x();
    auto y = j.y();
    // Clip any bad controls
    // TODO: Need to subtract the deadzone out of the control to prevent a jump
    constexpr auto low_cutoff = 0.02f;
    constexpr auto high_cutoff = 1.01f;
    if (abs(x) < low_cutoff || abs(x) > high_cutoff) { x = 0.0f; }
    if (abs(y) < low_cutoff || abs(y) > high_cutoff) { y = 0.0f; }

    // Scale the x factor when turning to make turning smoother and make more sense
    // Expo is in joystick.cpp
    auto TURN_SCALE = 0.5f;
    auto scaled_x = x * TURN_SCALE;
    
    float m1 = y - scaled_x;
    float m2 = y + scaled_x;

    Serial.printf("Setting motors to: %10.2f,\t%10.2f\n", m1, m2);

    // constexpr auto current_scale = 1000.0f * 10.0f;
    // controller.setCurrents(current_scale * m1, current_scale * m2);
    constexpr auto duty_scale = 100000.0f;
    constexpr auto control_scale = 1.0f;
    controller.setDuties(duty_scale * m1 * control_scale, duty_scale * m2 * control_scale);
    vTaskDelay(20u / portTICK_PERIOD_MS);
  }
}

void ble_reset() {
  // Give things a second
  Serial.println("About to reset BT");
  vTaskDelay(1000u / portTICK_PERIOD_MS);
  Serial.println("BLEDevice::deinit >>");
  BLEDevice::deinit();
  Serial.println("BLEDevice::deinit <<");
  controller.setTX(nullptr);
  ble_init();
}

// Set up our bluetooth connection
void radio_init() {
  ble_init();
  vTaskDelay(300 / portTICK_PERIOD_MS);
}

void radio_run(Joystick &j) {

  switch (bleState) {
  case BLEState::INIT: ble_init(); break;
  case BLEState::SCANNING: ble_scanning(); break;
  case BLEState::FOUND_DEVICE: ble_found_device(); break;
  case BLEState::CONNECTED: ble_connected(); break;
  case BLEState::READING_DEVICE_INFO: ble_get_device_info(); break;
  // Not sure if we need this state
  case BLEState::PAIRED: ble_paired(j); break;
  case BLEState::DISCONNECTED:
    Serial.println("BLE Client Disconnected");
    vTaskDelay(1000u / portTICK_RATE_MS);
    ble_reset();
    break;
  default:
    // Should reset radio and reconnect?
    // Check if connection is still valid. If we disconnected, reset!
    Serial.println("In a weird state");
    break;
  }
  // Serial.print("Running radio"); probably don't want to do this if we hope for low latency and are running often to
  // check if new data has arrived

  // This is the main body of our loop to make sure that things are getting processed
  // This should only run if we connected to a device properly

  // TODO have separate RX and TX tasks that only unblock once we get data to send or recieve data to process
  // Or could have a semaphore that unblocks and checks both the TX and RX queue
  // Check if we have data in our TX queue and if so send it

  // Check to see if we've received data in our RX queue, if so read and process it, checking if we have a full packet,
  // then notify anyone waiting that we have new data to check out

  // pRXCharacteristic->writeValue(timeSinceBoot.c_str(), timeSinceBoot.length());
  // pTXCharacteristic->readRawData();
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
