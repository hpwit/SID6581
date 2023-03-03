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

#include "SID6581.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"


//#include "freertos/queue.h"

#define SID_CS 2
#define SID_WRITE 1
#define SID_CS_1 1
#define SID_WRITE_1 0

#define SID_CS_2 3
#define SID_WRITE_2 2

#define SID_CS_3 5
#define SID_WRITE_3 4

#define SID_CS_4 7
#define SID_WRITE_4 6

static TaskHandle_t  xPushToRegisterHandle = NULL;
static QueueHandle_t xSIDQueue = NULL;
static bool          i2s_begun = false;
static bool          spi_begun = false;
static bool          exitRegisterTask = false;


typedef struct
{
  uint8_t data;
  uint8_t chip;
  uint8_t address;
} SID_Register_t;



SID6581::~SID6581()
{
  end();
}


SID6581::SID6581()
{
  reg_mux = xSemaphoreCreateMutex();
  xSemaphoreGive(reg_mux);
}


void SID6581::end()
{
  if( spi_begun ) {
    exitRegisterTask = true;
    soundOff();
    while(exitRegisterTask) vTaskDelay(1);
    sid_spi->end();
    spi_begun = false;
  }
  if( i2s_begun ) {
    i2s_driver_uninstall( i2s_num );
    i2s_begun = false;
  }
  log_i("SID6581 successfully ended");
}


