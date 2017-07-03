/* TEAM TRUCE */

#include <NewPing.h>
#include <FileIO.h>
#include <EEPROM.h>
#include <string.h>
#include <stdlib.h>

#define TRIGGER 9
#define ECHO 10
#define MAX_DISTANCE 600
#define BUTTON 2
#define CHECKLED 13
#define LOG_THRESHOLD 80
#define LOG_PATH "/mnt/sd/arduino.log"

volatile byte working = LOW;
NewPing sonar(TRIGGER, ECHO, MAX_DISTANCE);

String getTimeStamp() {
  String result;
  Process time;
  time.begin("date");
  time.addParameter("+%D-%T");  
  time.run();

  while(time.available()>0) {
    char c = time.read();
    if(c != '\n')
      result += c;
  }

  return result;
}

void clearEeprom()
{
  for(int i=0; i<EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
}

void setDateToEeprom(String date)
{
  char buf[18];

  date.toCharArray(buf, sizeof(buf));
  
  for(int i=0; i<EEPROM.length(), i<sizeof(buf); i++) {
    EEPROM.write(i, buf[i]);  
  }
}

void getDateFromEeprom(char buf[18])
{
  for(int i=0; i<EEPROM.length(), i<sizeof(buf); i++) {
    buf[i] = EEPROM.read(i);
  }
}

boolean isEepromClean()
{
  if(EEPROM.read(0) == '0')
    return true;
  else 
    return false;
}

boolean isOneMinuteSinceLastLog()
{
  char buf[18], actual[18];

  if(!isEepromClean()) {
    getDateFromEeprom(buf);
    String aus = getTimeStamp();
    aus.toCharArray(actual, sizeof(aus));

    //actual
    char mdy[9]; //mounth,day,year 
    char hms[9]; //hours,mins,secs

    //old
    char omdy[9];
    char ohms[9];

    for(int i=0,j=0; i<=17; i++) {
      if(i<7) {
        mdy[i] = actual[i];
        omdy[i] = buf[i];
      } else if(i==8) {
        mdy[i] = '\0';
        omdy[i] = '\0';
      } else if(i>8 && i!=17) {
        hms[j++] = actual[i];
        ohms[j++] = buf[i];
      } else if(i==17) {
        hms[j] = '\0';
        ohms[j] = '\0';
      }
    }

    if(strcmp(mdy, omdy) == 0) {
      if((hms[0] == ohms[0]) && (hms[1] == ohms[1])) {  //nuova e vecchia data uguale
        char h1[3];
        char h2[3];

        h1[0] = hms[3];
        h1[1] = hms[4];
        h1[2] = '\0';
        h2[0] = ohms[3];
        h2[1] = ohms[4];
        h2[2] = '\0';

        if(abs(atoi(h1) - atoi(h2)) >= 1) {
          return true;
        } else {
          return false;
        }
        
      } else {
        return true;
      }
    } else {
      return true;
    }  
  } else {
    return true;
  }
}

/* quando l'onboard led è acceso -> arduino funziona */
void toggle()
{
  working = !working;
  digitalWrite(CHECKLED, working);
  Serial.println(working ? "ON" : "OFF"); /* da passare all'STM32 */
}

void setup()
{
  Bridge.begin();
  FileSystem.begin();
  Serial.begin(115200);
  pinMode(CHECKLED, OUTPUT);
  pinMode(BUTTON, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON), toggle, FALLING);
  clearEeprom();
  File log_file = FileSystem.open(LOG_PATH, FILE_APPEND);
  if(!log_file)
    Serial.println("C'è qualche problema con la scheda SD :S");
  else
    Serial.println("BOOTED CORRECTLY BRO :)");
}

void loop()
{
  if(working) {
    int buf = sonar.ping_cm();
    Serial.println(buf);
    if(buf <= LOG_THRESHOLD) {
      String log_row = getTimeStamp() + " -> rilevato ostacolo a " + String(buf) + " cm";
      if(isOneMinuteSinceLastLog()) { //logga anche se non è passato un minuto, non dare scampo al bug, trovalo e fottilo!
        File log_file = FileSystem.open(LOG_PATH, FILE_APPEND); //non controllo se è andato a buon fine, dovrei avere qualcosa per lo stderr
        log_file.println(log_row);
        log_file.close();
        setDateToEeprom(getTimeStamp());
        
      }
    }
    delay(200);
  }
}

