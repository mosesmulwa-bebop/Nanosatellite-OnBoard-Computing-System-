

void setup()
{
     Serial.begin(9600);
    
}
void loop()
{
     int adc  = analogRead(0) ;    //reading analog voltage and storing it in an integer 
     Serial.print(adc);

     }
     
 
