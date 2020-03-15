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

// Global states
bool button2_on=0;
int program=0;
static int N_PROGRAMS=4;
bool logging=0;

// Program states
int ix=0;
bool state=0;
int state2=0;
const int n_states = 10;
int states[n_states];
bool bool_states[n_states];

void setup()
{
  Serial.begin(9600);
  reset_states();

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

void logger(){
  Serial.print("State: ");
  Serial.println(state);
  Serial.print("button2_on: ");
  Serial.println(button2_on);
  Serial.print("value[4]: ");
  Serial.println(value[4]/4);
}

// LED stuff
void fadeall(uint8_t decay) { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(decay); } }

void reset_states(){
  ix = 0;
  state = 0;
  state2 = 1;
  for(int i=0; i<n_states; i=i+2){
    states[i] = random(NUM_LEDS-1);
    states[i+1] = NUM_LEDS-1-states[i];
    bool_states[i] = random(1);
    bool_states[i+1] = (1+bool_states[i])%2;
  }
}

void step_0(){
    // Calculate Up or Down
  if (value[1]==1){
    if (button2_on==0){
      if (state==0){
        state = 1;
      } else {
        state = 0;
      };
      button2_on = 1;
    };
  }
  else{
    button2_on = 0;
  };
  
  // Update index
  if(state==0){
    ix = (ix+1)%NUM_LEDS;
  } else {
    ix = (ix-1)%NUM_LEDS;
    // BUG something funny happening to state after this step
    if(ix<=0){
      ix = NUM_LEDS;
    };
  };

  // Set LEDs
  fadeall(value[4]);
  leds[ix] = CHSV(value[2], 255, 255);
  leds[(ix+max(0,((value[5]-13)/4)))%NUM_LEDS] = CHSV((value[5]+value[2])%255, 255, 255);

  delay(value[3]/4);
};

void step_1(){
  if(value[1]==1){
    ix = 255;
    state2 = value[5];
  } else{
    if(state==0){
      ix = ix + (255-value[3])/(value[4]/2 + 1);
      if(ix>=255){
        ix = 255;
        state = 1;
      };
    } else {
      ix = ix - (255-value[3])/(value[4]/2 + 1);
      if(ix<=value[3]){
        ix = value[3];
        state = 0;
      };
    };
    state2 = value[2];   
  };  
  for (int i=0; i<NUM_LEDS; i++){
    leds[i] = CHSV(state2, 255, ix);
  }
};

void step_2(){
  if (state2%2==1){
    state2 = (state2+1)%n_states;
  };
  if (value[1]==1){
    if (button2_on==0){
      if (state==0){
        state2 = (state2+2)%n_states;
        if (state2 > 2){
          bool_states[state2-1] = (1+bool_states[state2-2])%2;
          states[state2-1] = (NUM_LEDS - states[state2-2])-1;
        };
        button2_on = 1;
      };
    };
  } else{
    button2_on = 0;
  };

  for(int i=0; i<state2; i++){
    if(bool_states[i]==0){
      states[i] = states[i]+1;
      if((states[i] >= NUM_LEDS)){
        states[i] = NUM_LEDS-1;
        bool_states[i] = 1;
      };
    } else {
      states[i] = states[i]-1;
      if(states[i]<=0){
        states[i] = 0;
        bool_states[i] = 0;
      };
    };    
    leds[states[i]] = CHSV((value[2]+(255/n_states)*(i-(i%2)))%255, 255, 255);
  }  
  fadeall(value[4]);

  for(int i=0; i<state2-1; i++){
    for(int j=(i+1); j<state2; j++){
      if(abs(states[i]-states[j])<2){
        bool_states[i] = !bool_states[i];
        bool_states[j] = !bool_states[j];
      }
    }
  }

  delay(value[3]/4);  
}

void clean_data(){
  if(value[0]>=N_PROGRAMS){
    value[0]=program;
  };
}

static inline int8_t sgn(int val) {
 if (val < 0) return -1;
 if (val==0) return 0;
 return 1;
}

void loop() {
  // Radio Stuff
  while (Mirf.dataReady()) { //When the program is received, the received data is output from the serial port
    Mirf.getData(value);
  }

  if(logging){
    logger();
  }

  clean_data();

  if(value[0]!=program){
    reset_states();
    program=value[0];
  }
  
  // LED stuff
  switch(program){
    case 0:
      step_0();
      break;
    case 1:
      step_1();
      break;
    case 2:
      step_2();
      break;
    case 3:
      step_0();
      break;
  }
  FastLED.show(); 
}
