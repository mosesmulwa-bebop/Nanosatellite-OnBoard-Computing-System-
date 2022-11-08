#define BUTTON_PIN 2
#define INPUT_INTERRUPT_PIN 3
#define OUTPUT_INTERRUPT_PIN 4

//variables to keep track of the timing of recent interrupts
unsigned long button_time = 0;  
unsigned long last_button_time = 0; 
unsigned long button_time2 = 0;  
unsigned long last_button_time2 = 0; 
int my_delay = 5000;

void(* resetFunc)(void) = 0;

void setup(){
    Serial.begin(115200);
    Serial.println("Program Start");
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(INPUT_INTERRUPT_PIN, INPUT_PULLUP);
    pinMode(OUTPUT_INTERRUPT_PIN, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), isr, FALLING);
    attachInterrupt(digitalPinToInterrupt(INPUT_INTERRUPT_PIN), my_reset, FALLING);
}

void loop(){
   digitalWrite(OUTPUT_INTERRUPT_PIN, HIGH);
   Serial.print("HIGH ");
   delay(100);
   digitalWrite(OUTPUT_INTERRUPT_PIN, LOW);
   Serial.println("LOW");
   delay(my_delay);
}

void isr(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        my_delay = 30000;
        Serial.println("Simulating Hanging");
        last_button_time = button_time;
    }
}

void my_reset(){
      button_time2 = millis();
    if (button_time2 - last_button_time2 > 250){
        
        Serial.println("Resset received");
        resetFunc();
        last_button_time2 = button_time2;
    }
        
}
