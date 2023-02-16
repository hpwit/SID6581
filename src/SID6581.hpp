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

// #if defined BOARD_HAS_PSRAM
//   #define sid_malloc  ps_malloc
//   #define sid_calloc  ps_calloc
//   #define sid_realloc ps_realloc
// #else
  #define sid_malloc  malloc
  #define sid_calloc  calloc
  #define sid_realloc realloc
//#endif

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

#include <FS.h>
#include <SPI.h>
#include "modules/MD5Hash/SID_MD5.h"
#include "driver/i2s.h"

#if defined (CONFIG_IDF_TARGET_ESP32S3)
  #include "hal/spi_types.h"
#endif



/*
struct reg_4bit_dataLE_t
{ // pulsewidth hi data bits
  bool : 1;
  bool : 1;
  bool : 1;
  bool : 1;
  bool D3 : 1;
  bool D2 : 1;
  bool D1 : 1;
  bool D0 : 1;
  bool operator [](uint8_t bitpos) {
    switch(bitpos%8) {
      case 0: return D0;
      case 1: return D1;
      case 2: return D2;
      case 3: return D3;
      default: return false;
    }
  }
};
*/


/*
struct reg_4bit_dataBE_t
{ // pulsewidth hi data bits
  bool D0 : 1;
  bool D1 : 1;
  bool D2 : 1;
  bool D3 : 1;
  bool : 1;
  bool : 1;
  bool : 1;
  bool : 1;
  bool operator [](uint8_t bitpos) {
    switch(bitpos%8) {
      case 4: return D3;
      case 5: return D2;
      case 6: return D1;
      case 7: return D0;
      default: return false;
    }
  }
};
*/


/*
typedef struct {
  struct bits_t {
    bool tx_done: 1;
    bool rx_done: 1;
    bool preamble: 1;
    bool sync: 1;
    bool header_valid: 1;
    bool header_err: 1;
    bool crc_err: 1;
    bool cad_done: 1;

    bool cad_detected: 1;
    bool timeout: 1;
    uint8_t _res: 6;
  };

  union {
    bits_t bits;
    uint16_t word;
  };
} subghz_irq_status_t;
*/
/*
struct reg_8bit_ctrl_t
{ // control reg data bits
  bool gate     :1;
  bool sync     :1;
  bool ringmode :1;
  bool test     :1;
  bool triangle :1;
  bool sawtooth :1;
  bool square   :1;
  bool noise    :1;
};*/



/*
struct SID_Reg_Data4_t
{
  union {
    uint8_t raw;
    reg_4bit_dataBE_t pins;
  };
  uint8_t value() { return raw - ( (raw >> 4)<<4 ); }
  void print( SID_Reg_Address_t addr ) { addr.print(); for( int i=0;i<8;i++)  Serial.printf(" %s ", i>3 ? pins[i]?"1":"0" : pins[i]?"+":"-" ); }
};
*/

/*
struct SID_Reg_Data12_t
{ // generic lo/hi 12 bit data
  SID_Reg_Data8_t lo;
  SID_Reg_Data4_t hi;
};*/

/*
struct reg_8bit_pulse_t
{
  uint8_t _res  : 4; // unused
  uint8_t pw_hi : 4;
};
*/

/*
struct SID_Reg_Data16_t
{ // generic lo/hi 16 bit data
  SID_Reg_Data8_t lo;
  SID_Reg_Data8_t hi;
};
*/
/*
struct SID_Reg_Data12_t
{
  SID_Reg_Data4_Lo_Nibble_t lo;
  SID_Reg_Data8_t hi;
};
*/
/*
struct SID_Reg_Data11_t
{ // generic lo/hi 11 bit data
  SID_Reg_Data8_t lo;
  SID_Reg_Data3_t hi;
};
*/


struct reg_3bit_dataLO_t { // filter cutoff lo data bits
  bool D0 : 1;
  bool D1 : 1;
  bool D2 : 1;
  uint8_t _res: 5;
  bool operator [](uint8_t bitpos) {
    switch(bitpos) {
      case 0: return D0;
      case 1: return D1;
      case 2: return D2;
      default: return false;
    }
  }
};

