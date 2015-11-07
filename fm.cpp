#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#define FREQ 44100.0f
#define LEN 1.0f
#define BUFFER_SIZE (int)(LEN * FREQ)

enum EnvelopeState {
  AttackState = 0,
  DecayState,
  SustainState,
  ReleaseState,
  OffState
};

class Envelope {
public:
  Envelope(float attack, float decay, float sustain = 0.0f, float release = 0.1f) : 
    m_Attack(attack), 
    m_Decay(decay),
    m_Sustain(sustain),
    m_Release(release)
  {
    m_Output = 0.0f;
    m_Timer = 0.0;
    m_State = OffState;
  }

  void set(bool note) {
    m_Timer = 0.0f;
    m_State = note ? AttackState : ReleaseState;
  }

  void update(float dt) {
    switch(m_State) {
    case AttackState:
      m_Output = m_Attack > 0.0 ? m_Timer / m_Attack : 1.0f;

      if(m_Timer + dt > m_Attack) {
	m_Timer = 0.0f;
	m_State = DecayState;
      }
      break;
    case DecayState: 
      {
	float a = m_Decay > 0.0 ? m_Timer / m_Decay : 1.0f;
	m_Output = a * m_Sustain + (1.0f - a) * 1.0f;
	
	if(m_Timer + dt > m_Decay) {
	  m_Timer = 0.0f;
	  m_State = SustainState;
	}
      }
      break;  
    case SustainState:
      m_Output = m_Sustain;
      break;
    case ReleaseState:
      {
	float a = m_Release > 0.0 ? m_Timer / m_Release : 1.0f;
	m_Output = (1.0f - a) * m_Sustain;
	
	if(m_Timer + dt > m_Release) {
	  m_Timer = 0.0f;
	  m_State = OffState;
	}
      }
 
      break;
    case OffState:
      m_Output = 0.0f;
      break;
    }
    
    m_Timer += dt;
  }

  float output() {
    return m_Output;
  }

private:
  float m_Attack;
  float m_Decay;
  float m_Sustain;
  float m_Release;
  float m_Output;
  float m_Timer;

  EnvelopeState m_State;
};

class Osc {
public:
  Osc(float freq, float volume) {
    m_w = 2.0f * M_PI * freq;
    m_Output = 0.0f;
    m_Timer = 0.0f;
    m_Volume = volume;
    m_OutputPointer = NULL;
    m_Modulation = 0.0f;
  }

  void update(float dt) {
    m_Output = sin(m_w * m_Timer + m_Modulation);
    m_Timer += dt;
    m_Modulation = 0.0;

    if(m_OutputPointer != NULL)
      *m_OutputPointer += m_Output;
  }

  float *modulatorInput() {
    return &m_Modulation;
  }

  void setOutputPointer(float *ptr) {
    m_OutputPointer = ptr;
  }

  float output() {
    return m_Output * m_Volume;
  }

  float modulate(float mod) {
    m_Modulation += mod;
  }

  void setFreq(float freq) {
    m_w = 2.0f * M_PI * freq;
    m_Timer = 0.0f;
  }

private:
  float m_w;
  float m_Output;
  float m_Timer;
  float m_Volume;
  float m_Modulation;
  float *m_OutputPointer;
};

class Synth {
public:
  virtual void playNote(float freq) = 0;
  virtual void stopNote() = 0;
  virtual float output(float dt) = 0;
};

class FMSynth : public Synth {
public:
  FMSynth() {
    m_EnvA = new Envelope(0.02f, 0.8f, 0.0f);
    m_OscA = new Osc(440.0f, 0.25f);

    m_EnvB = new Envelope(0.0f, 0.8f, 0.0f);
    m_OscB = new Osc(440.0f, 1.5f);

    m_EnvC = new Envelope(0.0f, 0.9f, 0.0f);
    m_OscC = new Osc(440.0f, 1.0f);

    m_OscC->setOutputPointer(m_OscA->modulatorInput());
    m_OscB->setOutputPointer(m_OscA->modulatorInput());
  }

