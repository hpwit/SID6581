//MIT License
//
//Copyright (c) 2020 Yves BAZIN
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#ifndef SID6581_h
#define SID6581_h

#define CS 2
#define WRITE 1
#define CS_1 1
#define WRITE_1 0

#define CS_2 3
#define WRITE_2 2

#define CS_3 5
#define WRITE_3 4

#define CS_4 7
#define WRITE_4 6

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



#define SID_FC_LO 21
#define SID_FC_HI 22
#define SID_RES_FILT 23
#define SID_MOD_VOL 24


#define SID_QUEUE_SIZE 100


#include "SPI.h"
#include "FS.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "driver/i2s.h"
#include "freertos/queue.h"

static TaskHandle_t xPlayerTaskHandle = NULL;
static TaskHandle_t SIDPlayerTaskHandle = NULL;
static TaskHandle_t SIDPlayerLoopTaskHandle = NULL;
static TaskHandle_t PausePlayTaskLocker = NULL;
static TaskHandle_t xPushToRegisterHandle = NULL;
 static QueueHandle_t _sid_queue;
 static QueueHandle_t _sid_voicesQueues[15];
 static uint16_t _sid_play_notes[96];
static TaskHandle_t _sid_xHandletab[15];
 static uint8_t keyboardnbofvoices;
volatile static bool _sid_voices_busy[15];
volatile static bool _sid_taskready_busy[15];
//to save the

struct _sid_register_to_send{
    uint8_t data;
    uint8_t chip;
    uint8_t address;
};

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

struct _sid_command {
    int note;
    int velocity;
    uint16_t duration;
};







class sid_instrument{
public:
    
    int attack;
    int sustain;
    int decay;
    int release;
    int pulse;
    int waveform;
    int type_instrument;
    virtual void start_sample(int voice,int note){}
    
    virtual void next_instruction(int voice,int note){}
    virtual void after_off(int voice,int note){}
    int start;
    int loop;
};


static sid_instrument *current_instruments[9];






class SID6581 {
public:
    SID6581();
    bool begin(int clock_pin,int data_pin, int latch );
    bool begin(int clock_pin,int data_pin, int latch,int sid_clock_pin);
    
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
    
    void sidSetVolume( int chip,uint8_t vol);
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
    void setFilterFrequency(int chip,int filterfrequency);
    void setResonance(int chip,int resonance);
    void setFilter1(int chip,int filt1);
    void setFilter2(int chip,int filt2);
    void setFilter3(int chip,int filt3);
    void setFilterEX(int chip,int filtex);
    void set3OFF(int chip,int _3off);
    void setHP(int chip,int hp);
    void setBP(int chip,int bp);
    void setLP(int chip,int lp);
    
    void pushToVoice(int voice,uint8_t address,uint8_t data);
    void pushRegister(int chip,int address,int data);
    void resetsid();
    void feedTheDog();
    
protected:
    
    int  latch_pin;
    void stop();
    uint8_t voice=7;
    _sid_control sid_control[3];
    void playNextInt();
    bool stopped=false;
    int numberToPlay=0;
    int numberOfSongs=0;
    int currentSong=0;
    bool paused=false;
    _sid_voice voices[6];
    const int _maxnumber=80;
    
    static void playSIDTunesTask(void *pvParameters);
    static void _pushRegister(void *pvParameters);
    static void playSIDTunesLoopTask(void *pvParameters);
    songstruct listsongs[80];
    uint8_t save24;
    uint8_t volume;
    uint8_t * sidvalues;
    unsigned int readFile2(fs::FS &fs, const char * path);
    uint8_t adcswrre,dataspi,chipcontrol;
    void clearcsw(int chip);
    //void resetsid();
    void push();
    
    void setA(uint8_t val);
    void setcsw();
    void setD(uint8_t val);
    void cls();
    SPIClass * sid_spi = NULL;
    const int sid_spiClk = 20000000;
    
    
};
static   SID6581 sid;



template<int voice>
class VoiceTask{
public:
    
    static void  vTaskCode( void * pvParameters )
    {
        _sid_taskready_busy[voice]=true;
         uint32_t start_time;
        _sid_command element;
        for( ;; )
        {
            /* Task code goes here. */
            //Serial.printf("wait voice:%d\n",voice);
            xQueueReceive(_sid_voicesQueues[voice], &element, portMAX_DELAY);
            //Serial.println("té");
            //   Serial.println("tép  s");
            int i=0;
            
            current_instruments[voice]->start_sample(voice,element.note);
            start_time=millis();
            while(uxQueueMessagesWaiting( _sid_voicesQueues[voice] )==0)
            {
                if(element.duration>0)
                {
                    if(millis()-start_time>=element.duration)
                    {
                        Serial.println("stop from duration");
                         _sid_voices_busy[voice]=false;
                        //Serial.printf("d %d\n",_sid_voices_busy[voice]);
                        current_instruments[voice]->after_off(voice,element.note);
                       
                        sid.setGate(voice,0);
                        break;
                    }

                }
                
                if(element.velocity>0)
                {
                    //Serial.println("next onstructiopn");
                    current_instruments[voice]->next_instruction(voice,element.note);
                    
                }
                
                else
                {
                    //Serial.println("onstop");
                    _sid_voices_busy[voice]=false;
                    current_instruments[voice]->after_off(voice,element.note);
                    
                    sid.setGate(voice,0);
                    break;
                }
                
                
            }
            
        }
    }
    
};