struct reg_4bit_lo_nibble_t
{
  bool D0 : 1;
  bool D1 : 1;
  bool D2 : 1;
  bool D3 : 1;
  uint8_t _res : 4;
  bool operator [](uint8_t bitpos) {
    switch(bitpos) {
      case 0: return D0;
      case 1: return D1;
      case 2: return D2;
      case 3: return D3;
      default: return false;
    }
  }
};

struct reg_5bit_addrLE_t
{ // address pins
  bool A0 : 1;
  bool A1 : 1;
  bool A2 : 1;
  bool A3 : 1;
  bool A4 : 1;
  uint8_t _res : 3;
  bool operator [](uint8_t bitpos) {
    switch(bitpos) {
      case 0: return A0;
      case 1: return A1;
      case 2: return A2;
      case 3: return A3;
      case 4: return A4;
      default: return false;
    }
  }
} ;

struct reg_8bit_data_t
{ // data pins
  bool D0 : 1;
  bool D1 : 1;
  bool D2 : 1;
  bool D3 : 1;
  bool D4 : 1;
  bool D5 : 1;
  bool D6 : 1;
  bool D7 : 1;
  bool operator [](uint8_t bitpos) {
    switch(bitpos) {
      case 0: return D0;
      case 1: return D1;
      case 2: return D2;
      case 3: return D3;
      case 4: return D4;
      case 5: return D5;
      case 6: return D6;
      case 7: return D7;
      default: return false;
    }
  }
};


struct SID_Reg_Address_t
{ // register address
  union {
    uint8_t bytes[1];
    reg_5bit_addrLE_t pins;
    uint8_t value;
  };
  SID_Reg_Address_t( uint8_t addr ) { value=addr; }
  SID_Reg_Address_t operator+(int offset ) {
    SID_Reg_Address_t ret = SID_Reg_Address_t(value);
    ret.value += offset;
    return ret;
  }
  SID_Reg_Address_t operator-(int offset ) {
    SID_Reg_Address_t ret = SID_Reg_Address_t(value);
    ret.value -= offset;
    return ret;
  }
  void print() {  Serial.printf("$%02x ", value); }
};


struct SID_Reg_Data3_t
{ // 3 first bits of filter cutoff (FC_LO)
  union {
    uint8_t bytes[1];
    reg_3bit_dataLO_t pins;
    uint8_t value;
  };
  void print( SID_Reg_Address_t addr ) { addr.print(); for( int i=7;i>=0;i-- ) Serial.printf(" %s ", i<=3 ? pins[i]?"1":"0" : pins[i]?"+":"-" ); }
};


struct SID_Reg_Data4_Lo_Nibble_t
{ // 4 first bytes of pulsewitdh (PW_HI)
  union {
    uint8_t bytes[1];
    reg_4bit_lo_nibble_t pins;
    uint8_t value;
  };
  //uint8_t value() { return raw - ( (raw >> 4)<<4 ); }
  void print( SID_Reg_Address_t addr ) { addr.print(); for( int i=7;i>=0;i-- )  Serial.printf(" %s ", i<4 ? pins[i]?"1":"0" : pins[i]?"+":"-" ); }
};


struct SID_Reg_Data8_t
{ // generic 8bits reg data
  SID_Reg_Data8_t( uint8_t val=0 ) : value(val) { }
  union {
    uint8_t bytes[1];
    reg_8bit_data_t pins;
    uint8_t value;
  };
  void print( SID_Reg_Address_t addr ) { addr.print(); for( int i=7;i>=0;i-- ) Serial.printf(" %d ", pins[i] ); }
};


