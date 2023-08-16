// Implementation of all the graphics that we'll draw on our screen

#include "display.h"
#include "bmp.h"
#include "radio.h"

constexpr uint16_t JOY_SIZE = 24;
constexpr uint16_t BLE_SIZE = 24;
constexpr uint16_t BATTERY_WIDTH = 16;
constexpr uint16_t BATTERY_HEIGHT = 8;

#ifdef USER_LCD_T_QT_PRO_S3
constexpr auto TFT_ROTATION = 2u;
#else
constexpr auto TFT_ROTATION = 0u;
#endif

static TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT); // Invoke custom library
static TFT_eSprite joy = TFT_eSprite(&tft);
static TFT_eSprite battery = TFT_eSprite(&tft);
static TFT_eSprite ble = TFT_eSprite(&tft);

void TFT_sleep() {
  Serial.println("Setting TFT (again) to deep-sleep ");
  // tft.writecommand(0x10); // Sleep (backlight still on ...)
  digitalWrite(TFT_BL, LOW);
  // tft_inited = false;
  delay(5); // needed!
}

void TFT_wake() {
  tft.writecommand(0x11); // WAKEUP
  delay(120);             // needed! PWR neeeds to stabilize!
  digitalWrite(TFT_BL, HIGH);
}

void draw_joystick(Joystick &j) {
  static int last_x = -1;
  static int last_y = -1;

  int x = JOY_SIZE / 2 * j.x();
  int y = JOY_SIZE / 2 * -j.y();

  x += JOY_SIZE / 2.0f;
  y += JOY_SIZE / 2.0f;

  if (last_x != x || last_y != y) {
    tft.setRotation(TFT_ROTATION);
    // Serial.print("posx: ");
    // Serial.print(x);
    // Serial.print("\tposy: ");
    // Serial.println(y);
    joy.fillSprite(TFT_BLACK);
    joy.drawCircle(JOY_SIZE / 2, JOY_SIZE / 2, JOY_SIZE / 2 - 1, TFT_GOLD);
    joy.fillCircle(x, y, 3, TFT_GOLD);
    joy.pushSprite(TFT_WIDTH - JOY_SIZE, 0);
    last_x = x;
    last_y = y;
  }
}

void draw_battery(float battery_voltage) {
  // static float last_battery_voltage = 0.0f;
  constexpr float MAX_VOLTAGE = 4.2f;
  constexpr float MIN_VOLTAGE = 3.1f;

  auto percent = (battery_voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE);

  percent = (percent < 0 ? 0 : (percent > 1 ? 1 : percent));

  // Serial.printf("Battery Percent: %f\n", percent);

  tft.setRotation(TFT_ROTATION);
  // battery.fillRect((BATTERY_WIDTH - 3) * (1-percent), 1,  1, BATTERY_HEIGHT - 1, TFT_BLACK);
  battery.fillSprite(TFT_BLACK);
  // Draw the outline
  battery.drawRect(0, 0, BATTERY_WIDTH - 1, BATTERY_HEIGHT - 1, TFT_GREEN);
  // Draw the percent full
  battery.fillRect(1, 1, (BATTERY_WIDTH - 3) * percent, BATTERY_HEIGHT - 1, TFT_GREEN);
  // Draw the tab
  battery.fillRect(BATTERY_WIDTH - 2, 2, 2, BATTERY_HEIGHT - 5, TFT_GREEN);

  battery.pushSprite(0, 0);
  // battery.pushSprite(TFT_HEIGHT - BATTERY_HEIGHT - 2 ,TFT_WIDTH - BATTERY_WIDTH - 2);
}

void draw_ble_state() {

  static int animator{};

  animator = animator >= BLE_SIZE ? 0 : animator + 1;

  ble.fillSprite(TFT_BLACK);

  switch (bleState) {
  case BLEState::INIT: ble.drawCircle(BLE_SIZE / 2, BLE_SIZE / 2, BLE_SIZE / 2 - 1, TFT_BLUE); break;
  case BLEState::SCANNING: ble.drawCircle(BLE_SIZE / 2, BLE_SIZE / 2, animator / 2 - 1, TFT_BLUE); break;
  case BLEState::FOUND_DEVICE:
    ble.drawCircle(BLE_SIZE / 2, BLE_SIZE / 2, BLE_SIZE - animator / 2 - 1, TFT_GREENYELLOW);
    break;
  case BLEState::CONNECTED: ble.drawCircle(BLE_SIZE / 2, BLE_SIZE / 2, BLE_SIZE - animator / 2 - 1, TFT_GREEN); break;
  case BLEState::READING_DEVICE_INFO: ble.drawCircle(BLE_SIZE / 2, BLE_SIZE / 2, animator / 2 - 1, TFT_CYAN); break;
  case BLEState::PAIRED: ble.drawCircle(BLE_SIZE / 2, BLE_SIZE / 2, animator / 2 + 1, TFT_GREEN); break;
  default: ble.fillCircle(BLE_SIZE / 2, BLE_SIZE / 2, BLE_SIZE / 2 - 1, TFT_RED); break;
  }

  ble.pushSprite((TFT_WIDTH - BLE_SIZE) / 2, 0);
}

void draw_controller_state(const vesc::controller &controller) {
  const auto hw = "Connected to: " + controller.getHW();

  auto line = 32ul;
  constexpr auto lh = 12u;
  if (hw.length()) { tft.drawString(hw.c_str(), 0, line); line += lh; }

  std::array<char, 100> s;
  auto values = controller.values();
  sprintf(s.data(), "vescid:\t%i", values.vesc_id);
  tft.drawString(s.data(), 0, line); line += lh;
  sprintf(s.data(), "Vin:\t%4.1f", values.v_in);
  tft.drawString(s.data(), 0, line); line += lh;
  sprintf(s.data(), "current:\t%4.1f", values.current_in);
  tft.drawString(s.data(), 0, line); line += lh;
  sprintf(s.data(), "rpm:\t%10.1f", values.rpm);
  tft.drawString(s.data(), 0, line); line += lh;
  sprintf(s.data(), "ah:\t%4.1f", values.amp_hours);
  tft.drawString(s.data(), 0, line); line += lh;
  sprintf(s.data(), "fault:\t%4i", values.fault_code);
  tft.drawString(s.data(), 0, line); line += lh;

}

void init_tft() {
  tft.init();
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);

  joy.createSprite(JOY_SIZE, JOY_SIZE);
  joy.fillSprite(TFT_BLACK);
  joy.fillCircle(JOY_SIZE / 2, JOY_SIZE / 2, 4, TFT_ORANGE);

  battery.createSprite(BATTERY_WIDTH, BATTERY_HEIGHT);
  battery.fillSprite(TFT_BLACK);

  ble.createSprite(BLE_SIZE, BLE_SIZE);
  ble.fillSprite(TFT_BLACK);

  if (TFT_BL > 0) {          // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library
                                            // in the User Setup file TTGO_T_Display.h
  }

  tft.setSwapBytes(true);
  tft.setRotation(TFT_ROTATION);
  tft.pushImage(0, 0, TFT_HEIGHT, TFT_WIDTH, steveplusplus);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  tft.fillScreen(TFT_BLACK);
}