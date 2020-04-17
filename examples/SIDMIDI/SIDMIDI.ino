//#include <Softwareserial.h>
//HardwareSerial Serial11(1);
#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27

#define SID_ENVELOPPE 1
#define SID_SAMPLE 0
//
#define MIDI_NOTE_ON 0x9
#define MIDI_NOTE 0x9001
#define MIDI_VELOCITY 0x9002


#define MIDI_CONTROL_CHANGE 0xb
#define MIDI_CONTROLLER_NUMBER 0xb100
#define MIDI_BANK_SELECT_MSB 0xb000
#define MIDI_BANK_SELECT_LSB 0xb020

#define MIDI_PROGRAM_CHANGE 0xc
#define MIDI_PROGAM_CHANGE_NUMBER 0xC001

#define NUMBER_OF_VOICES 6

int voices[NUMBER_OF_VOICES];
int lastvoice=0;

void playNote(int note) {
  for(int i=0;i<NUMBER_OF_VOICES;i++) {
    int new_voice=(lastvoice+1+i)%NUMBER_OF_VOICES;
    if(voices[new_voice]==0) {
      voices[new_voice]=note;
      SIDKeyBoardPlayer::playNote(new_voice,_sid_play_notes[note-21+9],0);
      lastvoice=new_voice;
      break;
    }
    Serial.printf("voice %d not free\n",i);
  }
}


void stopNote(int note) {
  for(int i=0;i<NUMBER_OF_VOICES;i++) {
    if(voices[i]==note) {
      SIDKeyBoardPlayer::stopNote(i);
      voices[i]=0;
      break;
    }
  }
}


void setup() {
  // initialize serial:
  Serial.begin(115200);
  Serial1.begin(31250,SERIAL_8N1, 34);    //start listening on pin 34 for midi instruction
  sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);
  SIDKeyBoardPlayer::KeyBoardPlayer(NUMBER_OF_VOICES);
  sid.sidSetVolume(0,15);
  sid.sidSetVolume(1,15);
}


int notetoplay;
uint8_t status=0;
uint16_t next_data=99;
uint16_t MID_LSB;
uint16_t MID_MSB;
uint16_t MID_PRG;

 // uint8_t notetoplay;
void  change_instrument() {
  switch(MID_LSB) {
    case 122:
      if(MID_PRG==0)
        SIDKeyBoardPlayer::changeAllInstruments<sid_piano2>();
      else
          SIDKeyBoardPlayer::changeAllInstruments<sid_piano5>();
    break;
    case 112:
      SIDKeyBoardPlayer::changeAllInstruments<sid_piano>();
    break;
    case 123:
      SIDKeyBoardPlayer::changeAllInstruments<sid_piano3>();
    break;
    case 125:
      SIDKeyBoardPlayer::changeAllInstruments<sid_piano4>();
    break;
  }
}


void loop() {
  if(Serial1.available()) {  //if user typed something through the USB...
    uint8_t received=Serial1.read();
    uint8_t command=received>>4;
    if(received!=0xF8 && received!=0xFe) {
      //Serial.printf("\n--------\nreceiving %x %d\n",received,received);
      if (received >=0x90) {
        status=command;
        switch(command) {
          case MIDI_NOTE_ON:
            next_data=MIDI_NOTE;
            Serial.println("note on");
          break;
          case MIDI_CONTROL_CHANGE:
            next_data=MIDI_CONTROLLER_NUMBER;
            Serial.println("controller change");
          break;
          case MIDI_PROGRAM_CHANGE:
            next_data=MIDI_PROGAM_CHANGE_NUMBER;
            Serial.println("program change");
          break;
          default:
            next_data=99;
            //Serial.println("ddafule");
          break;
        }
      } else {
        // Serial.printf("nxt data %d\n",next_data);
        switch(next_data) {
          case MIDI_NOTE:
            notetoplay=received;
            next_data=MIDI_VELOCITY;
            Serial.printf("received note %d\n",notetoplay);
          break;
          case MIDI_VELOCITY:
            next_data=MIDI_NOTE;
            if(received==0)
              stopNote(notetoplay);
            else
              playNote(notetoplay);
            Serial.printf("received velocity %d\n",received);
          break;
          case MIDI_CONTROLLER_NUMBER:
            next_data=received+0xB000;
            Serial.printf("next data:%x\n",next_data);
          break;
          case MIDI_BANK_SELECT_MSB:
            MID_MSB=received;
              Serial.printf("received control program %x %d\n",MIDI_BANK_SELECT_MSB , received);
            next_data=MIDI_CONTROLLER_NUMBER;
          break;
          case MIDI_BANK_SELECT_LSB:
            MID_LSB=received;
              Serial.printf("received control program %x %d\n",MIDI_BANK_SELECT_LSB , received);
            next_data=MIDI_CONTROLLER_NUMBER;
          break;
          case MIDI_PROGAM_CHANGE_NUMBER:
            MID_PRG=received;
            Serial.printf("received new program %d\n", received);
            change_instrument();
            next_data=99;
          break;
        }
      }
    }
  }
}
