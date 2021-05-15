// Potentiometer is connected to GPIO 32 (Analog ADC1_CH6) 
const int potPin = 32;
uint32_t sum, count;
String entry;
// variable for storing the potentiometer value
int potValue = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
}

void loop() {
  // Reading potentiometer value
  potValue = analogRead(potPin);
  sum+=potValue;
  count++;
  Serial.print("potValue:  ");Serial.print(potValue); Serial.print("\tProm:  ");Serial.println(sum/count);
  entry=Serial.read();
  if (entry=="s"){
    count=0; sum=0;
  }
  delay(1000);
}
