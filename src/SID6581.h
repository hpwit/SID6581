//
//  SID6581.h
//  
//
//  Created by Yves BAZIN on 06/03/2020.
//

#ifndef SID6581_h
#define SID6581_h

#define CS 2
#define WRITE 1
#define RESET 0
#define MASK_ADDRESS 0b11111000
#define MASK_CSWRRE 0b111


#include "SPI.h"
#include "FS.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

 static TaskHandle_t xPlayerTaskHandle = NULL;
 static TaskHandle_t SIDPlayerTaskHandle = NULL;
static TaskHandle_t SIDPlayerLoopTaskHandle = NULL;
static TaskHandle_t PausePlayTaskLocker = NULL;

struct songstruct{
    fs::FS  *fs;
     char filename[80];
};
class SID6581 {
public:
    SID6581();
    bool begin(int clock_pin,int data_pin, int latch );
    void play();
    void playNext();
    void playPrev();
    void soundOff();
    void soundOn();
    void pausePlay();
    void sidSetVolume( uint8_t volume);
    int latch_pin;
    void addSong(fs::FS &fs,  const char * path);
    void playSongNumber(int number);
    void stopPlayer();
    void setVoice(uint8_t vo);
    
protected:
    void stop();
    uint8_t voice=7;
    void playNextInt();
    bool stopped=false;
    int numberToPlay=0;
    int numberOfSongs=0;
    int currentSong=0;
    bool paused=false;
    const int _maxnumber=80;
    void feedTheDog();
    static void playSIDTunesTask(void *pvParameters);
    static void playSIDTunesLoopTask(void *pvParameters);
    songstruct listsongs[80];
    uint8_t save24;
    uint8_t volume;
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
