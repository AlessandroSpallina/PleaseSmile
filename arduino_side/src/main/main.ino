#include <NewPing.h>
#include <FileIO.h>
#include <EEPROM.h>
#include <string.h>
#include <stdlib.h>
#include "ThingSpeak.h"
#include "ThingSpeak.h"
#include "YunClient.h"

#define TRIGGER 9
#define ECHO 10
#define MAX_DISTANCE 600
#define BUTTON 2
#define CHECKLED 13
#define LOG_THRESHOLD 80
#define LOG_PATH "/www/truce.log"

volatile byte working = LOW;
NewPing sonar(TRIGGER, ECHO, MAX_DISTANCE);
YunClient client;
unsigned long myChannelNumber = 297969;
const char * myWriteAPIKey = "HQL3LQJQI41CZEES";

String getTimeStamp() {
  String result;
  Process time;
  time.begin("date");
  time.addParameter("+%D-%H:%M");  
  time.run();

  while(time.available()>0) {
    char c = time.read();
    if(c != '\n')
      result += c;
  }

  return result;
}

/*void clearEeprom()
{
  for(int i=0; i<EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
}

boolean isEepromClean()
{
  if(EEPROM.read(0) == '0')
    return true;
  else 
    return false;
}*/


void toggle()
{
  working = !working;
  digitalWrite(CHECKLED, working);
  Serial.println(working ? "ON" : "OFF"); /* da passare all'STM32 */
}

void setup() {
  Bridge.begin();
  ThingSpeak.begin(client);

  FileSystem.begin();
  Serial.begin(115200);
  
  Serial.println("#BOOT: serial + filesystem + bridge ok");
  
  pinMode(CHECKLED, OUTPUT);
  pinMode(BUTTON, INPUT);
  //pinMode(1, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON), toggle, FALLING);

  Serial.println("#BOOT: attached interrupt ok");
  
  //Serial.println("EEPROM Written magic code");
    
  //Serial.println("#BOOT: eeprom ok");

  File log_file = FileSystem.open(LOG_PATH, FILE_APPEND);
  if(!log_file)
    Serial.println("C'è qualche problema con la scheda SD :S");
  else
    Serial.println("BOOTED CORRECTLY BRO :)");

  if(EEPROM.read(0) != 5) {
    Serial.println("NOTICE: EEPROM doen't contains magic number.");
    while(1);
  }

}

void loop() {
  if(working) {
    int buf = sonar.ping_cm();
    Serial.println(buf);
    
    if(buf <= LOG_THRESHOLD) {
        String log_row = getTimeStamp() + " -> rilevato ostacolo a " + String(buf) + " cm";
        File log_file = FileSystem.open(LOG_PATH, FILE_APPEND);
        log_file.println(log_row);      
        log_file.close();
        ThingSpeak.writeField(myChannelNumber, 1, buf, myWriteAPIKey);
        delay(20000);
    }

    
  }
}

