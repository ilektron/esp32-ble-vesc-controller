#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#include <Arduino.h>

#include "VescUart.h"
#include "display.h"
#include "esp_adc_cal.h"
#include "joystick.h"
#include "radio.h"
#include "soc/rtc_wdt.h"
#include <BluetoothSerial.h>
#include <Button2.h>
#include <array>

#define ADC_EN 14 // ADC_EN is the ADC detection enable port
#define ADC_VIN_PIN 34
#define BUTTON_1 35
#define BUTTON_2 0

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

Joystick joystick;

QueueHandle_t xBLEQueue;
QueueHandle_t xDisplayQueue;

// Globals
int vref = 1100; // ADC voltage reference

// define two tasks for Blink & AnalogRead
void TaskButton(void *pvParameters);
void TaskAnalogReadVin(void *pvParameters);
void TaskDisplay(void *pvParameters);
void TaskRadio(void *pvParameters);

float battery_voltage = 0.0f;

// the setup function runs once when you press reset or power the board
void setup() {

  rtc_wdt_protect_off();
  rtc_wdt_disable();
  // initialize serial communication at 115200 bits per second:
  Serial.begin(1000000);
  Serial.println("Start");

  // Deep sleep code
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);

  Serial.println("Creating queues");
  auto xBLEQueue = xQueueCreate(10, sizeof(unsigned long));
  auto xDisplayQueue = xQueueCreate(10, sizeof(unsigned long));

  Serial.println("Creating Tasks");
  xTaskCreatePinnedToCore(TaskButton, "TaskButton", // A name just for humans
                          2024, // This stack size can be checked & adjusted by reading the Stack Highwater
                          xBLEQueue,
                          3, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
                          nullptr, ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(TaskAnalogReadVin, "AnalogReadVin",
                          2024, // Stack size
                          nullptr,
                          2, // Priority
                          nullptr, ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(TaskDisplay, "Display",
                          2048, // Stack size
                          nullptr,
                          1, // Priority
                          nullptr, ARDUINO_RUNNING_CORE);

  // TODO: Could pin this to the alternate core so that we have the most reliable communication
  xTaskCreatePinnedToCore(TaskRadio, "Radio",
                          8192 * 2, // Stack size
                          nullptr,
                          4, // Priority
                          nullptr, ARDUINO_RUNNING_CORE);

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop() {
  // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskButton(void *pvParameters) // This is a task.
{
  (void)pvParameters; // Avoids unused parameter error

  btn1.setPressedHandler([](Button2 &b) {
    // Right Button
  });

  btn2.setPressedHandler([](Button2 &b) {
    // Left Button
  });

  constexpr auto xDelay = 10u / portTICK_PERIOD_MS;
  for (;;) // A Task shall never return or exit.
  {
    btn1.loop();
    btn2.loop();
    vTaskDelay(xDelay); // one tick delay (15ms) in between reads for stability
  }
}

void TaskAnalogReadVin(void *pvParameters) // This is a task.
{
  (void)pvParameters;
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
  esp_adc_cal_value_t val_type =
      esp_adc_cal_characterize(static_cast<adc_unit_t>(ADC_UNIT_1), static_cast<adc_atten_t>(ADC1_CHANNEL_6),
                               static_cast<adc_bits_width_t>(ADC_WIDTH_BIT_12), 1100, &adc_chars);

  // Check type of calibration value used to characterize ADC
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    vref = adc_chars.vref;
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
  } else {
    Serial.println("Default Vref: 1100mV");
  }

  constexpr auto xDelay = 30u / portTICK_PERIOD_MS;

  // Get zero levels for the joystick
  joystick.set_zeros(analogRead(A4), analogRead(A5));

  for (;;) {
    uint16_t v = analogRead(ADC_VIN_PIN);
    battery_voltage = (static_cast<float>(v) / 4095.0f) * 2.0f * 3.3f * (vref / 1000.0f);
    joystick.set_pos(analogRead(A4), analogRead(A5));
    if (battery_voltage < 3.1f) {
      Serial.println("Battery voltage low, entering sleep");
      esp_deep_sleep_start();
    }
    vTaskDelay(xDelay); // one tick delay (15ms) in between reads for stability
  }
}

void TaskDisplay(void *pvParameters) // This is a task.
{
  (void)pvParameters;

  init_tft();

  constexpr auto xDelay = 30u / portTICK_PERIOD_MS;
  for (;;) {
    draw_joystick(joystick);
    draw_battery(battery_voltage);
    draw_ble_state();
    draw_controller_state(controller);
    vTaskDelay(xDelay);
  }
}

// We might want to split this into 2 tasks, an RX and a TX task so that we handle data processing more efficiently
void TaskRadio(void *pvParameters) // This is a task.
{
  (void)pvParameters;

  radio_init();

  constexpr auto xDelay = 1u / portTICK_PERIOD_MS;
  for (;;) {
    radio_run(joystick);
    vTaskDelay(xDelay);
  }
}