bool SID6581::begin(int spi_clock_pin,int spi_data_pin, int latch,int sid_clock_pin)
{

  if( !i2s_begun )
  {
    //const i2s_port_t i2s_num = (i2s_port_t)2;
    const i2s_config_t i2s_config =
    {
      .mode =(i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = 31250,
      .bits_per_sample = (i2s_bits_per_sample_t)16,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    #if defined ESP_IDF_VERSION_MAJOR && ESP_IDF_VERSION_MAJOR >= 4
      .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB),
    #else
      .communication_format = (i2s_comm_format_t) (I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    #endif
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL3, // 0 = default interrupt priority
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = false
    };
    const i2s_pin_config_t pin_config =
    {
      .bck_io_num = sid_clock_pin,
      .ws_io_num = -1,//NULL,
      .data_out_num = -1, //NULL,
      .data_in_num = I2S_PIN_NO_CHANGE
    };



    i2s_driver_install( i2s_num, &i2s_config, 0, NULL );   //install and start i2s driver

    i2s_set_pin( i2s_num, &pin_config );

    i2s_begun = true;
    log_i("I2S config done, using port#%d on pin %d", i2s_num, sid_clock_pin);
  } else {
    log_d("Skipping I2S config");
  }

  return begin( spi_clock_pin, spi_data_pin, latch );
}



bool SID6581::begin( int spi_clock_pin,int spi_data_pin, int latch )
{

  #ifdef BOARD_HAS_PSRAM
    if( !psramInit() ) {
      log_e("Failed to init psram, crash will probably occur");
    } else {
      log_d("Psram successfully detected");
    }
  #endif

  /*
    We set up the spi bus which connects to the 74HC595
    */

  if( !spi_begun ) {
    #if defined (CONFIG_IDF_TARGET_ESP32S3)
      sid_spi = new SPIClass(SID_SPI_HOST);
    #else
      switch( SID_SPI_HOST ) {
        case SID_SPI1_HOST: sid_spi = new SPIClass(HSPI); break;
        case SID_SPI2_HOST: sid_spi = new SPIClass(VSPI); break;
        default:
        case SID_SPI3_HOST: sid_spi = new SPIClass(SPI); break;
      }
    #endif
    if( sid_spi==NULL ) return false;
    sid_spi->begin( spi_clock_pin, MISO, spi_data_pin, -1 );
    _latch_pin = latch;
    pinMode( latch, OUTPUT );
    log_i("SPI Initialized (clock pin=%d, data_pin=%d, core=%d)", spi_clock_pin, spi_data_pin, xPortGetCoreID() );
    spi_begun = true;
  }

  _spi_clock_pin = spi_clock_pin;
  _spi_data_pin  = spi_data_pin;


  if( xPushToRegisterHandle == NULL ) {
    log_i("Attaching SID Register Queue to core #%d with priority %d", SID_QUEUE_CORE, SID_QUEUE_PRIORITY );
    xSIDQueue = xQueueCreate( SID_QUEUE_SIZE, sizeof( SID_Register_t ) );
    xTaskCreatePinnedToCore(SID6581::xPushRegisterTask, "xPushRegisterTask", 1024, this, SID_QUEUE_PRIORITY, &xPushToRegisterHandle, SID_QUEUE_CORE);
  }

  resetsid();
  return true;
}

void SID6581::clearQueue()
{
  if( xSIDQueue != NULL ) {
    xQueueReset( xSIDQueue );
  }
}


double SID6581::getFrequencyHz(int voice)
{
  return (double)getFrequency(voice)/16.777216;
}


int SID6581::getFrequency(int voice)
{
  int chip=voice/3;
  return sidregisters[chip*32+voice*7]+sidregisters[chip*32+voice*7+1]*256;
}


int SID6581::getPulse(int voice)
{
  int chip=voice/3;
  return sidregisters[chip*32+voice*7+2]+sidregisters[chip*32+voice*7+3]*256;
}


int SID6581::getAttack(int voice)
{
  int chip=voice/3;
  uint8_t reg=sidregisters[chip*32+voice*7+5];
  return (reg>>4);
}


int SID6581::getDecay(int voice)
{
  int chip=voice/3;
  uint8_t reg=sidregisters[chip*32+voice*7+5];
  return (reg&0xf);
}


int SID6581::getSustain(int voice)
{
  int chip=voice/3;
  uint8_t reg=sidregisters[chip*32+voice*7+6];
  return (reg>>4);
}


int SID6581::getRelease(int voice)
{
  int chip=voice/3;
  uint8_t reg=sidregisters[chip*32+voice*7+6];
  return (reg & 0xf);
}


int SID6581::getGate(int voice)
{
  int chip=voice/3;
  uint8_t reg=sidregisters[chip*32+voice*7+4];
  return (reg & 0x1);
}


int SID6581::getTest(int voice)
{
  int chip=voice/3;
  uint8_t reg=sidregisters[chip*32+voice*7+4];
  return ((reg & 0x8)>>3);
}


int SID6581::getSync(int voice)
{
  int chip=voice/3;
  uint8_t reg=sidregisters[chip*32+voice*7+4];
  return ((reg & 0x2)>>1);
}


int SID6581::getRingMode(int voice)
{
  int chip=voice/3;
  uint8_t reg=sidregisters[chip*32+voice*7+4];
  return ((reg & 0x4)>>2);
}


int SID6581::getWaveForm(int voice)
{
  int chip=voice/3;
  uint8_t reg=sidregisters[chip*32+voice*7+4];
  return (reg  &0xF0);
}


int SID6581::get3OFF(int chip)
{
  uint8_t reg=sidregisters[chip*32+24];
  return (reg >>7);
}


int SID6581::getHP(int chip)
{
  uint8_t reg=sidregisters[chip*32+24];
  return (reg >>6) & 1;
}


int SID6581::getBP(int chip)
{
  uint8_t reg=sidregisters[chip*32+24];
  return (reg >>5) & 1;
}


int SID6581::getLP(int chip)
{
  uint8_t reg=sidregisters[chip*32+24];
  return (reg >>4) & 1;
}


int SID6581::getSidVolume(int chip)
{
  uint8_t reg=sidregisters[chip*32+24];
  return ((reg & 0xf));
}


int SID6581::getFilterFrequency(int chip)
{
  // TODO: fix this, return value is wrong !!
  // - - - - - 2 1 0
  // a 9 8 7 6 5 4 3
  return sidregisters[chip*32+0x15] + sidregisters[chip*32+0x16]*256;
}


int SID6581::getResonance(int chip)
{
  return sidregisters[chip*32+0x17] >>4;
}


int SID6581::getFilter1(int chip)
{
  uint8_t reg=sidregisters[chip*32+0x17];
  return (reg & 1);
}


int SID6581::getFilter2(int chip)
{
  uint8_t reg=sidregisters[chip*32+0x17];
  return (reg>>1) & 1;
}


int SID6581::getFilter3(int chip)
{
  uint8_t reg=sidregisters[chip*32+0x17];
  return (reg>>2) & 1;
}


int SID6581::getFilterEX(int chip)
{
  uint8_t reg=sidregisters[chip*32+0x17];
  return (reg>>3) & 1;
}


int SID6581::getRegister(int chip,int addr)
{
  return sidregisters[chip*32+addr];
}

uint8_t SID6581::getVolume(int chip)
{
  return sid_control[chip].volume;
}



void SID6581::setFrequencyHz(int voice,double frequencyHz)
{
  //calculate the frequency used by the 6581
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
  // log_v(voices[voice].freq_hi);
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
  voices[voice].waveform = waveform;
  voices[voice].control_reg = waveform + (voices[voice].control_reg & 0x01);
  pushToVoice(voice,4,voices[voice].control_reg);
}


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
  sid_control[chip].mode_vol=(sid_control[chip].mode_vol & 0xf0 )+( vol & 0x0f);
  pushRegister(chip,0x18,sid_control[chip].mode_vol);
}


