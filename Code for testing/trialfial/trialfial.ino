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
//#if CONFIG_FREERTOS_UNICORE
//  static const BaseType_t app_cpu = 0;
//#else
//  static const BaseType_t app_cpu = 1;
//#endif

static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

/*Definitions for the Temp Sensor*/
#define ADC_VREF_mV    5000.0 // in millivolt
#define ADC_RESOLUTION 4096.0
#define PIN_LM35       34 // ESP32 pin GIOP34 (ADC6) connected to LM35

// libraries for the SD Card
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Definitions for current sensor
#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;


float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float loadvoltage = 0;
float power_mW = 0;

// Definitions for LED
#define RED_PIN 25
#define GREEN_PIN 32
#define BLUE_PIN 33

int red=0;
int green=255;
int blue=0;

//**Definitions for Communication*/
#define RXp2 16
#define TXp2 17

//***Definitions for WatchDog*/
#define OUTPUT_INTERRUPT_PIN 4
#define INPUT_INTERRUPT_PIN 15
/*Settings*/
// Counter and bools for recovery
int temp_high_counter = 0;
bool temp_is_high = false;
#define TEMP_THRESHOLD 30.0 //In Celsisus

int current_high_counter = 0;
bool current_is_high = false;
#define CURRENT_THRESHOLD 2 //In mA

String emergency_string ="A";





// Settings
static const int msg_queue_len = 5;     // Size of msg_queue
static const int curr_msg_queue_len = 5;
static const TickType_t timerComm_delay = 30150 / portTICK_PERIOD_MS;
static const TickType_t timerCommBlue_delay = 32150 / portTICK_PERIOD_MS;
static const TickType_t timerHigh_delay = 6000 / portTICK_PERIOD_MS;
static const TickType_t timerLow_delay = 6150 / portTICK_PERIOD_MS;

// Timer Handles
static TimerHandle_t timerLow = NULL;
static TimerHandle_t timerHigh =NULL;
static TimerHandle_t timerComm =NULL;
static TimerHandle_t timerCommBlue =NULL;

// led bools
bool is_blue= false;
bool is_red = false;


// Message struct
typedef struct Message {
  char date_time[20];
  float temp;
} Message;

typedef struct CurrMessage {
  float current_mA;
  float loadvoltage;
} CurrMessage;

// Globals
static QueueHandle_t msg_queue;
static QueueHandle_t curr_msg_queue;
String all_temps ="OT,";
String all_currents ="OC,";
String all_loads ="OL,";
String all_obc ="OB,";

/*****/
////// SD CARD FUCNTIONS
void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}


void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
      Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}



//***
// TIMER CALLBACKS

void timerCommCallback(TimerHandle_t xTimer) {
 all_obc.concat(emergency_string);
  
 Serial.println("--------------Comm Activated----------------------"); 
 Serial.println(all_temps);
 Serial.println(all_currents);
 Serial2.println(all_currents);
 Serial2.println(all_temps);
 Serial2.println(all_loads);
 Serial.println(all_loads);
 Serial2.println(all_obc);
 Serial.println(all_obc);
 red=0; 
 green=0;
 blue= 255;
 is_blue=true;
 all_temps ="OT,";
 all_currents ="OC,";
 all_loads ="OL,";
 all_obc ="OB,";
}
void timerCommBlueCallback(TimerHandle_t xTimer) {

 if(is_blue==true && temp_is_high == false && current_is_high == false){
  Serial.println("-----------NORMAL MODE--------------");
  red=0; 
  green=255;
  blue= 0;
  is_blue=false;
 }
 else{
  Serial.println("-----------EMERGENCY MODE--------------");
  red=255; 
  green=0;
  blue= 0;
  is_blue=false;
 }

}
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

    
    
    if(tempC > TEMP_THRESHOLD && temp_is_high == false){
          red=255;
          green=0;
          blue=0;
          temp_high_counter = 1;
          temp_is_high = true;
          Serial.println("---------Switching to Emergency Mode-----------------"); 
     }
    else if(tempC > TEMP_THRESHOLD && temp_is_high == true){
          temp_high_counter++;
          if (temp_high_counter == 5){
             emergency_string = "B";
          }
    }
    else if(tempC < TEMP_THRESHOLD && temp_is_high ==true){
          red=0;
          green=255;
          blue=0;
          temp_is_high=false;
          temp_high_counter = 0;
          emergency_string ="A";
          Serial.println("-----------------Back to Normal Mode----------------");
          
    }
    
      
       // print the temperature in the Serial Monitor:
        Serial.print("Temperature: ");
        Serial.print(tempC);   // print the temperature in °C
        Serial.println("°C");

        
        
        // Construct message and send
        strcpy(temp_msg.date_time, some_time);
        temp_msg.temp = tempC;
        String stringTemp = String(tempC, 1);
        all_temps.concat(stringTemp);
        String comma = ",";
        all_temps.concat(comma);
        xQueueSend(msg_queue, (void *)&temp_msg, 10);

    
    vTaskDelay(8000 / portTICK_PERIOD_MS);
  }
}


