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
    _sid_queue = xQueueCreate( SID_QUEUE_SIZE, sizeof( _sid_register_to_send ) );
    xTaskCreate(SID6581::_pushRegister, "_pushRegister", 2048, this,1, &xPushToRegisterHandle);
    
    // sid_spi->beginTransaction(SPISettings(sid_spiClk, LSBFIRST, SPI_MODE0));
    resetsid();
    volume=15;
    numberOfSongs=0;
    currentSong=0;
    
}

void SID6581::sidSetMaxVolume( uint8_t vol)
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
        //sid_spi->endTransaction();
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
    uint32_t counttime=0;
    uint32_t counttime1=0;
    
    //sid->sid_spi->beginTransaction(SPISettings(sid->sid_spiClk, LSBFIRST, SPI_MODE0));
    for (uint32_t i=0;i<sizet;i++)
    {
        //Serial.printf("%d %d %d\n",*(uint16_t*)(d+2),*(uint8_t*)(d),*(uint8_t*)(d+1));
       
        
        if(*(uint8_t*)d==24) //we delaonf with the sound
        {
            sid->save24=*(uint8_t*)(d+1);
            uint8_t value=*(uint8_t*)(d+1);
            value=value& 0xf0 +( ((value& 0x0f)*sid->volume)/15)  ;
            *(uint8_t*)(d+1)=value;
        }
         sid->pushRegister(0,*(uint8_t*)d,*(uint8_t*)(d+1));

//        if( ( *(uint8_t*)d >=0  && *(uint8_t*)d <7 ) || *(uint8_t*)d>20)
//        {
//            Serial.printf(" %d,%d,",*(uint8_t*)d,*(uint8_t*)(d+1));
//                sid->pushRegister(0,*(uint8_t*)d,*(uint8_t*)(d+1));
//            Serial.printf("%d,", counttime1-counttime);
//            counttime=counttime1;
//        }
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
        
        //counttime1= counttime1 + *(uint16_t*)(d+2);
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
    //sid->sid_spi->endTransaction();
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
    clearcsw(0);
    
}
/*
 struct _sid_voice
 {
 uint16_t frequency;
 uint16_t pulse;
 uint8_t waveform;
 uint8_t attack;
 uint8_t decay;
 uint8_t sustain;
 uint8_t release;
 
 };*/


void SID6581::setFrequencyHz(int voice,double frequencyHz)
{
    //we do calculate the fréquency used by the 6581
    //Fout = (Fn * Fclk/16777216) Hz
    //Fn=Fout*16777216/Fclk here Fclk is the speed of you clock 1Mhz
    double fout=frequencyHz*16.777216;
    setFrequency(voice,round(fout));
}

void SID6581::setFrequency(int voice, uint16_t frequency)
{
//    if(voice <0 or voice >2)
//        return;
    voices[voice].frequency=frequency;
    voices[voice].freq_lo=frequency & 0xff;
    voices[voice].freq_hi=(frequency >> 8) & 0xff;
    // Serial.println(voices[voice].freq_hi);
    pushToVoice(voice,0,voices[voice].freq_lo);
    pushToVoice(voice,1,voices[voice].freq_hi);
}
void SID6581::setPulse(int voice, uint16_t pulse)
{
//    if(voice <0 or voice >2)
//        return;
    voices[voice].pulse=pulse;
    voices[voice].pw_lo=pulse & 0xff;
    voices[voice].pw_hi=(pulse >> 8) & 0x0f;
    pushToVoice(voice,2,voices[voice].pw_lo);
    pushToVoice(voice,3,voices[voice].pw_hi);
}
void SID6581::setEnv(int voice, uint8_t att,uint8_t decay,uint8_t sutain, uint8_t release)
{
//    if(voice <0 or voice >2)
//        return;
    setAttack(voice,  att);
    setDecay(voice,  decay);
    setSustain(voice, sutain);
    setRelease(voice, release);
    
}

void SID6581::setAttack(int voice, uint8_t att)
{
//    if(voice <0 or voice >2)
//        return;
    voices[voice].attack=att;
    voices[voice].attack_decay=(voices[voice].attack_decay & 0x0f) +((att<<4) & 0xf0);
    pushToVoice(voice,5,voices[voice].attack_decay);
}
void SID6581::setDecay(int voice, uint8_t decay)
{
//    if(voice <0 or voice >2)
//        return;
    voices[voice].decay=decay;
    voices[voice].attack_decay=(voices[voice].attack_decay & 0xf0) +(decay & 0x0f);
    pushToVoice(voice,5,voices[voice].attack_decay);
}

void SID6581::setSustain(int voice,uint8_t sustain)
{
//    if(voice <0 or voice >2)
//        return;
    voices[voice].sustain=sustain;
    voices[voice].sustain_release=(voices[voice].sustain_release & 0x0f) +((sustain<<4) & 0xf0);
    pushToVoice(voice,6,voices[voice].sustain_release);
}
void SID6581::setRelease(int voice,uint8_t release)
{
//    if(voice <0 or voice >2)
//        return;
    voices[voice].release=release;
    voices[voice].sustain_release=(voices[voice].sustain_release & 0xf0) +(release & 0x0f);
    pushToVoice(voice,6,voices[voice].sustain_release);
}


