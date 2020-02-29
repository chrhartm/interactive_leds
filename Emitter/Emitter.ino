//Radio Stuff
#include "nRF24L01.h"
Nrf24l Mirf = Nrf24l(10, 9);
byte value[4];

// Button stuff
const int N_PROGRAMS = 2;
const int PIN_TOGGLE1 = 2;
const int PIN_BUTTON1 = 3;
const int PIN_BUTTON2 = 4;
const int PIN_ROTATE1 = 0;
const int PIN_ROTATE2 = 1;
const int PIN_ROTATE3 = 2;
const int PIN_ROTATE4 = 3;

int toggle1 = 0;
int button1 = 0;
int button2 = 0;
int rotate1 = 0;
int rotate2 = 0;
int rotate3 = 0;
int rotate4 = 0;

int program = 0;
bool button1_on = 0;

void setup()
{
  Serial.begin(9600);
  // Radio stuff
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  Mirf.setTADDR((byte *)"slave3");
  Mirf.payload = sizeof(value);
  Mirf.channel = 90;
  Mirf.config();

  // Button stuff
  pinMode(PIN_TOGGLE1, INPUT);
  pinMode(PIN_BUTTON1, INPUT);
  pinMode(PIN_BUTTON2, INPUT);
}

void read_sensors(){
  toggle1 = digitalRead(PIN_TOGGLE1);
  button1 = digitalRead(PIN_BUTTON1);
  button2 = digitalRead(PIN_BUTTON2);
  rotate1 = 1023-analogRead(PIN_ROTATE1);
  rotate2 = 1023-analogRead(PIN_ROTATE2);
  rotate3 = 1023-analogRead(PIN_ROTATE3);
  rotate4 = 1023-analogRead(PIN_ROTATE4);
}

void calc(){
  // Calculate program ID
  if (button1==HIGH){
    if (button1_on==0){
      program = (program+1)%N_PROGRAMS;
      button1_on = 1;
    };
  }
  else{
    button1_on = 0;
  };

  // Normalize sensor readings
  value[0] = rotate1/4;
  value[1] = rotate2/4;
  value[2] = rotate3/16;
  value[3] = rotate4/4;
}

void logging(){
  Serial.println(toggle1==HIGH);
  Serial.println(button1==HIGH);
  Serial.println(button2==HIGH);
  Serial.println(rotate1);
  Serial.println(rotate2);
  Serial.println(rotate3);
  Serial.println(rotate4);
  Serial.println(button1_on);
  Serial.println(program);
  Serial.println("");
}

void send_values(){
  Mirf.setTADDR((byte *)"slave1");
  Mirf.config();
  Mirf.send(value);                
  while (Mirf.isSending()) delay(1);

  Mirf.setTADDR((byte *)"slave2");
  Mirf.config();
  Mirf.send(value);
  while (Mirf.isSending()) delay(1);

  Mirf.setTADDR((byte *)"slave3");
  Mirf.config();
  Mirf.send(value);
  while (Mirf.isSending()) delay(1); 
}

void loop()
{
  // Box stuff
  read_sensors();
  calc();
  //logging();
  if (toggle1==HIGH){
    send_values();
  };
}
