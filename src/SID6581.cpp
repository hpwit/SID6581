//
//  SID6581.cpp
//  
//
//  Created by Yves BAZIN on 06/03/2020.
//

#include "SID6581.h"
#include "FS.h"
#include "SPI.h"
#include "Arduino.h"

#define CS 7
#define WRITE 6
#define RESET 5
#define MASK_ADDRESS 31
#define MASK_CSWRRE 0b11100000

SID6581::SID6581(){}

bool SID6581::begin(int clock_pin,int data_pin, int latch )
{
    /*
     We set up the spi bus which connects to the 74HC595
     */
    sid_spi = new SPIClass(HSPI);
    if(sid_spi==NULL)
        return false;
     sid_spi->begin(clock_pin,NULL,data_pin,NULL);
    latch_pin=latch;
     pinMode(latch, OUTPUT);
    Serial.println("SID Initialized");
    volume=15;
    numberOfSongs=0;
    currentSong=0;
}

void SID6581::sidSetVolume( uint8_t vol)
{
    volume=vol;
}


void SID6581::addSong(fs::FS &fs,  const char * path)
{
    songstruct p1;
    p1.fs=(fs::FS *)&fs;
    //char h[250];
    sprintf(p1.filename,"%s",path);
    //p1.filename=h;
    listsongs[numberOfSongs]=p1;
    numberOfSongs++;
}


void SID6581::pausePlay()
{
    
    if(xPlayerTaskHandle!=NULL)
    {
        
        if(!paused)
        {

            paused=true;
        }
        else
        {
            soundOn();
        
            paused=false;
            if(PausePlayTaskLocker!=NULL)
                xTaskNotifyGive(PausePlayTaskLocker);
        }
    }
}

void SID6581::playSIDTunesLoopTask(void *pvParameters)
{
    SID6581 * sid= (SID6581 *)pvParameters;
    
    
    while(1 && !sid->stopped)
    {
        if(SIDPlayerTaskHandle  == NULL)
        {
            sid->playNextInt();
            SIDPlayerTaskHandle  = xTaskGetCurrentTaskHandle();
            //yield();
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            SIDPlayerTaskHandle  = NULL;
        }
    }
    SIDPlayerLoopTaskHandle=NULL;
    SIDPlayerTaskHandle=NULL;
    vTaskDelete(SIDPlayerLoopTaskHandle);
}

void SID6581::play()
{
    if(SIDPlayerLoopTaskHandle!=NULL)
    {
        Serial.println("allready Playing");
        return;
    }
    stopped=false;
    paused=false;
    currentSong=numberOfSongs-1;
    xTaskCreate(SID6581::playSIDTunesLoopTask, "playSIDTunesLoopTask", 4096, this,1, &SIDPlayerLoopTaskHandle);
}

void SID6581::stopPlayer()
{
    stop();
    stopped=true;
    if(SIDPlayerTaskHandle!=NULL)
        xTaskNotifyGive(SIDPlayerTaskHandle);
    SIDPlayerLoopTaskHandle=NULL;
    SIDPlayerTaskHandle=NULL;
}

void SID6581::stop()
{
    if(xPlayerTaskHandle!=NULL)
    {
        //we stop the current song
        //we unlock the pause locker in case of
        if(PausePlayTaskLocker!=NULL)
            xTaskNotifyGive(PausePlayTaskLocker);
        soundOff();
        resetsid();
        free(sidvalues);
        vTaskDelete(xPlayerTaskHandle);
        xPlayerTaskHandle=NULL;
        sid_spi->endTransaction();
    }
    
}



void SID6581::playNext()
{
    stop();
    if(SIDPlayerTaskHandle!=NULL)
            xTaskNotifyGive(SIDPlayerTaskHandle);
    
}

void SID6581::playPrev()
{
    stop();
    if(SIDPlayerTaskHandle!=NULL)
    {
        if(currentSong==0)
            currentSong=numberOfSongs-2;
        else
            currentSong=currentSong-2;
        xTaskNotifyGive(SIDPlayerTaskHandle);
    }
    
   
}


void SID6581::playNextInt()
{
    stop();
   
    currentSong=(currentSong+1)%numberOfSongs;
    playSongNumber(currentSong);

}

void SID6581::playSongNumber(int number)
{
    if(xPlayerTaskHandle!=NULL)
    {
        Serial.println("allready Playing");
        return;
    }
    if(number>_maxnumber)
    {
        Serial.println("Id too long");
        return;
    }
   /* if( listsongs[number]==0)
    {
        Serial.println("Non existing song");
        return;

    }*/
    Serial.println("playing song");

    paused=false;
    numberToPlay=number;
    xTaskCreate(SID6581::playSIDTunesTask, "playSIDTunesTask", 4096, this,1, &xPlayerTaskHandle);
    
}

