/*\
 *
 *

    MIT License

    Copyright (c) 2020 Yves BAZIN

    Permission is hereby granted, free of charge, to any person obtaining a copy of
    this software and associated documentation files (the "Software"), to deal in
    the Software without restriction, including without limitation the rights to
    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

 *
 *
\*/

#ifndef _SID6581_H_
#define _SID6581_H_

#ifdef BOARD_HAS_PSRAM
  #define sid_malloc  ps_malloc
  #define sid_calloc  ps_calloc
  #define sid_realloc ps_realloc
#else
  #define sid_malloc  malloc
  #define sid_calloc  calloc
  #define sid_realloc realloc
#endif


#define RESET 0
#define MASK_ADDRESS 0b11111000
#define MASK_CSWRRE 0b111
#define SID_WAVEFORM_TRIANGLE (1<<4)
#define SID_WAVEFORM_SAWTOOTH (1<<5)
#define SID_WAVEFORM_PULSE (1<<6)
#define SID_WAVEFORM_NOISE (1<<7)
#define SID_WAVEFORM_SILENCE 0
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

#define SID_QUEUE_SIZE 2000

#include "FS.h"
#include "modules/MD5Hash/SID_MD5.h"
#include "driver/i2s.h"


typedef struct
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
  uint8_t freq_lo,
          freq_hi,
          pw_lo,
          pw_hi,
          control_reg,
          attack_decay,
          sustain_release;
} SID_Voice_t;


typedef struct
{
  uint8_t volume;
  uint8_t filterfrequency;
  uint8_t res;
  uint8_t filt1,
          filt2,
          filt3,
          filtex;
  uint8_t _3off,
          hp,
          bp,
          lp;
  uint8_t fc_lo,
          fc_hi,
          res_filt,
          mode_vol;
} SID_Control_t;


typedef enum {
//SPI1 can be used as GPSPI only on ESP32
    SID_SPI1_HOST=0,    ///< SPI1
    SID_SPI2_HOST=1,    ///< SPI2
    SID_SPI3_HOST=2,    ///< SPI3
} sid_spi_host_device_t;



class SID6581
{
  public:

    SID6581();
    ~SID6581();

    BaseType_t  SID_QUEUE_CORE = 0; // for SPI task
    UBaseType_t SID_QUEUE_PRIORITY = 3;

    bool begin(int spi_clock_pin,int spi_data_pin, int latch );
    bool begin(int spi_clock_pin,int spi_data_pin, int latch,int sid_clock_pin);
    void end();

    void setTaskCore( BaseType_t uxCoreId ) { SID_QUEUE_CORE = uxCoreId; };
    void setTaskPriority( UBaseType_t uxPriority ) { SID_QUEUE_PRIORITY = uxPriority; };
    void clearQueue();
    bool xQueueIsQueueEmpty();

    void setSPIHost( sid_spi_host_device_t devnum ) { SID_SPI_HOST = devnum; }

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
    void pushRegister(int chip,int address,uint8_t data);
    void resetsid();
    void feedTheDog();

    int getSidVolume( int chip);
    int getFrequency(int voice);
    double getFrequencyHz(int voice);
    int getPulse(int voice);
    int getAttack(int voice);
    int getDecay(int voice);
    int getSustain(int voice);
    int getRelease(int voice);
    int getGate(int voice);
    int getWaveForm(int voice);
    int getTest(int voice);
    int getSync(int voice);
    int getRingMode(int voice);
    int getFilterFrequency(int chip);
    int getResonance(int chip);
    int getFilter1(int chip);
    int getFilter2(int chip);
    int getFilter3(int chip);
    int getFilterEX(int chip);
    int get3OFF(int chip);
    int getHP(int chip);
    int getBP(int chip);
    int getLP(int chip);

    int getRegister(int chip,int addr);

    SID_Voice_t voices[15];

    void clearcsw(int chip);
    //void resetsid();
    void push();

    void setA(uint8_t val);
    void setcsw();
    void setD(uint8_t val);
    void cls();
    void soundOn();
    void soundOff();
    void setMaxVolume(uint8_t vol);

  protected:

    sid_spi_host_device_t SID_SPI_HOST = SID_SPI1_HOST; // SID_SPI1_HOST=SPI, SID_SPI2_HOST=HSPI, SID_SPI3_HOST=VSPI_HOST

    const i2s_port_t i2s_num = (i2s_port_t)0;
    int  latch_pin;
    const uint32_t sid_spiClk = 25000000;
    int volume[5];
    SID_Control_t sid_control[5];
    uint8_t adcswrre, dataspi, chipcontrol;
    void playNextInt();
    static void xPushRegisterTask(void *pvParameters);

};



#ifdef SID_PLAYER
  #include "modules/Player/SID_Player.h"
#endif

#ifdef SID_INSTRUMENTS
  //#ifndef SID_PLAYER
  //  static SID6581 sid;
  //  #include "modules/Player/SID_Player.h"
  //#endif
  #include "modules/Instruments/SID_Instruments.h"
#endif


#endif /* SID6581_h */
