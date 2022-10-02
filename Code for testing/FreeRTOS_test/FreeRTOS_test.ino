// Use only one core for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Pins
static const int led_pin = LED_BUILTIN;


// Our task: blink an LED 
// Function should return nothing and accept one void pointer as a parameter
void toggleLED(void *parameter) {
  while(1){
    digitalWrite(led_pin, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS); // Use this because it's non blocking as opposed to the delay one.
    digitalWrite(led_pin, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}



void setup() {
  // Configure pin to be an output as normally done
  pinMode(led_pin, OUTPUT);

  // Task Creation to run forever  // use xTaskCreate for Vanilla FreeRTOS
  xTaskCreatePinnedToCore(
    toggleLED,    // Function to be called
    "Toggle LED", // Name of the task
    1024,         // Stack size (bytes in ESP32, words in FreeRTOS)
    NULL,         // Parameter to pass to function
    1,            // Task Priority, the higher the number, the higher the priority (0 to 25)
    NULL,         // Task Handle
    app_cpu       // CPU Core the task should run in
    ); 

   // If this was vanilla FreeRTOS, you'd want to call vTaskStartScheduler() in
  // main after setting up your tasks.

}

void loop() {
   // Do nothing
  // setup() and loop() run in their own task with priority 1 in core 1
  // on ESP32

}
