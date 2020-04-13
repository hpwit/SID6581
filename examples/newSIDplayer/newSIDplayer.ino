#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27
#include "SPIFFS.h"
#include "FS.h"
#include "SidPlayer.h"

SIDTunesPlayer * player;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    player=new SIDTunesPlayer();
    player->begin(SID_CLOCK,SID_DATA,SID_LATCH);

    if(!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    //the following line will go through all the files in the SPIFFS
    //Do not forget to do "Tools-> ESP32 Scketch data upload"
    player->addSongsFromFolder(SPIFFS,"/"); //add all the songs in the root directory 
    //player->addSongsFromFolder(SPIFFS,"/",".sid",true); //if you want to parse the directories recursively

    //list all information of the songs
  player->getSongslengthfromMd5(SPIFFS,"/soundlength.md5"); //this could take a bit of time so just be patient :)

    for(int i=0;i<player->getPlaylistSize();i++)
    {
        songstruct song=player->getSidFileInfo(i);
        Serial.printf("%s %s %s %s %s %d %d \n",song.filename,song.name,song.author,song.published,song.md5,song.subsongs,song.startsong);
        for(int g=0;g<song.subsongs;g++)
            {
                Serial.printf("        song n:%d  duration:%d\n",g,song.durations[g]);
            }


    }
  
    
    player->play();

    Serial.println();
    Serial.printf("author:%s\n",player->getAuthor());
    Serial.printf("published:%s\n",player->getPublished());
    Serial.printf("name:%s\n",player->getName());
    Serial.printf("nb tunes:%d default tunes:%d\n",player->getNumberOfTunesInSid(),player->getDefaultTuneInSid());

    delay(5000);
    player->playNextSongInSid();
    delay(5000);
    player->playNext();
    delay(5000);
    Serial.println();
    Serial.printf("author:%s\n",player->getAuthor());
    Serial.printf("published:%s\n",player->getPublished());
    Serial.printf("name:%s\n",player->getName());
    Serial.printf("nb tunes:%d default tunes:%d\n",player->getNumberOfTunesInSid(),player->getDefaultTuneInSid());
}


void loop() {
    delay(5000);
    Serial.println("Pause the song");
    player->pausePlay();
    delay(4000);
    Serial.println("restart the song");
    player->pausePlay();
    delay(3000);
    Serial.println("hi volume");
    player->SetMaxVolume(15);
    delay(3000);
    Serial.println("low volume ");
    player->SetMaxVolume(3);
    delay(3000);
    Serial.println("medium");
    player->SetMaxVolume(7);
    delay(6000);
    Serial.println("next song");
    player->playNext(); //sid.playPrev(); if you want to go backwards 
    delay(10000);
    //player->stopPlayer(); //to stop the plater completely
    //delay(10000);
    //player->play(); //to restart it
}