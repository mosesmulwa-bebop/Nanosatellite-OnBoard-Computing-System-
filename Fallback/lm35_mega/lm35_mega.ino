// Brave Learn ///
/// https://bravelearn.com ///
// Connect LM35 to Arduino as shown in the diagram
float reading; //declare variable in which temperature data is stored
void setup() {
  pinMode(A0, INPUT); // Tell Arduino to make its Analog A0 pin as Input reading pin
  // start the serial connection at 9600 bps:
   Serial.begin(9600);
}
void loop() { //Loops over
    //
    reading = (5.0 * analogRead(A0) * 100.0) / 1024; // Converts the analog voltage from sensor to digital reading where 5 is the supply voltage i.e. 5V
    // prints the data onto serial monitor
    Serial.print("Temperature is: "); //println prints next thing on a new line
    Serial.print((float)reading); // Prints current temperature on Monitor
    Serial.println(" *C");
    Serial.println("Visit: https://bravelearn.com ");
    Serial.println(" "); //Break space // Start reading on new line
    delay(10000); // 10 seconds delay before taking the new reading
}
