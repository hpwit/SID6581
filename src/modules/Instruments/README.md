
SID Instruments Usage example
-----------------------------


```C

#define SID_INSTRUMENTS
#include "SID6581.h"


class gameboystartup1 : public sid_instrument
{
  public:
    int i;
    virtual void start_sample(int voice,int note)
    {
        sid->setAttack(    voice, 2 ); // 0 (0-15)
        sid->setDecay(     voice, 0 ); // 2 (0-15)
        sid->setSustain(   voice, 15 ); // 15 (0-15)
        sid->setRelease(   voice, 1 ); // 10 (0-15)
        sid->setPulse(     voice, 2048 ); // 1000 ( 0 - 4096 )
        sid->setWaveForm(  voice, SID_WAVEFORM_PULSE ); // SID_WAVEFORM_PULSE (TRIANGLE/SAWTOOTH/PULSE)
        sid->setGate(      voice, 1 ); // 1 (0-1)
        sid->setFrequency( voice, note); // note 0-95
        //i=0;
    }
};


class gameboystartup2 : public sid_instrument
{
  public:
    int i;
    virtual void start_sample(int voice,int note)
    {
        sid->setAttack(    voice, 2 ); // 0 (0-15)
        sid->setDecay(     voice, 0 ); // 2 (0-15)
        sid->setSustain(   voice, 7 ); // 15 (0-15)
        sid->setRelease(   voice, 1 ); // 10 (0-15)
        sid->setPulse(     voice, 2048 ); // 1000 ( 0 - 4096 )
        sid->setWaveForm(  voice, SID_WAVEFORM_PULSE ); // SID_WAVEFORM_PULSE (TRIANGLE/SAWTOOTH/PULSE)
        sid->setGate(      voice, 1 ); // 1 (0-1)
        sid->setFrequency( voice, note); // note 0-95
        //i=0;
    }
};



void soundIntro( SID6581* sid )
{
  SIDKeyBoardPlayer::KeyBoardPlayer( sid, 1);
  sid->sidSetVolume( 0, 15 );
  delay(500);
  SIDKeyBoardPlayer::changeAllInstruments<gameboystartup1>( sid );
  SIDKeyBoardPlayer::playNote( 0, SID_PlayNotes[60], 120 ); // 1 octave = 12
  delay(120);
  SIDKeyBoardPlayer::changeAllInstruments<gameboystartup2>( sid );
  SIDKeyBoardPlayer::playNote( 0, SID_PlayNotes[84], 1000 ); // 1 octave = 12
  // fade out
  for( int t=0;t<16;t++ ) {
    delay( 1000/16 );
    sid->sidSetVolume( 0, 15-t );
  }
  SIDKeyBoardPlayer::stopNote( 0 );
  // back to full silence
  sid->setAttack(  0, 0 ); // 0 (0-15)
  sid->setDecay(   0, 0 ); // 2 (0-15)
  sid->setSustain( 0, 0 ); // 15 (0-15)
  sid->setRelease( 0, 0 ); // 10 (0-15)
  sid->setGate(    0, 0 ); // 1 (0-1)
  vTaskDelete( SID_xTaskHandles[0] );
  sid->resetsid();
}



void setup()
{

  Serial.begin(115200);

  sid.begin(  SID_CLOCK, SID_DATA, SID_LATCH );

  soundIntro( &sid );

}



void loop()
{

}

```