static uint16_t sample1[]={24, 0,0,22, 0,0,21, 0,4512,6, 0,193,5, 0,10,4, 0,10,3, 0,10,2, 0,10,1, 0,10,0, 0,10,23, 8,10,23, 0,49,22, 8,7,22, 0,12,21, 8,7,21, 0,12,6, 8,316,6, 0,12,5, 8,7,5, 0,12,4, 8,7,4, 0,12,3, 8,7,3, 0,12,2, 8,7,2, 0,12,1, 8,7,1, 0,12,0, 8,7,0, 0,12,24, 127,7,23, 241,11990,24, 31,543,21, 4,9,22, 137,554,0, 31,30,1, 31,29,2, 0,4,3, 8,10,5, 6,4,6, 250,7,4, 65,7,21, 6,13,22, 136,1769,24, 127,30,2, 8,18,3, 8,14150,0, 41,4,1, 31,60,21, 0,4,22, 136,187,24, 127,30,2, 16,61,3, 8,2628,0, 51,4,1, 31,60,21, 2,4,22, 135,187,24, 127,30,2, 24,18,3, 8,6676,0, 61,4,1, 31,60,21, 4,4,22, 134,187,24, 127,30,24, 127,18,2, 32,4781,3, 8,120,0, 71,4,1, 31,60,21, 6,4,22, 133,230,24, 127,30,2, 40,18,3, 8,4153,0, 81,4,1, 31,60,21, 0,4,22, 133,187,24, 127,30,2, 48,61,3, 8,2628,0, 91,4,1, 31,60,21, 2,4,22, 132,187,24, 127,30,2, 56,18,3, 8,6676,0, 101,4,1, 31,60,21, 4,4,22, 131,187,24, 127,30,24, 127,18,2, 64,4781,3, 8,120,0, 111,4,1, 31,60,21, 6,4,22, 130,230,24, 127,30,2, 72,18,3, 8,4153,0, 121,4,1, 31,60,21, 0,4,22, 130,187,24, 127,30,2, 80,61,3, 8,2628,0, 131,4,1, 31,60,21, 2,4,22, 129,187,24, 127,30,2, 88,18,3, 8,6676,0, 141,4,1, 31,60,21, 4,4,22, 128,187,24, 127,30,24, 127,18,2, 96,4781,3, 8,120,0, 151,4,1, 31,60,21, 6,4,22, 127,230,24, 127,30,2, 104,18,3, 8,4153,0, 161,4,1, 31,60,21, 0,4,22, 127,187,24, 127,30,2, 112,61,3, 8,2628,0, 171,4,1, 31,60,21, 2,4,22, 126,187,24, 127,30,2, 120,18,3, 8,6676,0, 181,4,1, 31,60,21, 4,4,22, 125,187,24, 127,30,24, 127,18,2, 128,4781,3, 8,120,0, 191,4,1, 31,60,21, 6,4,22, 124,230,24, 127,30,2, 136,18,3, 8,4153,0, 201,4,1, 31,60,21, 0,4,22, 124,187,24, 127,30,2, 144,61,3, 8,2628,0, 211,4,1, 31,60,21, 2,4,22, 123,187,24, 127,30,2, 152,18,3, 8,6676,0, 221,4,1, 31,60,21, 4,4,22, 122,187,24, 127,30,24, 127,18,2, 160,4781,3, 8,120,0, 231,4,1, 31,60,21, 6,4,22, 121,230,24, 127,30,2, 168,18,3, 8,4153,0, 241,4,1, 31,60,21, 0,4,22, 121,187,24, 127,30,2, 176,61,3, 8,2628,0, 251,4,1, 31,60,21, 2,4,22, 120,187,24, 127,30,2, 184,18,3, 8,6676,0, 5,4,1, 32,60,21, 4,4,22, 119,187,24, 127,30,24, 127,18,2, 192,4781,3, 8,120,0, 15,4,1, 32,60,21, 6,4,22, 118,230,24, 127,30,2, 200,18,3, 8,4153,0, 25,4,1, 32,60,21, 0,4,22, 118,187,24, 127,30,2, 208,61,3, 8,2628,0, 35,4,1, 32,60,21, 2,4,22, 117,187,24, 127,30,2, 216,18,3, 8,6676,0, 45,4,1, 32,60,21, 4,4,22, 116,187,24, 127,30,24, 127,18,2, 224,4781,3, 8,120,0, 55,4,1, 32,60,21, 6,4,22, 115,230,24, 127,30,2, 232,18,3, 8,4153,0, 65,4,1, 32,60,21, 0,4,22, 115,187,24, 127,30,2, 240,61,3, 8,2628,0, 75,4,1, 32,60,21, 2,4,22, 114,187,24, 127,30,2, 248,18,3, 8,6676,0, 85,4,1, 32,60,21, 4,4,22, 113,187,24, 127,30,24, 127,18,2, 0,4781,3, 9,120,0, 95,4,1, 32,60,21, 6,4,22, 112,230,24, 127,30,2, 8,18,3, 9,4153,0, 105,4,1, 32,60,21, 0,4,22, 112,187,24, 127,30,2, 16,61,3, 9,2628,0, 115,4,1, 32,60,21, 2,4,22, 111,187,24, 127,30,2, 24,18,3, 9,6676,0, 125,4,1, 32,60,21, 4,4,22, 110,187,24, 127,30,24, 127,18,2, 32,4781,3, 9,120,0, 135,4,1, 32,60,21, 6,4,22, 109,230,24, 127,30,2, 40,18,3, 9,4153,0, 145,4,1, 32,60,21, 0,4,22, 109,187,24, 127,30,2, 48,61,3, 9,2628,0, 155,4,1, 32,60,21, 2,4,22, 108,187,24, 127,30,2, 56,18,3, 9,6676,0, 165,4,1, 32,60,21, 4,4,22, 107,187,24, 127,30,24, 127,18,2, 64,4781,3, 9,120,0, 175,4,1, 32,60,21, 6,4,22, 106,230,24, 127,30,2, 72,18,3, 9,4153,0, 185,4,1, 32,60,21, 0,4,22, 106,187,24, 127,30,2, 80,61,3, 9,2628,0, 195,4,1, 32,60,21, 2,4,22, 105,187,24, 127,30,2, 88,18,3, 9,6676,0, 205,4,1, 32,60,21, 4,4,22, 104,187,24, 127,30,24, 127,18,2, 96,4781,3, 9,120,0, 215,4,1, 32,60,21, 6,4,22, 103,230,24, 127,30,2, 104,18,3, 9,4153,0, 225,4,1, 32,60,21, 0,4,22, 103,187,24, 127,30,2, 112,61,3, 9,2628,0, 235,4,1, 32,60,21, 2,4,22, 102,187,24, 127,30,2, 120,18,3, 9,6676,0, 245,4,1, 32,60,21, 4,4,22, 101,187,24, 127,30,24, 127,18,2, 128,4781,3, 9,120,0, 255,4,1, 32,60,21, 6,4,22, 100,230,24, 127,30,2, 136,18,3, 9,4153,0, 9,4,1, 33,60,21, 0,4,22, 100,187,24, 127,30,2, 144,61,3, 9,2628,0, 19,4,1, 33,60,21, 2,4,22, 99,187,24, 127,30,2, 152,18,3, 9,6676,0, 29,4,1, 33,60,21, 4,4,22, 98,187,24, 127,30,24, 127,18,2, 160,4781,3, 9,120,0, 39,4,1, 33,60,21, 6,4,22, 97,230,24, 127,30,2, 168,18,3, 9,4153,0, 49,4,1, 33,60,21, 0,4,22, 97,187,24, 127,30,2, 176,61,3, 9,2628,0, 59,4,1, 33,60,21, 2,4,22, 96,187,24, 127,30,2, 184,18,3, 9,6676,0, 69,4,1, 33,60,21, 4,4,22, 95,187,24, 127,30,24, 127,18,2, 192,4781,3, 9,120,0, 79,4,1, 33,60,21, 6,4,22, 94,230,24, 127,30,2, 200,18,3, 9,4153,0, 89,4,1, 33,60,21, 0,4,22, 94,187,24, 127,30,2, 208,61,3, 9,2628,0, 99,4,1, 33,60,21, 2,4,22, 93,187,24, 127,30,2, 216,18,3, 9,6676,0, 109,4,1, 33,60,21, 4,4,22, 92,187,24, 127,30,24, 127,18,2, 224,4781,3, 9,120,0, 119,4,1, 33,60,21, 6,4,22, 91,230,24, 127,30,2, 232,18,3, 9,4153,0, 129,4,1, 33,60,21, 0,4,22, 91,187,24, 127,30,2, 240,61,3, 9,2628,0, 139,4,1, 33,60,21, 2,4,22, 90,187,24, 127,30,2, 248,18,3, 9,6676,0, 149,4,1, 33,60,21, 4,4,22, 89,187,24, 127,30,24, 127,18,2, 0,4781,3, 10,120,0, 159,4,1, 33,60,21, 6,4,22, 88,230,24, 127,30,2, 8,18,3, 10,4153,0, 169,4,1, 33,60,21, 0,4,22, 88,187,24, 127,30,2, 16,61,3, 10,2628,0, 179,4,1, 33,60,21, 2,4,22, 87,187,24, 127,30,2, 24,18,3, 10,6676,0, 189,4,1, 33,60,21, 4,4,22, 86,187,24, 127,30,24, 127,18,2, 32,4781,3, 10,120,0, 199,4,1, 33,60,21, 6,4,22, 85,230,24, 127,30,2, 40,18,3, 10,4153,0, 209,4,1, 33,60,21, 0,4,22, 85,187,24, 127,30,2, 48,61,3, 10,2628,0, 219,4,1, 33,60,21, 2,4,22, 84,187,24, 127,30,2, 56,18,3, 10,6676,0, 229,4,1, 33,60,21, 4,4,22, 83,187,24, 127,30,24, 127,18,2, 64,4781,3, 10,120,0, 239,4,1, 33,60,21, 6,4,22, 82,230,24, 127,30,2, 72,18,3, 10,4153,0, 249,4,1, 33,60,21, 0,4,22, 82,187,24, 127,30,2, 80,61,3, 10,2628,0, 3,4,1, 34,60,21, 2,4,22, 81,187,24, 127,30,2, 88,18,3, 10,6676,0, 13,4,1, 34,60,21, 4,4,22, 80,187,24, 127,30,24, 127,18,2, 96,4781,3, 10,120,0, 23,4,1, 34,60,21, 6,4,22, 79,230,24, 127,30,2, 104,18,3, 10,4153,0, 33,4,1, 34,60,21, 0,4,22, 79,187,24, 127,30,2, 112,61,3, 10,2628,0, 43,4,1, 34,60,21, 2,4,22, 78,187,24, 127,30,2, 120,18,3, 10,6676,0, 53,4,1, 34,60,21, 4,4,22, 77,187,24, 127,30,24, 127,18,2, 128,4781,3, 10,120,0, 63,4,1, 34,60,21, 6,4,22, 76,230,24, 127,30,2, 136,18,3, 10,4153,0, 73,4,1, 34,60,21, 0,4,22, 76,187,24, 127,30,2, 144,61,3, 10,2628,0, 83,4,1, 34,60,21, 2,4,22, 75,187,24, 127,30,2, 152,18,3, 10,6676,0, 93,4,1, 34,60,21, 4,4,22, 74,187,24, 127,30,24, 127,18,2, 160,4781,3, 10,120,0, 103,4,1, 34,60,21, 6,4,22, 73,230,24, 127,30,2, 168,18,3, 10,4153,0, 113,4,1, 34,60,21, 0,4,22, 73,187,24, 127,30,2, 176,61,3, 10,2628,0, 123,4,1, 34,60,21, 2,4,22, 72,187,24, 127,30,2, 184,18,3, 10,6676,0, 133,4,1, 34,60,21, 4,4,22, 71,187,24, 127,30,24, 127,18,2, 192,4781,3, 10,120,0, 143,4,1, 34,60,21, 6,4,22, 70,230,24, 127,30,2, 200,18,3, 10,4153,0, 153,4,1, 34,60,21, 0,4,22, 70,187,24, 127,30,2, 208,61,3, 10,2628,0, 163,4,1, 34,60,21, 2,4,22, 69,187,24, 127,30,2, 216,18,3, 10,6676,0, 173,4,1, 34,60,21, 4,4,22, 68,187,24, 127,30,24, 127,18,2, 224,4781,3, 10,120,0, 183,4,1, 34,60,21, 6,4,22, 67,230,24, 127,30,2, 232,18,3, 10,4153,0, 193,4,1, 34,60,21, 0,4,22, 67,187,24, 127,30,2, 240,61,3, 10,2628,0, 203,4,1, 34,60,21, 2,4,22, 66,187,24, 127,30,2, 248,18,3, 10,6676,0, 213,4,1, 34,60,21, 4,4,22, 65,187,24, 127,30,24, 127,18,2, 0,4781,3, 11,120,0, 223,4,1, 34,60,21, 6,4,22, 64,230,24, 127,30,2, 8,18,3, 11,4153,0, 233,4,1, 34,60,21, 0,4,22, 64,187,24, 127,30,2, 16,61,3, 11,2628,0, 243,4,1, 34,60,21, 2,4,22, 63,187,24, 127,30,2, 24,18,3, 11,6676,0, 253,4,1, 34,60,21, 4,4,22, 62,187,24, 127,30,24, 127,18,2, 32,4781,3, 11,120,0, 7,4,1, 35,60,21, 6,4,22, 61,230,24, 127,30,2, 40,18,3, 11,4153,0, 32,4,1, 35,60,21, 0,4,22, 61,187,24, 127,30,2, 48,61,3, 11,2628,0, 57,4,1, 35,60,21, 2,4,22, 60,187,24, 127,30,2, 56,18,3, 11,6676,0, 82,4,1, 35,60,21, 4,4,22, 59,187,24, 127,30,24, 127,18,2, 64,4781,3, 11,120,0, 107,4,1, 35,60,21, 6,4,22, 58,230,24, 127,30,2, 72,18,3, 11,4153,0, 132,4,1, 35,60,21, 0,4,22, 58,187,24, 127,30,2, 80,61,3, 11,2628,0, 157,4,1, 35,60,21, 2,4,22, 57,187,24, 127,30,2, 88,18,3, 11,6676,0, 182,4,1, 35,60,21, 4,4,22, 56,187,24, 127,30,24, 127,18,2, 96,4781,3, 11,120,0, 207,4,1, 35,60,21, 6,4,22, 55,230,24, 127,30,2, 104,18,3, 11,4153,0, 232,4,1, 35,60,21, 0,4,22, 55,187,24, 127,30,2, 112,61,3, 11,2628,0, 207,4,1, 35,67,21, 2,4,22, 54,187,24, 127,30,2, 120,18,3, 11,6670,0, 182,4,1, 35,67,21, 4,4,22, 53,187,24, 127,30,24, 127,18,2, 128,4775,3, 11,120,0, 157,4,1, 35,67,21, 6,4,22, 52,229,24, 127,30,2, 136,18,3, 11,4144,0, 132,4,1, 35,67,21, 0,4,22, 52,187,24, 127,73,2, 144,18,3, 11,2622,0, 107,4,1, 35,67,21, 2,4,22, 51,187,24, 127,30,2, 152,18,3, 11,6670,0, 82,4,1, 35,67,21, 4,4,22, 50,187,24, 127,30,24, 127,18,2, 160,4775,3, 11,120,0, 57,4,1, 35,67,21, 6,4,22, 49,229,24, 127,30,2, 168,18,3, 11,4144,0, 32,4,1, 35,67,21, 0,4,22, 49,187,24, 127,73,2, 176,18,3, 11,2622,0, 7,4,1, 35,67,21, 2,4,22, 48,187,24, 127,30,2, 184,18,3, 11,6670,0, 238,4,1, 34,67,21, 4,4,22, 47,187,24, 127,30,24, 127,18,2, 192,4775,3, 11,120,0, 213,4,1, 34,67,21, 6,4,22, 46,229,24, 127,30,2, 200,18,3, 11,4144,0, 188,4,1, 34,67,21, 0,4,22, 46,187,24, 127,73,2, 208,18,3, 11,2622,0, 163,4,1, 34,67,21, 2,4,22, 45,187,24, 127,30,2, 216,18,3, 11,6670,0, 138,4,1, 34,67,21, 4,4,22, 44,187,24, 127,30,24, 127,18,2, 224,4775,3, 11,120,0, 113,4,1, 34,67,21, 6,4,22, 43,229,24, 127,30,2, 232,18,3, 11,4144,0, 88,4,1, 34,67,21, 0,4,22, 43,187,24, 127,73,2, 240,18,3, 11,2622,0, 63,4,1, 34,67,21, 2,4,22, 42,187,24, 127,30,2, 248,18,3, 11,6670,0, 38,4,1, 34,67,21, 4,4,22, 41,187,24, 127,30,24, 127,18,2, 0,4775,3, 12,120,0, 63,4,1, 34,74,21, 3,4,22, 41,236,24, 127,30,2, 8,18,3, 12,4132,0, 88,4,1, 34,74,21, 2,4,22, 41,193,24, 127,73,2, 16,18,3, 12,2607,0, 113,4,1, 34,74,21, 1,4,22, 41,193,24, 127,30,2, 24,18,3, 12,6658,0, 138,4,1, 34,74,21, 0,4,22, 41,193,24, 127,30,24, 127,18,2, 32,4759,3, 12,120,0, 163,4,1, 34,74,21, 7,4,22, 40,236,24, 127,30,2, 40,18,3, 12,4132,0, 188,4,1, 34,74,21, 6,4,22, 40,193,24, 127,73,2, 48,18,3, 12,2610,0, 213,4,1, 34,74,21, 5,4,22, 40,193,24, 127,30,2, 56,18,3, 12,6655,0, 238,4,1, 34,74,21, 4,4,22, 40,193,24, 127,30,24, 127,18,2, 64,4763,3, 12,120,0, 7,4,1, 35,74,21, 3,4,22, 40,236,24, 127,30,2, 72,18,3, 12,4132,0, 32,4,1, 35,148,21, 2,4,22, 40,236,24, 127,30,2, 80,18,3, 12,2535,0, 57,4,1, 35,60,21, 1,4,22, 40,193,24, 127,30,2, 88,18,3, 12,6670,0, 82,4,1, 35,60,21, 0,4,22, 40,193,24, 127,30,24, 127,18,2, 96,4775,3, 12,120,0, 107,4,1, 35,60,21, 7,4,22, 39,236,24, 127,30,2, 104,18,3, 12,4144,0, 132,4,1, 35,60,21, 6,4,22, 39,193,24, 127,30,2, 112,60,3, 12,2625,0, 157,4,1, 35,60,21, 5,4,22, 39,193,24, 127,30,2, 120,18,3, 12,6670,0, 182,4,1, 35,60,21, 4,4,22, 39,193,24, 127,30,24, 127,18,2, 128,4774,3, 12,120,0, 207,4,1, 35,60,21, 3,4,22, 39,235,24, 127,30,2, 136,18,3, 12,4147,0, 232,4,1, 35,60,21, 2,4,22, 39,193,24, 127,30,2, 144,60,3, 12,2625,0, 207,4,1, 35,67,21, 1,4,22, 39,193,24, 127,30,2, 152,18,3, 12,6664,0, 182,4,1, 35,67,21, 0,4,22, 39,193,24, 127,30,24, 127,18,2, 160,4766,3, 12,120,0, 157,4,1, 35,67,21, 7,4,22, 38,236,24, 127,30,2, 168,18,3, 12,4141,0, 132,4,1, 35,67,21, 6,4,22, 38,193,24, 127,73,2, 176,18,3, 12,2616,0, 107,4,1, 35,67,21, 5,4,22, 38,193,24, 127,30,2, 184,18,3, 12,6661,0, 82,4,1, 35,67,21, 4,4,22, 38,193,24, 127,30,24, 127,18,2, 192,4768,3, 12,120,0, 57,4,1, 35,67,21, 3,4,22, 38,236,24, 127,30,2, 200,18,3, 12,4141,0, 32,4,1, 35,67,21, 2,4,22, 38,193,24, 127,73,2, 208,18,3, 12,2616,0, 7,4,1, 35,67,21, 1,4,22, 38,193,24, 127,30,2, 216,18,3, 12,6661,0, 238,4,1, 34,67,21, 0,4,22, 38,193,24, 127,30,24, 127,18,2, 224,4768,3, 12,120,0, 213,4,1, 34,67,21, 7,4,22, 37,236,24, 127,30,2, 232,18,3, 12,4141,0, 188,4,1, 34,67,21, 6,4,22, 37,193,24, 127,73,2, 240,18,3, 12,2616,0, 163,4,1, 34,67,21, 5,4,22, 37,193,24, 127,30,2, 248,18,3, 12,6661,0, 138,4,1, 34,67,21, 4,4,22, 37,193,24, 127,30,24, 127,18,2, 0,4768,3, 13,120,0, 113,4,1, 34,67,21, 3,4,22, 37,758,24, 127,30,2, 248,18,3, 12,3625,0, 88,4,1, 34,67,21, 2,4,22, 37,193,24, 127,73,2, 240,18,3, 12,2617,0, 63,4,1, 34,67,21, 1,4,22, 37,193,24, 127,30,2, 232,18,3, 12,6662,0, 38,4,1, 34,67,21, 0,4,22, 37,193,24, 127,30,24, 127,18,2, 224,4760,3, 12,127,0, 63,4,1, 34,74,21, 7,4,22, 36,236,24, 127,30,2, 216,18,3, 12,4133,0, 88,4,1, 34,74,21, 6,4,22, 36,193,24, 127,73,2, 208,18,3, 12,2608,0, 113,4,1, 34,74,21, 5,4,22, 36,193,24, 127,30,2, 200,18,3, 12,6656,0, 138,4,1, 34,74,21, 4,4,22, 36,193,24, 127,30,24, 127,18,2, 192,4754,3, 12,127,0, 163,4,1, 34,74,21, 3,4,22, 36,236,24, 127,30,2, 184,18,3, 12,4133,0, 188,4,1, 34,74,21, 2,4,22, 36,193,24, 127,73,2, 176,18,3, 12,2608,0, 213,4,1, 34,74,21, 1,4,22, 36,193,24, 127,30,2, 168,18,3, 12,6656,0, 238,4,1, 34,74,21, 0,4,22, 36,193,24, 127,30,24, 127,18,2, 160,4754,3, 12,127,0, 7,4,1, 35,74,21, 7,4,22, 35,236,24, 127,30,2, 152,18,3, 12,4133,0, 32,4,1, 35,148,21, 6,4,22, 35,236,24, 127,30,2, 144,18,3, 12,2536,0, 57,4,1, 35,60,21, 5,4,22, 35,193,24, 127,30,2, 136,18,3, 12,6668,0, 82,4,1, 35,60,21, 4,4,22, 35,193,24, 127,30,24, 127,18,2, 128,4768,3, 12,127,0, 107,4,1, 35,60,21, 3,4,22, 35,236,24, 127,30,2, 120,18,3, 12,4148,0, 132,4,1, 35,60,21, 2,4,22, 35,193,24, 127,73,2, 112,18,3, 12,2623,0, 157,4,1, 35,60,21, 1,4,22, 35,193,24, 127,30,2, 104,18,3, 12,6668,0, 182,4,1, 35,60,21, 0,4,22, 35,193,24, 127,30,24, 127,18,2, 96,4768,3, 12,127,0, 207,4,1, 35,60,21, 7,4,22, 34,236,24, 127,30,2, 88,18,3, 12,4148,0, 232,4,1, 35,60,21, 6,4,22, 34,193,24, 127,73,2, 80,18,3, 12,2623,0, 207,4,1, 35,67,21, 5,4,22, 34,193,24, 127,30,2, 72,18,3, 12,6662,0, 182,4,1, 35,67,21, 4,4,22, 34,193,24, 127,30,24, 127,18,2, 64,4760,3, 12,127,0, 157,4,1, 35,67,21, 3,4,22, 34,236,24, 127,30,2, 56,18,3, 12,4139,0, 132,4,1, 35,67,21, 2,4,22, 34,193,24, 127,73,2, 48,18,3, 12,2617,0, 107,4,1, 35,67,21, 1,4,22, 34,193,24, 127,30,2, 40,18,3, 12,6662,0, 82,4,1, 35,67,21, 0,4,22, 34,193,24, 127,30,24, 127,18,2, 32,4763,3, 12,127,0, 57,4,1, 35,67,21, 7,4,22, 33,236,24, 127,30,2, 24,18,3, 12,4139,0, 32,4,1, 35,67,21, 6,4,22, 33,193,24, 127,73,2, 16,18,3, 12,2614,0, 7,4,1, 35,67,21, 5,4,22, 33,193,24, 127,30,2, 8,18,3, 12,6665,0, 238,4,1, 34,67,21, 4,4,22, 33,193,24, 127,30,24, 127,18,2, 0,4759,3, 12,127,0, 213,4,1, 34,67,21, 3,4,22, 33,236,24, 127,30,2, 248,18,3, 11,4139,0, 188,4,1, 34,67,21, 2,4,22, 33,193,24, 127,73,2, 240,18,3, 11,2617,0, 163,4,1, 34,67,21, 1,4,22, 33,193,24, 127,30,2, 232,18,3, 11,6662,0, 138,4,1, 34,67,21, 0,4,22, 33,193,24, 127,30,24, 127,18,2, 224,4763,3, 11,127,0, 113,4,1, 34,67,21, 7,4,22, 32,236,24, 127,30,2, 216,18,3, 11,4139,0, 88,4,1, 34,67,21, 6,4,22, 32,193,24, 127,73,2, 208,18,3, 11,2614,0, 63,4,1, 34,67,21, 5,4,22, 32,193,24, 127,30,2, 200,18,3, 11,6665,0, 38,4,1, 34,67,21, 4,4,22, 32,193,24, 127,30,24, 127,18,2, 192,4759,3, 11,127,0, 63,4,1, 34,74,21, 3,4,22, 32,236,24, 127,30,2, 184,18,3, 11,4133,0, 88,4,1, 34,74,21, 2,4,22, 32,193,24, 127,73,2, 176,18,3, 11,2608,0, 113,4,1, 34,74,21, 1,4,22, 32,193,24, 127,30,2, 168,18,3, 11,6656,0, 138,4,1, 34,74,21, 0,4,22, 32,193,24, 127,30,24, 127,18,2, 160,4754,3, 11,127,0, 163,4,1, 34,74,21, 7,4,22, 31,236,24, 127,30,2, 152,18,3, 11,4133,0, 188,4,1, 34,74,21, 6,4,22, 31,193,24, 127,73,2, 144,18,3, 11,2608,0, 213,4,1, 34,74,21, 5,4,22, 31,193,24, 127,30,2, 136,18,3, 11,6656,0, 238,4,1, 34,74,21, 4,4,22, 31,193,24, 127,30,24, 127,18,2, 128,4754,3, 11,127,0, 7,4,1, 35,74,21, 3,4,22, 31,236,24, 127,30,2, 120,18,3, 11,4133,0, 32,4,1, 35,148,21, 2,4,22, 31,236,24, 127,30,2, 112,18,3, 11,2536,0, 57,4,1, 35,60,21, 1,4,22, 31,193,24, 127,30,2, 104,18,3, 11,6668,0, 82,4,1, 35,60,21, 0,4,22, 31,193,24, 127,30,24, 127,18,2, 96,4768,3, 11,127,0, 107,4,1, 35,60,21, 7,4,22, 30,236,24, 127,30,2, 88,18,3, 11,4148,0, 132,4,1, 35,60,21, 6,4,22, 30,193,24, 127,73,2, 80,18,3, 11,2623,0, 157,4,1, 35,60,21, 5,4,22, 30,193,24, 127,30,2, 72,18,3, 11,6668,0, 182,4,1, 35,60,21, 4,4,22, 30,193,24, 127,30,24, 127,18,2, 64,4768,3, 11,127,0, 207,4,1, 35,60,21, 3,4,22, 30,236,24, 127,30,2, 56,18,3, 11,4148,0, 232,4,1, 35,60,21, 2,4,22, 30,193,24, 127,73,2, 48,18,3, 11,2623,0, 207,4,1, 35,67,21, 1,4,22, 30,193,24, 127,30,2, 40,18,3, 11,6662,0, 182,4,1, 35,67,21, 0,4,22, 30,193,24, 127,30,24, 127,18,2, 32,4760,3, 11,127,0, 157,4,1, 35,67,21, 7,4,22, 29,236,24, 127,30,2, 24,18,3, 11,4139,0, 132,4,1, 35,67,21, 6,4,22, 29,193,24, 127,73,2, 16,18,3, 11,2617,0, 107,4,1, 35,67,21, 5,4,22, 29,193,24, 127,30,2, 8,18,3, 11,6662,0, 82,4,1, 35,67,21, 4,4,22, 29,193,24, 127,30,24, 127,18,2, 0,4763,3, 11,127,0, 57,4,1, 35,67,21, 3,4,22, 29,236,24, 127,30,2, 248,18,3, 10,4139,0, 32,4,1, 35,67,21, 2,4,22, 29,193,24, 127,73,2, 240,18,3, 10,2614,0, 7,4,1, 35,67,21, 1,4,22, 29,193,24, 127,30,2, 232,18,3, 10,6665,0, 238,4,1, 34,67,21, 0,4,22, 29,193,24, 127,30,24, 127,18,2, 224,4759,3, 10,127,0, 213,4,1, 34,67,21, 7,4,22, 28,236,24, 127,30,2, 216,18,3, 10,4139,0, 188,4,1, 34,67,21, 6,4,22, 28,193,24, 127,73,2, 208,18,3, 10,2617,0, 163,4,1, 34,67,21, 5,4,22, 28,193,24, 127,30,2, 200,18,3, 10,6662,0, 138,4,1, 34,67,21, 4,4,22, 28,193,24, 127,30,24, 127,18,2, 192,4763,3, 10,127,0, 113,4,1, 34,67,21, 3,4,22, 28,236,24, 127,30,2, 184,18,3, 10,4139,0, 88,4,1, 34,67,21, 2,4,22, 28,193,24, 127,73,2, 176,18,3, 10,2614,0, 63,4,1, 34,67,21, 1,4,22, 28,193,24, 127,30,2, 168,18,3, 10,6665,0, 38,4,1, 34,67,21, 0,4,22, 28,193,24, 127,30,24, 127,18,2, 160,4759,3, 10,127,0, 63,4,1, 34,74,21, 7,4,22, 27,236,24, 127,30,2, 152,18,3, 10,4133,0, 88,4,1, 34,74,21, 6,4,22, 27,193,24, 127,73,2, 144,18,3, 10,2608,0, 113,4,1, 34,74,21, 5,4,22, 27,193,24, 127,30,2, 136,18,3, 10,6656,0, 138,4,1, 34,74,21, 4,4,22, 27,193,24, 127,30,24, 127,18,2, 128,4754,3, 10,127,0, 163,4,1, 34,74,21, 3,4,22, 27,236,24, 127,30,2, 120,18,3, 10,4133,0, 188,4,1, 34,74,21, 2,4,22, 27,193,24, 127,73,2, 112,18,3, 10,2608,0, 213,4,1, 34,74,21, 1,4,22, 27,193,24, 127,30,2, 104,18,3, 10,6656,0, 238,4,1, 34,74,21, 0,4,22, 27,193,24, 127,30,24, 127,18,2, 96,4754,3, 10,127,0, 7,4,1, 35,74,21, 7,4,22, 26,236,24, 127,30,2, 88,18,3, 10,4133,0, 32,4,1, 35,148,21, 6,4,22, 26,236,24, 127,30,2, 80,18,3, 10,2536,0, 57,4,1, 35,60,21, 5,4,22, 26,193,24, 127,30,2, 72,18,3, 10,6668,0, 82,4,1, 35,60,21, 4,4,22, 26,193,24, 127,30,24, 127,18,2, 64,4768,3, 10,127,0, 107,4,1, 35,60,21, 3,4,22, 26,236,24, 127,30,2, 56,18,3, 10,4148,0, 132,4,1, 35,60,21, 2,4,22, 26,193,24, 127,73,2, 48,18,3, 10,2623,0, 157,4,1, 35,60,21, 1,4,22, 26,193,24, 127,30,2, 40,18,3, 10,6668,0, 182,4,1, 35,60,21, 0,4,22, 26,193,24, 127,30,24, 127,18,2, 32,4768,3, 10,127,0, 207,4,1, 35,60,21, 7,4,22, 25,236,24, 127,30,2, 24,18,3, 10,4148,0, 232,4,1, 35,60,21, 6,4,22, 25,193,24, 127,73,2, 16,18,3, 10,2623,0, 207,4,1, 35,67,21, 5,4,22, 25,193,24, 127,30,2, 8,18,3, 10,6662,0, 182,4,1, 35,67,21, 4,4,22, 25,193,24, 127,30,24, 127,18,2, 0,4760,3, 10,127,0, 157,4,1, 35,67,21, 3,4,22, 25,236,24, 127,30,2, 248,18,3, 9,4139,0, 132,4,1, 35,67,21, 2,4,22, 25,193,24, 127,73,2, 240,18,3, 9,2617,0, 107,4,1, 35,67,21, 1,4,22, 25,193,24, 127,30,2, 232,18,3, 9,6662,0, 82,4,1, 35,67,21, 0,4,22, 25,193,24, 127,30,24, 127,18,2, 224,4763,3, 9,127,0, 57};
static uint16_t sample2[]={24,0,0, 0,0,0, 1,0,0, 2,0,0, 3,0,0, 4,0,0, 5,0,0, 6,0,0, 21,0,0, 22,0,0, 23,0,0, 24,0,0, 25,0,0, 26,0,0, 24,15,0, 22,32,0, 22,32,0, 2,192,0, 3,7,0, 22,30,0, 0,117,0, 1,6,0, 4,65,0, 22,30,0, 22,30,0, 2,240,0, 3,7,0, 22,32,0, 0,85,0, 1,6,0, 4,65,0, 23,241,0, 23,241,0, 5,0,0, 6,164,0, 4,64,0, 24,31,0, 23,241,0, 4,9,0, 22,32,0, 22,32,0, 2,32,0, 3,2,0, 22,65,0, 0,163,0, 1,14,0, 4,65,0, 22,65,0, 22,65,0, 2,80,0, 3,2,0, 22,66,0, 0,163,0, 1,14,0, 4,65,0, 22,66,0, 22,66,0, 2,128,0, 3,2,0, 22,67,0, 0,163,0, 1,14,0, 4,65,0, 23,241,0, 22,67,0, 2,176,0, 3,2,0, 22,68,0, 0,163,0, 1,14,0, 4,65,0, 22,68,0, 22,68,0, 2,224,0, 3,2,0, 22,69,0, 0,163,0, 1,14,0, 4,65,0, 22,69,0, 22,69,0, 2,16,0, 3,3,0, 22,70,0, 0,163,0, 1,14,0, 4,65,0, 22,70,0, 22,70,0, 2,64,0, 3,3,0, 22,71,0, 0,163,0, 1,14,0, 4,65,0, 23,241,0, 22,71,0, 2,112,0, 3,3,0, 22,72,0, 0,81,0, 1,7,0, 4,65,0, 22,72,0, 22,72,0, 2,160,0, 3,3,0, 22,73,0, 0,81,0, 1,7,0, 4,65,0, 22,73,0, 22,73,0, 2,208,0, 3,3,0, 22,74,0, 0,81,0, 1,7,0, 4,65,0, 22,74,0, 22,74,0, 2,0,0, 3,4,0, 22,75,0, 0,81,0, 1,7,0, 4,65,0, 23,241,0, 22,75,0, 2,48,0, 3,4,0, 22,76,0, 0,163,0, 1,14,0, 4,65,0, 22,76,0, 22,76,0, 2,96,0, 3,4,0, 22,77,0, 0,163,0, 1,14,0, 4,65,0, 22,77,0, 22,77,0, 2,144,0, 3,4,0, 22,78,0, 0,163,0, 1,14,0, 4,65,0, 22,78,0, 22,78,0, 2,192,0, 3,4,0, 22,79,0, 0,163,0, 1,14,0, 4,65,0, 23,241,0, 22,79,0, 2,240,0, 3,4,0, 22,80,0, 0,163,0, 1,14,0, 4,65,0, 22,80,0, 22,80,0, 2,32,0, 3,5,0, 22,81,0, 0,163,0, 1,14,0, 4,65,0, 22,81,0, 22,81,0, 2,80,0, 3,5,0, 22,82,0, 0,163,0, 1,14,0, 4,65,0, 22,82,0, 22,82,0, 2,128,0, 3,5,0, 22,81,0, 0,81,0, 1,7,0, 4,65,0, 23,241,0, 22,81,0, 2,176,0, 3,5,0, 22,80,0, 0,81,0, 1,7,0, 4,65,0, 22,80,0, 22,80,0, 2,224,0, 3,5,0, 22,79,0, 0,81,0, 1,7,0, 4,65,0, 22,79,0, 22,79,0, 2,16,0, 3,6,0, 22,78,0, 0,81,0, 1,7,0, 4,65,0};


