#pragma once

class Joystick {
  public:
		Joystick() : _x{}, _y{}, _z{}, _zx{}, _zy{}, _zz{}, _expo(2.0) {}
		Joystick(float mexpo) : _x{}, _y{}, _z{}, _zx{}, _zy{}, _zz{}, _expo(mexpo) {}
		~Joystick() = default;

		void set_zeros(int x, int y, int z = 0) { _zx = x; _zy = y; _zz = 0; }
		int get_zero_x() { return _zx; }
		int get_zero_y() { return _zy; }
		int get_zero_z() { return _zz; }

		void set_pos(int x, int y) { _x = x; _y = y; }
		void set_pos(int x, int y, int z) { _x = x; _y = y; _z = z; }
		// Gets the corrected values from -1 to 1 for the joystick
		float x();
		float y();
		float z();

		float expo() { return _expo; }
		void set_expo(float mexpo) { _expo = mexpo; }

	private:
		int _x;
		int _y;
		int _z;

		int _zx;
		int _zy;
		int _zz;

		// TODO Probably need expo for all the values?
		float _expo;
};