void readCurrent(void *parameters){
  
  CurrMessage curr_msg;
  while(1){
    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA();
    power_mW = ina219.getPower_mW();
    loadvoltage = busvoltage + (shuntvoltage / 1000);
    
//    Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
//    Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
//    Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
//    Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
//    Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
//    Serial.println("");

    int current_comparison = (int)current_mA;
    current_comparison = abs(current_comparison);
//    Serial.print("currentcomparison: ");
//    Serial.println(currentcomparison);

    if(current_comparison > CURRENT_THRESHOLD && current_is_high == false){
          red=255;
          green=0;
          blue=0;
          current_high_counter = 1;
          current_is_high = true;
          Serial.println("---------Switching to Emergency Mode Due To Current-----------------"); 
     }
    else if(current_comparison > CURRENT_THRESHOLD && current_is_high == true){
          current_high_counter++;
          if (current_high_counter == 2){
             emergency_string = "C";
          }
    }
    else if(current_comparison < CURRENT_THRESHOLD && current_is_high == true){
          red=0;
          green=255;
          blue=0;
          current_is_high=false;
          current_high_counter = 0;
          emergency_string ="A";
          Serial.println("-----------------Back to Normal Mode----------------");
          
    }

    curr_msg.current_mA = current_mA;
    curr_msg.loadvoltage = loadvoltage;
    String stringmA = String(current_mA, 3);
    all_currents.concat(stringmA);
    String comma = ",";
    all_currents.concat(comma);
    String stringLoad = String(loadvoltage, 2);
    all_loads.concat(stringLoad);
    all_loads.concat(comma);
    xQueueSend(curr_msg_queue, (void *)&curr_msg, 10);
  
    vTaskDelay(9000 / portTICK_PERIOD_MS);
  }
  
}


// Task: write Value in Message Queue to SD Card
void writeSD(void *parameters) {

  Message msg;

  // Loop forever
  while (1) {
    // See if there's a message in the queue (do not block)
    if (xQueueReceive(msg_queue, (void *)&msg, 0) == pdTRUE) {
      char first_temp[]= " Collected Temperature(°C): ";
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
     appendFile(SD, "/stuffy/temp.txt", msg.date_time);
     appendFile(SD, "/stuffy/temp.txt", first_temp);
     appendFile(SD, "/stuffy/temp.txt", " \n");
    }
  

 
  }
}

