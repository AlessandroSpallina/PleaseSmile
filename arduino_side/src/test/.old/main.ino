/* TEAM TRUCE */

const byte button = 2;
const byte checkLed = 13;
const byte trigger = 9;
const byte echo = 10;
volatile byte working = LOW;

/* quanto l'onboard led è acceso -> arduino funziona */
void toggle()
{
  working = !working;
  digitalWrite(checkLed, working);
  Serial.println(working ? "ON" : "OFF"); /* da passare all'STM32 */
}

void setup()
{
  Serial.begin(9600);
  pinMode(checkLed, OUTPUT);
  pinMode(trigger, OUTPUT);
  pinMode(button, INPUT);
  pinMode(echo, INPUT);
  attachInterrupt(digitalPinToInterrupt(button), toggle, FALLING);
}

void loop()
{
  if(working) {
    digitalWrite(trigger, LOW);
    digitalWrite(trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigger, LOW);

    long durata = pulseIn(echo, HIGH);
    long distanza = 0.034 * (durata/2);

    if(durata > 38000) /* fuori dalla portata del sensore: assumo 650 cm come massimo */
      Serial.println(650);
    else
      Serial.println(distanza);

    delay(500);
  }
}

