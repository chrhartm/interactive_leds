//Radio Stuff
#include <SPI.h>
#include "nRF24L01.h"

// LED Stuff
#include <FastLED.h>

// Radio stuff
Nrf24l Mirf = Nrf24l(10, 9);
uint8_t value[6];
uint8_t value_raw[7];
char address[] = "slave1";

// LED Stuff
#define NUM_LEDS 120
#define DATA_PIN 6
#define CLOCK_PIN 13
CRGB leds[NUM_LEDS];

// Global states
bool button2_on=0;
int program=0;
static int N_PROGRAMS=6;
bool logging=0;

// Program states
int ix=0;
bool state=0;
bool button_state=0;
int state2=0;
const byte divisors[] = {1, 2, 3, 4, 5, 6, 10}; // based on NUM_LEDS
const byte n_divisors = 7;
const int n_states = 10;
int states[n_states];
int states2[n_states];
bool bool_states[n_states];
const int change_const = 2;

void setup()
{
  Serial.begin(9600);
  reset_states();

  // Radio stuff
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  Mirf.setRADDR((byte *)address); //Set your own address (receiver address) using 5 characters
  Mirf.payload = sizeof(value_raw);
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
  value[6] = 0;
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
  button_state=0;
  for(int i=0; i<n_states; i=i+2){
    states[i] = random(NUM_LEDS-1);
    states[i+1] = NUM_LEDS-1-states[i];
    bool_states[i] = random(1);
    bool_states[i+1] = (1+bool_states[i])%2;
  }
}

void calc_button2_on(){
  if (value[1]==1){
    if (button2_on==0){
      if (button_state==0){
        button_state = 1;
      } else {
        button_state = 0;
      };
      button2_on = 1;
    };
  }
  else{
    button2_on = 0;
  };
}

void step_0(){
  // Calculate Up or Down
  calc_button2_on();
  state = button_state;
  
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
      ix = ix + (255-value[4])/(value[3]/2 + 1);
      if(ix>=255){
        ix = 255;
        state = 1;
      };
    } else {
      ix = ix - (255-value[4])/(value[3]/2 + 1);
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
    leds[states[i]] = CHSV((value[2]+(value[5]/n_states)*(i-(i%2)))%255, 255, 255);
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

void step_3(){
  calc_button2_on();
  if (!button_state){
    fadeall(100);
  };
  int dist = divisors[int(float(value[4])/254.0*(n_divisors-1))];
  for (int i=0; i<NUM_LEDS; i = i+(dist*2)){
    for (int j=0; j<(dist*2) && ((i+j)<NUM_LEDS); j++){
      state2 = (i+j+ix);
      if (state2>=NUM_LEDS){state2=state2%NUM_LEDS;};
      if (state2<0){state2=state2+NUM_LEDS;};
      if (j<dist){
        leds[state2] = CHSV(value[2], 255, 255);
      }
      if (j>=dist && button_state){
        leds[state2] = CHSV(value[5], 255, 255);
      };
    }
  }
  if (abs(ix)>=(dist*2)){
    ix = 0;
  };
  int tmp = (value[3]-128)/2;
  if (tmp>=0){
    ix = ix+1;
    delay(-(tmp-64));
  }
  else{
    ix = ix-1;
    delay(tmp+64);
  };
}

void step_4(){
  calc_button2_on();
  if (!button_state){
    fadeall(100);
  };
  for (int i=0; i<NUM_LEDS/2; i++){
    // Starting from center in both directions
    if (i < ix){
      leds[NUM_LEDS/2 + i] = CHSV(value[2], 255, 255);
      leds[NUM_LEDS/2 - i] = CHSV(value[2], 255, 255);
    } else{
      if (button_state){
        leds[NUM_LEDS/2 + i] = CHSV(value[5], 255, 150);
        leds[NUM_LEDS/2 - i] = CHSV(value[5], 255, 150);
      };
    };
  }
  if (ix>int(float(value[4])/255.0 * float(NUM_LEDS/2))){
    state = 0;
  }
  if (ix == 0){
    state = 1;
  };
  if (state){
    ix = ix+1;
  }
  else{
    ix = ix-1;
  };
  delay(value[3]/4);
}

void step_5(){
  if (value[1]==1){
    if (button2_on==0){
      if (state==0){
        state2 = (state2+1)%(n_states-1);
        bool_states[state2] = random(1);
        states[state2] = random(NUM_LEDS-1);
        states2[state2] = random(255-1);
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
    if(value[5]>random(1000)){bool_states[i] = !bool_states[i];}
    states2[i] = (states2[i]+value[2]/30)%255;
    leds[states[i]] = CHSV(states2[i], 255, 255);
  }
  fadeall(value[4]);

  delay(value[3]/4);
}

void clean_data(){
  if (value_raw[6] != (value_raw[0]+value_raw[1]+value_raw[2]+value_raw[3]+value_raw[4]+value_raw[5])%255){
    return;
  };

  for (int i=0; i<6; i++){
    value[i] = value_raw[i];
  }

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
    Mirf.getData(value_raw);
  }

  if(logging){
    logger();
  }

  clean_data();

  if(value[0]!= program){
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
      step_3();
      break;
    case 4:
      step_4();
      break;
    case 5:
      step_5();
      break;
  }
  FastLED.show(); 
}