void SID6581::setGate(int voice, int gate)
{
//    if(voice <0 or voice >2)
//        return;
    voices[voice].gate=gate;
    voices[voice].control_reg=(voices[voice].control_reg & 0xfE) + (gate & 0x1);
    pushToVoice(voice,4,voices[voice].control_reg);
    
}


void SID6581::setTest(int voice,int test)
{
//    if(voice <0 or voice >2)
//        return;
    voices[voice].test=test;
    voices[voice].control_reg=(voices[voice].control_reg & (0xff ^ SID_TEST )) + (test & SID_TEST);
    pushToVoice(voice,4,voices[voice].control_reg);
}

void SID6581::setSync(int voice,int sync)
{
//    if(voice <0 or voice >2)
//        return;
    voices[voice].sync=sync;
    voices[voice].control_reg=(voices[voice].control_reg & (0xff ^ SID_SYNC )) + (sync & SID_SYNC);
    pushToVoice(voice,4,voices[voice].control_reg);
}

void SID6581::setRingMode(int voice, int ringmode)
{
//    if(voice <0 or voice >2)
//        return;
    voices[voice].ringmode=ringmode;
    voices[voice].control_reg=(voices[voice].control_reg & (0xff ^ SID_RINGMODE)) + (ringmode & SID_RINGMODE);
    pushToVoice(voice,4,voices[voice].control_reg);
    
}


void SID6581::setWaveForm(int voice,int waveform)
{
//    if(voice <0 or voice >2)
//        return;
    voices[voice].waveform=waveform;
    voices[voice].control_reg= waveform + (voices[voice].control_reg & 0x01);
    pushToVoice(voice,4,voices[voice].control_reg);
    
}

//uint8_t volume;
//uint8_t filterfrequency;
//uint8_t res;
//uint8_t filt1,filt2,filt3,filtex;
//uint8_t _3off,hp,bp,lp;
//uint8_t fc_lo,fc_hi,res_filt,mode_vol;

void SID6581::set3OFF(int chip,int _3off)
{
    sid_control[chip]._3off=_3off;
    sid_control[chip].mode_vol=(sid_control[chip].mode_vol & (0xff ^ SID_3OFF )) + (_3off & SID_3OFF);
    pushRegister(chip,0x18,sid_control[chip].mode_vol);
}
void SID6581::setHP(int chip,int hp)
{
    sid_control[chip].hp=hp;
    sid_control[chip].mode_vol=(sid_control[chip].mode_vol & (0xff ^ SID_HP )) + (hp & SID_HP);
    pushRegister(chip,0x18,sid_control[chip].mode_vol);
}
void SID6581::setBP(int chip,int bp)
{
    sid_control[chip].bp=bp;
    sid_control[chip].mode_vol=(sid_control[chip].mode_vol & (0xff ^ SID_BP )) + (bp & SID_BP);
    pushRegister(chip,0x18,sid_control[chip].mode_vol);
}
void SID6581::setLP(int chip,int lp)
{
    sid_control[chip].lp=lp;
    sid_control[chip].mode_vol=(sid_control[chip].mode_vol & (0xff ^ SID_LP )) + (lp & SID_LP);
    pushRegister(chip,0x18,sid_control[chip].mode_vol);
}

void SID6581::sidSetVolume(int chip, uint8_t vol)
{
    sid_control[chip].volume=vol;
    sid_control[chip].mode_vol=(sid_control[chip].mode_vol & 0xf0 )+( vol & 0x0F);
    pushRegister(chip,0x18,sid_control[chip].mode_vol);
}



void  SID6581::setFilterFrequency(int chip,int filterfrequency)
{
    sid_control[chip].filterfrequency=filterfrequency;
    sid_control[chip].fc_lo=filterfrequency & 0b111;
    sid_control[chip].fc_hi=(filterfrequency>>3) & 0xff;
    pushRegister(chip,0x15,sid_control[chip].fc_lo);
    pushRegister(chip,0x16,sid_control[chip].fc_hi);
}

void SID6581::setResonance(int chip,int resonance)
{
    sid_control[chip].res=resonance;
    sid_control[chip].res_filt=(sid_control[chip].res_filt & 0x0f) +(resonance<<4);
    pushRegister(chip,0x17,sid_control[chip].res_filt);
}

