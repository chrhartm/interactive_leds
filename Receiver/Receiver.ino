//Radio Stuff
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// LED Stuff
#include <FastLED.h>

#define NUM_ENERGIES 20

// Radio stuff
RF24 radio(10,9);
uint8_t value[6];
uint8_t value_long[NUM_ENERGIES+1];
uint8_t value_raw[7];
uint8_t value_raw_long[NUM_ENERGIES+7+1];
uint8_t value_garbage[32];
const uint64_t pipe = 0xF0F0F0F0E1LL;

// LED Stuff
#define NUM_LEDS 120
#define DATA_PIN 6
#define CLOCK_PIN 13
CRGB leds[NUM_LEDS];

// Global states
bool button2_on=0;
int program=0;
static int N_PROGRAMS=10;
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
  radio.begin();
  radio.setRetries(0,0);
  radio.disableCRC();
  radio.enableDynamicPayloads();
  radio.setAutoAck(0);
  radio.setChannel(90);
  radio.openReadingPipe(1,pipe);
  radio.startListening();

  // LED stuff
  LEDS.addLeds<WS2812,DATA_PIN,RGB>(leds,NUM_LEDS);
  LEDS.setBrightness(255); //84
  value[0] = 2;
  value[1] = 0;
  value[2] = 150;
  value[3] = 250;
  value[4] = 240;
  value[5] = 0;
  value[6] = 0;

  value_raw[6] = 1;
}

void logger(){
  Serial.println(value_raw[0]);
  Serial.println(value_raw[1]);
  Serial.println(value_raw[2]);
  Serial.println(' ');
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
      ix = ix + 5;
      if(ix>=255){
        ix = 255;
        state = 1;
      };
    } else {
      ix = ix - 5;
      if(ix<=(-value[4]*5)){
        ix = 0;
        state = 0;
      };
    };
    state2 = value[2];   
  };  
  for (int i=0; i<NUM_LEDS; i++){
    if(ix>0){
      leds[i] = CHSV(state2, 255, ix);
    };
  }
  delay(value[3]/20);
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
  fadeall(100);
  for (int i=0; i<NUM_LEDS/2; i++){
    int col = (value[2]+((value[5]*i)/(NUM_LEDS/2)))%255;
    // Starting from center in both directions
    if (i < ix){
      if (!button_state){
        leds[NUM_LEDS/2 + i] = CHSV(col, 255, 255);
        leds[NUM_LEDS/2 - i] = CHSV(col, 255, 255);
      };
    } else{
      if (button_state){
        leds[NUM_LEDS/2 + i] = CHSV(col, 255, 255);
        leds[NUM_LEDS/2 - i] = CHSV(col, 255, 255);
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

void step_6(){
  int sum = 0;
  for (int i=0; i<NUM_ENERGIES; i++){
    sum += value_long[i];
  }
  sum = sum/NUM_ENERGIES;
  if ((sum > value[4])||value[1]){
    state2 = 255;
    ix = (int(value_long[NUM_ENERGIES]*(float(value[5])/255)) + value[2])%255;
  }
  else if (state2 > 5){
    state2 = state2-5;
  }
  else {
  };
  for (int i=0; i<NUM_LEDS; i++){
    leds[i] = CHSV(ix, 255, state2);
  }
  delay(value[3]/4);
}

void step_7(){
  calc_button2_on();

  int sum_low = 0;
  int sum_med = 0;
  int sum_high = 0;
  for(int i=0; i<NUM_ENERGIES/3; i++){
    sum_low = sum_low + value_long[i];
    sum_med = sum_med + value_long[i+NUM_ENERGIES/3];
    sum_high = sum_high + value_long[i+2*NUM_ENERGIES/3];
  }
  sum_low = sum_low / (NUM_ENERGIES/3);
  sum_med = sum_med / (NUM_ENERGIES/3);
  sum_high = sum_high / (NUM_ENERGIES/3);

  for (int i=0; i<NUM_LEDS-1; i++){
    if(button_state){
      leds[i] = leds[i+1];  
    }
    else{
      leds[NUM_LEDS-1-i] = leds[NUM_LEDS-2-i];
    };
  }
  int saturation = 255-value[5];
  if (sum_low + sum_med + sum_high > value[4]*3){
    if(button_state){
      leds[NUM_LEDS-5] = CHSV((sum_med+value[2])%255, saturation, sum_low);
      leds[NUM_LEDS-4] = CHSV((sum_med+value[2])%255, saturation, int(sum_low*0.8));
      leds[NUM_LEDS-3] = CHSV((sum_med+value[2])%255, saturation, int(sum_low*0.6));
      leds[NUM_LEDS-2] = CHSV((sum_med+value[2])%255, saturation, int(sum_low*0.4));
      leds[NUM_LEDS-1] = CHSV((sum_med+value[2])%255, saturation, int(sum_low*0.2));
    }
    else{
      leds[4] = CHSV((sum_med+value[2])%255, saturation, sum_low);
      leds[3] = CHSV((sum_med+value[2])%255, saturation, int(sum_low*0.8));
      leds[2] = CHSV((sum_med+value[2])%255, saturation, int(sum_low*0.6));
      leds[1] = CHSV((sum_med+value[2])%255, saturation, int(sum_low*0.4));
      leds[0] = CHSV((sum_med+value[2])%255, saturation, int(sum_low*0.2));
    };
  }
  else{
    if(button_state){
      leds[NUM_LEDS-1] = CHSV(0, 0, 0);
    }
    else
    {
      leds[0] = CHSV(0, 0, 0);
    }
  }
  delay(value[3]/4);
}

void step_8(){
  calc_button2_on();

  for (int i=0; i<NUM_ENERGIES; i++){
    int col = (value[2] + value[5]/30*i)%255;
    int saturation = 255;
    if(button_state){
      saturation = 255 - 255/NUM_ENERGIES * i;
    };
    for(int j=0; j<NUM_LEDS/NUM_ENERGIES; j++){
      if (value_long[i]>value[4]){
        leds[i*(NUM_LEDS/NUM_ENERGIES)+j] = CHSV(col, saturation, value_long[i]);
      };
    }
  }
  fadeall(value[3]);
  delay(value[3]/4);
}

void step_9(){
  int sum = 0;
  for (int i=0; i<NUM_ENERGIES; i++){
    sum += value_long[i];
  }
  sum = sum/NUM_ENERGIES;
  int brightness = min(sum + 100,255);
  int delta = min(value[4]/5+5,50);
  int pos = int(float(value_long[NUM_ENERGIES]+1)/float(256)*delta);
  for (int i=0; i<NUM_LEDS-delta; i=i+delta){
    int col = (value[2] + value[5]/30*i)%255;
    if(value[1]){
      leds[i+pos] = CHSV(col+value[5], 0, brightness);
    }
    else{
      leds[i+pos] = CHSV(col, 255, brightness);
    }
  }
  fadeall(value[3]);
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
  if (radio.available()) {
    uint8_t len = radio.getDynamicPayloadSize();
    if(len==7){
      radio.read(value_raw, len);
    }
    else if(len==(NUM_ENERGIES+7+1)){
      radio.read(value_raw_long, len);
      for(int i=0; i<7; i++){
        value_raw[i] = value_raw_long[i];
      }
      for(int i=0; i<NUM_ENERGIES+1; i++){
        value_long[i] = value_raw_long[i+7];
      }
    }
    else{
      radio.read(value_garbage,len);
    };
  };

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
    case 6:
      step_6();
      break;
    case 7:
      step_7();
      break;
    case 8:
      step_8();
      break;
    case 9:
      step_9();
      break;
  }
  FastLED.show(); 
}
