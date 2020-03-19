SID 6581 library
===========

This library is to control  up to 5 SID 6581 chips from the 1980s era with an esp32.
The program allows you to :
* Directly push the register to the  SID chip. Hence you can program like in the good all times :)
* Play old (or new) sid tunes
* Play notes over up to 15 voices (3 voices per SID chip)
* Design and play different instruments
* Assign up to one instrument per voice
The sound is played is the background so your mcu can do something else at the same time

NB: the SID chip requires a 1Mhz clock signal to work ** you can either provide it with an external clock circuit or use a pin of the esp32 to do it ** (clock generated thanks to I2s).

it should work with other mcu as it uses SPI but not tested.


Please look at the schematics for the setup of the shift registers and  MOS 6581

## To start
```
The object sid is automatically created.
//if you have a external circuit that gives you the 1Mhz clock you can use:
sid.begin(int clock_pin,int data_pin, int latch);


//if you do not have an external circuit the esp32 can create the 1Mhz signal uisng i2s using this
begin(int clock_pin,int data_pin, int latch,int sid_clock_pin);

here the sid_clock_pin will need to be plugged to the 02 pin or clock pin of the SID 6581 NB: this pin number has to be >=16

```


## To play a SIDtunes
You can play SIDTunes stored on the SPIFF or the SD card this will play on chip 0


NB 1: playSIDTunes will only work with WROOVER because I use PSRAM for the moment. all the rest will run with all esp32.

NB2: the player si for the moment outputing only on the first chip. if you have stereo sid example I will be more than happy to implement that too

Below the list of command to control the player

```
void addSong(fs::FS &fs,  const char * path); //add song to the playlist
void play(); //play in loop the playlist
void playNext(); //play next song of the playlist
void playPrev(); //play prev song of the playlist
void soundOff(); //cut off the sound
void soundOn(); //trun the sound on
void pausePlay(); //pause/play the player
void sidSetMaxVolume( uint8_t volume); //each sid tunes usually set the volume this function will allow to scale the volume
void stopPlayer(); //stop the player to restart use play()

```

### Example

```
#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27
#define SID_CLOCK_PIN 26
#include "SPIFFS.h"

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);

    sid.begin(SID_CLOCK,SID_DATA,SID_LATCH); //if you have an external clock circuit
    //or sid.begin(SID_CLOCK,SID_DATA,SID_LATCH,SID_CLOCK_PIN); //(if you do not have a external clock cicuit)

    if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return;
    }


    //the following line will go through all the files in the SPIFFS
    //Do not forget to do "Tools-> ESP32 Scketch data upload"
    File root = SPIFFS.open("/");
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){

        } else {
        Serial.print(" add file  FILE: ");
        Serial.print(file.name());
        Serial.print("\tSIZE: ");
        Serial.println(file.size());
        sid.addSong(SPIFFS,file.name()); //add all the files on the root of the spiff to the playlist
    }
        file = root.openNextFile();
    }
    sid.sidSetMaxVolume(7); //value between 0 and 15


    sid.play(); //it will play all songs in loop
}

void loop() {
//if you jsut want to hear the songs just comment the lines below


    delay(5000);
    Serial.println("Pause the song");
    sid.pausePlay();
    delay(4000);
    Serial.println("restart the song");
    sid.pausePlay();
    delay(3000);
    Serial.println("hi volume");
    sid.sidSetMaxVolume(15);
    delay(3000);
    Serial.println("low volume ");
    sid.sidSetMaxVolume(3);
    delay(3000);
    Serial.println("medium");
    sid.sidSetMaxVolume(7);
    delay(3000);

    delay(3000);
    Serial.println("next song");
    sid.playNext(); //sid.playPrev(); if you want to go backwards 
    delay(10000);

    //sid.stopPlayer(); //to stop the plater completly
    //delay(10000);
    //sid.play(); //to restart it

}
```



PS: to transform the .sid into register commands 