void SID6581::setFilter1(int chip,int filt1)
{
    sid_control[chip].filt1;
    sid_control[chip].res_filt=(sid_control[chip].res_filt & (0xff ^ SID_FILT1 )) + (filt1 & SID_FILT1);
    pushRegister(chip,0x17,sid_control[chip].res_filt);
}
void SID6581::setFilter2(int chip,int filt2)
{
    sid_control[chip].filt2;
    sid_control[chip].res_filt=(sid_control[chip].res_filt & (0xff ^ SID_FILT2 )) + (filt2 & SID_FILT2);
    pushRegister(chip,0x17,sid_control[chip].res_filt);
}
void SID6581::setFilter3(int chip,int filt3)
{
    sid_control[chip].filt3;
    sid_control[chip].res_filt=(sid_control[chip].res_filt & (0xff ^ SID_FILT3 )) + (filt3 & SID_FILT3);
    pushRegister(chip,0x17,sid_control[chip].res_filt);
}
void SID6581::setFilterEX(int chip,int filtex)
{
    sid_control[chip].filtex;
    sid_control[chip].res_filt=(sid_control[chip].res_filt & (0xff ^ SID_FILTEX )) + (filtex & SID_FILTEX);
    pushRegister(chip,0x17,sid_control[chip].res_filt);
}

void SID6581::soundOn()
{
    setcsw();
    setA(24);
    setD(save24);
    clearcsw(0);
    
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

void SID6581::clearcsw(int chip)
{
    //adcswrre = (adcswrre ^ (1<<WRITE) ) ^ (1<<CS) ;
   
    switch(chip)
    {
        case 0:
            //Serial.println("jj");
                    adcswrre = (adcswrre ^ (1<<WRITE) ) ^ (1<<CS) ;
            break;
        case 1:
           // Serial.println("on chip 1");
            chipcontrol=  0xff ^ ((1<<WRITE_1) | (1<<CS_1));
            break;
        case 2:
            // Serial.println("on chip 1");
            chipcontrol=  0xff ^ ((1<<WRITE_2) | (1<<CS_2));
            break;
        case 3:
            // Serial.println("on chip 1");
            chipcontrol=  0xff ^ ((1<<WRITE_3) | (1<<CS_3));
            break;
        
        case 4:
            // Serial.println("on chip 1");
            chipcontrol=  0xff ^ ((1<<WRITE_4) | (1<<CS_4));
            break;
    }
    push();
}

void SID6581::setcsw()
{
    adcswrre = adcswrre | (1<<WRITE) | (1<<CS);
    chipcontrol= 0xff;//(1<<WRITE_1) | (1<<CS_1);;
    push();
}

void SID6581::resetsid()
{
    cls();
    adcswrre=0;
    chipcontrol=0;
    push();
    delay(2);
    adcswrre=(1<< RESET);
    push();
    delay(2);
}


void SID6581::pushRegister(int chip,int address,int data)
{
    /*
    setcsw();
    //delay(1);
    //address=7*voice+address;
    setA(address);
    setD(data);
    
    clearcsw(chip);*/
    _sid_register_to_send sid_data;
    sid_data.address=address;
    sid_data.chip=chip;
    sid_data.data=data;
    xQueueSend(_sid_queue, &sid_data, portMAX_DELAY);
}


void SID6581::_pushRegister(void *pvParameters)
{
    SID6581 * sid= (SID6581 *)pvParameters;
    
    _sid_register_to_send sid_data;
    for(;;)
    {
         xQueueReceive(_sid_queue, &sid_data, portMAX_DELAY);
        sid->setcsw();
        //delay(1);
        //address=7*voice+address;
        sid->setA(sid_data.address);
        sid->setD(sid_data.data);
        
        sid->clearcsw(sid_data.chip);
    }

}


void SID6581::pushToVoice(int voice,uint8_t address,uint8_t data)
{
    
    // sid_spi->beginTransaction(SPISettings(sid_spiClk, LSBFIRST, SPI_MODE0));
    
   // setcsw();
    //delay(1);
    //Serial.printf("v:%d c:%d",voice,(int)(voice/3));
    address=7*(voice%3)+address;
    pushRegister((int)(voice/3),address,data);
   // setA(address);
    //setD(data);
    
    //clearcsw();
    //sid_spi->endTransaction();
    //delay(1);
    //Serial.printf("push %d %d\n",address,data);
    //REG_WRITE(GPIO_OUT_W1TS_REG,GPIO_OUT_W1TS_REG| (1<<27));
}

void SID6581::push()
{
    
    sid_spi->beginTransaction(SPISettings(sid_spiClk, LSBFIRST, SPI_MODE0));
    digitalWrite(latch_pin, 0);
    sid_spi->transfer(chipcontrol);
    sid_spi->transfer(adcswrre);
    sid_spi->transfer(dataspi);
    sid_spi->endTransaction();
    digitalWrite(latch_pin, 1);
    
}

void SID6581::setA(uint8_t val)
{
    adcswrre=((val<<3) & MASK_ADDRESS) | (adcswrre & MASK_CSWRRE);
}

void SID6581::setD(uint8_t val)
{
    dataspi=val;
}
void SID6581::cls()
{
    dataspi=0;
}
