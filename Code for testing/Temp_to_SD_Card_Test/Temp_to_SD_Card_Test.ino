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

// libraries for the SD Card
#include "FS.h"
#include "SD.h"
#include "SPI.h"

/**********************Settings**/
// Settings
static const int msg_queue_len = 5;     // Size of msg_queue




// Message struct
typedef struct Message {
  char date_time[20];
  float temp;
} Message;

// Globals
static QueueHandle_t msg_queue;


/*************************************************************************************/
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

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  // Configure Serial
  Serial.begin(115200);

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
  createDir(SD, "/stuffy");
  writeFile(SD, "/stuffy/temp.txt", "Temperature Values ");
  appendFile(SD, "/stuffy/temp.txt", "World!\n");
 /***END SD CARD SETUP***/

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
                          6000,
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
