#pragma once

#include "buffer.h"
#include "datatypes.h"
#include "packet.h"

#include <Arduino.h>
#include <bitset>
#include <stdint.h>

namespace vesc {

namespace mc_value {};

struct fw_params {
  uint8_t major;
  uint8_t minor;
  // TODO can we just limit hw to a certain length?
  std::string hw;
  // TODO Staticly allocate 12 bytes for the UUID?
  std::string uuid;
  uint8_t isPaired;
  uint8_t isTestFW;
  uint8_t hwType;
  uint8_t customConfigNum;

  // TODO How do we know if this firmware is loaded
};

// Could use with bitset and have just the bit number instead?
// Typed enumes make for ugly conversions
// These are used when requesting data from the vesc and are combined into a bitmask
namespace values {
constexpr const uint32_t TEMP_MOS = 1u << 0;
constexpr const uint32_t TEMP_MOTOR = 1u << 1;
constexpr const uint32_t CURRENT_MOTOR = 1u << 2;
constexpr const uint32_t CURRENT_IN = 1u << 3;
constexpr const uint32_t ID = 1u << 4;
constexpr const uint32_t IQ = 1u << 5;
constexpr const uint32_t DUTY_NOW = 1u << 6;
constexpr const uint32_t RPM = 1u << 7;
constexpr const uint32_t V_IN = 1u << 8;
constexpr const uint32_t AMP_HOURS = 1u << 9;
constexpr const uint32_t AMP_HOURS_CHARGED = 1u << 10;
constexpr const uint32_t WATT_HOURS = 1u << 11;
constexpr const uint32_t WATT_HOURS_CHARGED = 1u << 12;
constexpr const uint32_t TACHOMETER = 1u << 13;
constexpr const uint32_t TACHOMETER_ABS = 1u << 14;
constexpr const uint32_t FAULT_CODE = 1u << 15;
constexpr const uint32_t POSITION = 1u << 16;
constexpr const uint32_t VESC_ID = 1u << 17;
constexpr const uint32_t TEMP_MOSX = 1u << 18;
constexpr const uint32_t VD = 1u << 19;
constexpr const uint32_t VQ = 1u << 20;
}; // namespace values

struct mc_values {
  float v_in;
  float temp_mos;
  float temp_mos1;
  float temp_mos2;
  float temp_mos3;
  float temp_mos4;
  float temp_mos5;
  float temp_mos6;
  float temp_pcb;
  float temp_motor;
  float current_motor;
  float current_in;
  float id;
  float iq;
  float rpm;
  float duty_now;
  float amp_hours;
  float amp_hours_charged;
  float watt_hours;
  float watt_hours_charged;
  int tachometer;
  int tachometer_abs;
  float vd;
  float vq;
  mc_fault_code fault_code;
  float position;
  uint8_t vesc_id;
};

class controller {

public:
  // TODO change the second vesc id in order to read it
  controller() : _fw{}, _mc_values{}, _secondVescId{95} {}
  ~controller() = default;
  // TODO return comm result
  packet::VALIDATE_RESULT parse_command(vesc::packet &p) {
    auto res = p.validate();
    if (res == packet::VALIDATE_RESULT::VALID) {
      // Get the type of packet that is in this data
      auto type = p.data().get<uint8_t>();

      switch (type) {
      case COMM_FW_VERSION: handleFw(p, _fw); break;
      case COMM_GET_VALUES: handleGetValues(p); break;
      case COMM_GET_VALUES_SELECTIVE: handleGetValuesSelective(p, p.data().get<uint8_t>()); break;
      default: Serial.printf("Unhandled packet type: %i\n", type);
      }

      // Call any custom callbacks for this packet type
      auto callback = _callbacks.find(type);
      if (callback != _callbacks.end()) {
        // Call the callback that was assigned to this packet type
        // Maybe shouldn't be a map, could want multiple callbacks?
        // Could possibly just make an array of 256 values since we know that will be the max.
        callback->second(p);
      }
    }
    return res;
  }

  std::string getHW() const { return _fw.hw; }
  std::string getUUID() const { return _fw.uuid; }
  bool isPaired() const { return _fw.isPaired; }
  bool isTestFW() const { return _fw.isTestFW; }
  int type() const { return _fw.hwType; }

  float getVoltage() const { return _mc_values.v_in; }

  const mc_values &values() const { return _mc_values; }

  bool setCallback(uint8_t packet_type, std::function<void(packet &p)> f) {
    _callbacks[packet_type] = f;
    return true;
  }

  void clearCallback(uint8_t packet_type) { _callbacks.erase(packet_type); }

  void setTX(std::function<void(uint8_t *data, std::size_t len, bool response)> tx) { _tx = tx; }

  // Get information from the controller
  bool getValues(uint32_t mask = 0xFFFFFFFF) {
    if (_tx) {
      static vesc::buffer<1u> vp = {COMM_GET_VALUES};
      static vesc::packet vpacket(vp);
      _tx(vpacket, vpacket.len(), true);
    } else {
      return false;
    }
    return true;
  }

  // Set the current for a motor
  bool setCurrent(float current, int id = -1) {
    if (_tx) {
      constexpr const auto scale = 1000.0f;
      vesc::buffer<7u> payload;
      if (id > 0) {
        payload.append<uint8_t>(COMM_FORWARD_CAN);
        payload.append<uint8_t>(id);
      }

      payload.append<uint8_t>(COMM_SET_CURRENT);
      payload.append<int32_t>(current * scale);
      vesc::packet p(payload);

      _tx(p, p.len(), false);
    } else {
      return false;
    }
    return true;
  }
  bool setCurrents(float c1, float c2) { return setCurrent(c1) && setCurrent(c2, _secondVescId); }

