

#define RXp2 16
#define TXp2 17
#define COMM_INTERRUPT_PIN 20


//variables to keep track of the timing of recent interrupts
unsigned long button_time = 0;  
unsigned long last_button_time = 0;

bool comms_is_started = false;

String stuff = "This is a long day";
char some_other[] = "Some other thing";
int len = sizeof(some_other)/sizeof(some_other[0]);

void IRAM_ATTR start_comms(){
   button_time = millis();
    if (button_time - last_button_time > 250)
    {
       Serial.println("Starting Comms");
       comms_is_started = true;
       last_button_time = button_time;
    }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  pinMode(COMM_INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(COMM_INTERRUPT_PIN, start_comms, FALLING);
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);
}
void loop() {
   // Serial.println("Message Received: ");
   // Serial.println(Serial2.readString());
    if(comms_is_started){
      Serial2.println(stuff);
      Serial2.println(some_other);
      Serial.println("COMM_ENDED");
      Serial2.println("COMM_ENDED");
      comms_is_started = false;
    }
}
