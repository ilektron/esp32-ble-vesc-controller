#include "joystick.h"
#include <cmath>

constexpr auto ADC_RESOLUTION = 65536.0f/2;
constexpr auto EXPO = 2.0f;

const float calc_pos(int raw, int zero) {
  float scale = zero/(ADC_RESOLUTION/2.0f);
  float diff = (raw - zero)/(ADC_RESOLUTION/2.0f);
  diff = diff < 0 ? -pow(-diff, EXPO) : pow(diff, EXPO);
  // scale from -1 to 1?
  float val = (diff < 0) ? (diff / scale) : (diff * scale);

  return val;
}

float Joystick::x() {
  return calc_pos(_x, _zx);
}

float Joystick::y() {
  return calc_pos(_y, _zy);
}

float Joystick::z() {
  return calc_pos(_z, _zz);
}