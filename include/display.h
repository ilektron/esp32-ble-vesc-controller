// All functions for displaying graphics on the screen
#pragma once

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "joystick.h"
#include "vesc.h"


void init_tft();

// void TFT_sleep();
// void TFT_wake();

void draw_joystick(Joystick& j);
void draw_battery(float battery_voltage);
void draw_ble_state();
void draw_controller_state(const vesc::controller& controller);