struct SID_Reg_Data_Freq_t
{ // frequency data
  SID_Reg_Data_Freq_t( uint8_t lo, uint8_t hi ) : value( lo+hi*256 ) {}
  struct word16_t {
    SID_Reg_Data8_t lo;
    SID_Reg_Data8_t hi;
  };
  union {
    uint8_t bytes[2];
    word16_t data16;
    uint16_t value;
  };
  void print( SID_Reg_Address_t addr )
  {
    Serial.printf("[0x%02x] ", bytes[0]);
    data16.lo.print( addr );
    Serial.printf( "FREQ LO: %02x\n", data16.lo.value );
    Serial.printf("[0x%02x] ", bytes[1]);
    data16.hi.print( addr+1 );
    Serial.printf( "FREQ HI: %02x - ", data16.hi.value );
    Serial.printf("Frequency: 0x%04x\n", value );
  }
};


struct SID_Reg_Data_Pulse_t
{ // pulsewidth data
  SID_Reg_Data_Pulse_t( uint8_t lo, uint8_t hi ) : value( lo+hi*256 ) {}
  struct word12_t {
    SID_Reg_Data8_t lo;
    SID_Reg_Data4_Lo_Nibble_t hi;
  };
  union {
    uint8_t bytes[2];
    word12_t data12;
    uint16_t value;
  };
  void print( SID_Reg_Address_t addr )
  {
    Serial.printf("[0x%02x] ", bytes[0]);
    data12.lo.print( addr );
    Serial.printf( "PW LO: %02x\n", data12.lo.value );
    Serial.printf("[0x%02x] ", bytes[1]);
    data12.hi.print( addr+1 );
    Serial.printf( "PW HI: %02x - ", data12.hi.value );
    Serial.printf("Pulse Width: 0x%04x\n", value );
  }
};


struct SID_Reg_Data_Ctrl_t
{ // waveform data
  SID_Reg_Data_Ctrl_t( uint8_t flags ) : data( flags ) {}
  union {
    uint8_t bytes[1];
    reg_8bit_data_t flags;
    SID_Reg_Data8_t data;
  };
  bool noise()    { return flags.D7; }
  bool square()   { return flags.D6; }
  bool sawtooth() { return flags.D5; }
  bool triangle() { return flags.D4; }
  bool test()     { return flags.D3; }
  bool ringmode() { return flags.D2; }
  bool sync()     { return flags.D1; }
  bool gate()     { return flags.D0; }
  void print( SID_Reg_Address_t addr )
  {
    Serial.printf("[0x%02x] ", bytes[0]);
    data.print( addr ); Serial.printf("NOI:%d SQU:%d SAW:%d TRI:%d TES:%d RIN:%d SYN:%d GAT:%d\n", noise(), square(), sawtooth(), triangle(), test(), ringmode(), sync(), gate() );
  }
};


struct SID_Reg_Data_AtkDcy_t
{ // attack / decay registers
  SID_Reg_Data_AtkDcy_t( uint8_t nibbles ) : data( nibbles ) {}
  struct nibble_t {
    uint8_t decay  : 4;
    uint8_t attack : 4;
  };
  union {
    uint8_t bytes[1];
    nibble_t regs;
    SID_Reg_Data8_t data;
  };
  uint8_t attack() { return regs.attack; }
  uint8_t decay()  { return regs.decay; }
  void print( SID_Reg_Address_t addr )
  {
    Serial.printf("[0x%02x] ", bytes[0]);
    data.print( addr ); Serial.printf("Attack: %02x, Decay: %02x\n", attack(), decay() );
  }
};


struct SID_Reg_Data_StnRls_t
{ // sustain / release registers
  SID_Reg_Data_StnRls_t( uint8_t nibbles ) : data( nibbles ) {}
  struct nibble_t {
    uint8_t release : 4;
    uint8_t sustain : 4;
  };
  union {
    uint8_t bytes[1];
    nibble_t regs;
    SID_Reg_Data8_t data;
  };
  uint8_t sustain() { return regs.sustain; }
  uint8_t release() { return regs.release; }
  void print( SID_Reg_Address_t addr )
  {
    Serial.printf("[0x%02x] ", bytes[0]);
    data.print( addr ); Serial.printf("Sustain: %02x, Release: %02x\n", sustain(), release() );
  }
};


