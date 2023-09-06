#include "joystick.h"
#include <cmath>

// ADS1115 has max 16 bit resolution, which means 
// The joystick should center the output voltage fairly well
// The FJ6 joystick with the ads1115 should have a center value around 13500
constexpr auto ADC_RESOLUTION = 32767U;

// Assume that the joystick is centered when it is turned on. 
const float calc_pos(int raw, int zero, float expo) {
  float scale = zero; // /(ADC_RESOLUTION/2.0f);
  float diff = (raw - zero)/(scale);
  auto val = diff < 0 ? -pow(-diff, expo) : pow(diff, expo);

  return val;
}

float Joystick::x() {
  return calc_pos(_x, _zx, _expo);
}

float Joystick::y() {
  return calc_pos(_y, _zy, _expo);
}

float Joystick::z() {
  return calc_pos(_z, _zz, _expo);
}