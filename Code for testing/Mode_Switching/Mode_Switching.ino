/**
 *  Mode switching test
 *  Each mode has an associated set of tasks
 * 
 * Switch modes based on serial entry
 * Date: October 20, 2022
 * Author: Moses Mulwa
 * License: 0BSD
 */

// You'll likely need this on vanilla FreeRTOS
//#include semphr.h

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif


//Global
static SemaphoreHandle_t bin_sem = NULL;

// Task handles
static TaskHandle_t task_handle_mode1 = NULL;
static TaskHandle_t task_handle_mode2 = NULL;

bool task_mode1_is_suspended = NULL;
bool task_mode2_is_suspended = NULL;

char current_mode = 'n';
char next_mode = 's';

//*****************************************************************************
// Tasks

// Echo things back to serial port, turn on LED when while entering input
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
              if(task_handle_mode1 == NULL){
                xTaskCreatePinnedToCore(task_for_mode1,
                                "Task for mode1",
                                1024,
                                NULL,
                                1,
                                &task_handle_mode1,
                                app_cpu);
              }
              if(task_mode1_is_suspended == true){
                vTaskResume(task_handle_mode1);
                task_mode1_is_suspended = false;
              }
              if(task_handle_mode2 != NULL){
                vTaskSuspend(task_handle_mode2);
                task_mode2_is_suspended = true; 
              }
              current_mode = 'n';
              next_mode = NULL;
     }else if(next_mode == 'e' && current_mode != 'e'){
              Serial.println("Switching to Emergency Mode");
              if(task_handle_mode2 == NULL){
                xTaskCreatePinnedToCore(task_for_mode2,
                                "Task for mode2",
                                1024,
                                NULL,
                                1,
                                &task_handle_mode2,
                                app_cpu);
              }
              if(task_mode2_is_suspended == true){
                vTaskResume(task_handle_mode2);
                task_mode2_is_suspended = false;
              }
              if(task_handle_mode1 != NULL){
                vTaskSuspend(task_handle_mode1);
                task_mode1_is_suspended = true; 
              }
              current_mode = 'e';
              next_mode = NULL;
     }
  }
  
}

void task_for_mode1(void *parameters){
   while(1){
    Serial.print("Current mode: ");
    Serial.println(current_mode);
    Serial.println("Mode 1HIgh");
    
    vTaskDelay(2000 / portTICK_PERIOD_MS); 
    Serial.println("Mode 1Low");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}
void task_for_mode2(void *parameters){
  while(1){
    Serial.print("Current mode: ");
    Serial.println(current_mode);
    Serial.println("Mode 2 High");
    vTaskDelay(2000 / portTICK_PERIOD_MS); 
    Serial.println("Mode 2 Low");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
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

  
 // Create semaphore before it is used (in task or ISR)
  bin_sem = xSemaphoreCreateBinary();

 // Start command line interface (CLI) task
  xTaskCreatePinnedToCore(doCLI,
                          "Do CLI",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);

   xTaskCreatePinnedToCore(modeSwitcher,
                          "Mode Switcher",
                          1024,
                          NULL,
                          2,
                          NULL,
                          app_cpu);

  xTaskCreatePinnedToCore(task_for_mode1,
                          "Task for mode1",
                          1024,
                          NULL,
                          1,
                          &task_handle_mode1,
                          app_cpu);

//  xTaskCreatePinnedToCore(task_for_mode2,
//                          "Task for mode2",
//                          1024,
//                          NULL,
//                          1,
//                          &task_handle_mode2,
//                          app_cpu);

  
//
//   vTaskSuspend(task_handle_mode2);
//   task_mode2_is_suspended = true; 

  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // Execution should never get here
}
