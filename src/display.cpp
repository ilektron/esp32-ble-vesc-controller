// Implementation of all the graphics that we'll draw on our screen

#include "display.h"
#include "bmp.h"
#include "radio.h"

constexpr uint16_t JOY_SIZE = 24;
constexpr uint16_t BLE_SIZE = 24;
constexpr uint16_t BATTERY_WIDTH = 16;
constexpr uint16_t BATTERY_HEIGHT = 8;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

/*
static TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT); // Invoke custom library
static TFT_eSprite joy = TFT_eSprite(&tft);
static TFT_eSprite battery = TFT_eSprite(&tft);
static TFT_eSprite ble = TFT_eSprite(&tft);
*/

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 };

void draw_joystick(Joystick &j) {
  static int last_x = -1;
  static int last_y = -1;

  int x = JOY_SIZE / 2 * -j.x();
  int y = JOY_SIZE / 2 * j.y();

  x += JOY_SIZE / 2.0f;
  y += JOY_SIZE / 2.0f;

  if (last_x != x || last_y != y) {
    // tft.setRotation(0);
    // Serial.print("posx: ");
    // Serial.print(x);
    // Serial.print("\tposy: ");
    // Serial.println(y);
    // joy.fillSprite(TFT_BLACK);
    // joy.drawCircle(JOY_SIZE / 2, JOY_SIZE / 2, JOY_SIZE / 2 - 1, TFT_GOLD);
    // joy.fillCircle(x, y, 3, TFT_GOLD);
    // joy.pushSprite(TFT_WIDTH - JOY_SIZE, 0);
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

  // tft.setRotation(0);
  // battery.fillRect((BATTERY_WIDTH - 3) * (1-percent), 1,  1, BATTERY_HEIGHT - 1, TFT_BLACK);
  // battery.fillSprite(TFT_BLACK);
  // Draw the outline
  // battery.drawRect(0, 0, BATTERY_WIDTH - 1, BATTERY_HEIGHT - 1, TFT_GREEN);
  // Draw the percent full
  // battery.fillRect(1, 1, (BATTERY_WIDTH - 3) * percent, BATTERY_HEIGHT - 1, TFT_GREEN);
  // Draw the tab
  // battery.fillRect(BATTERY_WIDTH - 2, 2, 2, BATTERY_HEIGHT - 5, TFT_GREEN);

  // battery.pushSprite(0, 0);
  // battery.pushSprite(TFT_HEIGHT - BATTERY_HEIGHT - 2 ,TFT_WIDTH - BATTERY_WIDTH - 2);
}

void draw_ble_state() {

  static int animator{};

  animator = animator >= BLE_SIZE ? 0 : animator + 1;

  // ble.fillSprite(TFT_BLACK);

/*
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
  */
}

void draw_controller_state(const vesc::controller &controller) {
  const auto hw = "Connected to: " + controller.getHW();

  auto line = 32ul;
  constexpr auto lh = 12u;
  //if (hw.length()) { tft.drawString(hw.c_str(), 0, line); line += lh; }

  std::array<char, 100> s;
  auto values = controller.values();
  sprintf(s.data(), "vescid:\t%i", values.vesc_id);
  //tft.drawString(s.data(), 0, line); line += lh;
  sprintf(s.data(), "Vin:\t%4.1f", values.v_in);
  //tft.drawString(s.data(), 0, line); line += lh;
  sprintf(s.data(), "current:\t%4.1f", values.current_in);
  //tft.drawString(s.data(), 0, line); line += lh;
  sprintf(s.data(), "rpm:\t%10.1f", values.rpm);
  //tft.drawString(s.data(), 0, line); line += lh;
  sprintf(s.data(), "ah:\t%4.1f", values.amp_hours);
  //tft.drawString(s.data(), 0, line); line += lh;
  sprintf(s.data(), "fault:\t%4i", values.fault_code);
  //tft.drawString(s.data(), 0, line); line += lh;

}


void testdrawline() {
  int16_t i;

  display.clearDisplay(); // Clear display buffer

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, SSD1306_WHITE);
    display.display(); // Update screen with each newly-drawn line
    delay(1);
  }
  for(i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, display.height()-1, i, 0, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(0, display.height()-1, display.width()-1, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=display.width()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, i, 0, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, 0, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.height(); i+=4) {
    display.drawLine(display.width()-1, 0, 0, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  for(i=0; i<display.width(); i+=4) {
    display.drawLine(display.width()-1, 0, i, display.height()-1, SSD1306_WHITE);
    display.display();
    delay(1);
  }

  delay(2000); // Pause for 2 seconds
}

void testdrawrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, SSD1306_WHITE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}

void testfillrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=3) {
    // The INVERSE color is used so rectangles alternate white/black
    display.fillRect(i, i, display.width()-i*2, display.height()-i*2, SSD1306_INVERSE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}

void testdrawcircle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=2) {
    display.drawCircle(display.width()/2, display.height()/2, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfillcircle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=3) {
    // The INVERSE color is used so circles alternate white/black
    display.fillCircle(display.width() / 2, display.height() / 2, i, SSD1306_INVERSE);
    display.display(); // Update screen with each newly-drawn circle
    delay(1);
  }

  delay(2000);
}

void testdrawroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, SSD1306_WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfillroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    // The INVERSE color is used so round-rects alternate white/black
    display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, SSD1306_INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawtriangle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=5) {
    display.drawTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, SSD1306_WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfilltriangle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=5) {
    // The INVERSE color is used so triangles alternate white/black
    display.fillTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, SSD1306_INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawchar(void) {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  for(int16_t i=0; i<256; i++) {
    if(i == '\n') display.write(' ');
    else          display.write(i);
  }

  display.display();
  delay(2000);
}

void testdrawstyles(void) {
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Hello, world!"));

  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(3.141592);

  display.setTextSize(2);             // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.print(F("0x")); display.println(0xDEADBEEF, HEX);

  display.display();
  delay(2000);
}

void testscrolltext(void) {
  display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(F("scroll"));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}

void testdrawbitmap(void) {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
}

#define XPOS   0 // Indexes into the 'icons' array in function below
#define YPOS   1
#define DELTAY 2

void testanimate(const uint8_t *bitmap, uint8_t w, uint8_t h) {
  int8_t f, icons[NUMFLAKES][3];

  // Initialize 'snowflake' positions
  for(f=0; f< NUMFLAKES; f++) {
    icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
    icons[f][YPOS]   = -LOGO_HEIGHT;
    icons[f][DELTAY] = random(1, 6);
    Serial.print(F("x: "));
    Serial.print(icons[f][XPOS], DEC);
    Serial.print(F(" y: "));
    Serial.print(icons[f][YPOS], DEC);
    Serial.print(F(" dy: "));
    Serial.println(icons[f][DELTAY], DEC);
  }

  for(;;) { // Loop forever...
    display.clearDisplay(); // Clear the display buffer

    // Draw each snowflake:
    for(f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, SSD1306_WHITE);
    }

    display.display(); // Show the display buffer on the screen
    delay(200);        // Pause for 1/10 second

    // Then update coordinates of each flake...
    for(f=0; f< NUMFLAKES; f++) {
      icons[f][YPOS] += icons[f][DELTAY];
      // If snowflake is off the bottom of the screen...
      if (icons[f][YPOS] >= display.height()) {
        // Reinitialize to a random position, just off the top
        icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
        icons[f][YPOS]   = -LOGO_HEIGHT;
        icons[f][DELTAY] = random(1, 6);
      }
    }
  }
}

void init_tft() {
  /*
  tft.init();
  tft.setRotation(1);
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
  tft.setRotation(3);
  tft.pushImage(0, 0, TFT_HEIGHT, TFT_WIDTH, steveplusplus);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  tft.fillScreen(TFT_BLACK);
  */

 if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);
  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). These examples demonstrate both approaches...

  testdrawline();      // Draw many lines

  testdrawrect();      // Draw rectangles (outlines)

  testfillrect();      // Draw rectangles (filled)

  testdrawcircle();    // Draw circles (outlines)

  testfillcircle();    // Draw circles (filled)

  testdrawroundrect(); // Draw rounded rectangles (outlines)

  testfillroundrect(); // Draw rounded rectangles (filled)

  testdrawtriangle();  // Draw triangles (outlines)

  testfilltriangle();  // Draw triangles (filled)

  testdrawchar();      // Draw characters of the default font

  testdrawstyles();    // Draw 'stylized' characters

  testscrolltext();    // Draw scrolling text

  testdrawbitmap();    // Draw a small bitmap image

  // Invert and restore display, pausing in-between
  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  delay(1000);

  testanimate(logo_bmp, LOGO_WIDTH, LOGO_HEIGHT); // Animate bitmaps

}