1) I use the fantastic program of Ken Händel
https://haendel.ddns.net/~ken/#_latest_beta_version
```
java -jar jsidplay2_console-4.1.jar --audio LIVE_SID_REG --recordingFilename export_file sid_file

use SID_REG instead of LIVE_SID_REG to keep you latptop quiet


```
2) I use the program traduct_2.c
to compile it for your platform:
```
>gcc traduct_2.c -o traduct
```
to use it
```
./traduct export_file > final_file

```
Put the final_file on a SD card or in your SPIFF


## Details of the other control commands

You have full control of the SID chip via the following commands
```
void sidSetVolume( uint8_t chip,uint8_t vol); set the volume off a specific SID chip (start with 0)

below the chip number is deduced using the voice number (start with 0)
chip number = voice/3
voice on the chip = voice%3
hence if you have two chips, if voice=4,  sid.setFrequencyHz(voice, 440) will put 440Hz on the 2nd  voice of the chip n°1

void setFrequency(int voice, uint16_t frequency); //this function set the 16 bit frequency is is not the frequency in Hertz
                The frequency is determined by the following equation:
                Fout = (Fn * Fclk/16777216) Hz
                Where Fn is the 16-bit number in the Frequency registers and Fclk is the system clock applied to the 02 input (pin 6). For a standard 1.0 Mhz clock, the frequency is given by:
                Fout = (Fn * 0.0596) Hz
void setFrequencyHz(int voice,double frequencyHz); //Use this function to set up the frequency in Hertz ex:setFrquencyHz(0,554.37) == setFrequency(0,9301) 
void setPulse(int voice, uint16_t pulse);
void setEnv(int voice, uint8_t att,uint8_t decay,uint8_t sutain, uint8_t release);
void setAttack(int voice, uint8_t att);
void setDecay(int voice, uint8_t decay);
void setSustain(int voice,uint8_t sutain);
void setRelease(int voice,uint8_t release);
void setGate(int voice, int gate);
void setWaveForm(int voice,int waveform);
        Waveforms available:
                SID_WAVEFORM_TRIANGLE 
                SID_WAVEFORM_SAWTOOTH 
                SID_WAVEFORM_PULSE 
                SID_WAVEFORM_NOISE 
void setTest(int voice,int test);
void setSync(int voice,int sync);

void setRingMode(int voice, int ringmode);
void setFilterFrequency(int chip,int filterfrequency);
void setResonance(int chip,int resonance);
void setFilter1(int chip,int filt1);
void setFilter2(int chip,int filt2);
void setFilter3(int chip,int filt3);
void setFilterEX(int chip,int filtex);
void set3OFF(int chip,int _3off);
void setHP(int chip,int hp);
void setBP(int chip,int bp);
void setLP(int chip,int lp);


For advance control:

void pushToVoice(int voice,uint8_t address,uint8_t data);

This function will allow you to push a data to a specific register of a specific voice

example:
sid.pushToVoice(0,SID_FREG_HI,255) //to push 255 on the register FREQ_HI of voice 0
possible values:
        SID_FREQ_LO
        SID_FREQ_HI
        SID_PW_LO
        SID_PW_HI
        SID_CONTROL_REG
        SID_ATTACK_DECAY
        SID_SUSTAIN_RELEASE



void pushRegister(uint8_t chip,int address,int data);

This function will allow you to push directly a value to a register of a specific chip
example:

sid.pushRegister(0,SID_MOD_VOL,15) //will put the sound at maximum
possible values:
        SID_FREQ_LO_0
        SID_FREQ_HI_0
        SID_PW_LO_0
        SID_PW_HI_0
        SID_CONTROL_REG_0
        SID_ATTACK_DECAY_0
        SID_SUSTAIN_RELEASE_0
        SID_FREQ_LO_1
        SID_FREQ_HI_1
        SID_PW_LO_1
        SID_PW_HI_1
        SID_CONTROL_REG_1
        SID_ATTACK_DECAY_1
        SID_SUSTAIN_RELEASE_1
        SID_FREQ_LO_2
        SID_FREQ_HI_2
        SID_PW_LO_2
        SID_PW_HI_2
        SID_CONTROL_REG_2
        SID_ATTACK_DECAY_2
        SID_SUSTAIN_RELEASE_2
        SID_FC_LO
        SID_FC_HI
        SID_RES_FILT
        SID_MOD_VOL
NB:
    sid.pushToVoice(0,SID_FREG_HI,255) == sid.pushRegister(0,SID_FREQ_HI_0,255)



void resetsid();
this function will reset all the SID chip

```
Below a code example

