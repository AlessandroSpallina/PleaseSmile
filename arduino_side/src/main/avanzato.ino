#include <SoftwareSerial.h>

int tx = 11;
int rx = 10;
int state_read = 0;

SoftwareSerial ss(rx, tx);

void setup() {
  Serial.begin(9600);
  ss.begin(9600);
}

void loop() {
  if(ss.available()){
    state_read=ss.read();
    if(state_read == 0) {
      Serial.print("SORPRESA: ");
      Serial.println(state_read);
    } else {
      Serial.print("DURATA: ");
      Serial.println(state_read);  
    }
    
    }
}
