
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27
#include "SPIFFS.h"
#include "FS.h"
#include "SidPlayer.h"

volatile bool debounceToggle=false;
volatile bool next_file=false;
volatile bool paused=false;
#define PIN_NEXT 5
#define PIN_PAUSE 19



SIDTunesPlayer * player;


void IRAM_ATTR ISR_Next() {
    if(debounceToggle || next_file)
      return;
       debounceToggle=true;  
   next_file=true;
;
    debounceToggle=false;
    
}

void IRAM_ATTR ISR_Pause() {
    if(debounceToggle || paused)
      return;
       debounceToggle=true;  
   paused=true;
    debounceToggle=false;
    
}


void myCallback(  sidEvent event ) {

  Serial.printf("event %d\n",event);
  switch( event ) {
   case SID_NEW_TRACK: 
     Serial.printf( "New track: %s\n",player->getFilename() );
   break;
   case SID_START_PLAY: 
     Serial.printf( "Start play: %s\n",player->getFilename() );
     break;
    case SID_END_PLAY: 
     Serial.printf( "stopping play: %s\n",player->getFilename() );
   break;
       case SID_PAUSE_PLAY: 
     Serial.printf( "pauqsing play: %s\n",player->getFilename() );
   break;
          case SID_RESUME_PLAY: 
     Serial.printf( "resume play: %s\n",player->getFilename() );
   break;
   
   case SID_END_SONG:
     Serial.println("End of track");
   break;
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
    pinMode(PIN_NEXT,INPUT_PULLDOWN);
    pinMode(PIN_PAUSE,INPUT_PULLDOWN);
     attachInterrupt(PIN_NEXT, ISR_Next, RISING);
     attachInterrupt(PIN_PAUSE, ISR_Pause, RISING);
  
      player=new SIDTunesPlayer();
   player->begin(SID_CLOCK,SID_DATA,SID_LATCH);
    player->setEventCallback(myCallback);
   
     
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
           player->addSong(SPIFFS,file.name()); //add all the files on the root of the spiff to the playlist
        }
        file = root.openNextFile();
    }

player->play();

 Serial.println();
  Serial.printf("author:%s\n",player->getAuthor());
   Serial.printf("published:%s\n",player->getPublished());
    Serial.printf("name:%s\n",player->getName());
    Serial.printf("nb tunes:%d default tunes:%d\n",player->getNumberOfTunesInSid(),player->getDefaultTuneInSid());

}


void loop() {
    
    if(next_file)
    {
        player->playNext();
        delay(500);
        Serial.println();
        Serial.printf("author:%s\n",player->getAuthor());
        Serial.printf("published:%s\n",player->getPublished());
        Serial.printf("name:%s\n",player->getName());
        Serial.printf("name:%s\n",player->getFilename());
        
        Serial.printf("nb tunes:%d default tunes:%d\n",player->getNumberOfTunesInSid(),player->getDefaultTuneInSid());
        Serial.printf("speed %d\n",player->speed);
        next_file=false;
    }
    if(paused)
    {
        player->pausePlay();
        delay(500);
        paused=false;
    }
}