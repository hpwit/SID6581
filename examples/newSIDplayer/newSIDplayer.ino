//#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27
#include "SPIFFS.h"
#include "FS.h"
#include "mos6501b.hpp"
   
//uint8_t mem[0xffff];
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
           cpu.addSong(SPIFFS,file.name()); //add all the files on the root of the spiff to the playlist
        }
        file = root.openNextFile();
    }

cpu.playTunes();

 Serial.println();
  Serial.printf("author:%s\n",cpu.getAuthor());
   Serial.printf("published:%s\n",cpu.getPublished());
    Serial.printf("name:%s\n",cpu.getName());
    Serial.printf("nb tunes:%d default tunes:%d\n",cpu.getNumberOfTunesInSid(),cpu.getDefaultTuneInSid());

   delay(5000);
 cpu.playNextSongInSid();
delay(5000);
 cpu.playNextSIDFile();
 delay(5000);
  Serial.println();
  Serial.printf("author:%s\n",cpu.getAuthor());
   Serial.printf("published:%s\n",cpu.getPublished());
    Serial.printf("name:%s\n",cpu.getName());
    Serial.printf("nb tunes:%d default tunes:%d\n",cpu.getNumberOfTunesInSid(),cpu.getDefaultTuneInSid());

}


void loop() {
 
Serial.printf("Frequency voice 1:%d voice 2:%d voice 3:%d\n",sid.getFrequency(0),sid.getFrequency(1),sid.getFrequency(2));
Serial.printf("Waveform voice 1:%d voice 2:%d voice 3:%d\n",sid.getWaveForm(0),sid.getWaveForm(1),sid.getWaveForm(2));
Serial.printf("Pulse voice 1:%d voice 2:%d voice 3:%d\n",sid.getPulse(0),sid.getPulse(1),sid.getPulse(2));

 vTaskDelay(100);
 
}