  void playNote(float freq) {
    m_OscA->setFreq(freq);
    m_OscB->setFreq(2.0f * freq);
    m_OscC->setFreq(4.0f * freq);

    m_EnvA->set(true);
    m_EnvB->set(true);
    m_EnvC->set(true);
  }

  void stopNote() {
    m_EnvA->set(false);
    m_EnvB->set(false);
    m_EnvC->set(false);
  }

  float output(float dt) {
    m_EnvA->update(dt);
    m_EnvB->update(dt);
    m_EnvC->update(dt);

    m_OscC->update(dt);
    m_OscB->update(dt);
    m_OscA->update(dt);

    return m_OscA->output() * m_EnvA->output();
  }

private:
  Envelope *m_EnvA;
  Envelope *m_EnvB; 
  Envelope *m_EnvC;
  Osc *m_OscA;
  Osc *m_OscB;
  Osc *m_OscC;
};

class StringSynth : public Synth {
public:
  StringSynth(float harm = 1.5f) {
    m_Harm = harm;

    m_BufferSize = 8;
    for(int i = 0; i < m_BufferSize; i++) {
      m_Buffer[i] = 0.0f;
    }

    m_Index = 0.0f;
    m_Run = 0.0f;
  }

  void playNote(float freq) {
    // initialize buffer with noise
    m_BufferSize = FREQ / freq * 1.0f;
    for(int i = 0; i < m_BufferSize; i++) {
      m_Buffer[i] = 2.0f * ((float)rand() / ((float)RAND_MAX)) - 1.0f;
      float t = (float)i / FREQ;
      m_Buffer[i] = m_Buffer[i] + 1.0 * sin(2.0f * M_PI * freq * t * m_Harm);
    }

    m_Index = 0;
    m_Run = 0;

    // make a full runs on the buffer
    for(int run = 0; run < 1; run ++) {
      for(int i = 0; i < m_BufferSize; i++) {
	size_t idx1 = (m_BufferSize + i) % m_BufferSize;
	size_t idx2 = (m_BufferSize + i - 1) % m_BufferSize;
	
	m_Buffer[i] = 0.5f * (m_Buffer[idx1] + m_Buffer[idx2]);
      }
    }
  }

  void stopNote() {
  }

  float output(float dt) {
    size_t idx1 = (m_BufferSize + m_Index) % m_BufferSize;
    size_t idx2 = (m_BufferSize + m_Index - 1) % m_BufferSize;
      
    float a = 0.8f;
    float damp = 0.995f;
    m_Buffer[m_Index] = (a * m_Buffer[idx1] + (1.0f - a) * m_Buffer[idx2]) * damp;
    float out = m_Buffer[m_Index] * 0.25f;

    m_Index++;
    if(m_Index >= m_BufferSize) {
      m_Index = 0;
      m_Run++;
    }

    return out;
  }

private:
  float m_Buffer[48000];
  size_t m_BufferSize;
  size_t m_Index;
  int m_Run;
  float m_Harm;
};

StringSynth synth(1.5);

float NoteToFreq(int note) {
  return powf(2.0f, ((float)note - 49.0f) / 12.0f) * 440.0f;
}

// piano note
void playNote(int note, float duration) {
  float freq = NoteToFreq(note);
  synth.playNote(freq);

  float dt = 1.0f / FREQ;
  float timer = 0.0f;
  while(timer < duration) {
    float out = synth.output(dt);
    fwrite(&out, sizeof(float), 1, stdout);
    timer += dt;
  }
}

int charToNote(char c) {
  int note = 0;
  switch(c) {
  case 'a': note = 1; break;
  case 'b': note = 3; break;
  case 'c': note = 4; break;
  case 'd': note = 6; break;
  case 'e': note = 8; break;
  case 'f': note = 9; break;
  case 'g': note = 11; break;
  }

  return note;
}

float tempo = 120.0f / 120.0f;

