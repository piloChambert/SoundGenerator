#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint16_t period;
  uint16_t counter;
  uint8_t output;
} SquareGenerator;

SquareGenerator *CreateSquareGenerator() {
  SquareGenerator *gen = (SquareGenerator *)malloc(sizeof(SquareGenerator));
  gen->period = 32;
  gen->counter = 0;
  gen->output = 1;

  return gen;
}

void UpdateSquareGenerator(SquareGenerator *gen) {
  gen->counter++;
  if(gen->counter >= gen->period) {
    gen->output = 1 - gen->output;
    gen->counter = 0;
  }
}


typedef struct {
  uint16_t period;
  uint16_t counter;
  uint32_t rng;

  uint8_t output;
} NoiseGenerator;

NoiseGenerator *CreateNoiseGenerator() {
  NoiseGenerator *gen = (NoiseGenerator *)malloc(sizeof(NoiseGenerator));
  gen->period = 10;
  gen->counter = 0;
  gen->rng = 1;

  return gen;
}

void UpdateNoiseGenerator(NoiseGenerator *gen) {
  gen->counter++;
  if(gen->counter >= gen->period) {
    // update random generator
    gen->rng ^= (((gen->rng & 1) ^ ((gen->rng >> 3) & 1)) << 17);
    gen->rng >>= 1;

    gen->counter = 0;    
  }

  gen->output = gen->rng & 1;
  //printf("%d\n", (int) gen->output);
}

#define ENV_HOLD 1
#define ENV_ALTERNATE 2
#define ENV_ATTACK 4
#define ENV_CONTINUE 8

typedef struct _EnvelopeGenerator {
  uint16_t period;
  uint16_t counter;
  float output;

  uint8_t flags;

  // state function
  void (*state)(struct _EnvelopeGenerator *);
} EnvelopeGenerator;

// forward state declaration
void StateAttack(EnvelopeGenerator *gen);
void StateDecay(EnvelopeGenerator *gen);
void StateHold0(EnvelopeGenerator *gen);
void StateHold1(EnvelopeGenerator *gen);

void (*states[4])(struct _EnvelopeGenerator *);
int transition[2][16];

// state declaration
void StateDecay(EnvelopeGenerator *gen) {
  gen->output =  1.0f - (float)gen->counter / (float)gen->period;

  gen->counter++;
  if(gen->counter > gen->period) {
    gen->counter = 0;
    
    gen->output = 0;
    
    // change state
    //printf("%d %x\n", transition[0][gen->flags], states[transition[0][gen->flags]]);
    gen->state = states[transition[0][gen->flags]];
  }
}

void StateAttack(EnvelopeGenerator *gen) {
  gen->output = (float)gen->counter / (float)gen->period;

  gen->counter++;
  if(gen->counter > gen->period) {
    gen->counter = 0;
    
    // change state
    gen->state = states[transition[1][gen->flags]];
  }
}

void StateHold0(EnvelopeGenerator *gen) {
  // stay in this state 
  gen->output = 0;
}

void StateHold1(EnvelopeGenerator *gen) {
  // stay in this state 
  gen->output = 1.0f;
}

void (*states[4])(struct _EnvelopeGenerator *) = {
  StateDecay,
  StateAttack,
  StateHold0,
  StateHold1
};

// states, flags
int transition[2][16] = {
  // when in state decay
  {  2,  2,  2,  2, -1, -1, -1, -1,  0,  2,  1,  3,  -1, -1,  1, -1},
  { -1, -1, -1, -1,  2,  2,  2,  2, -1, -1,  0, -1,   1,  3,  0,  2}
};

EnvelopeGenerator *CreateEnvelopeGenerator() {
  EnvelopeGenerator *gen = (EnvelopeGenerator *)malloc(sizeof(EnvelopeGenerator));
  gen->period = 4;
  gen->counter = 0;
  gen->output = 0;
  gen->flags = 0;

  if(gen->flags & ENV_ATTACK)
    gen->state = StateAttack;
  else
    gen->state = StateDecay;

  return gen;
}

void SetEnvelopeFlags(EnvelopeGenerator *gen, uint8_t flags) {
  gen->flags = flags & 0xF;
  
  if(gen->flags & ENV_ATTACK)
    gen->state = StateAttack;
  else
    gen->state = StateDecay;
}

#define FREQ 44100.0
#define LEN 0.5 // seconds
#define BUFFER_SIZE (int)(LEN * FREQ)

#define A (int)round(FREQ / 880.0)
#define Bb (int)round(FREQ / 932.33)
#define B (int)round(FREQ / 987.77)
#define C (int)round(FREQ / 523.25)
#define Db (int)round(FREQ / 554.37)
#define D (int)round(FREQ / 587.33)
#define Eb (int)round(FREQ / 622.25)
#define E (int)round(FREQ / 659.25)
#define F (int)round(FREQ / 698.46)
#define Gb (int)round(FREQ / 739.99)
#define G (int)round(FREQ / 783.99)

float DACTable[16] = {
  0.0,
  
  1.0
};

int main(int argc, char *argv[]) {
  // 8bits square wave output
  float buffer[BUFFER_SIZE];

  SquareGenerator *genA = CreateSquareGenerator();
  SquareGenerator *genB = CreateSquareGenerator();
  SquareGenerator *genC = CreateSquareGenerator();
  NoiseGenerator *genN = CreateNoiseGenerator();
  EnvelopeGenerator *envGen = CreateEnvelopeGenerator();

  envGen->period = 8000;
  SetEnvelopeFlags(envGen, 0);

  genA->period = E;
  genB->period = B;
  genC->period = E*0.5;
  genN->period = 10;

  int i;
  for(i = 0; i < BUFFER_SIZE; i++) {
    envGen->state(envGen);
    UpdateSquareGenerator(genA);
    UpdateSquareGenerator(genB);
    UpdateSquareGenerator(genC);
    UpdateNoiseGenerator(genN);
    static int c = 0;
    
    c++;
    if(c > 10) {
      genN->period += 1;
      c = 0;
    }

    buffer[i] = /*(genA->output - 0.5) + (genB->output - 0.5) + (genC->output - 0.5)*/ + (genN->output - 0.5);

    buffer[i] = buffer[i]*envGen->output * 0.05;
    //printf("%f\n", buffer[i]);
  }

  fwrite(buffer, sizeof(float), BUFFER_SIZE, stdout);
  fflush(stdout);
}
