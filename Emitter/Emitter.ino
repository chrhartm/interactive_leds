//Radio Stuff
#include "nRF24L01.h"
Nrf24l Mirf = Nrf24l(10, 9);
byte value[6];

const int N_PROGRAMS = 4;
// Button stuff
const int PIN_TOGGLE1 = 2;
const int PIN_BUTTON1 = 3;
const int PIN_BUTTON2 = 4;
const int PIN_ROTATE1 = 3;
const int PIN_ROTATE2 = 2;
const int PIN_ROTATE3 = 1;
const int PIN_ROTATE4 = 0;

// faster if slower
const int change_speed = 2000;
// faster is faster
const int change_const = 1;

int toggle1 = 0;
int button1 = 0;
int button2 = 0;
int rotate1 = 0;
int rotate2 = 0;
int rotate3 = 0;
int rotate4 = 0;

int program = 0;
bool button1_on = 0;
int sign[4];

void setup()
{
  Serial.begin(9600);
  // Radio stuff
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  Mirf.setTADDR((byte *)"slave1");
  Mirf.payload = sizeof(value);
  Mirf.channel = 90;
  Mirf.config();

  // Button stuff
  pinMode(PIN_TOGGLE1, INPUT);
  pinMode(PIN_BUTTON1, INPUT);
  pinMode(PIN_BUTTON2, INPUT);

  sign[0] = change_const;
  sign[1] = change_const;
  sign[2] = change_const;
  sign[3] = change_const;
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

  value[0] = program;
  value[1] = button2;

  if (toggle1==HIGH){
    // Normalize sensor readings
    value[2] = rotate1/4;
    value[3] = rotate2/4;
    value[4] = rotate3/4;
    value[5] = rotate4/4;
  } else {
    // Change faster if rotate higher
    if ((random(change_speed) - rotate1) < 0){
      value[2] = (value[2] + sign[0])%255;
      if (value[2] + sign[0] > 254 || value[2] + sign[0] <1){
        sign[0] = sign[0] * (-1);
      };
    };
    if ((random(change_speed) - rotate2) < 0){
      value[3] = (value[3] + sign[1])%255;
      if (value[3] + sign[1] > 254 || value[3] + sign[1] <1){
        sign[1] = sign[1] * (-1);
      };
    };
    if ((random(change_speed) - rotate3) < 0){
      value[4] = (value[4] + sign[2])%255;
      if (value[4] + sign[2] > 254 || value[4] + sign[2] <1){
        sign[2] = sign[2] * (-1);
      };
    };
    if ((random(change_speed) - rotate4) < 0){
      value[5] = (value[5] + sign[3])%255;
      if (value[5] + sign[3] > 254 || value[5] + sign[3] <1){
        sign[3] = sign[3] * (-1);
      };
    };
  };
}

void logging(){
  Serial.print("Toggle 1: ");
  Serial.println(toggle1==HIGH);
  Serial.print("Button 1: ");
  Serial.println(button1==HIGH);
  Serial.print("Button 2: ");
  Serial.println(button2==HIGH);
  Serial.print("Rotate 1: ");
  Serial.println(rotate1);
  Serial.print("Rotate 2: ");
  Serial.println(rotate2);
  Serial.print("Rotate 3: ");
  Serial.println(rotate3);
  Serial.print("Rotate 4: ");
  Serial.println(rotate4);
  Serial.print("Program: ");
  Serial.println(program);
  Serial.print("Value 2: ");
  Serial.println(value[2]);
  Serial.print("Value 3: ");
  Serial.println(value[3]);
  Serial.print("Value 4: ");
  Serial.println(value[4]);
  Serial.print("Value 5: ");
  Serial.println(value[5]);
  Serial.print("Sign 0: ");
  Serial.println(sign[0]);
  Serial.println(" ");
}

void send_values(){
  Mirf.send(value);
  while (Mirf.isSending()) delay(1);
}

void loop()
{
  read_sensors();
  calc();
  //logging();
  send_values();
}
