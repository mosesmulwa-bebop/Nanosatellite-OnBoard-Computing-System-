/**
 * Writing temp values to an SD Card
 * 
 * One task reads analog value from a temp sensor. It then store this value in a queue.
 * The second task reads this queue and writes it to an SD Card
 * 
 * Date: October 9, 2022
 * Author: Moses Mulwa
 * License: 0BSD
 */

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif


/**Definitions for the Temp Sensor***/
#define ADC_VREF_mV    3300.0 // in millivolt
#define ADC_RESOLUTION 4096.0
#define PIN_LM35       34 // ESP32 pin GIOP34 (ADC6) connected to LM35


// Settings
static const int msg_queue_len = 5;     // Size of msg_queue




// Message struct: used to wrap strings (not necessary, but it's useful to see
// how to use structs here)
typedef struct Message {
  char date_time[20];
  float temp;
} Message;

// Globals
static QueueHandle_t msg_queue;

//*****************************************************************************
// Tasks

// Task: read Temperature 
void readTemp(void *parameters) {

  Message temp_msg;
  char some_time[] = "10:48:34";  
  // Loop forever
  while (1) {
    // read the ADC value from the temperature sensor
    int adcVal = analogRead(PIN_LM35);
    // convert the ADC value to voltage in millivolt
    float milliVolt = adcVal * (ADC_VREF_mV / ADC_RESOLUTION);
    // convert the voltage to the temperature in °C
    float tempC = milliVolt / 10;
  
    if(tempC < 40.0 && tempC != 0.00){
      
       // print the temperature in the Serial Monitor:
        Serial.print("Temperature: ");
        Serial.print(tempC);   // print the temperature in °C
        Serial.println("°C");
        
        // Construct message and send
        strcpy(temp_msg.date_time, some_time);
        temp_msg.temp = tempC;
        xQueueSend(msg_queue, (void *)&temp_msg, 10);
    }
    
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

// Task: write Value in Message Queue to SD Card
void writeSD(void *parameters) {

  Message msg;

  // Loop forever
  while (1) {
    // See if there's a message in the queue (do not block)
    if (xQueueReceive(msg_queue, (void *)&msg, 0) == pdTRUE) {
      Serial.println("Temp in Queue");
      Serial.println(msg.temp);
      Serial.println(msg.date_time);
    }
  

 
  }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Temp reading and writing to SD Card---");
 

  // Create queues
  
  msg_queue = xQueueCreate(msg_queue_len, sizeof(Message));

  // Start CLI task
  xTaskCreatePinnedToCore(readTemp,
                          "Read temp",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);

  // Start blink task
  xTaskCreatePinnedToCore(writeSD,
                          "Write SD",
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
