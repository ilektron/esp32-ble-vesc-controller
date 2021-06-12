#pragma once

class Joystick {
  public:
		Joystick() = default;
		~Joystick() = default;

    void set_zeros(int x, int y) { _zx = x; _zy = y; }
		int get_zero_x() { return _zx; }
		int get_zero_y() { return _zy; }

		void set_pos(int x, int y) { _x = x; _y = y; }
		// Gets the corrected values from -1 to 1 for the joystick
		float x();
		float y();

	private:
		int _x;
		int _y;

		int _zx;
		int _zy;
};