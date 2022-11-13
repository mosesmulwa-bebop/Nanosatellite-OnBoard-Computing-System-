int button = 13;
int led = 12;
int buzzer = 11;
int reverse = 10;
void setup()
{
  pinMode(13, INPUT);
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
}

void loop()
{
  
    if(digitalRead(button) == 1)
    {
      digitalWrite(led, HIGH);
      digitalWrite(buzzer, HIGH);
      digitalWrite(reverse, HIGH);
    }
  else{
       digitalWrite(led, LOW);
       digitalWrite(buzzer, LOW);
       digitalWrite(reverse, LOW);
  }
    
}
