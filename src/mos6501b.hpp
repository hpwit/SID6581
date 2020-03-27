//
//  mos6501b.hpp
//  
//
//  Created by Yves BAZIN on 25/03/2020.
//

#ifndef mos6501b_hpp
#define mos6501b_hpp

#include <stdio.h>
//
//  mos6501.h
//
//
//  Created by Yves BAZIN on 25/03/2020.
//



#include <stdio.h>
#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include "FS.h"
#include "SID6581.h"
#include "freertos/queue.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"


static QueueHandle_t  _sidtunes_voicesQueues;
static TaskHandle_t SIDTUNESSerialPlayerTaskHandle = NULL;
static TaskHandle_t SIDTUNESSerialPlayerTaskLock= NULL;


struct serial_command {
    uint8_t data;
    uint8_t address;
    uint16_t duration;
};

enum flag_enum{
flag_N= 128, flag_V= 64, flag_B= 16, flag_D= 8, flag_I= 4, flag_Z= 2, flag_C= 1
};

enum mode_enum {
    mode_imp,
    mode_imm,
    mode_abs,
    mode_absx,
    mode_absy,
    mode_zp,
    mode_zpx,
    mode_zpy,
    mode_ind,
    mode_indx,
    mode_indy,
    mode_acc,
    mode_rel,
    mode_xxx
};
enum inst_enum{
    inst_adc,
    inst_and,
    inst_asl,
    inst_bcc, inst_bcs, inst_beq, inst_bit, inst_bmi, inst_bne, inst_bpl, inst_brk, inst_bvc, inst_bvs, inst_clc,
    inst_cld, inst_cli, inst_clv, inst_cmp, inst_cpx, inst_cpy, inst_dec, inst_dex, inst_dey, inst_eor, inst_inc, inst_inx, inst_iny, inst_jmp,
    inst_jsr, inst_lda, inst_ldx, inst_ldy, inst_lsr, inst_nop, inst_ora, inst_pha, inst_php, inst_pla, inst_plp, inst_rol, inst_ror, inst_rti,
    inst_rts, inst_sbc, inst_sec, inst_sed, inst_sei, inst_sta, inst_stx, inst_sty, inst_tax, inst_tay, inst_tsx, inst_txa, inst_txs, inst_tya,
    inst_xxx
};


