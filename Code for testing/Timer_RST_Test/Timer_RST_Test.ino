/**
 *  Timer RST test
 * 
 * Turn off LED for some time and then turn it on with another timer that is delayed
 * If timer are not reset before they timeout
 * Writing to serial is used as an interrupt that resets the timers before they expire
 * Date: October 20, 2022
 * Author: Moses Mulwa
 * License: 0BSD
 */

// You'll likely need this on vanilla FreeRTOS
//#include timers.h

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
static const TickType_t timerLow_delay = 5000 / portTICK_PERIOD_MS;
static const TickType_t timerHigh_delay = 6000 / portTICK_PERIOD_MS;

// Pins (change this if your Arduino board does not have LED_BUILTIN defined)
static const int rst_pin = LED_BUILTIN;

// Globals
static TimerHandle_t timerLow = NULL;
static TimerHandle_t timerHigh =NULL;

//*****************************************************************************
// Callbacks

// Turn off LED when timer expires
void timerLowCallback(TimerHandle_t xTimer) {
  digitalWrite(rst_pin, LOW);
  Serial.println("Timer Low expired");
}
//Turn on LED when timer high expires and start timerLow
void timerHighCallback(TimerHandle_t xTimer) {
 digitalWrite(rst_pin, HIGH);
  Serial.println("Timer High expired");
  xTimerStart(timerLow, portMAX_DELAY);
}

//*****************************************************************************
// Tasks

// Echo things back to serial port, turn on LED when while entering input
void doCLI(void *parameters) {

  char c;

//  // Configure LED pin
//  pinMode(led_pin, OUTPUT);

  while (1) {

    // See if there are things in the input serial buffer
    if (Serial.available() > 0) {

      // If so, echo everything back to the serial port
      c = Serial.read();
      Serial.print(c);

//      // Turn on the LED
//      digitalWrite(led_pin, HIGH);

      // Start timer (if timer is already running, this will act as
      // xTimerReset() instead)
      xTimerStart(timerLow, portMAX_DELAY);
      xTimerStart(timerHigh, portMAX_DELAY);
    }
  }
}


//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  // Configure Serial
  Serial.begin(115200);
  // Configure LED pin
  pinMode(rst_pin, OUTPUT);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Timer Solution---");

  // Create a one-shot timer
  timerLow = xTimerCreate(
                      "timer low",     // Name of timer
                      timerLow_delay,            // Period of timer (in ticks)
                      pdFALSE,              // Auto-reload
                      (void *)0,            // Timer ID
                      timerLowCallback);  // Callback function

  // Create an auto-reload timer
  timerHigh = xTimerCreate(
                      "timer high",     // Name of timer
                      timerHigh_delay,            // Period of timer (in ticks)
                      pdTRUE,              // Auto-reload
                      (void *)1,            // Timer ID
                      timerHighCallback);  // Callback function

   xTimerStart(timerLow, portMAX_DELAY);
   xTimerStart(timerHigh, portMAX_DELAY);
 

 // Start command line interface (CLI) task
  xTaskCreatePinnedToCore(doCLI,
                          "Do CLI",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);

  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // Execution should never get here
}
