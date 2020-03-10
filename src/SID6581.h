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
#define SID_WAVEFORM_TRIANGLE (1<<4)
#define SID_WAVEFORM_SAWTOOTH (1<<5)
#define SID_WAVEFORM_PULSE (1<<6)
#define SID_WAVEFORM_NOISE (1<<7)
#define SID_GATE 0
#define SID_TEST (1<<3)
#define SID_SYNC 1
#define SID_RINGMODE (1<<2)
#define SID_3OFF (1<<7)
#define SID_HP (1<<6)
#define SID_BP (1<<5)
#define SID_LP (1<<4)
#define SID_FILT1 1
#define SID_FILT2 2
#define SID_FILT3 4
#define SID_FILTEX 8
#define SID_FREQ_LO 0
#define SID_FREQ_HI 1
#define SID_PW_LO 2
#define SID_PW_HI 3
#define SID_CONTROL_REG 4
#define SID_ATTACK_DECAY 5
#define SID_SUSTAIN_RELEASE 6

#define SID_FREQ_LO_0 0
#define SID_FREQ_HI_0 1
#define SID_PW_LO_0 2
#define SID_PW_HI_0 3
#define SID_CONTROL_REG_0 4
#define SID_ATTACK_DECAY_0 5
#define SID_SUSTAIN_RELEASE_0 6

#define SID_FREQ_LO_1 7
#define SID_FREQ_HI_1 8
#define SID_PW_LO_1 9
#define SID_PW_HI_1 10
#define SID_CONTROL_REG_1 11
#define SID_ATTACK_DECAY_1 12
#define SID_SUSTAIN_RELEASE_1 13

#define SID_FREQ_LO_2 14
#define SID_FREQ_HI_2 15
#define SID_PW_LO_2 16
#define SID_PW_HI_2 17
#define SID_CONTROL_REG_2 18
#define SID_ATTACK_DECAY_2 19
#define SID_SUSTAIN_RELEASE_2 20



#define SID_FC_HO 21
#define SID_FC_HI 22
#define SID_RES_FILT 23
#define SID_MOD_VOL 24

#include "SPI.h"
#include "FS.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

static TaskHandle_t xPlayerTaskHandle = NULL;
static TaskHandle_t SIDPlayerTaskHandle = NULL;
static TaskHandle_t SIDPlayerLoopTaskHandle = NULL;
static TaskHandle_t PausePlayTaskLocker = NULL;


struct _sid_voice
{
    uint16_t frequency;
    uint16_t pulse;
    uint8_t waveform;
    uint8_t attack;
    uint8_t decay;
    uint8_t sustain;
    uint8_t release;
    uint8_t gate;
    uint8_t sync;
    uint8_t test;
    uint8_t ringmode;
    uint8_t freq_lo,freq_hi,pw_lo,pw_hi,control_reg,attack_decay,sustain_release;
    
};
struct _sid_control
{
    uint8_t volume;
    uint8_t filterfrequency;
    uint8_t res;
    uint8_t filt1,filt2,filt3,filtex;
    uint8_t _3off,hp,bp,lp;
    uint8_t fc_lo,fc_hi,res_filt,mode_vol;
    
    
};

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
    void sidSetMaxVolume( uint8_t volume);
    void addSong(fs::FS &fs,  const char * path);
    void playSongNumber(int number);
    void stopPlayer();
    void setVoice(uint8_t vo);
    
    void sidSetVolume( uint8_t vol);
    void setFrequency(int voice, uint16_t frequency);
    void setFrequencyHz(int voice,double frequencyHz);
    void setPulse(int voice, uint16_t pulse);
    void setEnv(int voice, uint8_t att,uint8_t decay,uint8_t sutain, uint8_t release);
    void setAttack(int voice, uint8_t att);
    void setDecay(int voice, uint8_t decay);
    void setSustain(int voice,uint8_t sutain);
    void setRelease(int voice,uint8_t release);
    void setGate(int voice, int gate);
    void setWaveForm(int voice,int waveform);
    void setTest(int voice,int test);
    void setSync(int voice,int sync);
    void setRingMode(int voice, int ringmode);
    void setFilterFrequency(int filterfrequency);
    void setResonance(int resonance);
    void setFilter1(int filt1);
    void setFilter2(int filt2);
    void setFilter3(int filt3);
    void setFilterEX(int filtex);
    void set3OFF(int _3off);
    void setHP(int hp);
    void setBP(int bp);
    void setLP(int lp);
    
    void pushToVoice(int voice,uint8_t address,uint8_t data);
    void pushRegister(int address,int data);
    void resetsid();
    
protected:
    
    int  latch_pin;
    void stop();
    uint8_t voice=7;
    _sid_control sid_control;
    void playNextInt();
    bool stopped=false;
    int numberToPlay=0;
    int numberOfSongs=0;
    int currentSong=0;
    bool paused=false;
    _sid_voice voices[3];
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
    //void resetsid();
    void push();
    
    void setA(uint8_t val);
    void setcsw();
    void setD(uint8_t val);
    void cls();
    SPIClass * sid_spi = NULL;
    const int sid_spiClk = 20000000;
    
    
};

#endif /* SID6581_h */
