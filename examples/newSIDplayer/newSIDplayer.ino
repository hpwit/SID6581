#include "SID6581.h"
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
           // sid.addSong(SPIFFS,file.name()); //add all the files on the root of the spiff to the playlist
        }
        file = root.openNextFile();
    }



 cpu.play(SPIFFS,"/Warhawk.sid");
 // cpu.play(SPIFFS,"/Wizball.sid");
   
   
}


void loop() {
  //do not put anything here for the moment
  //the background playing is not good 
}