typedef struct instruction instruction;
struct instruction{
    enum inst_enum inst;
    enum mode_enum mode;
};
static volatile instruction opcodes[256]=
{
    {inst_brk, mode_imp},                            // 0x00
    {inst_ora, mode_indx},                            // 0x01
    {inst_xxx, mode_xxx},                            // 0x02
    {inst_xxx, mode_xxx},                            // 0x03
    {inst_xxx, mode_zp},                            // 0x04
    {inst_ora, mode_zp},                            // 0x05
    {inst_asl, mode_zp},                            // 0x06
    {inst_xxx, mode_xxx},                            // 0x07
    {inst_php, mode_imp},                            // 0x08
    {inst_ora, mode_imm},                            // 0x09
    {inst_asl, mode_acc},                            // 0x0a
    {inst_xxx, mode_xxx},                            // 0x0b
    {inst_xxx, mode_abs},                            // 0x0c
    {inst_ora, mode_abs},                            // 0x0d
    {inst_asl, mode_abs},                            // 0x0e
    {inst_xxx, mode_xxx},                            // 0x0f
    {inst_bpl, mode_rel},                            // 0x10
    {inst_ora, mode_indy},                            // 0x11
    {inst_xxx, mode_xxx},                            // 0x12
    {inst_xxx, mode_xxx},                            // 0x13
    {inst_xxx, mode_xxx},                            // 0x14
    {inst_ora, mode_zpx},                            // 0x15
    {inst_asl, mode_zpx},                            // 0x16
    {inst_xxx, mode_xxx},                            // 0x17
    {inst_clc, mode_imp},                            // 0x18
    {inst_ora, mode_absy},                            // 0x19
    {inst_xxx, mode_xxx},                            // 0x1a
    {inst_xxx, mode_xxx},                            // 0x1b
    {inst_xxx, mode_xxx},                            // 0x1c
    {inst_ora, mode_absx},                            // 0x1d
    {inst_asl, mode_absx},                            // 0x1e
    {inst_xxx, mode_xxx},                            // 0x1f
    {inst_jsr, mode_abs},                            // 0x20
    {inst_and, mode_indx},                            // 0x21
    {inst_xxx, mode_xxx},                            // 0x22
    {inst_xxx, mode_xxx},                            // 0x23
    {inst_bit, mode_zp},                            // 0x24
    {inst_and, mode_zp},                            // 0x25
    {inst_rol, mode_zp},                            // 0x26
    {inst_xxx, mode_xxx},                            // 0x27
    {inst_plp, mode_imp},                            // 0x28
    {inst_and, mode_imm},                            // 0x29
    {inst_rol, mode_acc},                            // 0x2a
    {inst_xxx, mode_xxx},                            // 0x2b
    {inst_bit, mode_abs},                            // 0x2c
    {inst_and, mode_abs},                            // 0x2d
    {inst_rol, mode_abs},                            // 0x2e
    {inst_xxx, mode_xxx},                            // 0x2f
    {inst_bmi, mode_rel},                            // 0x30
    {inst_and, mode_indy},                            // 0x31
    {inst_xxx, mode_xxx},                            // 0x32
    {inst_xxx, mode_xxx},                            // 0x33
    {inst_xxx, mode_xxx},                            // 0x34
    {inst_and, mode_zpx},                            // 0x35
    {inst_rol, mode_zpx},                            // 0x36
    {inst_xxx, mode_xxx},                            // 0x37
    {inst_sec, mode_imp},                            // 0x38
    {inst_and, mode_absy},                            // 0x39
    {inst_xxx, mode_xxx},                            // 0x3a
    {inst_xxx, mode_xxx},                            // 0x3b
    {inst_xxx, mode_xxx},                            // 0x3c
    {inst_and, mode_absx},                            // 0x3d
    {inst_rol, mode_absx},                            // 0x3e
    {inst_xxx, mode_xxx},                            // 0x3f
    {inst_rti, mode_imp},                            // 0x40
    {inst_eor, mode_indx},                            // 0x41
    {inst_xxx, mode_xxx},                            // 0x42
    {inst_xxx, mode_xxx},                            // 0x43
    {inst_xxx, mode_zp},                            // 0x44
    {inst_eor, mode_zp},                            // 0x45
    {inst_lsr, mode_zp},                            // 0x46
    {inst_xxx, mode_xxx},                            // 0x47
    {inst_pha, mode_imp},                            // 0x48
    {inst_eor, mode_imm},                            // 0x49
    {inst_lsr, mode_acc},                            // 0x4a
    {inst_xxx, mode_xxx},                            // 0x4b
    {inst_jmp, mode_abs},                            // 0x4c
    {inst_eor, mode_abs},                            // 0x4d
    {inst_lsr, mode_abs},                            // 0x4e
    {inst_xxx, mode_xxx},                            // 0x4f
    
    {inst_bvc, mode_rel},                            // 0x50
    {inst_eor, mode_indy},                            // 0x51
    {inst_xxx, mode_xxx},                            // 0x52
    {inst_xxx, mode_xxx},                            // 0x53
    {inst_xxx, mode_xxx},                            // 0x54
    {inst_eor, mode_zpx},                            // 0x55
    {inst_lsr, mode_zpx},                            // 0x56
    {inst_xxx, mode_xxx},                            // 0x57
    {inst_cli, mode_imp},                            // 0x58
    {inst_eor, mode_absy},                            // 0x59
    {inst_xxx, mode_xxx},                            // 0x5a
    {inst_xxx, mode_xxx},                            // 0x5b
    {inst_xxx, mode_xxx},                            // 0x5c
    {inst_eor, mode_absx},                            // 0x5d
    {inst_lsr, mode_absx},                            // 0x5e
    {inst_xxx, mode_xxx},                            // 0x5f
    
    {inst_rts, mode_imp},                            // 0x60
    {inst_adc, mode_indx},                            // 0x61
    {inst_xxx, mode_xxx},                            // 0x62
    {inst_xxx, mode_xxx},                            // 0x63
    {inst_xxx, mode_zp},                            // 0x64
    {inst_adc, mode_zp},                            // 0x65
    {inst_ror, mode_zp},                            // 0x66
    {inst_xxx, mode_xxx},                            // 0x67
    {inst_pla, mode_imp},                            // 0x68
    {inst_adc, mode_imm},                            // 0x69
    {inst_ror, mode_acc},                            // 0x6a
    {inst_xxx, mode_xxx},                            // 0x6b
    {inst_jmp, mode_ind},                            // 0x6c
    {inst_adc, mode_abs},                            // 0x6d
    {inst_ror, mode_abs},                            // 0x6e
    {inst_xxx, mode_xxx},                            // 0x6f
    
    {inst_bvs, mode_rel},                            // 0x70
    {inst_adc, mode_indy},                            // 0x71
    {inst_xxx, mode_xxx},                            // 0x72
    {inst_xxx, mode_xxx},                            // 0x73
    {inst_xxx, mode_xxx},                            // 0x74
    {inst_adc, mode_zpx},                            // 0x75
    {inst_ror, mode_zpx},                            // 0x76
    {inst_xxx, mode_xxx},                            // 0x77
    {inst_sei, mode_imp},                            // 0x78
    {inst_adc, mode_absy},                            // 0x79
    {inst_xxx, mode_xxx},                            // 0x7a
    {inst_xxx, mode_xxx},                            // 0x7b
    {inst_xxx, mode_xxx},                            // 0x7c
    {inst_adc, mode_absx},                            // 0x7d
    {inst_ror, mode_absx},                            // 0x7e
    {inst_xxx, mode_xxx},                            // 0x7f
    
    {inst_xxx, mode_imm},                            // 0x80
    {inst_sta, mode_indx},                            // 0x81
    {inst_xxx, mode_xxx},                            // 0x82
    {inst_xxx, mode_xxx},                            // 0x83
    {inst_sty, mode_zp},                            // 0x84
    {inst_sta, mode_zp},                            // 0x85
    {inst_stx, mode_zp},                            // 0x86
    {inst_xxx, mode_xxx},                            // 0x87
    {inst_dey, mode_imp},                            // 0x88
    {inst_xxx, mode_imm},                            // 0x89
    {inst_txa, mode_acc},                            // 0x8a
    {inst_xxx, mode_xxx},                            // 0x8b
    {inst_sty, mode_abs},                            // 0x8c
    {inst_sta, mode_abs},                            // 0x8d
    {inst_stx, mode_abs},                            // 0x8e
    {inst_xxx, mode_xxx},                            // 0x8f
    
    {inst_bcc, mode_rel},                            // 0x90
    {inst_sta, mode_indy},                            // 0x91
    {inst_xxx, mode_xxx},                            // 0x92
    {inst_xxx, mode_xxx},                            // 0x93
    {inst_sty, mode_zpx},                            // 0x94
    {inst_sta, mode_zpx},                            // 0x95
    {inst_stx, mode_zpy},                            // 0x96
    {inst_xxx, mode_xxx},                            // 0x97
    {inst_tya, mode_imp},                            // 0x98
    {inst_sta, mode_absy},                            // 0x99
    {inst_txs, mode_acc},                            // 0x9a
    {inst_xxx, mode_xxx},                            // 0x9b
    {inst_xxx, mode_xxx},                            // 0x9c
    {inst_sta, mode_absx},                            // 0x9d
    {inst_xxx, mode_absx},                            // 0x9e
    {inst_xxx, mode_xxx},                            // 0x9f
    
    {inst_ldy, mode_imm},                            // 0xa0
    {inst_lda, mode_indx},                            // 0xa1
    {inst_ldx, mode_imm},                            // 0xa2
    {inst_xxx, mode_xxx},                            // 0xa3
    {inst_ldy, mode_zp},                            // 0xa4
    {inst_lda, mode_zp},                            // 0xa5
    {inst_ldx, mode_zp},                            // 0xa6
    {inst_xxx, mode_xxx},                            // 0xa7
    {inst_tay, mode_imp},                            // 0xa8
    {inst_lda, mode_imm},                            // 0xa9
    {inst_tax, mode_acc},                            // 0xaa
    {inst_xxx, mode_xxx},                            // 0xab
    {inst_ldy, mode_abs},                            // 0xac
    {inst_lda, mode_abs},                            // 0xad
    {inst_ldx, mode_abs},                            // 0xae
    {inst_xxx, mode_xxx},                            // 0xaf
    
    {inst_bcs, mode_rel},                            // 0xb0
    {inst_lda, mode_indy},                            // 0xb1
    {inst_xxx, mode_xxx},                            // 0xb2
    {inst_xxx, mode_xxx},                            // 0xb3
    {inst_ldy, mode_zpx},                            // 0xb4
    {inst_lda, mode_zpx},                            // 0xb5
    {inst_ldx, mode_zpy},                            // 0xb6
    {inst_xxx, mode_xxx},                            // 0xb7
    {inst_clv, mode_imp},                            // 0xb8
    {inst_lda, mode_absy},                            // 0xb9
    {inst_tsx, mode_acc},                            // 0xba
    {inst_xxx, mode_xxx},                            // 0xbb
    {inst_ldy, mode_absx},                            // 0xbc
    {inst_lda, mode_absx},                            // 0xbd
    {inst_ldx, mode_absy},                            // 0xbe
    {inst_xxx, mode_xxx},                            // 0xbf
    
    {inst_cpy, mode_imm},                            // 0xc0
    {inst_cmp, mode_indx},                            // 0xc1
    {inst_xxx, mode_xxx},                            // 0xc2
    {inst_xxx, mode_xxx},                            // 0xc3
    {inst_cpy, mode_zp},                            // 0xc4
    {inst_cmp, mode_zp},                            // 0xc5
    {inst_dec, mode_zp},                            // 0xc6
    {inst_xxx, mode_xxx},                            // 0xc7
    {inst_iny, mode_imp},                            // 0xc8
    {inst_cmp, mode_imm},                            // 0xc9
    {inst_dex, mode_acc},                            // 0xca
    {inst_xxx, mode_xxx},                            // 0xcb
    {inst_cpy, mode_abs},                            // 0xcc
    {inst_cmp, mode_abs},                            // 0xcd
    {inst_dec, mode_abs},                            // 0xce
    {inst_xxx, mode_xxx},                            // 0xcf
    
    {inst_bne, mode_rel},                            // 0xd0
    {inst_cmp, mode_indy},                            // 0xd1
    {inst_xxx, mode_xxx},                            // 0xd2
    {inst_xxx, mode_xxx},                            // 0xd3
    {inst_xxx, mode_zpx},                            // 0xd4
    {inst_cmp, mode_zpx},                            // 0xd5
    {inst_dec, mode_zpx},                            // 0xd6
    {inst_xxx, mode_xxx},                            // 0xd7
    {inst_cld, mode_imp},                            // 0xd8
    {inst_cmp, mode_absy},                            // 0xd9
    {inst_xxx, mode_acc},                            // 0xda
    {inst_xxx, mode_xxx},                            // 0xdb
    {inst_xxx, mode_xxx},                            // 0xdc
    {inst_cmp, mode_absx},                            // 0xdd
    {inst_dec, mode_absx},                            // 0xde
    {inst_xxx, mode_xxx},                            // 0xdf
    
    {inst_cpx, mode_imm},                            // 0xe0
    {inst_sbc, mode_indx},                            // 0xe1
    {inst_xxx, mode_xxx},                            // 0xe2
    {inst_xxx, mode_xxx},                            // 0xe3
    {inst_cpx, mode_zp},                            // 0xe4
    {inst_sbc, mode_zp},                            // 0xe5
    {inst_inc, mode_zp},                            // 0xe6
    {inst_xxx, mode_xxx},                            // 0xe7
    {inst_inx, mode_imp},                            // 0xe8
    {inst_sbc, mode_imm},                            // 0xe9
    {inst_nop, mode_acc},                            // 0xea
    {inst_xxx, mode_xxx},                            // 0xeb
    {inst_cpx, mode_abs},                            // 0xec
    {inst_sbc, mode_abs},                            // 0xed
    {inst_inc, mode_abs},                            // 0xee
    {inst_xxx, mode_xxx},                            // 0xef
    
    {inst_beq, mode_rel},                            // 0xf0
    {inst_sbc, mode_indy},                            // 0xf1
    {inst_xxx, mode_xxx},                            // 0xf2
    {inst_xxx, mode_xxx},                            // 0xf3
    {inst_xxx, mode_zpx},                            // 0xf4
    {inst_sbc, mode_zpx},                            // 0xf5
    {inst_inc, mode_zpx},                            // 0xf6
    {inst_xxx, mode_xxx},                            // 0xf7
    {inst_sed, mode_imp},                            // 0xf8
    {inst_sbc, mode_absy},                            // 0xf9
    {inst_xxx, mode_acc},                            // 0xfa
    {inst_xxx, mode_xxx},                            // 0xfb
    {inst_xxx, mode_xxx},                            // 0xfc
    {inst_sbc, mode_absx},                            // 0xfd
    {inst_inc, mode_absx},                            // 0xfe
    {inst_xxx, mode_xxx}
};

