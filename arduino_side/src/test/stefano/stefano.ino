byte counter;
byte PWM_Input=128, low, high;
int dataIn;
void setup(){
pinMode(3, OUTPUT);
Serial.begin(19200);

Serial.println("$I'm counting ");
}


void loop(){
Serial.println(5);


if(Serial.available()){
  if(low = Serial.read() != 0) {
    Serial.println(low);
    Serial.println("QUI\n");
    delay(1000);

  }
}

}
