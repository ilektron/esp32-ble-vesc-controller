# Communication Protocol with VESC

In this document, we'll attempt to describe the communication protocol with a VESC. As far as I can tell, there exists no official documentation other than the code itself that outlines how to communicate with a VESC. Without an official standard, that means it is likely that there will be breaking changes without notice in difrerent versions of the VESC firmware, however, it does seem that the maintainers of VESC firmware try to maintain backwards compatibility for some simple commands to obtain firmware version and basic configuration and control.

This protocol is used on many of the serial communication ports. In this repository, we'll use it specifically for communicating with a VESC over BLE.

## Bluetooth Low Energy 5.0 (BLE)

Many newer commercial versions of VESC include an NRF chip for communication over Bluetooth. Some older versions might use the Bluetooth classic Serial Port Profile (SPP). Newer versions use the BLE service structure that allows a device to advertise it's services to potential clients. The SPP and BLE versions are incompatible. In this repository we'll connect using BLE, which the VESC-Tool also uses at this time.

### Discovery

A VESC with an NRF chip utilizing BLE will advertise it's serial communication according to the NRF specific standard for a BLE UART. 

#### Service UUIDs

The NRF chip advertises a UART service with UUID common to all NRF UART services. It is not unique to VESCs. The following service IDs are used to identify the NRF UART

When discovering BLE services, the service UUID for the UART might not be the top level UUID. It is important to look at all the UUIDs in a discovered device.

##### UART Service UUID - 6e400001-b5a3-f393-e0a9-e50e24dcca9e

##### RX Characteristic - 6e400002-b5a3-f393-e0a9-e50e24dcca9e

The client can send data to the server by writing to this characteristic.

##### TX Characteristic - 6e400003-b5a3-f393-e0a9-e50e24dcca9e

The server can send data to the client as notifications.
If the client has enabled notifications for this characteristic, we can subscribe to notifications and know immediately when data is available to be read.


## VESC Serial Communication

The VESC uses a fairly simple packet structure for use in serial communication and uses a CRC to validate the contents of the packet

### Packet Structure

| Start Byte | Payload Length | Payload | `3` | CRC | `\0` |
| --- | --- | --- | --- | --- | --- |

#### Start Byte

The first byte of a packet is either the integer value 2 or 3. The start byte signifies whether or not the payload is longer than 255 bytes. The value represents the combined length of the Start Byte and the Payload Length

##### Start Byte: `2`

For a packet with length `n`

| bit   | 0   | 1   | 2, ..., len + 2 | n - 4 | n - 3   | n - 2   | n - 1 |
| ---   | --- | --- | ---             | ---   | ---     | ---     | ---   |
| value | `2` | len | payload         | `3`   | CRC MSB | CRC LSB | `\0`  |

##### Start Byte: `3`

For a packet with length `n`

| bit   | 0   | 1       | 2       | 3, ..., len + 2 | n - 4 | n - 3   | n - 2   | n - 1  |
| ---   | --- | ---     | ---     | ---             | ---   | ---     | ---     | ---    |
| value | `3` | len MSB | len LSB | payload         | `3`   | CRC MSB | CRC LSB | `\0` |

### Payloads

The different payloads can be found in the [bldc](https://github.com/vedderb/bldc) repository in `datatypes.h`

#### 