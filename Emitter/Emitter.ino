//Radio Stuff
#include "nRF24L01.h"
Nrf24l Mirf = Nrf24l(10, 9);
byte value[4];

// Box stuff
uint8_t hue, decay, delayval, brightness;

// Button stuff
const int pin_toggle1 = 2;
const int pin_button1 = 3;
const int pin_button2 = 4;
const int pin_rotate1 = 0;
const int pin_rotate2 = 1;
const int pin_rotate3 = 2;
const int pin_rotate4 = 3;
int toggle1 = 0;
int button1 = 0;
int button2 = 0;
int rotate1 = 0;
int rotate2 = 0;
int rotate3 = 0;
int rotate4 = 0;

void setup()
{
  Serial.begin(9600);
  // Radio stuff
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  Mirf.setTADDR((byte *)"slave2");
  Mirf.payload = sizeof(value);
  Mirf.channel = 90;
  Mirf.config();

  // Button stuff
  pinMode(pin_toggle1, INPUT);
}

void read(){
  toggle1 = digitalRead(pin_toggle1);
  button1 = digitalRead(pin_button1);
  button2 = digitalRead(pin_button2);
  rotate1 = 1023-analogRead(pin_rotate1);
  rotate2 = 1023-analogRead(pin_rotate2);
  rotate3 = 1023-analogRead(pin_rotate3);
  rotate4 = 1023-analogRead(pin_rotate4);
  //printsensors();
}

void printsensors(){
  Serial.println(toggle1==HIGH);
  Serial.println(button1==HIGH);
  Serial.println(button2==HIGH);
  Serial.println(rotate1);
  Serial.println(rotate2);
  Serial.println(rotate3);
  Serial.println(rotate4);
}

void loop()
{
  // Box stuff
  read();
  hue = rotate1/4;
  decay = rotate2/4;
  delayval = rotate3/16;
  brightness = rotate4/4;

  // Radio stuff
  value[0] = hue;
  value[1] = decay;
  value[2] = delayval;
  value[3] = brightness;  
  
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

  delay(1000);
}
