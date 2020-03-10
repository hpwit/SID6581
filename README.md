This library is to control SID 6581 chips from the 1980s era with an Esp32.
The program allows you to push directly the register to the  SID chip. hence you can program like in the good all times :)
it should work with other mcu as it uses SPI but not tested.

You have full control of the SID chip
via the following command
```
void sidSetVolume( uint8_t vol);

void setFrequency(int voice, uint16_t frequency);
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


void setFilterFrequency(int filterfrequency);
void setResonance(int resonance);
void setFilter1(int filt1);
void setFilter2(int filt2);
void setFilter3(int filt3);
void setFilterEX(int filtex);
void set3OFF(int _3off);
void setHP(int hp);
void setBP(int bp);
void setLP(int lp);


For advance control:

void pushToVoice(int voice,uint8_t address,uint8_t data);

This function will allow you to push a data to a specific register off a specific voice

example:
pushToVoice(0,SID_FREG_HI,255) //to push 255 on the register FREQ_HI of voice 0
possible values:
        SID_FREQ_LO
        SID_FREQ_HI
        SID_PW_LO
        SID_PW_HI
        SID_CONTROL_REG
        SID_ATTACK_DECAY
        SID_SUSTAIN_RELEASE



void pushRegister(int address,int data);

This function will allow you to push directly a value to a register
example:

pushRegister(SID_MOD_VOL,15) //will put the sound at maximum
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
        SID_FC_HO
        SID_FC_HI
        SID_RES_FILT
        SID_MOD_VOL
NB:
    pushToVoice(0,SID_FREG_HI,255) == pushRegister(SID_FREQ_HI_0,255)



void resetsid();
this function will reset the SID chip

```
Below a code example

```
#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27
SID6581 sid;

void setup() {
    Serial.begin(115200);
    sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);

    sid.sidSetVolume(15); 
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



NB: playSIDTunes will only work with WROOVER because I use PSRAM for the moment. all the rest will run with all esp32.
I intend on writing a MIDI translation to SID chip. Hence a Play midi will be availble soon.

look at the schematics for the setup of the shift registers and  MOS 6581

Example to play a SIDtunes
```
#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27
#include "SPIFFS.h"
SID6581 sid;

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
sid.sidSetVolume(7); //value between 0 and 15


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
        sid.sidSetVolume(15);
        delay(3000);
        Serial.println("low volume ");
        sid.sidSetVolume(3);
        delay(3000);
        Serial.println("medium");
        sid.sidSetVolume(7);
        delay(3000);

        delay(3000);
        Serial.println("nexxt song");
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

here it goes

Updated 6 March 2020
