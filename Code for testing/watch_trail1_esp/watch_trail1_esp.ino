
#define OUTPUT_INTERRUPT_PIN 4
#define INPUT_INTERRUPT_PIN 15  


// Settings
static const TickType_t timerHigh_delay = 6000 / portTICK_PERIOD_MS;
static const TickType_t timerLow_delay = 6150 / portTICK_PERIOD_MS;



// Globals
static TimerHandle_t timerLow = NULL;
static TimerHandle_t timerHigh =NULL;

//*****************************************************************************
// Callbacks
//Turn on LED when timer high expires 
void timerHighCallback(TimerHandle_t xTimer) {
  digitalWrite(OUTPUT_INTERRUPT_PIN, HIGH);
  Serial.println("Timer High expired");
  
}
// Turn off LED when timer expires and start timerHigh
void timerLowCallback(TimerHandle_t xTimer) {
  digitalWrite(OUTPUT_INTERRUPT_PIN, LOW);
  Serial.println("Timer Low expired");
  xTimerStart(timerHigh, portMAX_DELAY);
}




//variables to keep track of the timing of recent interrupts
unsigned long button_time = 0;  
unsigned long last_button_time = 0; 

void IRAM_ATTR isr() {
    button_time = millis();
    if (button_time - last_button_time > 250)
    {
       Serial.println("Timers being reset");
       xTimerStart(timerLow, portMAX_DELAY);
       xTimerStart(timerHigh, portMAX_DELAY);
       last_button_time = button_time;
    }

}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Program started");
    pinMode(INPUT_INTERRUPT_PIN, INPUT_PULLUP);
    pinMode(OUTPUT_INTERRUPT_PIN, OUTPUT);
    attachInterrupt(INPUT_INTERRUPT_PIN, isr, HIGH);


  timerHigh = xTimerCreate(
                      "timer high",     // Name of timer
                      timerHigh_delay,            // Period of timer (in ticks)
                      pdFALSE,              // Auto-reload
                      (void *)1,            // Timer ID
                      timerHighCallback);  // Callback function

  timerLow = xTimerCreate(
                      "timer low",     // Name of timer
                      timerLow_delay,            // Period of timer (in ticks)
                      pdTRUE,              // Auto-reload
                      (void *)0,            // Timer ID
                      timerLowCallback);  // Callback function

  

   xTimerStart(timerLow, portMAX_DELAY);
   xTimerStart(timerHigh, portMAX_DELAY);

   vTaskDelete(NULL);
    
}

void loop() {
    
}