struct SID_Reg_Data_Cutoff_t
{ // filter data
  struct word11_t {
    SID_Reg_Data3_t lo;
    SID_Reg_Data8_t hi;
  };
  union {
    uint8_t bytes[2];
    uint16_t word;
    word11_t data11;
  };
  uint16_t frequency() { return (data11.hi.value<<3) + data11.lo.value; }
  void set( uint16_t freq ) { data11.lo.value = freq & 0b111; data11.hi.value = (freq>>3) & 0xff; }
};


struct SID_Reg_Data_ResFil_t
{ // res / filter registers
  struct word8_t {
    bool filt1  : 1;
    bool filt2  : 1;
    bool filt3  : 1;
    bool filtEx : 1;
    uint8_t resonance : 4;
  };
  union {
    uint8_t bytes[1];
    SID_Reg_Data8_t data;
    word8_t regs;
  };
  uint8_t resonance() { return regs.resonance; }
  bool filtEx() { return regs.filtEx; }
  bool filt1()  { return regs.filt1; }
  bool filt2()  { return regs.filt2; }
  bool filt3()  { return regs.filt3; }
  void print( SID_Reg_Address_t addr ) { data.print( addr ); Serial.printf( "Resonance: 0x%02x, Filters [Ex:%d fil3:%d fil2:%d fil1:%d]\n", resonance(), filtEx(), filt3(), filt2(), filt1()  ); }
};


struct SID_Reg_Data_ModVol_t
{ // mode / volume registers
  struct word8_t {
    uint8_t volume : 4;
    bool lp :1;
    bool bp :1;
    bool hp :1;
    bool mode3off :1;
  };
  union {
    uint8_t bytes[1];
    SID_Reg_Data8_t data;
    word8_t regs;
  };
  uint8_t volume() { return regs.volume; }
  bool mode3off()  { return regs.mode3off; }
  bool hp()        { return regs.hp; }
  bool bp()        { return regs.bp; }
  bool lp()        { return regs.lp; }
  void print( SID_Reg_Address_t addr )
  {
    data.print( addr );
    Serial.printf( "3off:%d hp:%d bp:%d lp:%d Volume: 0x%02x\n", mode3off(), hp(), bp(), lp(), volume() );
  }
};


struct SID_Filter_Register_t
{ // cutoff/res_filr/mod_vol registers
  struct regs_t {
    SID_Reg_Data_Cutoff_t cutoff;
    SID_Reg_Data_ResFil_t res_fil;
    SID_Reg_Data_ModVol_t mod_vol;
  };
  union {
    uint8_t bytes[4];
    SID_Reg_Data8_t data[4];
    regs_t regs;
  };
  void print( SID_Reg_Address_t addr )
  {
    Serial.printf("[0x%02x] ", bytes[0]); regs.cutoff.data11.lo.print( addr );   Serial.printf( "FC LO: 0x%02x\n", regs.cutoff.data11.lo.value );
    Serial.printf("[0x%02x] ", bytes[1]); regs.cutoff.data11.hi.print( addr+1 ); Serial.printf( "FC HI: 0x%02x - ", regs.cutoff.data11.hi.value );
    Serial.printf("Filter: 0x%04x \n", regs.cutoff.frequency() );
    Serial.printf("[0x%02x] ", bytes[2]); regs.res_fil.print(addr+2);
    Serial.printf("[0x%02x] ", bytes[3]); regs.mod_vol.print(addr+3);
  }
};


struct SID_Misc_Register_t
{ // misc registers
  struct regs_t {
    SID_Reg_Data8_t potX;
    SID_Reg_Data8_t potY;
    SID_Reg_Data8_t osc3;
    SID_Reg_Data8_t env3;
  };
  union {
    uint8_t bytes[4];
    SID_Reg_Data8_t data[4];
    regs_t regs;
  };
  void print( SID_Reg_Address_t addr )
  {
    Serial.printf("[0x%02x] ", bytes[0]); regs.potX.print( addr );   Serial.printf("POTX: 0x%02x\n", regs.potX.value );
    Serial.printf("[0x%02x] ", bytes[1]); regs.potY.print( addr+1 ); Serial.printf("POTY: 0x%02x\n", regs.potY.value );
    Serial.printf("[0x%02x] ", bytes[2]); regs.osc3.print( addr+2 ); Serial.printf("OSC3: 0x%02x\n", regs.osc3.value );
    Serial.printf("[0x%02x] ", bytes[3]); regs.env3.print( addr+3 ); Serial.printf("ENV3: 0x%02x\n", regs.env3.value );
  }
};


