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



it should work with other mcu as it uses SPI but not tested.

NB: playSIDTunes will only work with WROOVER because I use PSRAM for the moment. all the rest will run with all esp32.
I intend on writing a MIDI translation to SID chip. Hence a Play midi will be availble soon.

Please look at the schematics for the setup of the shift registers and  MOS 6581

## To start
```
The object sid is automatically created hence to start the sid controller
sid.begin(int clock_pin,int data_pin, int latch );

```


## To play a SIDtunes
You can play SIDTunes stored on the SPIFF or the SD card this will play on chip 0

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
#include "SPIFFS.h"

void setup() {
// put your setup code here, to run once:
Serial.begin(115200);

sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);

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
sid.sidSetVolume(0,15);
delay(3000);
Serial.println("low volume ");
sid.sidSetVolume(0,3);
delay(3000);
Serial.println("medium");
sid.sidSetVolume(0,7);
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






here it goes

Updated 18 March 2020