```
#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27


void setup() {
    Serial.begin(115200);
    sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);

    sid.sidSetVolume(0,15); 
    sid.setGate(0,1);//if not set you will not hear anything
    sid.setAttack(0,1);
    sid.setSustain(0,15);
    sid.setDecay(0,1);
    sid.setRelease(0,1);
    sid.setPulse(0,512);
    sid.setWaveForm(0,SID_WAVEFORM_TRIANGLE);

}

void loop() {
    sid.setWaveForm(0,SID_WAVEFORM_TRIANGLE);
    for(int i=0;i<255;i++)
    {
        sid.setFrequency(0,i+(255-i)*256);
        delay(10);  
    }

    sid.setWaveForm(0,SID_WAVEFORM_PULSE);
    for(int i=0;i<5000;i++)
    {
        sid.setFrequency(0,i%255+(255-i%255)*256);
        float pulse=1023*(cos((i*3.14)/1000)+1)+2047;
        sid.setPulse(0,(int)pulse);
        delayMicroseconds(1000);
    }

}

```


## Keyboard Player

You can turn the SID Chip into a synthetizer with up to 15 voices.
The following commands will allow you to create instruments and simplify the creation of music. It can also be used for MIDI see example.

### to start the keyboard
```
all the function of the KeyBoardPlayer are static so always preceded by SIDKeyBoardPlayer::

SIDKeyBoardPlayer::KeyBoardPlayer(int number_of_voices); //prepares everything for 6 simultaneous voices

```
### play a note
```
All this commands will play a note on a specific voice for a certain duration

SIDKeyBoardPlayer::playNote(int voice,uint16_t note,int duration)
or
SIDKeyBoardPlayer::playNoteHz(int voice,int frequencyHz,int duration)
or
SIDKeyBoardPlayer::playNoteVelocity(int voice,uint16_t note,int velocity,int duration)  //if you have an instrument that uses this value

NB: the duration is in milliseconds

example:

#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27


void setup() {
// initialize serial:
        Serial.begin(115200);
        sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);
        SIDKeyBoardPlayer::KeyBoardPlayer(6);
        sid.sidSetVolume(0,15); 

        
}

void loop()
{
    SIDKeyBoardPlayer::playNoteHz(2,440,2000); //will play a A4 for 2 seconds
    while(SIDKeyBoardPlayer::isVoiceBusy(2)); //as the note are played in the background you need to wait a bit
    delay(1000); 

}

```
You can play several voices at the sametime

```

#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27


void setup() {
// initialize serial:
    Serial.begin(115200);
    sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);
    SIDKeyBoardPlayer::KeyBoardPlayer(6);
    sid.sidSetVolume(0,15); 
    sid.sidSetVolume(1,15); //if you have two chips

}

void loop()
{
    SIDKeyBoardPlayer::playNoteHz(2,440,6000); //will play a A4 for 2 seconds
    delay(1000); 
    SIDKeyBoardPlayer::playNoteHz(0,880,2000);
    delay(500); 
    SIDKeyBoardPlayer::playNoteHz(1,1760,2000);
    delay(500); 
    SIDKeyBoardPlayer::playNoteHz(3,220,2000); //we will play on voice 0 of the second chip 
    while(SIDKeyBoardPlayer::areAllVoiceBusy());
    delay(2000);
}
```

NB: if the duration is equal to 0, then the sound will not stop until you call the function stopNote. 

```

#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27


void setup() {
        // initialize serial:
        Serial.begin(115200);
        sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);
        SIDKeyBoardPlayer::KeyBoardPlayer(6);
        sid.sidSetVolume(0,15); 
        sid.sidSetVolume(1,15); //if you have two chips

}

void loop()
{
        SIDKeyBoardPlayer::playNoteHz(2,440,0); //will play a A4 for 2 seconds
        delay(1000); 
        SIDKeyBoardPlayer::stopNote(2);
        delay(500);
 
}
```

### To change instrument
The library give you the possibility to change instruments there are 5 in 'store'

