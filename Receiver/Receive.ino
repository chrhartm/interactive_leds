//Radio Stuff
#include <SPI.h>
#include "nRF24L01.h"

// LED Stuff
#include <FastLED.h>

// Radio stuff
Nrf24l Mirf = Nrf24l(10, 9);
uint8_t value[4];
char address[] = "slave2";

// LED Stuff
#define NUM_LEDS 60 
#define DATA_PIN 6
#define CLOCK_PIN 13
CRGB leds[NUM_LEDS];
uint8_t hue, decay, delayval, brightness;

void setup()
{
  Serial.begin(9600);

  // Radio stuff
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  Mirf.setRADDR((byte *)address); //Set your own address (receiver address) using 5 characters
  Mirf.payload = sizeof(value);
  Mirf.channel = 90;             //Set the used channel
  Mirf.config();
  Serial.println("Listening...");  //Start listening to received data

  // LED stuff
  LEDS.addLeds<WS2812,DATA_PIN,RGB>(leds,NUM_LEDS);
  LEDS.setBrightness(84);
  hue = 0;
  decay = 250;
  delayval = 60;
  brightness = 255;
}

// LED stuff
void fadeall(uint8_t decay) { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(decay); } }

void loop()
{
  // Radio Stuff
  while (Mirf.dataReady()) { //When the program is received, the received data is output from the serial port
    Mirf.getData(value);
  }

  // LED stuff
  hue = value[0];
  decay = value[1];
  delayval = value[2];
  brightness = value[3];
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, 255, brightness);
    FastLED.show(); 
    fadeall(decay);
    delay(delayval);
  }
}
