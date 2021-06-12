#include "joystick.h"

constexpr auto ADC_RESOLUTION = 4095.0f;

float Joystick::x() {
  float scale_x = _zx/(ADC_RESOLUTION/2.0f);
  float fx = (_x - _zx)/(ADC_RESOLUTION/2.0f);
  float x = (fx < 0) ? (fx / scale_x) : (fx * scale_x);

	return x;
}

float Joystick::y() {
	// Figure out the proportion from zero to the value
  float scale_y = _zy/(ADC_RESOLUTION/2.0f);
  float fy = (_y - _zy)/(ADC_RESOLUTION/2.0f);
  float y = (fy < 0) ? fy / scale_y : fy * scale_y;

	return y;
}