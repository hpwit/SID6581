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
    
  
}

void loop() {
  //DO not forget to execute "ESP Sketch data load" to import the file in you SPIFF
  
 sid.playSIDTunes(SPIFFS,"/hurling.txt"); 
 //or sid.playSIDTunes(SD,"/hurling.txt") if you're using SD
 
}