struct SID_Voice_Register_t
{ // one voice
  struct voice_reg8_t {
    uint8_t freq_lo;
    uint8_t freq_hi;
    uint8_t pw_lo;
    uint8_t pw_hi;
    uint8_t ctrl_reg;
    uint8_t atkdcy;
    uint8_t stnrls;
  };
  union {
    uint8_t bytes[7];
    SID_Reg_Data8_t regdata[7];
    voice_reg8_t reg8;
  };
  uint16_t freq()    { return SID_Reg_Data_Freq_t  (reg8.freq_lo, reg8.freq_hi).value; }
  uint16_t pulse()   { return SID_Reg_Data_Pulse_t (reg8.pw_lo, reg8.pw_hi).value; }
  uint8_t attack()   { return SID_Reg_Data_AtkDcy_t(reg8.atkdcy).attack(); }
  uint8_t decay()    { return SID_Reg_Data_AtkDcy_t(reg8.atkdcy).decay(); }
  uint8_t sustain()  { return SID_Reg_Data_StnRls_t(reg8.stnrls).sustain(); }
  uint8_t release()  { return SID_Reg_Data_StnRls_t(reg8.stnrls).release(); }
  uint8_t waveform() { return SID_Reg_Data_Ctrl_t  (reg8.ctrl_reg).data.value; }
  bool noise()       { return SID_Reg_Data_Ctrl_t  (reg8.ctrl_reg).noise(); }
  bool square()      { return SID_Reg_Data_Ctrl_t  (reg8.ctrl_reg).square(); }
  bool sawtooth()    { return SID_Reg_Data_Ctrl_t  (reg8.ctrl_reg).sawtooth(); }
  bool triangle()    { return SID_Reg_Data_Ctrl_t  (reg8.ctrl_reg).triangle(); }
  bool test()        { return SID_Reg_Data_Ctrl_t  (reg8.ctrl_reg).test(); }
  bool ringmode()    { return SID_Reg_Data_Ctrl_t  (reg8.ctrl_reg).ringmode(); }
  bool sync()        { return SID_Reg_Data_Ctrl_t  (reg8.ctrl_reg).sync(); }
  bool gate()        { return SID_Reg_Data_Ctrl_t  (reg8.ctrl_reg).gate(); }
  void print(SID_Reg_Address_t addr)
  {
    Serial.printf("OBJ[%d]\n", sizeof(SID_Voice_Register_t) );
    SID_Reg_Data_Freq_t( reg8.freq_lo, reg8.freq_hi ).print( addr );
    SID_Reg_Data_Pulse_t(reg8.pw_lo, reg8.pw_hi).print( addr+2 );
    SID_Reg_Data_Ctrl_t(reg8.ctrl_reg).print( addr+4 );
    SID_Reg_Data_AtkDcy_t(reg8.atkdcy).print( addr+5 );
    SID_Reg_Data_StnRls_t(reg8.stnrls).print( addr+6 );
  }
};




struct SID_Registers_t
{ // all registers for one SID chip
  SID_Registers_t( uint8_t _regs[29] ) { memcpy(data, _regs, 29); }
  struct sid_regs_t {
    SID_Voice_Register_t  voices[3]; // 21 bytes
    SID_Filter_Register_t filters;   // 4 bytes
    SID_Misc_Register_t   misc;      // 4 bytes
  };

  union {
    uint8_t bytes[29];
    SID_Reg_Data8_t data[29];
    sid_regs_t regs;
  };

  SID_Filter_Register_t filters()          { return regs.filters; }
  SID_Misc_Register_t   misc()             { return regs.misc; }
  SID_Voice_Register_t  voice(uint8_t idx) { return regs.voices[idx%3]; }

