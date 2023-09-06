// All functions for displaying graphics on the screen
#pragma once

#include <TFT_eSPI.h>
#include <SPI.h>
#include "joystick.h"
#include "vesc.h"


void init_tft();

void TFT_sleep();
void TFT_wake();

void draw_joystick(Joystick& j);
void draw_raw_joystick_values(Joystick &j);
void draw_battery(float battery_voltage);
void draw_ble_state();
void draw_controller_state(const vesc::controller& controller);
void draw_controller2_state(const vesc::controller& controller);