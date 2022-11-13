

#define RXp2 16
#define TXp2 17



//variables to keep track of the timing of recent interrupts
unsigned long button_time = 0;  
unsigned long last_button_time = 0;

bool comms_is_started = false;
float my_ints[] = {50.0,40.7,50.6,56.4,567.3,546.2,78.0};
int my_len = sizeof(my_ints)/sizeof(my_ints[0]);
char my_temps[]="TEMP: ";

String all_temps ="ALL TEMPS: ";
String stuff = "This is a long day";
char some_other[] = "Some other thing";
int len = sizeof(some_other)/sizeof(some_other[0]);
bool done_func = false;

//float val = 56.8;
//char sz[20] = {' '} ;
//int val_int = (int) val;   
//float val_float = (abs(val) - abs(val_int)) * 100000;// compute the integer part of the float
//int val_fra = (int)val_float;
//sprintf(sz, " %d.%d ", val_int, val_fra); 
//strcat(my_temps,sz);

 



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);
}
void loop() {
   // Serial.println("Message Received: ");
   // Serial.println(Serial2.readString());

      if(done_func == false){
        for(int i=0; i < my_len; i++){
          String stringOne = String(my_ints[i], 3);
          all_temps.concat(stringOne);
          String comma = ",";
          all_temps.concat(comma);
        }
        done_func = true;
      }
   
      Serial2.println(stuff);
      Serial2.println(some_other);
      Serial.println(my_temps);
      Serial2.println(my_temps);
      
      Serial.println(all_temps);
      Serial2.println(all_temps);
      Serial.println("COMM_ENDED");
      Serial2.println("COMM_ENDED");
     delay(2000);
    
}
