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
}

void SID6581::playSIDTunes(fs::FS &fs,const char * path)
{
    resetsid();

    unsigned int sizet=readFile2(fs,path);
    if(sizet==0)
    {
        Serial.println("error reading the file");
        if(sidvalues==NULL)
            free(sidvalues);
    }
    uint8_t *d=sidvalues;
    sid_spi->beginTransaction(SPISettings(sid_spiClk, MSBFIRST, SPI_MODE0));
    for (uint32_t i=0;i<sizet;i++)
    {
       //Serial.printf("%d %d %d\n",*(uint16_t*)(d+2),*(uint8_t*)(d),*(uint8_t*)(d+1));
        setcsw();
        setA(*(uint8_t*)d);
        setD(*(uint8_t*)(d+1));
        clearcsw();

        if(*(uint16_t*)(d+2)>4)
            delayMicroseconds(*(uint16_t*)(d+2)-4);
        else
            delayMicroseconds(*(uint16_t*)(d+2));
        d+=4;
    }
    sid_spi->endTransaction();
    resetsid();
    free(sidvalues);

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
    Serial.println("- read from file:");
    unsigned int l = file.readBytesUntil('\n', buffer, sizeof(buffer));
    buffer[l] = 0;
    
    sscanf((const char *)buffer,"%lu",&sizet);
    Serial.printf("%d instructions\n",sizet);
    sidvalues=(uint8_t *)ps_malloc(sizet*4);
    if(sidvalues==NULL)
    {
        Serial.println("impossible to create memory buffer");
    }
    else
    {
        Serial.println("memory buffer ok");
    }
    l = file.read( sidvalues, sizet*4);//sizeof(SIDVALUES));
    Serial.printf("size :%u\n",l);
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
