// digital pin 2 is the hall pin
int hall_pin = 2;
// set number of hall trips for RPM reading (higher improves accuracy)
//100
float hall_thresh = 10.0;
float wheel_diameter = 0.6; // in meters
void setup() {
  // initialize serial communication at 9600 bits per second:
  //Serial.begin(115200);
  Serial.begin(9600);
  // make the hall pin an input:
  pinMode(hall_pin, INPUT);
}

// the loop routine runs over and over again forever:
void loop() {
  // preallocate values for tach
  float hall_count = 1.0;
  float start = micros();
  bool on_state = false;
  // counting number of times the hall sensor is tripped
  // but without double counting during the same trip
  while(true){
    if (digitalRead(hall_pin)==0){
      if (on_state==false){
        on_state = true;
        hall_count+=1.0;
      }
    } else{
      on_state = false;
    }
    
    if (hall_count>=hall_thresh){
      break;
    }
  }
  
  // print information about Time and RPM
  float end_time = micros();
  float time_passed = ((end_time-start)/1000000.0);
  Serial.print("Time Passed: ");
  Serial.print(time_passed);
  Serial.println("s");
  float rpm_val = (hall_count/time_passed)*60.0;
  Serial.print(rpm_val);
  Serial.println(" RPM");
  float speed_km_h = 0.1885 * rpm_val * wheel_diameter; 
  Serial.print(speed_km_h);
  Serial.println(" KM/H");
  delay(1);        // delay in between reads for stability
}