1) Change instrument for all voices

```
to change the instruments for all voices
changeAllInstruments<instrument>();
possible values:
            sid_piano
            sid_piano2
            sid_piano3
            sid_piano4
            sid_piano5
            
 example:
 
 
 #include "SID6581.h"
 #define SID_CLOCK 25
 #define SID_DATA 33
 #define SID_LATCH 27
 
 void playtunes()
 {
     for(int i=0;i<3;i++)
     {
         SIDKeyBoardPlayer::playNoteHz(0,220*(i+1),1500);
         while(SIDKeyBoardPlayer::isVoiceBusy(0));
         delay(100);
     }
}
 
 void setup() {
     // initialize serial:
     Serial.begin(115200);
     sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);
     SIDKeyBoardPlayer::KeyBoardPlayer(6);
     sid.sidSetVolume(0,15); 
     sid.sidSetVolume(1,15); //if you have two chips
     
 }
 
 void loop()
 {
     SIDKeyBoardPlayer::changeAllInstruments<sid_piano>();
     playtunes();
    SIDKeyBoardPlayer::changeAllInstruments<sid_piano2>();
     playtunes();
     SIDKeyBoardPlayer::changeAllInstruments<sid_piano3>();
     playtunes();
     SIDKeyBoardPlayer::changeAllInstruments<sid_piano4>();
     playtunes();
     SIDKeyBoardPlayer::changeAllInstruments<sid_piano5>();
     playtunes();
     delay(1000);
 }
            

```

2) Change instrument for a specific voice

```

to change the instruments for all voices
changeInstrumentOnVoice<instrument>(uint8_t voice);
possible values:
        sid_piano
        sid_piano2
        sid_piano3
        sid_piano4
        sid_piano5

example:

#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27

void playtunes()
{
    for(int i=0;i<3;i++)
    {
        SIDKeyBoardPlayer::playNoteHz(i,220*(i+1),1500); //playing each note on a specific voice
        while(SIDKeyBoardPlayer::isVoiceBusy(i));
        delay(100);
    }
}

void setup() {
    // initialize serial:
    Serial.begin(115200);
    sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);
    SIDKeyBoardPlayer::KeyBoardPlayer(6);
    sid.sidSetVolume(0,15); 
    sid.sidSetVolume(1,15); //if you have two chips

}

void loop()
{
        SIDKeyBoardPlayer::changeAllInstruments<sid_piano2>();
        playtunes(); //all the voices will play the same instrument
        SIDKeyBoardPlayer::changeInstrumentOnVoice<sid_piano>(1);
        playtunes(); //ther second voice will be palyed with a different instrument
        delay(1000);
}


```


### To create an instrument
the library allows to create your own instruments.
 ```
 each instrument is a class
 
 
 class new_instrument:public sid_instrument{
     public:
         
         new_instrument(){
            //her goes the code to initialise your instrument
                }
         
         virtual void start_sample(int voice,int note){
         //here goes the code when the note starts
            }
         
         virtual void next_instruction(int voice,int note){
         //here goes the code when you're sustaining the note
        }

         virtual void after_off(int voice,int note){
         //here goes the code when the note ends (here to play release for instance
         }
 };
 
 to activate the intrument
 
 SIDKeyBoardPlayer::changeAllInstruments<new_instrument>();
 
 ```

