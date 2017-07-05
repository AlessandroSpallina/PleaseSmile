byte counter;
byte PWM_Input=128, low, high;
int dataIn;
void setup(){
pinMode(3, OUTPUT);
Serial.begin(115200);
}


void loop(){

analogWrite(3, PWM_Input);

Serial.print("$I'm counting ");
Serial.print(counter);
Serial.print('#');
Serial.print(char(low));
Serial.print(char(high));
counter++;
while(1){
  delay(1);
  Serial.print(2);
if(Serial.read()=='$'){
  delay(1);
  low = Serial.read();
  delay(1);
  high = Serial.read();
  break;}}

  dataIn = word(high, low);

 if(dataIn>680)
 dataIn=680;
 if(dataIn<400)
 dataIn=400;
 PWM_Input =  map(dataIn, 400, 680, 0, 255);




}