void playString(const char *str) {
  int len = strlen(str);
  
  int note = 0;
  int octave = 3;
  float duration = 0.45f;

  for(int i = 0; i < len; i++) {
    char c = tolower(str[i]);

    if(c >= 'a' && c <= 'g') {
      // it's a new note, play previous one
      if(note != 0) 
	playNote(note + octave * 12, duration);
      
      // start a new one
      note = charToNote(c);
    }
    else if(c == '-') {
      note--;
    }
    else if(c == '+') {
      note++;
    }
    else if(c >= '0' && c <= '9') {
      // change duration
      float t = (c - '0');
      duration = tempo * t / 4.0f;
    }
    else if(c == 'o' && i < len - 1) {
      if(note != 0)
	playNote(note + octave * 12, duration);
      note = 0;

      i++;
      char val = tolower(str[i]);
      if(val >= '0' && val <= '9') {
	octave = val - '0';
      }
    }
  }

  // play last note
  if(note != 0)
    playNote(note + octave * 12, duration);
}

int stringNote[6] = {20, 25, 30, 35, 39, 44};

class Guitar {
public:
  Guitar() {
  }

  void play(const char *s[]) {

    float duration = 0.5f * tempo / 4.0f;
    float len = strlen(s[0]);

    //fprintf(stderr, "%s\n", s[0]);

    // get the update for each string
    for(int l = 0; l < len; l++) {
      for(int i = 0; i < 6; i++) {
	if(s[i][l] != '-') {
	  if(s[i][l] == 'o') { // damp
	    
	  } else { // it's a note
	    int note = (s[i][l] - '0') + stringNote[i];
	    m_Strings[i].playNote(NoteToFreq(note));
	  }
	}	
      }

      // generate sound
      for(int d = 0; d < FREQ * duration; d++) {
	float output = 0.0f;
	for(int i = 0; i < 6; i++) {
	  // write sound
	  output += m_Strings[i].output(1.0f / FREQ);
	}
	
	fwrite(&output, sizeof(float), 1, stdout);
      }
      fflush(stdout);
    }
  }
private:
  FMSynth m_Strings[6];

};

Guitar guitar;

int main(int argc, char *argv[]) {
  for(int i = 0; i < 4; i++) {
    /*
    playString("o2 A2 E2 o3 A2 o2 E2");
    playString("o1 F2 o2 C2 F2 C2");
    playString("o1 D2 o2 A2 D2 A2 D2 A2 D2 A");
    */

    //playString("o1 E2 E2 F2 F+4 o2 A2 o1 F+2 o2 A2 o1 F+2 F+2 F2 E2 o2 B2 o1 E2 E4");
    //playString("o2 E9");
    
    //Lynyrd Skynyrd Simple Man
    
    tempo = 1.0f;
    const char *chord[] = {"--------------------3-----3-------------------------------------",
			   "0-2-3-----3-------------2-------2---0-----0---------------------",
			   "--------2-------2-----0-------0---0-----2-------2-------2-------",
			   "------0-------0---0---------0---------2-------2---2---2-------2-",
			   "------------1-------------------------------1-------1-------1---",
			   "----------------------------------------------------------3-----"};
    
    // RHCP Under the bridge (not good)
    /*
    tempo = 1.0f;
    const char *chord[] = {"------------------------------4-2-----------------------------------------------------------------4-2-----------------------------------",
			   "5-----------------------4-2-0-------4-------4-----------------2-4-5-5-----------------------4-2-0-------4-------4-----------------2-4-5-",
			   "----4-------4---------------------------4---------4-----4-2-0-----------4-------4---------------------------4---------4-----4-2-0-------",
			   "--------2---------2---------------------------3-----------------------------2---------2---------------------------3---------------------",
			   "3-------------3-----------------2-----------------------------------3-------------3-----------------2-----------------------------------",
			   "----------------------------------------------------------------------------------------------------------------------------------------"};
    */

    /*
    tempo = 0.2f;
    const char *chord[] = {"----------------------------------------------------------------------------------",
			   "0--------------------------------0----------------0-----0-0-----------------------",
			   "2---------------------------------2----------------2---2---2----------------------",
			   "2----------------------------------2----------------2-2-----2---------------------",
			   "----------------------------------------------------------------------------------",
			   "----------------------------------------------------------------------------------"};
    */
    guitar.play(chord);
  }
}