  uint8_t potX()      { return misc().regs.potX.value; }
  uint8_t potY()      { return misc().regs.potY.value; }
  uint8_t osc3()      { return misc().regs.osc3.value; }
  uint8_t env3()      { return misc().regs.env3.value; }

  uint16_t cutoff()   { return filters().regs.cutoff.frequency(); }
  uint8_t volume()    { return filters().regs.mod_vol.volume(); }
  uint8_t resonance() { return filters().regs.res_fil.resonance(); }
  bool filtEx()       { return filters().regs.res_fil.filtEx(); }
  bool filt1()        { return filters().regs.res_fil.filt1(); }
  bool filt2()        { return filters().regs.res_fil.filt2(); }
  bool filt3()        { return filters().regs.res_fil.filt3(); }
  bool mode3off()     { return filters().regs.mod_vol.mode3off(); }
  bool hp()           { return filters().regs.mod_vol.hp(); }
  bool bp()           { return filters().regs.mod_vol.bp(); }
  bool lp()           { return filters().regs.mod_vol.lp(); }

  SID_Reg_Data8_t operator [](uint8_t address) { return data[address%29]; }

  void print()
  {
    for( int i=0;i<3;i++ ) {
      voice(i).print( SID_Reg_Address_t(i*sizeof(SID_Voice_Register_t)) );
    }
    Serial.printf("OBJ[%d]\n", sizeof(SID_Filter_Register_t) );
    regs.filters.print( SID_Reg_Address_t(0x15) );

    Serial.printf("OBJ[%d]\n", sizeof(SID_Misc_Register_t) );
    regs.misc.print( SID_Reg_Address_t(0x19) );
  }
};


// struct SID_Voice_Regs_t {
//   SID_Voice_Regs_t( uint8_t bytes[7] ) : freq(bytes[0], bytes[1]), pw(bytes[2],bytes[3]), ctrl_reg(bytes[4]), atkdcy(5), stnrls(6) { }
//   SID_Reg_Data_Freq_t   freq;     // 16bits: 2 bytes
//   SID_Reg_Data_Pulse_t  pw;       // 16bits: 2 bytes
//   SID_Reg_Data_Ctrl_t   ctrl_reg; // 8bits : 1 byte
//   SID_Reg_Data_AtkDcy_t atkdcy;   // 8bits : 2 nibbles
//   SID_Reg_Data_StnRls_t stnrls;   // 8bits : 2 nibbles
// };


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







typedef enum
{
  #if defined (CONFIG_IDF_TARGET_ESP32S3)
    SID_SPI1_HOST=SPI1_HOST,    ///< SPI1 (SID/SD)
    SID_SPI2_HOST=SPI2_HOST,    ///< SPI2 (TFT/TOUCH)
    SID_SPI3_HOST=SPI3_HOST,    ///< SPI3 (PSRAM)
    #define SPI_HOST_DEFAULT SID_SPI1_HOST
  #elif defined (CONFIG_IDF_TARGET_ESP32) //|| defined (CONFIG_IDF_TARGET_ESP32S2)
    //SPI1 can be used as GPSPI only on ESP32
    SID_SPI1_HOST=0,    ///< SPI1
    SID_SPI2_HOST=1,    ///< SPI2
    SID_SPI3_HOST=2,    ///< SPI3
    #define SPI_HOST_DEFAULT SID_SPI1_HOST
  #else
    #error "Unsupported architecture, feel free to unlock and contribute!"
  #endif
} sid_spi_host_device_t;


class SID6581
{
  public:

    SID6581();
    ~SID6581();

    BaseType_t  SID_QUEUE_CORE = 0; // for SPI task
    UBaseType_t SID_QUEUE_PRIORITY = 3;

    void setTaskCore( BaseType_t uxCoreId ) { SID_QUEUE_CORE = uxCoreId; };
    void setTaskPriority( UBaseType_t uxPriority ) { SID_QUEUE_PRIORITY = uxPriority; };

    bool begin(int spi_clock_pin,int spi_data_pin, int latch );
    bool begin(int spi_clock_pin,int spi_data_pin, int latch,int sid_clock_pin);
    void end();