1)  Simple instrument
 ```
 
 #include "SID6581.h"
 #define SID_CLOCK 25
 #define SID_DATA 33
 #define SID_LATCH 27
 
 void playtunes()
 {
     for(int i=0;i<3;i++)
     {
         SIDKeyBoardPlayer::playNoteHz(0,220*(i+1),1500);
         while(SIDKeyBoardPlayer::isVoiceBusy(0));
         delay(100);
     }
 }
 
 class new_instrument:public sid_instrument{
         public:

         
         new_instrument()
         {
         
         }
         
         virtual void start_sample(int voice,int note)
         {
                 sid.setAttack(voice,0);
                 sid.setSustain(voice,1);
                 sid.setDecay(voice,10);
                 sid.setRelease(voice,1);
                 set.setPulse(voice,3000);
                 sid.setWaveForm(voice,SID_WAVEFORM_PULSE);
                 sid.setFrequency(voice,note);
                 sid.setGate(voice,1);
                 
         }
         
         virtual void next_instruction(int voice,int note)
         {
         
         //always add a vTaskDelay() to allow other task to work
            vTaskDelay(20);
         
         }
         
         virtual void after_off(int voice,int note){
                 
                 sid.setFrequency(voice,0);
                 sid.setGate(voice,0);                
         }
 
 
 };
 
 void setup() {
     // initialize serial:
     Serial.begin(115200);
     sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);
     SIDKeyBoardPlayer::KeyBoardPlayer(6);
     sid.sidSetVolume(0,15); 
     sid.sidSetVolume(1,15); //if you have two chips
     SIDKeyBoardPlayer::changeAllInstruments<new_instrument>();
     
 }
 
 void loop()
 {
     playtunes();
     delay(1000);
 }

```
2) something mode complicated

```
#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27

void playtunes()
{
    for(int i=0;i<3;i++)
    {
        SIDKeyBoardPlayer::playNoteHz(0,220*(i+1),1000);
        while(SIDKeyBoardPlayer::isVoiceBusy(0));
        delay(100);
    }
}

class new_instrument:public sid_instrument{
    public:

    int i;         
    new_instrument()
    {

    }

    virtual void start_sample(int voice,int note)
    {
        sid.setAttack(voice,0);
        sid.setSustain(voice,1);
        sid.setDecay(voice,10);
        sid.setRelease(voice,3);
        sid.setPulse(voice,3000);
        sid.setWaveForm(voice,SID_WAVEFORM_PULSE);
        sid.setFrequency(voice,note);
        sid.setGate(voice,1);
        i=0;

    }

    virtual void next_instruction(int voice,int note)
    {

        sid.setPulse(voice, 3000+500*cos(2*3.14*i/10)); //we make the pulse vary each time
        vTaskDelay(30);
        i=(i+1)%10;

    }

    virtual void after_off(int voice,int note){

        sid.setGate(voice,0);

    }


};

void setup() {
    // initialize serial:
    Serial.begin(115200);
    sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);
    SIDKeyBoardPlayer::KeyBoardPlayer(6);
    sid.sidSetVolume(0,15); 
    sid.sidSetVolume(1,15); //if you have two chips
    SIDKeyBoardPlayer::changeAllInstruments<new_instrument>();

}

void loop()
{
    playtunes();
    delay(1000);
}
```
 
 3) to go further
 if you have sample from a sid as a set of register calls you can do this
 ```
 class sid_piano:public sid_instrument{
     public:
         int i;
         int flo,fhi,plo,phi;
         sid_piano()
         {
         
            df2=sample1;
         }
         virtual void start_sample(int voice,int note)
         {
            i=0;
         }
         uint16_t *df2;
         virtual void next_instruction(int voice,int note)
         {
         
             i=(i+1)%1918;
             
             if(i==0)
             i=1651;
             
             
             switch(df2[i*3])
             {
                 case 0:
                    flo=df2[i*3+1];
                 break;
                 
                 
                 case 1:
                    fhi=df2[i*3+1];
                 sid.setFrequency(voice,((fhi*256+flo)*note/7977)); //i am modifyinhg the final note with rule if you do not modify this it will be always the same sound
                 
                 break;
                 
                 default:
                     if(df2[i*3]<7)
                     sid.pushRegister(voice/3,df2[i*3]+(voice%3)*7,df2[i*3+1]);
                 
                 break;
             }
             
             vTaskDelay(df2[i*3+2]/1000);
         
         }
         
         virtual void after_off(int voice,int note){
             
             sid.setFrequency(voice,0);
             sid.setGate(voice,0);
             i=0;
         }
 
 
 };
 ```


## MIDI

There is an example of a simple midi implementation. 

To plug the Midi to the esp32 please look around internet it will depend on what is available around you I use a 4N25 optocoupleur but you could find a lot of different implementations.

NB: the number  found for the change of the instruments are those found in my yamaha P-140 user guide.

Do not hesitate if you have questions


here it goes

Updated 18 March 2020