void writeCurrSD(void *parameters) {

  CurrMessage msg1;

  // Loop forever
  while (1) {
    // See if there's a message in the queue (do not block)
    if (xQueueReceive(curr_msg_queue, (void *)&msg1, 0) == pdTRUE) {
      char first_curr[]= " Current(mA): ";
      float val = msg1.current_mA; 
      char sz[20] = {' '} ;
      int val_int = (int) val;   
      float val_float = (abs(val) - abs(val_int)) * 100000;// compute the integer part of the float
      int val_fra = (int)val_float;
      sprintf (sz, "%d.%d", val_int, val_fra); 
      strcat(first_curr,sz);

      Serial.println(first_curr);
      Serial.println("Curr in Queue");

      appendFile(SD, "/stuffy/current.txt", first_curr);
      appendFile(SD, "/stuffy/current.txt", " \n");

      char first_load[]= " Load(V): ";
      float val1 = msg1.loadvoltage; 
      char sz1[20] = {' '} ;
      int val_int1 = (int) val1;   
      float val_float1 = (abs(val1) - abs(val_int1)) * 100000;// compute the integer part of the float
      int val_fra1 = (int)val_float1;
      sprintf (sz1, "%d.%d", val_int1, val_fra1); 
      strcat(first_load,sz1);
      
      
      Serial.println(first_load);
      Serial.println("Load Voltage in Queue");

      appendFile(SD, "/stuffy/current.txt", first_load);
      appendFile(SD, "/stuffy/current.txt", " \n");
//      Serial.println(msg.temp);
//      Serial.println(msg.date_time);
      //writeFile(SD, "/stuffy/temp.txt", msg.date_time);
//     appendFile(SD, "/stuffy/temp.txt", msg.date_time);
//     appendFile(SD, "/stuffy/temp.txt", first_temp);
//     appendFile(SD, "/stuffy/temp.txt", " \n");
    }
  

 
  }
}

//Task: Blink RGB
void blinkRGB(void *parameters){
  while(1){
    analogWrite(RED_PIN, red);
    analogWrite(GREEN_PIN, green);
    analogWrite(BLUE_PIN, blue);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  
}
//***
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  // Configure Serial
  Serial.begin(115200);

  // Communication setup
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Temp reading and writing to SD Card---");

  //SD Card setup
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  Serial.println("Stufffffffffffff");

  uint32_t currentFrequency;
    
  Serial.println("Hello!");

  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  Serial.println("Measuring voltage and current with INA219 ...");
  
  createDir(SD, "/stuffy");
  writeFile(SD, "/stuffy/temp.txt", "Temperature Values ");
  writeFile(SD, "/stuffy/current.txt", "Current Values ");
  appendFile(SD, "/stuffy/temp.txt", "World!\n");
 /*END SD CARD SETUP*/

  // Create queues
  
  msg_queue = xQueueCreate(msg_queue_len, sizeof(Message));
  curr_msg_queue = xQueueCreate(curr_msg_queue_len, sizeof(CurrMessage));

  // Timers
  //***Timer setup
  pinMode(INPUT_INTERRUPT_PIN, INPUT_PULLUP);
  pinMode(OUTPUT_INTERRUPT_PIN, OUTPUT);
  attachInterrupt(INPUT_INTERRUPT_PIN, isr, HIGH);

  timerHigh = xTimerCreate( "timer high", timerHigh_delay, pdFALSE, (void *)1, timerHighCallback);  
  timerLow = xTimerCreate( "timer low", timerLow_delay, pdTRUE,(void *)0,timerLowCallback);
  timerComm = xTimerCreate( "timer comm", timerComm_delay, pdTRUE,(void *)2,timerCommCallback); 
  timerCommBlue = xTimerCreate( "timer blue", timerCommBlue_delay, pdTRUE,(void *)3,timerCommBlueCallback); 

  xTimerStart(timerLow, portMAX_DELAY);
  xTimerStart(timerHigh, portMAX_DELAY); 
  xTimerStart(timerComm, portMAX_DELAY);
  xTimerStart(timerCommBlue, portMAX_DELAY);
  
  // Start Temp task
  xTaskCreatePinnedToCore(readTemp,
                          "Read temp",
                          3000,
                          NULL,
                          1,
                          NULL,
                          app_cpu);

    xTaskCreatePinnedToCore(readCurrent,
                          "Read Current",
                          4000,
                          NULL,
                          1,
                          NULL,
                          app_cpu);      

  // Start blink task
  xTaskCreatePinnedToCore(writeSD,
                          "Write SD",
                          6000,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
                          
  xTaskCreatePinnedToCore(writeCurrSD,
                          "Write Curr SD",
                          7000,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
                          
  xTaskCreatePinnedToCore(blinkRGB,
                          "Blink RGB",
                          6000,
                          NULL,
                          1,
                          NULL,
                          pro_cpu);                        

  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // Execution should never get here
}
