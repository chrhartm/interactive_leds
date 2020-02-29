//Radio Stuff
#include <SPI.h>
#include "nRF24L01.h"

// LED Stuff
#include <FastLED.h>

// Radio stuff
Nrf24l Mirf = Nrf24l(10, 9);
uint8_t value[6];
char address[] = "slave1";

// LED Stuff
#define NUM_LEDS 60 
#define DATA_PIN 6
#define CLOCK_PIN 13
CRGB leds[NUM_LEDS];
int ix=0;
bool state=0;
int state2=0;

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

  // LED stuff
  LEDS.addLeds<WS2812,DATA_PIN,RGB>(leds,NUM_LEDS);
  LEDS.setBrightness(84);
  value[0] = 0;
  value[1] = 0;
  value[2] = 253;
  value[3] = 253;
  value[4] = 253;
  value[5] = 0;
}

// LED stuff
void fadeall(uint8_t decay) { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(decay); } }

void step_0(){
  if(value[1]==0){
    ix = (ix+1)%NUM_LEDS;
  } else {
    ix = (ix-1)%NUM_LEDS;
    if(ix==0){
      ix = NUM_LEDS;
    };
  };
  
  leds[ix] = CHSV(value[2], 255, 255);
  leds[(ix+max(0,((value[5]-13)/4)))%NUM_LEDS] = CHSV((value[5]+value[2])%255, 255, 255);
  fadeall(value[4]);
  
  delay(value[3]/4);
};

void step_1(){
  if(value[1]==1){
    ix = 255;
    state2 = value[5];
  } else{
    if(state==0){
      ix = ix + 1;
      if(ix>=255){
        ix = 255;
        state = 1;
      };
    } else {
      ix = ix - 1;
      if(ix<=value[3]){
        ix = value[3];
        state = 0;
      };
    };
  };
  state2 = state2 + sgn(value[2] - state2);
  for (int i=0; i<NUM_LEDS; i++){
    leds[i] = CHSV(state2, 255, ix);
  }
  delay(value[4]/4);
};

static inline int8_t sgn(int val) {
 if (val < 0) return -1;
 if (val==0) return 0;
 return 1;
}

void loop()
{
  // Radio Stuff
  while (Mirf.dataReady()) { //When the program is received, the received data is output from the serial port
    Mirf.getData(value);
  }
  // LED stuff
  switch(value[0]){
    case 0:
      step_0();
      break;
    case 1:
      step_1();
      break;
  }
  FastLED.show(); 
}
