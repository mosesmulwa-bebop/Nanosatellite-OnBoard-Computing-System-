/**
 *  Final Project Compilation
 * Date: October 20, 2022
 * Author: Moses Mulwa
 * License: 0BSD
 */


// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

/*****Definitions for the Temp Sensor***/
#define ADC_VREF_mV    3300.0 // in millivolt
#define ADC_RESOLUTION 4096.0
#define PIN_LM35       34 // ESP32 pin GIOP34 (ADC6) connected to LM35


//**************Definitions for Communication
#define RXp2 16
#define TXp2 17
//**************Definitions for Current Sensor
#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

//***************Definitions for WatchDog
#define OUTPUT_INTERRUPT_PIN 19
#define INPUT_INTERRUPT_PIN 18

//**************Definitions for RGB
#define RED_PIN 35
#define GREEN_PIN 32
#define BLUE_PIN 33

int red = 0;
int green = 255;
int blue = 0;

//Global
static SemaphoreHandle_t bin_sem = NULL;
static const int msg_queue_len = 7;     // Size of msg_queue

// Message struct
typedef struct Message {
  char date_time[20];
  float temp;
  float current_mA;
  float loadvoltage;
} Message;

// Globals
static QueueHandle_t msg_queue;
static const TickType_t timerHigh_delay = 6000 / portTICK_PERIOD_MS;
static const TickType_t timerLow_delay = 6150 / portTICK_PERIOD_MS;
static const TickType_t timerComm_delay = 70150 / portTICK_PERIOD_MS;

// Task handles
static TaskHandle_t read_sensors_handle = NULL;
static TaskHandle_t write_sd_card_handle = NULL;
static TaskHandle_t communication_handle = NULL;

bool read_sensors_is_suspended = NULL;
bool write_sd_card_is_suspended = NULL;
bool communication_is_suspended = NULL;

// Timer Handles
static TimerHandle_t timerLow = NULL;
static TimerHandle_t timerHigh =NULL;
static TimerHandle_t timerComm =NULL;
 

char current_mode = 'n';
char next_mode = 's';




//**********************TIMER STUFF***********************************
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

