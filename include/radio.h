#pragma once

#include "vesc.h"
#include "joystick.h"

enum class BLEState {
  INIT,
  SCANNING,
  FOUND_DEVICE,
  CONNECTED,
  READING_DEVICE_INFO,
  PAIRED,
  DISCONNECTED
};

extern BLEState bleState;
extern vesc::controller controller;

void radio_init();
void radio_run(Joystick& j);