void SID6581::setMaxVolume(uint8_t vol)
{
  for(int i=0;i<5;i++) {
    volume[i] = vol;
    sidSetVolume(i,  vol);
  }
}


void SID6581::soundOn()
{
  for(int i=0;i<5;i++) {
    int currentVolume = getSidVolume(i);
    if( currentVolume == volume[i] ) continue; // no change
    sidSetVolume(i,  volume[i]);
  }
}



void SID6581::soundOff()
{
  for(int i=0;i<5;i++) {
    volume[i]=getSidVolume(i);
    sidSetVolume(i,  0);
    if(volume[i]>0) {
      sidSetVolume(i,  0);
    }
//     // fermtaggle, really!
//     setAttack ( i, 0 ); // 0 (0-15)
//     setDecay  ( i, 0 ); // 2 (0-15)
//     setSustain( i, 0 ); // 15 (0-15)
//     setRelease( i, 0 ); // 10 (0-15)
//     setGate   ( i, 0 ); // 1 (0-1)
//     setPulse(     i, 0 ); // 1000 ( 0 - 4096 )
//     setWaveForm(  i, SID_WAVEFORM_SILENCE ); // SID_WAVEFORM_PULSE (TRIANGLE/SAWTOOTH/PULSE)
//     setFrequency( i, 0); // note 0-95
  }
}


void SID6581::setFilterFrequency(int chip,int filterfrequency)
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
  //sid_control[chip].filt1;
  sid_control[chip].res_filt=(sid_control[chip].res_filt & (0xff ^ SID_FILT1 )) + (filt1 & SID_FILT1);
  pushRegister(chip,0x17,sid_control[chip].res_filt);
}


void SID6581::setFilter2(int chip,int filt2)
{
  //sid_control[chip].filt2;
  sid_control[chip].res_filt=(sid_control[chip].res_filt & (0xff ^ SID_FILT2 )) + (filt2 & SID_FILT2);
  pushRegister(chip,0x17,sid_control[chip].res_filt);
}


void SID6581::setFilter3(int chip,int filt3)
{
  //sid_control[chip].filt3;
  sid_control[chip].res_filt=(sid_control[chip].res_filt & (0xff ^ SID_FILT3 )) + (filt3 & SID_FILT3);
  pushRegister(chip,0x17,sid_control[chip].res_filt);
}


void SID6581::setFilterEX(int chip,int filtex)
{
  //sid_control[chip].filtex;
  sid_control[chip].res_filt=(sid_control[chip].res_filt & (0xff ^ SID_FILTEX )) + (filtex & SID_FILTEX);
  pushRegister(chip,0x17,sid_control[chip].res_filt);
}


