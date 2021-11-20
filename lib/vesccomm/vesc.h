#pragma once

#include "buffer.h"
#include "datatypes.h"
#include "packet.h"

#include <Arduino.h>
#include <stdint.h>

namespace vesc {
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
  controller() = default;
  ~controller() = default;
  // TODO return comm result
  packet::VALIDATE_RESULT parse_command(vesc::packet &p) {
    auto res = p.validate();
    if (res == packet::VALIDATE_RESULT::VALID) {
      auto type = p.data().get<uint8_t>();
      switch (type) {
      case COMM_FW_VERSION: handleFw(p); break;
      case COMM_GET_VALUES: handleGetValues(p); break;
      case COMM_GET_VALUES_SELECTIVE: handleGetValuesSelective(p, p.data().get<uint8_t>()); break;
      default: Serial.printf("Unhandled packet type: %i\n", type);
      }
    }
    return res;
  }

  std::string getHW() const { return _fw.hw; }
  std::string getUUID() const { return _fw.uuid; }
  bool isPaired() const { return _fw.isPaired; }
  bool isTestFW() const { return _fw.isTestFW; }
  int type() const { return _fw.hwType; }

private:
  fw_params _fw;
  mc_values _mc_values;
  std::map<uint8_t, std::function<void(packet &p)>> _callbacks;

  void handleFw(packet &p) {
    auto &buf = p.data();
    if (buf.len() > 2u) {
      _fw.major = buf.get<uint8_t>();
      _fw.minor = buf.get<uint8_t>();
      _fw.hw = buf.get_string();
    }

    if (buf.len() > 12u) {
      for (auto i = 0u; i < 12; i++) {
        _fw.uuid.push_back(buf.get<int8_t>());
      }
    }

    if (buf.len() > 1u) { _fw.isPaired = buf.get<uint8_t>(); }
    if (buf.len() > 1u) { _fw.isTestFW = buf.get<uint8_t>(); }
    if (buf.len() > 1u) { _fw.hwType = buf.get<uint8_t>(); }
    if (buf.len() > 1u) { _fw.customConfigNum = buf.get<uint8_t>(); }
  }

  void handleGetValues(packet &p) { handleGetValuesSelective(p, 0xFFFFFFFF); }

  void handleGetValuesSelective(packet &p, uint32_t mask) {

    auto &buf = p.data();
    // mask locations
    if (mask & (1ul << 0)) { _mc_values.temp_mos = buf.getfp(2, 10.0f); }
    if (mask & (1ul << 1)) { _mc_values.temp_motor = buf.getfp(2, 10.0f); }
    if (mask & (1ul << 2)) { _mc_values.current_motor = buf.getfp(4, 100.0f); }
    if (mask & (1ul << 3)) { _mc_values.current_in = buf.getfp(4, 100.0f); }
    if (mask & (1ul << 4)) { _mc_values.id = buf.getfp(4, 100.0f); }
    if (mask & (1ul << 5)) { _mc_values.iq = buf.getfp(4, 100.0f); }
    if (mask & (1ul << 6)) { _mc_values.duty_now = buf.getfp(2, 1000.0f); }
    if (mask & (1ul << 7)) { _mc_values.rpm = buf.getfp(4, 1.0f); }
    if (mask & (1ul << 8)) { _mc_values.v_in = buf.getfp(2, 10.0f); }
    if (mask & (1ul << 9)) { _mc_values.amp_hours = buf.getfp(4, 10000.0f); }
    if (mask & (1ul << 10)) { _mc_values.amp_hours_charged = buf.getfp(4, 10000.0f); }
    if (mask & (1ul << 11)) { _mc_values.watt_hours = buf.getfp(4, 10000.0f); }
    if (mask & (1ul << 12)) { _mc_values.watt_hours_charged = buf.getfp(4, 10000.0f); }
    if (mask & (1ul << 13)) { _mc_values.tachometer = buf.get<int32_t>(); }
    if (mask & (1ul << 14)) { _mc_values.tachometer_abs = buf.get<int32_t>(); }
    if (mask & (1ul << 15)) { _mc_values.fault_code = mc_fault_code(buf.get<uint8_t>()); }

    if (buf.len() >= 4) {
      if (mask & (1ul << 16)) { _mc_values.position = buf.getfp(4, 1000000.0f); }
    } else {
      _mc_values.position = -1.0;
    }

    if (buf.len() >= 1) {
      if (mask & (1ul << 17)) { _mc_values.vesc_id = buf.get<uint8_t>(); }
    } else {
      _mc_values.vesc_id = 255u;
    }

    if (buf.len() >= 6) {
      if (mask & (1ul << 18)) {
        _mc_values.temp_mos1 = buf.getfp(2, 10.0f);
        _mc_values.temp_mos2 = buf.getfp(2, 10.0f);
        _mc_values.temp_mos3 = buf.getfp(2, 10.0f);
      }
    }

    if (buf.len() >= 8) {
      if (mask & (1ul << 19)) { _mc_values.vd = buf.getfp(4, 1000.0f); }
      if (mask & (1ul << 20)) { _mc_values.vq = buf.getfp(4, 1000.0f); }
    }
  }
};
}; // namespace vesc