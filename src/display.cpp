// Implementation of all the graphics that we'll draw on our screen

#include "display.h"
#include "bmp.h"

constexpr uint16_t JOY_SIZE = 24;
constexpr uint16_t BATTERY_WIDTH = 16;
constexpr uint16_t BATTERY_HEIGHT = 8;

TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT); // Invoke custom library
TFT_eSprite joy = TFT_eSprite(&tft);
TFT_eSprite battery = TFT_eSprite(&tft);

void TFT_sleep() {
  Serial.println("Setting TFT (again) to deep-sleep ");
  //tft.writecommand(0x10); // Sleep (backlight still on ...)
  digitalWrite(TFT_BL, LOW);
  //tft_inited = false;
  delay(5); // needed!
}

void TFT_wake() {
  tft.writecommand(0x11); // WAKEUP
  delay(120); // needed! PWR neeeds to stabilize!
  digitalWrite(TFT_BL, HIGH);
}

void draw_joystick(Joystick& j) {
  static int last_x = -1;
  static int last_y = -1;
  
  int x = JOY_SIZE/2 * -j.x();
  int y = JOY_SIZE/2 * j.y();

  x += JOY_SIZE/2.0f;
  y += JOY_SIZE/2.0f;

  if (last_x != x || last_y != y) {
    tft.setRotation(0);
    //Serial.print("posx: ");
    //Serial.print(x);
    //Serial.print("\tposy: ");
    //Serial.println(y);
    joy.fillSprite(TFT_BLACK);
    joy.drawCircle(JOY_SIZE/2, JOY_SIZE/2, JOY_SIZE/2 - 1, TFT_GOLD);
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

  auto percent = (battery_voltage - MIN_VOLTAGE)/(MAX_VOLTAGE - MIN_VOLTAGE);

  percent = (percent < 0 ? 0 : (percent > 1 ? 1 : percent));

  // Serial.printf("Battery Percent: %f\n", percent);

  tft.setRotation(0);
  //battery.fillRect((BATTERY_WIDTH - 3) * (1-percent), 1,  1, BATTERY_HEIGHT - 1, TFT_BLACK);
  battery.fillSprite(TFT_BLACK);
  // Draw the outline
  battery.drawRect(0, 0, BATTERY_WIDTH - 1, BATTERY_HEIGHT - 1, TFT_GREEN);
  // Draw the percent full
  battery.fillRect(1, 1, (BATTERY_WIDTH - 3) * percent, BATTERY_HEIGHT - 1, TFT_GREEN);
  // Draw the tab
  battery.fillRect(BATTERY_WIDTH-2, 2, 2, BATTERY_HEIGHT - 5, TFT_GREEN);

  battery.pushSprite(0, 0);
  // battery.pushSprite(TFT_HEIGHT - BATTERY_HEIGHT - 2 ,TFT_WIDTH - BATTERY_WIDTH - 2);
}


void init_tft() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);

  joy.createSprite(JOY_SIZE, JOY_SIZE);
  joy.fillSprite(TFT_BLACK);
  joy.fillCircle(JOY_SIZE/2, JOY_SIZE/2, 4, TFT_ORANGE);

  battery.createSprite(BATTERY_WIDTH, BATTERY_HEIGHT);
  battery.fillSprite(TFT_BLACK);

  if (TFT_BL > 0) {                           // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
      pinMode(TFT_BL, OUTPUT);                // Set backlight pin to output mode
      digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
  }

  tft.setSwapBytes(true);
  tft.setRotation(3);
  tft.pushImage(0, 0, TFT_HEIGHT, TFT_WIDTH, steveplusplus);
  vTaskDelay(5000/portTICK_PERIOD_MS);
  tft.fillScreen(TFT_BLACK);
}