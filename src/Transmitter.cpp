#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#include <Arduino.h>

#ifndef CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS
#define CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS
#endif

#include <TFT_eSPI.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <Button2.h>
#include "esp_adc_cal.h"
#include "bmp.h"

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_VIN_PIN         34
#define BUTTON_1            35
#define BUTTON_2            0

enum class DisplayState : uint8_t { idle = 0, adc, joystick, sleep };

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

// Globals
DisplayState state = DisplayState::idle;
int vref = 1100; // ADC voltage reference
int posx = -1;   // Joystick position for x
int posy = -1;   // Joystick position for y

// define two tasks for Blink & AnalogRead
void TaskButton( void *pvParameters );
void TaskAnalogReadVin( void *pvParameters );
void TaskDisplay( void *pvParameters );
void TaskGetFollowerCount( void *pvParameters );

String voltage{};

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

void next_screen() {
  if (state == DisplayState::sleep) {
    state = DisplayState::idle;
  } else {
    state = static_cast<DisplayState>(static_cast<uint8_t>(state) + 1u);
  }
  Serial.println(static_cast<uint8_t>(state), HEX);
}

void previous_screen() {
  if (state == DisplayState::idle) {
    state = DisplayState::sleep;
  } else {
    state = static_cast<DisplayState>(static_cast<uint8_t>(state) - 1u);
  }
  Serial.println(static_cast<uint8_t>(state), HEX);
}

// the setup function runs once when you press reset or power the board
void setup() {

  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  Serial.println("Start");

  // Deep sleep code
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);

  xTaskCreatePinnedToCore(
    TaskButton
    ,  "TaskButton"   // A name just for humans
    ,  2024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskAnalogReadVin
    ,  "AnalogReadVin"
    ,  2024  // Stack size
    ,  NULL
    ,  2  // Priority
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskDisplay
    ,  "Display"
    ,  6000  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop()
{
  // Empty. Things are done in Tasks.
}

void draw_default() {
  tft.setRotation(0);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("LeftButton:", tft.width() / 2, tft.height() / 2 - 16);
  tft.drawString("[TikTok Follwers]", tft.width() / 2, tft.height() / 2 );
  tft.drawString("RightButton:", tft.width() / 2, tft.height() / 2 + 16);
  tft.drawString("[Voltage Monitor]", tft.width() / 2, tft.height() / 2 + 32 );
  tft.setTextDatum(TL_DATUM);
}

void draw_adc() {
  constexpr auto multiplier = 5u;
  static auto last_voltage = voltage;
  if (last_voltage.compareTo(voltage) != 0) {
    tft.setRotation(1);
    tft.setTextSize(multiplier);
    Serial.println(voltage);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(voltage,  tft.width() / 2, tft.height() / 2 );
    last_voltage = voltage;
  }
  
}

void draw_joystick() {
  static int last_posx = -1;
  static int last_posy = -1;
  constexpr auto scale = 4095/134;

  if (last_posx/scale != posx/scale || last_posy/scale != posy/scale) {
    tft.setRotation(1);
    Serial.print("posx: ");
    Serial.print(posx/scale);
    Serial.print("\tposy: ");
    Serial.println(posy/scale);
    //tft.fillScreen(TFT_BLACK);
    tft.drawPixel(posx / scale, posy / scale, TFT_GREEN);
    tft.drawPixel(last_posx / scale, last_posy / scale, TFT_BLACK);
    last_posx = posx;
    last_posy = posy;
  }
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskButton(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  btn1.setPressedHandler([](Button2 & b) {
    Serial.println("Next screen");
    next_screen();
  });

  btn2.setPressedHandler([](Button2 & b) {
    Serial.println("Previous screen");
    previous_screen();
  });

  constexpr auto xDelay = 10u/portTICK_PERIOD_MS;
  for (;;) // A Task shall never return or exit.
  {
    btn1.loop();
    btn2.loop();
    vTaskDelay(xDelay);  // one tick delay (15ms) in between reads for stability
  }
}

void TaskAnalogReadVin(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  // Check of calibration

  /*
    ADC_EN is the ADC detection enable port
    If the USB port is used for power supply, it is turned on by default.
    If it is powered by battery, it needs to be set to high level
  */
  pinMode(ADC_EN, OUTPUT);
  digitalWrite(ADC_EN, HIGH);

  // Initialize the ADC
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC1_CHANNEL_6, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);
  
  //Check type of calibration value used to characterize ADC
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
      Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
      vref = adc_chars.vref;
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
      Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
  } else {
      Serial.println("Default Vref: 1100mV");
  }

  constexpr auto xDelay = 30u/portTICK_PERIOD_MS;

  for (;;)
  {
    uint16_t v = analogRead(ADC_VIN_PIN);
    float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    posy = analogRead(A4);
    posx = analogRead(A5);
    if (battery_voltage < 3.1f) {
      Serial.println("Battery voltage low, entering sleep");
      esp_deep_sleep_start();
    }
    if (state == DisplayState::adc) {
      voltage = String(battery_voltage) + "V";

    }
    vTaskDelay(xDelay);  // one tick delay (15ms) in between reads for stability
  }
}

void TaskDisplay(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);

  if (TFT_BL > 0) {                           // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
      pinMode(TFT_BL, OUTPUT);                // Set backlight pin to output mode
      digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
  }

  tft.setSwapBytes(true);
  tft.pushImage(0, 0,  240, 135, steveplusplus);
  vTaskDelay(5000/portTICK_PERIOD_MS);

  constexpr auto xDelay = 30u/portTICK_PERIOD_MS;
  for (;;)
  {
    static auto last_state = DisplayState::idle;
    if (last_state != state) {
      tft.fillScreen(TFT_BLACK);
    }
    switch (state) {
      case DisplayState::sleep:
        TFT_sleep();
        esp_deep_sleep_start();
        TFT_wake(); // May never get here
        break;
      case DisplayState::adc:
        draw_adc();
        break;
      case DisplayState::joystick:
        draw_joystick();
        break;
      case DisplayState::idle:
      default:
        draw_default();
        break;
    }

    last_state = state;
    vTaskDelay(xDelay); 
  }
}