    void clearQueue();
    bool xQueueIsQueueEmpty();

    void setSPIHost( sid_spi_host_device_t devnum ) { SID_SPI_HOST = devnum; }

    void setInvertedPins( bool invert ) { _reverse_data = invert; } // use this if pins D{0-7} to 75HC595 are in reverse order

    void sidSetVolume(int chip, uint8_t vol);
    void setFrequency(int voice, uint16_t frequency);
    void setFrequencyHz(int voice, double frequencyHz);
    void setPulse(int voice, uint16_t pulse);
    void setEnv(int voice, uint8_t att,uint8_t decay,uint8_t sutain, uint8_t release);
    void setAttack(int voice, uint8_t att);
    void setDecay(int voice, uint8_t decay);
    void setSustain(int voice,uint8_t sutain);
    void setRelease(int voice,uint8_t release);
    void setGate(int voice, int gate);
    void setWaveForm(int voice, int waveform);
    void setTest(int voice, int test);
    void setSync(int voice, int sync);
    void setRingMode(int voice, int ringmode);
    void setFilterFrequency(int chip, int filterfrequency);
    void setResonance(int chip, int resonance);
    void setFilter1(int chip, int filt1);
    void setFilter2(int chip, int filt2);
    void setFilter3(int chip, int filt3);
    void setFilterEX(int chip, int filtex);
    void set3OFF(int chip, int _3off);
    void setHP(int chip, int hp);
    void setBP(int chip, int bp);
    void setLP(int chip,int lp);

    void pushToVoice(int voice, uint8_t address, uint8_t data);
    void pushRegister(int chip, int address, uint8_t data);
    void resetsid();

    uint8_t sidregisters[15*32];// bytes
    //uint8_t* getRegisters(int chip=0) { return &sidregisters[chip*32]; }
    uint8_t* getRegisters(int chip=0) { return &sidregisters[chip*32]; }
    SID_Registers_t copyRegisters(int chip=0) { return SID_Registers_t( getRegisters(chip) ); }

    // per-voice getters
    int getFrequency( int voice );
    double getFrequencyHz( int voice );
    int getPulse( int voice );
    int getAttack( int voice );
    int getDecay( int voice );
    int getSustain( int voice );
    int getRelease( int voice );
    int getGate( int voice );
    int getWaveForm( int voice );
    int getTest( int voice );
    int getSync( int voice );
    int getRingMode( int voice );

    // per-chip getters
    int getSidVolume( int chip=0 );
    int getFilterFrequency( int chip=0 );
    int getResonance( int chip=0 );
    int getFilter1( int chip=0 );
    int getFilter2( int chip=0 );
    int getFilter3( int chip=0 );
    int getFilterEX( int chip=0 );
    int get3OFF( int chip=0 );
    int getHP( int chip=0 );
    int getBP( int chip=0 );
    int getLP( int chip=0 );

    // per instance getters
    uint8_t getVolume( int chip=0 );
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

  private:
    unsigned char byte_reverse(unsigned char b);

  protected:

    sid_spi_host_device_t SID_SPI_HOST = SPI_HOST_DEFAULT;

    SPIClass      *sid_spi = NULL;
    //uint8_t       sidregisters[15*32];// bytes

    const i2s_port_t i2s_num = (i2s_port_t)0;
    int _latch_pin;
    int _spi_clock_pin;
    int _spi_data_pin;
    bool _reverse_data = false;
    const uint32_t sid_spiClk = 25000000;
    int volume[5];
    SID_Control_t sid_control[5];
    uint8_t adcswrre, dataspi, chipcontrol;
    void playNextInt();
    static void xPushRegisterTask(void *pvParameters);

};



#if defined SID_PLAYER
  #include "modules/Player/SID_Player.hpp"
#endif

#if defined SID_INSTRUMENTS
  //#ifndef SID_PLAYER
  //  static SID6581 sid;
  //  #include "modules/Player/SID_Player.h"
  //#endif
  #include "modules/Instruments/SID_Instruments.hpp"
#endif


#endif /* SID6581_h */