  bool setRPM(float rpm, int id = -1) {
    if (_tx) {
      constexpr const auto scale = 1.0f;
      vesc::buffer<7u> payload;
      if (id > 0) {
        payload.append<uint8_t>(COMM_FORWARD_CAN);
        payload.append<uint8_t>(id);
      }

      payload.append<uint8_t>(COMM_SET_RPM);
      payload.append<int32_t>(rpm * scale);
      vesc::packet p(payload);

      _tx(p, p.len(), false);
    } else {
      return false;
    }
    return true;
  }

  bool setRPMs(float rpm1, float rpm2) { return setRPM(rpm1) && setRPM(rpm2, _secondVescId); }

  bool setDuty(float duty, int id = -1) {
    if (_tx) {
      constexpr const auto scale = 1.0f;
      vesc::buffer<7u> payload;
      if (id > 0) {
        payload.append<uint8_t>(COMM_FORWARD_CAN);
        payload.append<uint8_t>(id);
      }

      payload.append<uint8_t>(COMM_SET_DUTY);
      payload.append<int32_t>(duty * scale);
      vesc::packet p(payload);

      _tx(p, p.len(), false);
    } else {
      return false;
    }
    return true;
  }

  bool setDuties(float duty1, float duty2) { return setDuty(duty1) && setDuty(duty2, _secondVescId); }

  // Ping all address to see which devices are connected
  void scanCAN() {}

private:
  fw_params _fw;
  mc_values _mc_values;
  uint8_t _secondVescId;
  std::map<uint8_t, std::function<void(packet &p)>> _callbacks;
  std::function<void(uint8_t *data, std::size_t len, bool response)> _tx;
  std::function<void(const uint8_t *data, std::size_t len)> _rx;

  void handleFw(packet &p, fw_params &out) {
    auto &buf = p.data();
    if (buf.len() > 2u) {
      out.major = buf.get<uint8_t>();
      out.minor = buf.get<uint8_t>();
      out.hw = buf.get_string();
    }

    if (buf.len() > 12u) {
      out.uuid.clear();
      for (auto i = 0u; i < 12; i++) {
        out.uuid.push_back(buf.get<int8_t>());
      }
    }

    if (buf.len() > 1u) { out.isPaired = buf.get<uint8_t>(); }
    if (buf.len() > 1u) { out.isTestFW = buf.get<uint8_t>(); }
    if (buf.len() > 1u) { out.hwType = buf.get<uint8_t>(); }
    if (buf.len() > 1u) { out.customConfigNum = buf.get<uint8_t>(); }
  }

  void handleGetValues(packet &p) { handleGetValuesSelective(p, 0xFFFFFFFF); }

  // TODO: Change to use std::bitset and define these shifted values
  void handleGetValuesSelective(packet &p, uint32_t mask) {

    std::bitset<32> m(mask);
    auto &buf = p.data();
    // mask locations
    if (mask & values::TEMP_MOS) { _mc_values.temp_mos = buf.getfp(2, 10.0f); }
    if (mask & values::TEMP_MOTOR) { _mc_values.temp_motor = buf.getfp(2, 10.0f); }
    if (mask & values::CURRENT_MOTOR) { _mc_values.current_motor = buf.getfp(4, 100.0f); }
    if (mask & values::CURRENT_IN) { _mc_values.current_in = buf.getfp(4, 100.0f); }
    if (mask & values::ID) { _mc_values.id = buf.getfp(4, 100.0f); }
    if (mask & values::IQ) { _mc_values.iq = buf.getfp(4, 100.0f); }
    if (mask & values::DUTY_NOW) { _mc_values.duty_now = buf.getfp(2, 1000.0f); }
    if (mask & values::RPM) { _mc_values.rpm = buf.getfp(4, 1.0f); }
    if (mask & values::V_IN) { _mc_values.v_in = buf.getfp(2, 10.0f); }
    if (mask & values::AMP_HOURS) { _mc_values.amp_hours = buf.getfp(4, 10000.0f); }
    if (mask & values::AMP_HOURS_CHARGED) { _mc_values.amp_hours_charged = buf.getfp(4, 10000.0f); }
    if (mask & values::WATT_HOURS) { _mc_values.watt_hours = buf.getfp(4, 10000.0f); }
    if (mask & values::WATT_HOURS_CHARGED) { _mc_values.watt_hours_charged = buf.getfp(4, 10000.0f); }
    if (mask & values::TACHOMETER) { _mc_values.tachometer = buf.get<int32_t>(); }
    if (mask & values::TACHOMETER_ABS) { _mc_values.tachometer_abs = buf.get<int32_t>(); }
    if (mask & values::FAULT_CODE) { _mc_values.fault_code = mc_fault_code(buf.get<uint8_t>()); }

    if (buf.len() >= 4) {
      if (mask & values::POSITION) { _mc_values.position = buf.getfp(4, 1000000.0f); }
    } else {
      _mc_values.position = -1.0;
    }

    if (buf.len() >= 1) {
      if (mask & values::VESC_ID) { _mc_values.vesc_id = buf.get<uint8_t>(); }
    } else {
      _mc_values.vesc_id = 255u;
    }

    if (buf.len() >= 6) {
      if (mask & values::TEMP_MOSX) {
        _mc_values.temp_mos1 = buf.getfp(2, 10.0f);
        _mc_values.temp_mos2 = buf.getfp(2, 10.0f);
        _mc_values.temp_mos3 = buf.getfp(2, 10.0f);
      }
    }

    if (buf.len() >= 8) {
      if (mask & values::VD) { _mc_values.vd = buf.getfp(4, 1000.0f); }
      if (mask & values::VQ) { _mc_values.vq = buf.getfp(4, 1000.0f); }
    }
  }
};
}; // namespace vesc