class sid_piano5:public sid_instrument{
public:
    int i;
    int flo,fhi,plo,phi;
    sid_piano5()
    {

        df2=sample2;
    }
    virtual void start_sample(int voice,int note)
    {
        i=30;
    }
    uint16_t *df2;
    virtual void next_instruction(int voice,int note)
    {
        
        i=(i+1)%187;
        
        if(i==0)
            i=43;
        
        
        switch(df2[i*3])
        {
            case 0:
                flo=df2[i*3+1];
                break;
                
                
            case 1:
                fhi=df2[i*3+1];
                sid.setFrequency(voice,((fhi*256+flo)*note)/3747);
                break;
                
            default:
                if(df2[i*3]<7)
                    sid.pushRegister(voice/3,df2[i*3]+(voice%3)*7,df2[i*3+1]);
                
                break;
        }
        //sid.feedTheDog();
        vTaskDelay(df2[i*3+2]/1000+2);
        
    }
    
    virtual void after_off(int voice,int note){
        
        sid.setFrequency(voice,0);
        sid.setGate(voice,0);
        i=30;
    }
    
    
};

class sid_piano:public sid_instrument{
public:
    int i;
    int flo,fhi,plo,phi;
    sid_piano()
    {

        df2=sample1;
    }
    virtual void start_sample(int voice,int note)
    {
        i=0;
    }
    uint16_t *df2;
    virtual void next_instruction(int voice,int note)
    {
        
        i=(i+1)%1918;
        
        if(i==0)
            i=1651;
        
        
        switch(df2[i*3])
        {
            case 0:
                flo=df2[i*3+1];
                break;
                
                
            case 1:
                fhi=df2[i*3+1];
                sid.setFrequency(voice,((fhi*256+flo)*note/7977));
                
                break;
                
            default:
                if(df2[i*3]<7)
                    sid.pushRegister(voice/3,df2[i*3]+(voice%3)*7,df2[i*3+1]);
                
                break;
        }
        
        vTaskDelay(df2[i*3+2]/1000);
        
    }
    