void SID6581::setVoice(uint8_t vo)
{
    voice=vo;
}
 void SID6581::playSIDTunesTask(void *pvParameters)
{
    SID6581 * sid= (SID6581 *)pvParameters;
    sid->resetsid();
    songstruct p;
    p=sid->listsongs[sid->numberToPlay];
    unsigned int sizet=sid->readFile2(*p.fs,p.filename);
    if(sizet==0)
    {
        Serial.println("error reading the file");
        if(sid->sidvalues==NULL)
            free(sid->sidvalues);
        xPlayerTaskHandle=NULL;
        if(SIDPlayerTaskHandle!=NULL)
            xTaskNotifyGive(SIDPlayerTaskHandle);
        vTaskDelete(NULL);
       
    }
    
    Serial.printf("palying:%s\n",p.filename);
    uint8_t *d=sid->sidvalues;
    sid->sid_spi->beginTransaction(SPISettings(sid->sid_spiClk, MSBFIRST, SPI_MODE0));
    for (uint32_t i=0;i<sizet;i++)
    {
       //Serial.printf("%d %d %d\n",*(uint16_t*)(d+2),*(uint8_t*)(d),*(uint8_t*)(d+1));
        sid->setcsw();
        
        if(*(uint8_t*)d==24) //we delaonf with the sound
        {
            sid->save24=*(uint8_t*)(d+1);
            uint8_t value=*(uint8_t*)(d+1);
            value=value& 0xf0 +( ((value& 0x0f)*sid->volume)/15)  ;
            *(uint8_t*)(d+1)=value;
        }
        
        sid->setA(*(uint8_t*)d);
        sid->setD(*(uint8_t*)(d+1));
        sid->clearcsw();
        
    /* code for voice selection needes to be refined
        int ad=*(uint8_t*)d;
        
        if(ad<=6 && (sid->voice&1))
        {
            sid->setA(*(uint8_t*)d);
            sid->setD(*(uint8_t*)(d+1));
            sid->clearcsw();
        }
        else
        {
            if(ad>=7 && ad <=113 && (sid->voice&2))
            {
                sid->setA(*(uint8_t*)d);
                sid->setD(*(uint8_t*)(d+1));
                sid->clearcsw();
            }
            else
            {
                if(ad>=14 && ad<=20  && (sid->voice&4))
                {
                    sid->setA(*(uint8_t*)d);
                    sid->setD(*(uint8_t*)(d+1));
                    sid->clearcsw();
                }
                else
                {
                    if(ad>20)
                    {
                        sid->setA(*(uint8_t*)d);
                        sid->setD(*(uint8_t*)(d+1));
                        sid->clearcsw();
                    }
                }
            }
        }
        */


        if(*(uint16_t*)(d+2)>4)
            delayMicroseconds(*(uint16_t*)(d+2)-4);
        else
            delayMicroseconds(*(uint16_t*)(d+2));
        d+=4;
        sid->feedTheDog();
        if(sid->paused)
        {
            sid->soundOff();
            PausePlayTaskLocker  = xTaskGetCurrentTaskHandle();
            //yield();
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            PausePlayTaskLocker=NULL;
        }
    }
    sid->sid_spi->endTransaction();
    sid->resetsid();
    free(sid->sidvalues);
    Serial.println("end Play");
    xPlayerTaskHandle=NULL;
    if(SIDPlayerTaskHandle!=NULL)
        xTaskNotifyGive(SIDPlayerTaskHandle);
    vTaskDelete(NULL);

}
void SID6581::feedTheDog(){
    // feed dog 0
    TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE; // write enable
    TIMERG0.wdt_feed=1;                       // feed dog
    TIMERG0.wdt_wprotect=0;                   // write protect
    // feed dog 1
    TIMERG1.wdt_wprotect=TIMG_WDT_WKEY_VALUE; // write enable
    TIMERG1.wdt_feed=1;                       // feed dog
    TIMERG1.wdt_wprotect=0;                   // write protect
}

void SID6581::soundOff()
{
    setcsw();
    setA(24);
    setD(save24 &0xf0);
    clearcsw();
    
}


void SID6581::soundOn()
{
    setcsw();
    setA(24);
    setD(save24);
    clearcsw();
    
}
unsigned int SID6581::readFile2(fs::FS &fs, const char * path)
{
    unsigned int sizet;
    Serial.printf("Reading file: %s\r\n", path);
    
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return 0;
    }
    char buffer[30];
    unsigned int l = file.readBytesUntil('\n', buffer, sizeof(buffer));
    buffer[l] = 0;
    
    sscanf((const char *)buffer,"%lu",&sizet);
    Serial.printf("%d instructions\n",sizet);
    sidvalues=(uint8_t *)ps_malloc(sizet*4);
    if(sidvalues==NULL)
    {
        Serial.println("impossible to create memory buffer");
        return 0;
    }
    else
    {
        Serial.println("memory buffer ok");
    }
    l = file.read( sidvalues, sizet*4);//sizeof(SIDVALUES));
    return sizet;
}

void SID6581::clearcsw()
{
    adcswrre = (adcswrre ^ (1<<WRITE) ) ^ (1<<CS) ;
    push();
}
void SID6581::resetsid()
{
    cls();
    adcswrre=0;
    push();
    delay(2);
    adcswrre=(1<< RESET);
    push();
    delay(2);
}
void SID6581::push()
{
    
    digitalWrite(latch_pin, 0);
    //REG_WRITE(GPIO_OUT_W1TC_REG,GPIO_OUT_W1TC_REG| (1<<27));
    //Serial.println("ee");
    //delay(10);
    sid_spi->transfer(adcswrre);
    sid_spi->transfer(dataspi);
    digitalWrite(latch_pin, 1);
    //REG_WRITE(GPIO_OUT_W1TS_REG,GPIO_OUT_W1TS_REG| (1<<27));
}
void SID6581::setA(uint8_t val)
{
     adcswrre=(val & MASK_ADDRESS) | (adcswrre & MASK_CSWRRE);
}
void SID6581::setcsw()
{
    adcswrre = adcswrre | (1<<WRITE) | (1<<CS);
    push();
}
void SID6581::setD(uint8_t val)
{
    dataspi=val;
}
void SID6581::cls()
{
    dataspi=0;
}
