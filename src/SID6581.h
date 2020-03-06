//
//  SID6581.h
//  
//
//  Created by Yves BAZIN on 06/03/2020.
//

#ifndef SID6581_h
#define SID6581_h

#define CS 7
#define WRITE 6
#define RESET 5
#define MASK_ADDRESS 31
#define MASK_CSWRRE 0b11100000


#include "SPI.h"
#include "FS.h"


class SID6581 {
public:
    SID6581();
    bool begin(int clock_pin,int data_pin, int latch );
    void playSIDTunes(fs::FS &fs,const char * path);
    int latch_pin;
    
protected:
    uint8_t * sidvalues;
    unsigned int readFile2(fs::FS &fs, const char * path);
    uint8_t adcswrre,dataspi;
    void clearcsw();
    void resetsid();
    void push();
    
    void setA(uint8_t val);
    void setcsw();
    void setD(uint8_t val);
    void cls();
    SPIClass * sid_spi = NULL;
    const int sid_spiClk = 20000000;
    
    
};

#endif /* SID6581_h */
