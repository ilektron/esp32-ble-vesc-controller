#pragma once

enum class BLEState {
  INIT,
  SCANNING,
  FOUND_DEVICE,
  CONNECTED,
  READING_DEVICE_INFO,
  PAIRED
};

extern BLEState ble_state;

void radio_init();
void radio_run();