void SID6581::clearcsw(int chip)
{
  //adcswrre = (adcswrre ^ (1<<SID_WRITE) ) ^ (1<<SID_CS) ;
  chipcontrol=0xff;
  switch(chip) {
    case 0: adcswrre = (adcswrre ^ (1<<SID_WRITE) ) ^ (1<<SID_CS);   break;
    case 1: chipcontrol=  0xff ^ ((1<<SID_WRITE_1) | (1<<SID_CS_1)); break;
    case 2: chipcontrol=  0xff ^ ((1<<SID_WRITE_2) | (1<<SID_CS_2)); break;
    case 3: chipcontrol=  0xff ^ ((1<<SID_WRITE_3) | (1<<SID_CS_3)); break;
    case 4: chipcontrol=  0xff ^ ((1<<SID_WRITE_4) | (1<<SID_CS_4)); break;
  }
  push();
}


void SID6581::setcsw()
{
  adcswrre = adcswrre | (1<<SID_WRITE) | (1<<SID_CS);
  chipcontrol= 0xff;//(1<<SID_WRITE_1) | (1<<SID_CS_1);;
  dataspi=0;
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


void SID6581::pushRegister(int chip,int address,uint8_t data)
{
  // log_v("chip %d %x %x\n",chip,address,data);
  xSemaphoreTake(reg_mux, portMAX_DELAY);
  sidregisters[chip*32+address] = data;
  xSemaphoreGive(reg_mux);

  SID_Register_t sid_data;
  sid_data.address = address;
  sid_data.chip    = chip;
  sid_data.data    = data;
  xQueueSend( xSIDQueue, &sid_data, portMAX_DELAY );
}


bool SID6581::xQueueIsQueueEmpty()
{
  if( xSIDQueue == NULL ) return true;
  SID_Register_t tmp;
  if( xQueuePeek( xSIDQueue, &tmp, 0 )  == pdTRUE ) {
    return false;
  }
  return true;
}



unsigned char SID6581::byte_reverse(unsigned char b)
{
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}


void SID6581::xPushRegisterTask(void *pvParameters)
{
  SID6581 *sid = (SID6581 *)pvParameters;
  SID_Register_t* sid_data = (SID_Register_t*)sid_calloc(1, sizeof(SID_Register_t*));
  for(;;) {
    xQueueReceive( xSIDQueue, sid_data, portMAX_DELAY );
    // log_v("core s:%d\n",xPortGetCoreID());
    sid->setcsw();
    sid->setA(sid_data->address);
    sid->setD(sid_data->data);
    sid->clearcsw(sid_data->chip);
    if( exitRegisterTask ) break;
  }
  exitRegisterTask = false;
  free( sid_data );
  log_w("Leaving push register task");
  vTaskDelete(NULL);
}



void SID6581::push()
{
  if( spi_begun ) {
    sid_spi->beginTransaction(SPISettings(sid_spiClk, LSBFIRST, SPI_MODE0));
    digitalWrite(_latch_pin, 0);
    if(_reverse_data) dataspi = byte_reverse( dataspi );
    sid_spi->transfer(chipcontrol);
    sid_spi->transfer(adcswrre);
    sid_spi->transfer(dataspi);
    sid_spi->endTransaction();
    digitalWrite(_latch_pin, 1);
    //delayMicroseconds(1);
  }
}


void SID6581::pushToVoice(int voice,uint8_t address,uint8_t data)
{
  address=7*(voice%3)+address;
  pushRegister((int)(voice/3),address,data);
}



void SID6581::setA(uint8_t val)
{
  adcswrre=((val<<3) & MASK_ADDRESS) | (adcswrre & MASK_CSWRRE);
}


void SID6581::setD(uint8_t val)
{
  dataspi= val;
}


void SID6581::cls()
{
  dataspi=0;
}



#pragma GCC diagnostic pop