void timerCommCallback(TimerHandle_t xTimer) {
  if(current_mode != 'c'){
      BaseType_t task_woken = pdFALSE;
      Serial.println("Timer Comm starting Comms");
      next_mode = 'c';
      // Give semaphore to tell task that new mode should be switched
      xSemaphoreGiveFromISR(bin_sem, &task_woken);
    }
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



//*****************************************************************************
// Tasks
// Task: read Temperature 
void read_sensors(void *parameters) {

  Message sensor_msg;
  char some_time[] = "10:48:34";  
  // Loop forever
  while (1) {
    // read the ADC value from the temperature sensor
    int adcVal = analogRead(PIN_LM35);
    // convert the ADC value to voltage in millivolt
    float milliVolt = adcVal * (ADC_VREF_mV / ADC_RESOLUTION);
    // convert the voltage to the temperature in ??C
    float tempC = milliVolt / 10;

    float shuntvoltage = ina219.getShuntVoltage_mV();
    float busvoltage = ina219.getBusVoltage_V();
    float current_mA = ina219.getCurrent_mA();
    float power_mW = ina219.getPower_mW();
    float loadvoltage = busvoltage + (shuntvoltage / 1000);

  
    if(tempC < 40.0 && tempC != 0.00){
      
       // print the temperature in the Serial Monitor:
        Serial.print("Temperature: ");
        Serial.print(tempC);   // print the temperature in ??C
        Serial.println("??C");
        
        // Construct message and send
        strcpy(sensor_msg.date_time, some_time);
        sensor_msg.temp = tempC;
        sensor_msg.current_mA = current_mA;
        sensor_msg.loadvoltage = loadvoltage;
        xQueueSend(msg_queue, (void *)&sensor_msg, 10);
    }


    
  
    Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
    Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
    Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
    Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
    Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
    Serial.println("");
    
    vTaskDelay(7000 / portTICK_PERIOD_MS);
  }
}

// Task: write Value in Message Queue to SD Card
void write_sd_card(void *parameters) {

  Message msg;

  // Loop forever
  while (1) {
    // See if there's a message in the queue (do not block)
    if (xQueueReceive(msg_queue, (void *)&msg, 0) == pdTRUE) {
      char first_temp[]= " Collected Temperature(??C): ";
      float val = msg.temp; 
      char sz[20] = {' '} ;
      int val_int = (int) val;   
      float val_float = (abs(val) - abs(val_int)) * 100000;// compute the integer part of the float
      int val_fra = (int)val_float;
      sprintf (sz, "%d.%d", val_int, val_fra); 
      
      strcat(first_temp,sz);
      
      Serial.println(first_temp);
      Serial.println("Temp in Queue");
      Serial.println(msg.temp);
      Serial.println(msg.date_time);
      //writeFile(SD, "/stuffy/temp.txt", msg.date_time);
//     appendFile(SD, "/stuffy/temp.txt", msg.date_time);
//     appendFile(SD, "/stuffy/temp.txt", first_temp);
//     appendFile(SD, "/stuffy/temp.txt", " \n");

     char curr_ma[] = " Current(mA): ";
     float curr_val = msg.current_mA;
     char sz2[20] = {' '} ;
     int curr_val_int = (int) curr_val;   
     float curr_val_float = (abs(curr_val) - abs(curr_val_int)) * 100000;// compute the integer part of the float
     int curr_val_fra = (int)curr_val_float;
     sprintf (sz2, "%d.%d", curr_val_int, curr_val_fra); 
     strcat(curr_ma,sz2);
     
     char load_vo[] = " Load Voltage(V): ";
     float load_val = msg.loadvoltage;
     char sz3[20] = {' '} ;
     int load_val_int = (int) load_val;   
     float load_val_float = (abs(load_val) - abs(load_val_int)) * 100000;// compute the integer part of the float
     int load_val_fra = (int)load_val_float;
     sprintf (sz3, "%d.%d", load_val_int, load_val_fra); 
     strcat(load_vo,sz3);
     
//     appendFile(SD, "/stuffy/current.txt", curr_ma);
//     appendFile(SD, "/stuffy/current.txt", load_vo);
//     appendFile(SD, "/stuffy/current.txt", " \n");
    }
  

 
  }
}

//Task: Blink RGB
void blink_RGB(void *parameters){
//  analogWrite(RED_PIN, red);
//  analogWrite(GREEN_PIN, green);
//  analogWrite(BLUE_PIN, blue);
Serial.println("LED BLINK");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

//Task: Send Communication to COMMS
void communication(void *parameters) {
  
    Serial.println(" This is some communication");
    Serial2.println(" This is some communication");
    vTaskDelay(7000 / portTICK_PERIOD_MS);
    Serial.println(" This is second communication");
    Serial2.println(" This is second communication");

//    BaseType_t task_woken = pdFALSE;
//    next_mode = 'n';
//    xSemaphoreGiveFromISR(bin_sem, &task_woken);
  
}



// Listen to Serial Port
void doCLI(void *parameters) {

  char c;

  while (1) {

    // See if there are things in the input serial buffer
    if (Serial.available() > 0) {

      // If so, echo everything back to the serial port
      c = Serial.read();
      Serial.print(c);
      BaseType_t task_woken = pdFALSE;
      if(c == 'n'){
        if(current_mode != 'n'){
          next_mode = 'n';
          // Give semaphore to tell task that new mode should be switched
          xSemaphoreGiveFromISR(bin_sem, &task_woken);
        }
      }else if(c == 'e'){
        if(current_mode != 'e'){
          next_mode = 'e';
          // Give semaphore to tell task that new mode should be switched
          xSemaphoreGiveFromISR(bin_sem, &task_woken);
        }
      }
      else if(c == 'c'){
        if(current_mode != 'c'){
          next_mode = 'c';
          // Give semaphore to tell task that new mode should be switched
          xSemaphoreGiveFromISR(bin_sem, &task_woken);
        }
      }
      

    }
  }
}

void modeSwitcher(void *parameters){
  // Loop forever, wait for semaphore, and switch mode
  while (1) {
      xSemaphoreTake(bin_sem, portMAX_DELAY);
      Serial.println("Taken Semaphore"); 
      if(next_mode == 'n' && current_mode != 'n'){
              Serial.println("Switching to Normal Mode");
              if(read_sensors_handle == NULL){
                xTaskCreatePinnedToCore(read_sensors, "Read From Sensors", 3000, NULL, 1, &read_sensors_handle, app_cpu);
              }
              if(write_sd_card_handle == NULL){
                xTaskCreatePinnedToCore(write_sd_card, "Write To SD Card", 6000, NULL, 1, &write_sd_card_handle, app_cpu);
              }
              if(read_sensors_is_suspended == true){
                vTaskResume(read_sensors_handle);
                read_sensors_is_suspended = false;
              }
              if(write_sd_card_is_suspended == true){
                vTaskResume(write_sd_card_handle);
                write_sd_card_is_suspended = false;
              }
              if(communication_handle != NULL){
                vTaskSuspend(communication_handle);
                communication_is_suspended = true; 
              }
              current_mode = 'n';
              red = 0;
              green = 255;
              blue = 0;
              
              next_mode = NULL;
     }else if(next_mode == 'e' && current_mode != 'e'){
              Serial.println("Switching to Emergency Mode");
              if(read_sensors_handle == NULL){
                xTaskCreatePinnedToCore(read_sensors, "Read From Sensors", 6000, NULL, 1, &read_sensors_handle, app_cpu);
              }
              if(write_sd_card_is_suspended != true){
                vTaskSuspend(write_sd_card_handle);
                write_sd_card_is_suspended = true;
              }
              if(communication_is_suspended != true){
                vTaskSuspend(communication_handle);
                communication_is_suspended = true;
              }
              current_mode = 'e';
              red = 255;
              green = 0;
              blue = 0;
              next_mode = NULL;
     }else if(next_mode == 'c' && current_mode != 'c'){
              Serial.println("Switching to Communication Mode");
              if(communication_handle == NULL){
                xTaskCreatePinnedToCore(communication, "Communication", 6000, NULL, 1, &communication_handle, app_cpu);
              }
              if(write_sd_card_is_suspended != true){
                vTaskSuspend(write_sd_card_handle);
                write_sd_card_is_suspended = true;
              }
              if(read_sensors_is_suspended != true){
                vTaskSuspend(read_sensors_handle);
                read_sensors_is_suspended = true;
              }
              current_mode = 'c';
              red = 0;
              green = 0;
              blue = 255;
              next_mode = NULL;
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
  Serial.println("---FreeRTOS Mode Switching Solution---");


 

  // Create queues for sensors
  
  msg_queue = xQueueCreate(msg_queue_len, sizeof(Message));

  // Communication setup
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);


  //*********Current sensor setup
  uint32_t currentFrequency;
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  //*****************Timer setup
  pinMode(INPUT_INTERRUPT_PIN, INPUT_PULLUP);
  pinMode(OUTPUT_INTERRUPT_PIN, OUTPUT);
  attachInterrupt(INPUT_INTERRUPT_PIN, isr, HIGH);

  timerHigh = xTimerCreate( "timer high", timerHigh_delay, pdFALSE, (void *)1, timerHighCallback);  
  timerLow = xTimerCreate( "timer low", timerLow_delay, pdTRUE,(void *)0,timerLowCallback);
  timerComm = xTimerCreate( "timer comm", timerComm_delay, pdTRUE,(void *)2,timerCommCallback);  

  

   xTimerStart(timerLow, portMAX_DELAY);
   xTimerStart(timerHigh, portMAX_DELAY);
   xTimerStart(timerComm, portMAX_DELAY);


 // Create semaphore before it is used (in task or ISR)
  bin_sem = xSemaphoreCreateBinary();
 
//  xTaskCreatePinnedToCore(doCLI, "Do CLI", 1024, NULL, 1, NULL, app_cpu);
//  xTaskCreatePinnedToCore(modeSwitcher, "Mode Switcher", 1024, NULL, 2, NULL, app_cpu);
  xTaskCreatePinnedToCore(blink_RGB, "Blink RGB", 3000, NULL, 1, NULL, app_cpu);
//  xTaskCreatePinnedToCore(read_sensors, "Read From Sensors", 6000, NULL, 1, &read_sensors_handle, app_cpu);
//  xTaskCreatePinnedToCore(write_sd_card, "Write To SD Card", 6000, NULL, 1, &write_sd_card_handle, app_cpu);
  
  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // Execution should never get here
}
