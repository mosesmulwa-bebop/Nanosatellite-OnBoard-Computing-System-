#define RED_PIN 35
#define GREEN_PIN 32
#define BLUE_PIN 33

void setup() {}
 
void loop() {
  analogWrite(RED_PIN, random(0,255));
  analogWrite(GREEN_PIN, random(0,255));
  analogWrite(BLUE_PIN, random(0,255));
  delay(500);
}
