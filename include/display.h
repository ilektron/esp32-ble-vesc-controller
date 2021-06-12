// All functions for displaying graphics on the screen
#pragma once

#include <TFT_eSPI.h>
#include <SPI.h>
#include "joystick.h"


void init_tft();

void TFT_sleep();
void TFT_wake();

void draw_joystick(Joystick& j);
void draw_battery(float battery_voltage);