    virtual void after_off(int voice,int note){
        
        sid.setFrequency(voice,0);
        sid.setGate(voice,0);
        i=0;
    }
    
    
};



class sid_piano4:public sid_instrument{
public:
    int i;
    sid_piano4()
    {
 
        
    }
    
    virtual void start_sample(int voice,int note)
    {
        sid.setAttack(voice,3);
        sid.setSustain(voice,9);
        sid.setDecay(voice,9);
        sid.setRelease(voice,13);
        sid.setPulse(voice,3000);
        sid.setWaveForm(voice,SID_WAVEFORM_SAWTOOTH);
        sid.setFrequency(voice,note);
        sid.setGate(voice,1);
        i=0;
    }
    
    virtual void next_instruction(int voice,int note)
    {
        
        sid.setFrequency(voice,note+20*(1+i/50)*(cos(2*3.14*i/10)));
        sid.setGate(voice,(i/3)%2);
        //x
        i++;
        vTaskDelay(20);
        
    }
    
    virtual void after_off(int voice,int note){
        sid.setFrequency(voice,0);
        sid.setGate(voice,0);
        i=0;
    }
    
    
};

class sid_piano3:public sid_instrument{
public:
    int i;
    
    sid_piano3()
    {

    }
    
    virtual void start_sample(int voice,int note)
    {
        sid.setAttack(voice,0);
        sid.setSustain(voice,1);
        sid.setDecay(voice,10);
        sid.setRelease(voice,1);
        sid.setPulse(voice,3000);
        sid.setWaveForm(voice,SID_WAVEFORM_SAWTOOTH);
        sid.setFrequency(voice,note);
        sid.setGate(voice,1);
        i=0;
    }
    