class MOS6501{
public:
    uint16_t cycles,wval,pc;
    long int buff,buffold,waitframe,wait,waitframeold,totalinframe;
    uint8_t data_offset;
    uint16_t init_addr,play_addr;
    uint8_t subsongs;
    uint8_t startsong;
    uint8_t speed;
    uint16_t load_addr;
    uint8_t *mem;
    SID6581 * _sid;
    
    
    uint8_t a,x,y,p,s,bval;
    bool frame;
    
    MOS6501(){
        _sid=&sid;
            }
    
    uint8_t getmem(uint16_t addr);
    
    void setmem(uint16_t addr,uint8_t value);
    
    uint16_t pcinc();
    
    void cpuReset();
    void cpuResetTo(uint16_t npc, uint16_t na);
    uint8_t getaddr(mode_enum mode);
    void setaddr(mode_enum mode,uint8_t val);
    void putaddr(mode_enum mode,uint8_t val);
    void setflags(flag_enum flag, int cond);
    void push(uint8_t val);
    uint8_t pop ();
    void branch(bool flag);
    uint16_t cpuParse();
    uint16_t cpuJSR(uint16_t npc, uint8_t na);
    void getNextFrame(uint16_t npc, uint8_t na);
    void play(fs::FS &fs, const char * path);
    static void SIDTUNESSerialPlayerTask(void * parameters);
};

static MOS6501 cpu;
#endif /* mos6501b_hpp */