    virtual void next_instruction(int voice,int note)
    {
        
        vTaskDelay(20);
        
    }
    
    virtual void after_off(int voice,int note){
        
        sid.setFrequency(voice,0);
        sid.setGate(voice,0);
        i=0;
    }
    
    
};


class sid_piano2:public sid_instrument{
public:
    int i;
    
    sid_piano2()
    {

    }
    
    virtual void start_sample(int voice,int note)
    {
        sid.setAttack(voice,0);
        sid.setSustain(voice,15);
        sid.setDecay(voice,2);
        sid.setRelease(voice,10);
        sid.setPulse(voice,1000);
        sid.setWaveForm(voice,SID_WAVEFORM_PULSE);
        sid.setGate(voice,1);
        i=0;
    }
 
    virtual void next_instruction(int voice,int note)
    {
        
        // Serial.println(note+10*(cos(2*3.14*i/100)));
        sid.setFrequency(voice,note+20*(cos(2*3.14*i/10)));
        //x
        i++;
        vTaskDelay(20);
        
    }
    
    virtual void after_off(int voice,int note){
        //Serial.printf("on eta int %d\n",voice);
        //sid.setFrequency(voice,0);
        sid.setGate(voice,0);
        i=0;
    }
    
    
};

class SIDKeyBoardPlayer{
public:

    static void KeyBoardPlayer(int nbofvoices)
    {
        sid.resetsid();
        createnot();
        keyboardnbofvoices=(nbofvoices%15)+1;
        for(int i=0;i<nbofvoices;i++)
        {
            _sid_taskready_busy[i]=false;
        }
        for(int i=0;i<nbofvoices;i++)
        {
            
            _sid_voicesQueues[i]= xQueueCreate( 100, sizeof( _sid_command ) );
            _sid_voices_busy[i]=false;
        }
        if(nbofvoices>=1)
        {
            xTaskCreate(
                        VoiceTask<0>::vTaskCode,      /* Function that implements the task. */
                        "NAME1",          /* Text name for the task. */
                        2048,      /* Stack size in words, not bytes. */
                        ( void * ) 1,    /* Parameter passed into the task. */
                        tskIDLE_PRIORITY,/* Priority at which the task is created. */
                        & _sid_xHandletab[0] );
        }
        if(nbofvoices>=2)
        {
            xTaskCreate(
                        VoiceTask<1>::vTaskCode,      /* Function that implements the task. */
                        "NAME2",          /* Text name for the task. */
                        2048,      /* Stack size in words, not bytes. */
                        ( void * ) 1,    /* Parameter passed into the task. */
                        tskIDLE_PRIORITY,/* Priority at which the task is created. */
                        & _sid_xHandletab[1] );
        }
        if(nbofvoices>=3)
        {
            xTaskCreate(
                        VoiceTask<2>::vTaskCode,      /* Function that implements the task. */
                        "NAME3",          /* Text name for the task. */
                        2048,      /* Stack size in words, not bytes. */
                        ( void * ) 1,    /* Parameter passed into the task. */
                        tskIDLE_PRIORITY,/* Priority at which the task is created. */
                        & _sid_xHandletab[2] );
        }
        if(nbofvoices>=4)
        {
            xTaskCreate(
                        VoiceTask<3>::vTaskCode,      /* Function that implements the task. */
                        "NAME4",          /* Text name for the task. */
                        2048,      /* Stack size in words, not bytes. */
                        ( void * ) 1,    /* Parameter passed into the task. */
                        tskIDLE_PRIORITY,/* Priority at which the task is created. */
                        & _sid_xHandletab[3] );
        }
        if(nbofvoices>=5)
        {
            xTaskCreate(
                        VoiceTask<4>::vTaskCode,      /* Function that implements the task. */
                        "NAME5",          /* Text name for the task. */
                        2048,      /* Stack size in words, not bytes. */
                        ( void * ) 1,    /* Parameter passed into the task. */
                        tskIDLE_PRIORITY,/* Priority at which the task is created. */
                        & _sid_xHandletab[4] );
        }
        if(nbofvoices>=6)
        {
            xTaskCreate(
                        VoiceTask<5>::vTaskCode,      /* Function that implements the task. */
                        "NAME6",          /* Text name for the task. */
                        2048,      /* Stack size in words, not bytes. */
                        ( void * ) 1,    /* Parameter passed into the task. */
                        tskIDLE_PRIORITY,/* Priority at which the task is created. */
                        & _sid_xHandletab[5] );
        }
        if(nbofvoices>=7)
        {
            xTaskCreate(
                        VoiceTask<6>::vTaskCode,      /* Function that implements the task. */
                        "NAME7",          /* Text name for the task. */
                        2048,      /* Stack size in words, not bytes. */
                        ( void * ) 1,    /* Parameter passed into the task. */
                        tskIDLE_PRIORITY,/* Priority at which the task is created. */
                        & _sid_xHandletab[6] );
        }
        if(nbofvoices>=8)
        {
            xTaskCreate(
                        VoiceTask<7>::vTaskCode,      /* Function that implements the task. */
                        "NAME8",          /* Text name for the task. */
                        2048,      /* Stack size in words, not bytes. */
                        ( void * ) 1,    /* Parameter passed into the task. */
                        tskIDLE_PRIORITY,/* Priority at which the task is created. */
                        & _sid_xHandletab[7] );
        }
        if(nbofvoices==9)
        {
            xTaskCreate(
                        VoiceTask<8>::vTaskCode,      /* Function that implements the task. */
                        "NAME9",          /* Text name for the task. */
                        2048,      /* Stack size in words, not bytes. */
                        ( void * ) 1,    /* Parameter passed into the task. */
                        tskIDLE_PRIORITY,/* Priority at which the task is created. */
                        & _sid_xHandletab[8] );
        }
        
        changeAllInstruments<sid_piano4>();
        while(allTaskReady());
        
        Serial.println("keyboard intiated");
        
    }
    
   template <typename instrument>static void changeAllInstruments()
    {
        for(int i=0;i<keyboardnbofvoices;i++)
        {
            current_instruments[i]=new instrument;
        }
    }
    
    template <typename instrument>static void changeInstrumentOnVoice(int voice)
    {
            if(voice<9)
                current_instruments[voice]=new instrument;
     
    }
    static void createnot()
    {
        double f=274.3352163421902;
        double  g=powf((double)2,(double)1/12);
        for(int i=0;i<95;i++)
        {
            uint16_t d=floor(f);
            _sid_play_notes[i]=d;
            f=f*g;
        }
    }
    
    static void playNoteVelocity(int voice,uint16_t note,int velocity,uint32_t duration)
    {
        _sid_command c;
        c.note=note;
        c.duration=duration;
        c.velocity=velocity;
        _sid_voices_busy[voice]=true;
        xQueueSend(_sid_voicesQueues[voice], &c, portMAX_DELAY);
    }
    
    static void playNote(int voice,uint16_t note,uint32_t duration)
    {
        _sid_command c;
        c.note=note;
        c.duration=duration;
        c.velocity=120;
        _sid_voices_busy[voice]=true;
        xQueueSend(_sid_voicesQueues[voice], &c, portMAX_DELAY);
    }
    static void stopNote(int voice)
    {
        _sid_command c;
        c.note=0;
        c.duration=0;
        c.velocity=0;
        _sid_voices_busy[voice]=true;
        xQueueSend(_sid_voicesQueues[voice], &c, portMAX_DELAY);
    }
    static void playNoteHz(int voice,int frequencyHz,uint32_t duration)
    {
        double fout=frequencyHz*16.777216;
        playNote(voice,(uint16_t)fout,duration);
    }
    static bool isVoiceBusy(int voice)
    {
        vTaskDelay(1);
       // Serial.printf("inf %d\n",_sid_voices_busy[voice]);
        return _sid_voices_busy[voice];
    }
    static bool areAllVoiceBusy()
    {
        bool res=false;
         vTaskDelay(1);
        for(int i=0;i<keyboardnbofvoices;i++)
        {
            if( _sid_voices_busy[i])
            {
                res=true;
                break;
            }
        }
        return res;
    }
    static bool allTaskReady(){
       
        vTaskDelay(1);
        for(int i=0;i<keyboardnbofvoices;i++)
        {
            if( _sid_taskready_busy[i]=false)
                return true;
        }
        return false;
    }
};


#endif /* SID6581_h */
