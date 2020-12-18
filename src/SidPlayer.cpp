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

#include "SidPlayer.h"
#include "SID6581.h"

uint8_t SIDTunesPlayer::getmem(uint16_t addr)
{
  return mem[addr];
}


void SIDTunesPlayer::setSpeed(uint32_t speed)
{
  int_speed = 100-(speed%101);
}


void SIDTunesPlayer::setmem(uint16_t addr,uint8_t value)
{
  if (addr >= 0xd400 and addr <=0xd620) {
    mem[addr] = value;
    int based=(addr>>8)-0xd4;
    long int t = 0;

    if(frame) {
      if(wait == 0)
        t = 20000;
      else
        t = wait;
      t = t-totalinframe;
      if(t<0)
        t = 20000;
      frame = false;
      reset = 0;
      wait = 0;
      totalinframe = 0;
    }

    uint16_t decal = t+buff-buffold;
    addr = (addr&0xFF)+32*based;
    //log_v("%x %x %d\n",addr,value,decal);
    //log_v("%d %d %ld")
    if((addr%32)%24==0 and (addr%32)>0) { //we deal with the sound
      //log_v("sound :%x %x\n",addr,value&0xf);
      //sidReg->save24=*(uint8_t*)(d+1);
      value = (value& 0xf0) + ( ((value& 0x0f)*volume)/15 );

    }
    // if((addr%32)%7==4) {
    //   int d=value&0xf0;
    //   if(d!=128 && d!=64 && d!=32 && d!=16 && d!=0)
    //   log_v("waveform:%d %d\n",value&0xf0,addr);
    // }
    sid.pushRegister(addr/32,addr%32,value);
    decal=(decal*int_speed)/100;
    //log_v("ff %d\n",decal);
    // vTaskDelay only takes milliseconds and the delays are in microseconds but the frame switch can be up to 19ms hence to avoid blocking the cpu
    // we do a delayMicroseconds for the microseconds part and and vTaskDelay for the millisecond part
    // maybe a minus to be added in oder to cope with the time to push and stuff
    delayMicroseconds(decal%1000);
    delay(decal/1000);
    // sid.feedTheDog();
    //mem[addr] = value;
    buffold=buff;
  } else {
    mem[addr] = value;
  }
}


uint16_t SIDTunesPlayer::pcinc()
{
  uint16_t tmp=pc++;
  pc &=0xffff;
  return tmp;
}


void SIDTunesPlayer::cpuReset()
{
  a    = 0;
  x    = 0;
  y    = 0;
  p    = 0;
  s    = 255;
  pc   = getmem(0xfffc);
  pc  |= 256 * getmem(0xfffd);
}


void SIDTunesPlayer::cpuResetTo(uint16_t npc, uint16_t na)
{
  a    = na | 0;
  x    = 0;
  y    = 0;
  p    = 0;
  s    = 255;
  pc   = npc;
}


uint8_t SIDTunesPlayer::getaddr(mode_enum mode)
{
  uint16_t ad,ad2;
  switch(mode) {
    case mode_imp:
      cycles += 2;
      return 0;
    case mode_imm:
      cycles += 2;
      return getmem(pcinc());
    case mode_abs:
      cycles += 4;
      ad = getmem(pcinc());
      ad |= getmem(pcinc()) << 8;
      return getmem(ad);
    case mode_absx:
      cycles += 4;
      ad = getmem(pcinc());
      ad |= 256 * getmem(pcinc());
      ad2 = ad + x;
      ad2 &= 0xffff;
      if ((ad2 & 0xff00) != (ad & 0xff00)) cycles++;
      return getmem(ad2);
    case mode_absy:
      cycles += 4;
      ad = getmem(pcinc());
      ad |= 256 * getmem(pcinc());
      ad2 = ad + y;
      ad2 &= 0xffff;
      if ((ad2 & 0xff00) != (ad & 0xff00)) cycles++;
      return getmem(ad2);
    case mode_zp:
      cycles += 3;
      ad = getmem(pcinc());
      return getmem(ad);
    case mode_zpx:
      cycles += 4;
      ad = getmem(pcinc());
      ad += x;
      return getmem(ad & 0xff);
    case mode_zpy:
      cycles += 4;
      ad = getmem(pcinc());
      ad += y;
      return getmem(ad & 0xff);
    case mode_indx:
      cycles += 6;
      ad = getmem(pcinc());
      ad += x;
      ad2 = getmem(ad & 0xff);
      ad++;
      ad2 |= getmem(ad & 0xff) << 8;
      return getmem(ad2);
    case mode_indy:
      cycles += 5;
      ad = getmem(pcinc());
      ad2 = getmem(ad);
      ad2 |= getmem((ad + 1) & 0xff) << 8;
      ad = ad2 + y;
      ad &= 0xffff;
      if ((ad2 & 0xff00) != (ad & 0xff00)) cycles++;
      return getmem(ad);
    case mode_acc:
      cycles += 2;
      return a;
    default:
    return 0;

  }
  return 0;
}


void SIDTunesPlayer::SetMaxVolume( uint8_t vol)
{
  volume=vol;
}


void SIDTunesPlayer::setaddr(mode_enum mode,uint8_t val)
{
  uint16_t ad,ad2;
  // FIXME: not checking pc addresses as all should be relative to a valid instruction
  switch(mode) {
    case mode_abs:
      cycles += 2;
      ad = getmem(pc - 2);
      ad |= 256 * getmem(pc - 1);
      setmem(ad, val);
      return;
    case mode_absx:
      cycles += 3;
      ad = getmem(pc - 2);
      ad |= 256 * getmem(pc - 1);
      ad2 = ad + x;
      ad2 &= 0xffff;
      if ((ad2 & 0xff00) != (ad & 0xff00)) cycles--;
      setmem(ad2, val);
      return;
    case mode_zp:
      cycles += 2;
      ad = getmem(pc - 1);
      setmem(ad, val);
      return;
    case mode_zpx:
      cycles += 2;
      ad = getmem(pc - 1);
      ad += x;
      setmem(ad & 0xff, val);
      return;
    case mode_acc:
      a = val;
      return;

    case mode_imm:
    case mode_absy:
    case mode_zpy:
    case mode_ind:
    case mode_indx:
    case mode_indy:
    case mode_rel:
    case mode_xxx:
    default:
    return;
  }
}


void SIDTunesPlayer::putaddr(mode_enum mode,uint8_t val)
{
  uint16_t ad,ad2;
  switch(mode) {
    case mode_abs:
      cycles += 4;
      ad = getmem(pcinc());
      ad |= getmem(pcinc()) << 8;
      setmem(ad, val);
      return;
    case mode_absx:
      cycles += 4;
      ad = getmem(pcinc());
      ad |= getmem(pcinc()) << 8;
      ad2 = ad + x;
      ad2 &= 0xffff;
      setmem(ad2, val);
      return;
    case mode_absy:
      cycles += 4;
      ad = getmem(pcinc());
      ad |= getmem(pcinc()) << 8;
      ad2 = ad + y;
      ad2 &= 0xffff;
      if ((ad2 & 0xff00) != (ad & 0xff00)) cycles++;
      setmem(ad2, val);
      return;
    case mode_zp:
      cycles += 3;
      ad = getmem(pcinc());
      setmem(ad, val);
      return;
    case mode_zpx:
      cycles += 4;
      ad = getmem(pcinc());
      ad += x;
      setmem(ad & 0xff, val);
      return;
    case mode_zpy:
      cycles += 4;
      ad = getmem(pcinc());
      ad += y;
      setmem(ad & 0xff,val);
      return;
    case mode_indx:
      cycles += 6;
      ad = getmem(pcinc());
      ad += x;
      ad2 = getmem(ad & 0xff);
      ad++;
      ad2 |= getmem(ad & 0xff) << 8;
      setmem(ad2, val);
      return;
    case mode_indy:
      cycles += 5;
      ad = getmem(pcinc());
      ad2 = getmem(ad);
      ad2 |= getmem((ad + 1) & 0xff) << 8;
      ad = ad2 + y;
      ad &= 0xffff;
      setmem(ad, val);
      return;
    case mode_acc:
      cycles += 2;
      a = val;
      return;
    case mode_imm:
    case mode_ind:
    case mode_rel:
    case mode_xxx:
    default:
    return;
  }
}


void SIDTunesPlayer::setflags(flag_enum flag, int cond)
{
  if (cond!=0) {
    p |= flag;
  } else {
    p &= ~flag & 0xff;
  }
}


void SIDTunesPlayer::push(uint8_t val)
{
  setmem(0x100 + s, val);
  if (s) s--;
}


uint8_t SIDTunesPlayer::pop ()
{
  if (s < 0xff) s++;
  return getmem(0x100 + s);
}


void SIDTunesPlayer::branch(bool flag)
{
  uint16_t dist = getaddr(mode_imm);
  // FIXME: while this was checked out, it still seems too complicated
  // make signed
  //log_v("dist:%d\n",dist);
  if (dist & 0x80) {
    dist = 0 - ((~dist & 0xff) + 1);
  }
  //dist=dist&0xff;
  // this here needs to be extracted for general 16-bit rounding needs
  wval= pc + dist;
  // FIXME: added boundary checks to wrap around. Not sure this is whats needed
  //if (wval < 0) wval += 65536;
  wval &= 0xffff;
  if (flag) {
    cycles += ((pc & 0x100) != (wval & 0x100)) ? 2 : 1;
    pc = wval;
  }
}


uint16_t SIDTunesPlayer::cpuParse()
{
  uint16_t c;
  cycles = 0;
  //log_v("core:%d\n",xPortGetCoreID());
  uint8_t opc = getmem(pcinc());
  //log_v("ops:%d %d\n",opc,opcodes[opc].inst);
  inst_enum cmd = opcodes[opc].inst;
  mode_enum addr = opcodes[opc].mode;
  //console.log(opc, cmd, addr);
  switch (cmd) {
    case inst_adc:
      wval = a + getaddr(addr) + ((p & flag_C) ? 1 : 0);
      setflags(flag_C, wval & 0x100);
      a = wval & 0xff;
      setflags(flag_Z, !a);
      setflags(flag_N, a & 0x80);
      setflags(flag_V, ((p & flag_C) ? 1 : 0) ^ ((p & flag_N) ? 1 : 0));
      break;
    case inst_and:
      bval = getaddr(addr);
      a &= bval;
      setflags(flag_Z, !a);
      setflags(flag_N, a & 0x80);
      break;
    case inst_asl:
      wval = getaddr(addr);
      wval <<= 1;
      setaddr(addr, wval & 0xff);
      setflags(flag_Z, !wval);
      setflags(flag_N, wval & 0x80);
      setflags(flag_C, wval & 0x100);
      break;
    case inst_bcc:
      branch(!(p & flag_C));
      break;
    case inst_bcs:
      branch(p & flag_C);
      break;
    case inst_bne:
      branch(!(p & flag_Z));
      break;
    case inst_beq:
      branch(p & flag_Z);
      break;
    case inst_bpl:
      branch(!(p & flag_N));
      break;
    case inst_bmi:
      branch(p & flag_N);
      break;
    case inst_bvc:
      branch(!(p & flag_V));
      break;
    case inst_bvs:
      branch(p & flag_V);
      break;
    case inst_bit:
      bval = getaddr(addr);
      setflags(flag_Z, !(a & bval));
      setflags(flag_N, bval & 0x80);
      setflags(flag_V, bval & 0x40);
      break;
    case inst_brk:
      pc=0;    // just quit per rockbox
      //push(pc & 0xff);
      //push(pc >> 8);
      //push(p);
      //setflags(flag_B, 1);
      // FIXME: should Z be set as well?
      //pc = getmem(0xfffe);
      //cycles += 7;
      break;
    case inst_clc:
      cycles += 2;
      setflags(flag_C, 0);
      break;
    case inst_cld:
      cycles += 2;
      setflags(flag_D, 0);
      break;
    case inst_cli:
      cycles += 2;
      setflags(flag_I, 0);
      break;
    case inst_clv:
      cycles += 2;
      setflags(flag_V, 0);
      break;
    case inst_cmp:
      bval = getaddr(addr);
      wval = a - bval;
      // FIXME: may not actually be needed (yay 2's complement)
      if(a < bval) wval += 256;
      //wval &=0xff;
      setflags(flag_Z, !wval);
      setflags(flag_N, wval & 0x80);
      setflags(flag_C, a >= bval);
      break;
    case inst_cpx:
      bval = getaddr(addr);
      wval = x - bval;
      // FIXME: may not actually be needed (yay 2's complement)
      if(x < bval) wval += 256;
      // wval &=0xff;
      setflags(flag_Z, !wval);
      setflags(flag_N, wval & 0x80);
      setflags(flag_C, x >= bval);
      break;
    case inst_cpy:
      bval = getaddr(addr);
      wval = y - bval;
      // FIXME: may not actually be needed (yay 2's complement)
      if(y < bval) wval += 256;
      //wval &=0xff;
      setflags(flag_Z, !wval);
      setflags(flag_N, wval & 0x80);
      setflags(flag_C, y >= bval);
      break;
    case inst_dec:
      bval = getaddr(addr);
      bval--;
      // FIXME: may be able to just mask this (yay 2's complement)
      //if(bval < 0) bval += 256;
      wval &=0xff;
      setaddr(addr, bval);
      setflags(flag_Z, !bval);
      setflags(flag_N, bval & 0x80);
      break;
    case inst_dex:
      cycles += 2;
      x--;
      // FIXME: may be able to just mask this (yay 2's complement)
      //if(x < 0) x += 256;
      setflags(flag_Z, !x);
      setflags(flag_N, x & 0x80);
      break;
    case inst_dey:
      cycles += 2;
      y--;
      // FIXME: may be able to just mask this (yay 2's complement)
      //if(y < 0) y += 256;
      setflags(flag_Z, !y);
      setflags(flag_N, y & 0x80);
      break;
    case inst_eor:
      bval = getaddr(addr);
      a ^= bval;
      setflags(flag_Z, !a);
      setflags(flag_N, a & 0x80);
      break;
    case inst_inc:
      bval = getaddr(addr);
      bval++;
      bval &= 0xff;
      setaddr(addr, bval);
      setflags(flag_Z, !bval);
      setflags(flag_N, bval & 0x80);
      break;
    case inst_inx:
      cycles += 2;
      x++;
      x &= 0xff;
      setflags(flag_Z, !x);
      setflags(flag_N, x & 0x80);
      break;
    case inst_iny:
      cycles += 2;
      y++;
      y &= 0xff;
      setflags(flag_Z, !y);
      setflags(flag_N, y & 0x80);
      break;
    case inst_jmp:
      cycles += 3;
      wval = getmem(pcinc());
      wval |= 256 * getmem(pcinc());
      switch (addr) {
        case mode_abs:
          pc = wval;
        break;
        case mode_ind:
          pc = getmem(wval);
          pc |= 256 * getmem((wval + 1) & 0xffff);
          cycles += 2;
        break;
        default:
        break; // wat ?
      }
      break;
    case inst_jsr:
      cycles += 6;
      push(((pc + 1) & 0xffff) >> 8);
      push((pc + 1) & 0xff);
      wval = getmem(pcinc());
      wval |= 256 * getmem(pcinc());
      pc = wval;
      break;
    case inst_lda:
      a = getaddr(addr);
      setflags(flag_Z, !a);
      setflags(flag_N, a & 0x80);
      break;
    case inst_ldx:
      x = getaddr(addr);
      setflags(flag_Z, !x);
      setflags(flag_N, x & 0x80);
      break;
    case inst_ldy:
      y = getaddr(addr);
      setflags(flag_Z, !y);
      setflags(flag_N, y & 0x80);
      break;
    case inst_lsr:
      bval = getaddr(addr);
      wval = bval;
      wval >>= 1;
      setaddr(addr, wval & 0xff);
      setflags(flag_Z, !wval);
      setflags(flag_N, wval & 0x80);
      setflags(flag_C, bval & 1);
      break;
    case inst_nop:
      cycles += 2;
      break;
    case inst_ora:
      bval = getaddr(addr);
      a |= bval;
      setflags(flag_Z, !a);
      setflags(flag_N, a & 0x80);
      break;
    case inst_pha:
      push(a);
      cycles += 3;
      break;
    case inst_php:
      push(p);
      cycles += 3;
      break;
    case inst_pla:
      a = pop();
      setflags(flag_Z, !a);
      setflags(flag_N, a & 0x80);
      cycles += 4;
      break;
    case inst_plp:
      p = pop();
      cycles += 4;
      break;
    case inst_rol:
      bval = getaddr(addr);
      c = (p & flag_C) ? 1 : 0;
      setflags(flag_C, bval & 0x80);
      bval <<= 1;
      bval |= c;
      bval &= 0xff;
      setaddr(addr, bval);
      setflags(flag_N, bval & 0x80);
      setflags(flag_Z, !bval);
      break;
    case inst_ror:
      bval = getaddr(addr);
      c = (p & flag_C) ? 128 : 0;
      setflags(flag_C, bval & 1);
      bval >>= 1;
      bval |= c;
      setaddr(addr, bval);
      setflags(flag_N, bval & 0x80);
      setflags(flag_Z, !bval);
      break;
    case inst_rti:
      // treat like RTS
    case inst_rts:
      wval = pop();
      wval |= 256 * pop();
      pc = wval + 1;
      cycles += 6;
      break;
    case inst_sbc:
      bval = getaddr(addr) ^ 0xff;
      wval = a + bval + (( p & flag_C) ? 1 : 0);
      setflags(flag_C, wval & 0x100);
      a = wval & 0xff;
      setflags(flag_Z, !a);
      setflags(flag_N, a > 127);
      setflags(flag_V, ((p & flag_C) ? 1 : 0) ^ ((p & flag_N) ? 1 : 0));
      break;
    case inst_sec:
      cycles += 2;
      setflags(flag_C, 1);
      break;
    case inst_sed:
      cycles += 2;
      setflags(flag_D, 1);
      break;
    case inst_sei:
      cycles += 2;
      setflags(flag_I, 1);
      break;
    case inst_sta:
      putaddr(addr, a);
      break;
    case inst_stx:
      putaddr(addr, x);
      break;
    case inst_sty:
      putaddr(addr, y);
      break;
    case inst_tax:
      cycles += 2;
      x = a;
      setflags(flag_Z, !x);
      setflags(flag_N, x & 0x80);
      break;
    case inst_tay:
      cycles += 2;
      y = a;
      setflags(flag_Z, !y);
      setflags(flag_N, y & 0x80);
      break;
    case inst_tsx:
      cycles += 2;
      x = s;
      setflags(flag_Z, !x);
      setflags(flag_N, x & 0x80);
      break;
    case inst_txa:
      cycles += 2;
      a = x;
      setflags(flag_Z, !a);
      setflags(flag_N, a & 0x80);
      break;
    case inst_txs:
      cycles += 2;
      s = x;
      break;
    case inst_tya:
      cycles += 2;
      a = y;
      setflags(flag_Z, !a);
      setflags(flag_N, a & 0x80);
      break;
    default:
      break;
  }
  return cycles;
}


uint16_t SIDTunesPlayer::cpuJSR(uint16_t npc, uint8_t na)
{
  uint16_t ccl = 0;
  a = na;
  x = 0;
  y = 0;
  p = 0;
  s = 255;
  pc = npc;
  push(0);
  push(0);
  int g=100;
  int timeout=0;
  while (pc > 1 && timeout<100000 && g>0 ) {
    timeout++;
    g=cpuParse();
    buff+=g;
    ccl +=g;
    //sid.feedTheDog ();
    //printf("cycles %d\n",buff);
    //if(pc>64000)
    //if(plmo>=13595)
    //log_v("pc: %d net instr:%d\n",pc,mem[pc+1]);
  }
  //plmo++;
  sid.feedTheDog();
  //log_v("time:%d %d %d\n",timeout,g,pc);
  //log_v("after parse:%d %d %x %x %x %ld\n",pc,wval ,mem[pc],mem[pc+1],mem[pc+2],plmo);
  return ccl;
}


void SIDTunesPlayer::getNextFrame(uint16_t npc, uint8_t na)
{
  for(int i=0;i<15000;i++) {
    totalinframe=cpuJSR(npc,na);
    wait=waitframe;
    frame=true;
    //waitframe=0;
    int nRefreshCIA = (int)( ((19954 * (getmem(0xdc04) | (getmem(0xdc05) << 8)) / 0x4c00) + (getmem(0xdc04) | (getmem(0xdc05) << 8))  )  /2 )    ;
    if ((nRefreshCIA == 0) or speed==0) nRefreshCIA = 48000;
    // printf("tota l:%d\n",nRefreshCIA);
    waitframe=nRefreshCIA;
  }
}


bool SIDTunesPlayer::isSIDPlayable(fs::File &file )
{
  // grabbed and converted from:
  // https://github.com/TheCodeTherapy/sid-player/blob/master/src/components/player/SIDPlayer.js

  uint8_t  i;
  uint8_t  offs;

  uint16_t timerMode[32];
  uint16_t preferred_SID_model[3];
  uint16_t SID_address[2];
  __attribute__((unused)) uint16_t loadAddress;
  __attribute__((unused)) uint16_t initAddress;
  __attribute__((unused)) uint16_t playAddress;
  uint8_t  subTune_amount;
  uint16_t SIDAmount;
  songstruct *p1;

  size_t len = file.size();
  uint8_t* fileData = (uint8_t*)malloc(len+1);

  if( fileData == NULL ) {
    Serial.printf("PrintSidInfo could not malloc %d bytes (%d free)\n", len+1+sizeof(songstruct), ESP.getFreeHeap() );
    return false;
  }

  file.seek(0);
  file.read( fileData, len );

  offs = fileData[7];

  loadAddress = ( fileData[8] + fileData[9] )
    ? ( fileData[8] * 256 ) + fileData[9]
    : ( fileData[offs + 1] * 256 ) + fileData[offs];

  for ( i = 0; i < 32; i++ ) {
    timerMode[31 - i] = fileData[0x12 + (i >> 3)] & (uint8_t)pow( 2, 7 - ( i % 8 ) );
  }

  /*
  char     strEnd;
  strEnd = 1;
  for ( i = 0; i < 32; i++ ) {
    if ( strEnd != 0 ) {
      strEnd = p1->name[i] = fileData[0x16 + i];
    } else {
      strEnd = p1->name[i] = 0;
    }
  }
  strEnd = 1;
  for ( i = 0; i < 32; i++ ) {
    if ( strEnd != 0 ) {
      strEnd = p1->author[i] = fileData[0x36 + i];
    } else {
      strEnd = p1->author[i] = 0;
    }
  }
  strEnd = 1;
  for ( i = 0; i < 32; i++ ) {
    if ( strEnd != 0 ) {
      strEnd = p1->published[i] = fileData[0x56 + i];
    } else {
      strEnd = p1->published[i] = 0;
    }
  }
  //free( p1 );
  */

  initAddress = ( fileData[0xA] + fileData[0xB] )
    ? ( fileData[0xA] * 256 ) + fileData[0xB]
    : loadAddress;

  playAddress = ( fileData[0xC] * 256 ) + fileData[0xD];
  subTune_amount = fileData[ 0xF ];

  preferred_SID_model[0] = ( fileData[0x77] & 0x30 ) >= 0x20 ? 8580 : 6581;
  preferred_SID_model[1] = ( fileData[0x77] & 0xC0 ) >= 0x80 ? 8580 : 6581;
  preferred_SID_model[2] = ( fileData[0x76] & 3 ) >= 3 ? 8580 : 6581;

  SID_address[1] = fileData[0x7A] >= 0x42 && ( fileData[0x7A] < 0x80 || fileData[0x7A] >= 0xE0 )
    ? 0xD000 + ( fileData[ 0x7A ] * 16 )
    : 0;

  SID_address[2] = fileData[0x7B] >= 0x42 && ( fileData[0x7B] < 0x80 || fileData[0x7B] >= 0xE0 )
    ? 0xD000 + ( fileData[0x7B] * 16 )
    : 0;

  SIDAmount = 1 + ( SID_address[1] > 0 ) + ( SID_address[ 2 ] > 0 );

  free(fileData);

  bool debugTimerMode = false;

  if( debugTimerMode ) {
    if ( timerMode[0] > 1 ) {
      Serial.println("[timerMode]:");
      for( i=0;i<4;i++ ) {
        Serial.print(" | " );
        for( int j=0;j<8;j++ ) {
          Serial.printf("0x%02X ", timerMode[i*8+j] );
        }
        Serial.println();
      }
    }
  }

  if( preferred_SID_model[0]!= 6581 || preferred_SID_model[1]!= 6581 || preferred_SID_model[2]!= 6581 ) {
    log_e("Unsupported SID Model:  0: %d  1: %d  2: %d",
      preferred_SID_model[0],
      preferred_SID_model[1],
      preferred_SID_model[2]
    );
    return false;
  }

  log_v("SID Addresses: Load: %X Init: %X Play: %X",
    loadAddress,
    initAddress,
    playAddress
  );

  int max_subTune_amount = 1;

  if( subTune_amount > max_subTune_amount ) {
    log_e("Too many (more than %d) subtunes: %d", max_subTune_amount, subTune_amount );
    return false;
  }

  int max_sid_amount = 1;

  if( SIDAmount > 1 ) {
    log_e("Too many (more than %d) Sid songs: %d", max_sid_amount, SIDAmount );
    return false;
  }
  return true;

}


bool SIDTunesPlayer::getInfoFromFile(fs::FS &fs, const char * path, songstruct * songinfo)
{
  uint8_t stype[5];
  if( &fs==NULL || path==NULL) {
    log_e("Invalid parameter");
    getcurrentfile = currentfile;
    return false;
  }
  if(!fs.exists(path)) {
    log_e("file %s unknown\n",path);
    getcurrentfile = currentfile;
    return false;
  }
  File file=fs.open(path);
  if(!file) {
    log_e("Unable to open %s\n",path);
    getcurrentfile = currentfile;
    return false;
  }

  memset(stype,0,5);
  file.read(stype,4);
  if(stype[0]!=0x50 || stype[1]!=0x53 || stype[2]!=0x49 || stype[3]!=0x44) {
    log_d("File type:%s not handled yet", stype);
    //getcurrentfile=currentfile;
    file.close();
    return false;
  }
  file.seek( 15);
  songinfo->subsongs = file.read();
  file.seek(17);
  songinfo->startsong = file.read();
  file.seek(22);
  file.read(songinfo->name,32);
  file.seek(0x36);
  file.read(songinfo->author,32);
  file.seek(0x56);
  file.read(songinfo->published,32);

  uint8_t val;
  uint16_t preferred_SID_model[3];
  file.seek(0x77);
  val = file.read();
  preferred_SID_model[0] = ( val & 0x30 ) >= 0x20 ? 8580 : 6581;
  preferred_SID_model[1] = ( val & 0xC0 ) >= 0x80 ? 8580 : 6581;
  file.seek(0x76);
  val = file.read();
  preferred_SID_model[2] = ( val & 3 ) >= 3 ? 8580 : 6581;

  bool playable = true; // isSIDPlayable( file );

  sprintf( songinfo->md5,"%s", md5.calcMd5( file ) );

  int max_subTune_amount = 1;
  if( songinfo->subsongs > max_subTune_amount ) {
    playable = false;
  }
  if( preferred_SID_model[0]!= 6581 || preferred_SID_model[1]!= 6581 || preferred_SID_model[2]!= 6581 ) {
    log_e("Unsupported SID Model:  0: %d  1: %d  2: %d",
      preferred_SID_model[0],
      preferred_SID_model[1],
      preferred_SID_model[2]
    );
    playable = false;
  }

  file.close();

  if( MD5Parser != NULL ) {

    if( songinfo->durations != nullptr ) free( songinfo->durations );
    songinfo->durations = (uint32_t*)sid_calloc( songinfo->subsongs, sizeof(uint32_t) );

    MD5Parser->getDurationsFromSIDPath( songinfo );
  }

  return playable;
}


void SIDTunesPlayer::getSongslengthfromMd5(fs::FS &fs, const char * path)
{
  char lom[320+35];
  char bu[320];
  char parsestr[32*10+35];
  int f,jd,k;
  log_i("Opening songslength file :%s ...\n",path);
  File md5File = fs.open(path);
  if(!md5File) {
      log_e("File not found");
      return;
  }
  size_t md5FileSize = md5File.size();
  size_t totalRead   = 0; // bytes read
  size_t totalProcessed = 0; // song length found
  uint8_t progress = 0, lastprogress = 0;
  if( md5FileSize == 0 ) {
    log_e("Invalid md5 file, will be deleted");
    md5File.close();
    fs.remove( path );
    return;
  }
  log_i("File open. Matching MD5 ...");
  while ( md5File.available() ) {
    String list = md5File.readStringUntil('\n');
    totalRead += list.length() + 1;

    progress = (100*totalRead)/md5FileSize;
    if( progress != lastprogress ) {
      lastprogress = progress;
      Serial.printf("MD5 Database Parsing Progress: %d%s\n", progress, "%" );
    }
    if( totalProcessed >= numberOfSongs ) {
      Serial.println("MD5 Database Parsing finished!");
      break; // all songs have their length, no need to finish
    }

    //log_w( "MD5 Entry length:%d", list.length() );
    String stringTwo;
    if( list[0]!=';' && list[0]!='[' ) { // probably a md5 ?
      for(int s=0;s<numberOfSongs;s++) {
        //log_v("dd %s %s\n",listsongs[s].filename,listsongs[s].md5);
        if( listsongs[s]->durations[0] != 0 ) continue; // already found
        stringTwo = String( listsongs[s]->md5 );

        if( list.startsWith( stringTwo ) ) {
          log_i("md5 info found for file %s\n",listsongs[s]->filename);
          memset( lom, 0, 320+35 );
          list.toCharArray( lom, list.length()+1 );
          //log_v("%s", lom);
          //log_v("%s", list);
          memset( parsestr, 0, 32*10+35 );
          sprintf( parsestr, "%s=%%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s", listsongs[s]->md5 );
          //log_v("parse :%s\n",parsestr);
          memset( bu, 0, 320 );
          sscanf( lom, parsestr,
            &bu[  0], &bu[ 10], &bu[ 20], &bu[ 30], &bu[ 40], &bu[ 50], &bu[ 60], &bu[ 70], &bu[ 80], &bu[ 90],
            &bu[100], &bu[110], &bu[120], &bu[130], &bu[140], &bu[150], &bu[160], &bu[170], &bu[180], &bu[190],
            &bu[200], &bu[210], &bu[220], &bu[230], &bu[240], &bu[250], &bu[260], &bu[270], &bu[280], &bu[290],
            &bu[300], &bu[310]
          );
          for( int m=0; m<32; m++ ) {
            if( bu[m*10] != 0 ) {
              f = jd = k = 0;
              sscanf( &bu[m*10], "%d:%d.%d", &f, &jd, &k );
              listsongs[s]->durations[m] = f*60*1000+jd*1000+k;
              Serial.printf("[Track #%d:%d] (%16s) %s / %d ms\n", s, m, (const char*)listsongs[s]->name, &bu[10*m], listsongs[s]->durations[m] );
              if( m == 0 ) totalProcessed++;
            }
          }
        }
      }
    }
  }
  md5File.close();
  log_i("Matching done.");
}

//The following is a function from tobozo
//thank to him

int SIDTunesPlayer::addSongsFromFolder( fs::FS &fs, const char* foldername, const char* filetype, bool recursive )
{

  File root = fs.open( foldername );
  if(!root){
    log_e("Failed to open %s directory\n", foldername);
    return 0;
  }
  if(!root.isDirectory()){
    log_e("%s is not a directory\b", foldername);
    return 0;
  }
  File file = root.openNextFile();
  while(file){
    // handle both uppercase and lowercase of the extension
    String fName = String( file.name() );
    fName.toLowerCase();

    if( fName.endsWith( filetype ) && !file.isDirectory() ) {
      if( addSong(fs, file.name() ) == -1 ) return -1;
      // log_v("[INFO] Added [%s] ( %d bytes )\n", file.name(), file.size() );
    } else if( file.isDirectory()  ) {
      if( recursive ) {
          int ret = addSongsFromFolder( fs, file.name(), filetype, true );
          if( ret == -1 ) return -1;
      } else {
          log_i("[INFO] Ignored [%s]\n", file.name() );
      }
    } else {
      log_i("[INFO] Ignored [%s]\n", file.name() );

    }
    file = root.openNextFile();
  }
  return 1;
}


songstruct SIDTunesPlayer::getSidFileInfo(int songnumber)
{
  if(songnumber<numberOfSongs && songnumber>=0) {
      return *listsongs[songnumber];
  }
  return *listsongs[0];
}


bool SIDTunesPlayer::playSidFile( songstruct *song )
{
  log_i("playing file:%s\n", song->filename );
  if(mem == NULL) {
    log_i("we create the memory buffer, available mem:%d %d\n",ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
    mem = (uint8_t*)calloc( 0x10000, 1 );
    if(mem == NULL) {
      log_e("not enough memory\n");
      is_error = true;
      error_type = NOT_ENOUGH_MEMORY;
      return false;
    }
  } else {
    memset( mem, 0x0, 0x10000 );
  }

  memset( name, 0, 32 );
  memset( author, 0, 32 );
  memset( published, 0, 32 );
  memset( currentfilename, 0, sizeof(currentfilename) );
  memset( sidtype, 0, 5 );

  if(!song->fs->exists( song->filename )) {
    log_e("File %s does not exist", song->filename );
    getcurrentfile = currentfile;
    return false;
  }
  File file=song->fs->open( song->filename );
  if(!file) {
    log_e("Unable to open file %s", song->filename );
    getcurrentfile = currentfile;
    return false;
  }

  file.read( sidtype, 4 );

  if(sidtype[0]!=0x50 || sidtype[1]!=0x53 || sidtype[2]!=0x49 || sidtype[3]!=0x44) {
    log_e("File type:%s not supported yet", sidtype );
    getcurrentfile = currentfile;
    file.close();
    return false;
  }

  file.seek(7);
  data_offset = file.read();

  uint8_t not_load_addr[2];
  file.read(not_load_addr,2);

  init_addr  = file.read()*256;
  init_addr += file.read();

  play_addr  = file.read()*256;
  play_addr += file.read();

  file.seek( 15);
  subsongs = file.read();

  file.seek(17);
  startsong = file.read();

  file.seek(21);
  speed = file.read();

  file.seek(22);
  file.read(name,32);

  file.seek(0x36);
  file.read(author,32);

  file.seek(0x56);
  file.read(published,32);

  if( speed == 0 )
    nRefreshCIAbase = 38000;
  else
    nRefreshCIAbase = 19950;

  file.seek(data_offset);
  load_addr = file.read()+(file.read()<<8);

  // CODE SMELL: should read file.size() - current offset
  size_t g = file.read(&mem[load_addr], file.size());
  //memset(&mem[load_addr+g],0xea,0x10000-load_addr+g);
  for(int i=0;i<32;i++) {
    uint8_t fm;
    file.seek( 0x12+(i>>3) );
    fm = file.read();
    speedsong[31-i]= (fm & (byte)pow(2,7-i%8))?1:0;
  }
  sprintf(currentfilename,"%s", song->filename );
  getcurrentfile = currentfile;
  //executeEventCallback(SID_START_PLAY);
  currentsong=startsong -1;
  file.close();

  if( MD5Parser != NULL ) {
    if( song->durations != nullptr ) free( song->durations );
    song->durations = (uint32_t*)sid_calloc( song->subsongs, sizeof(uint32_t) );
    MD5Parser->getDurationsFromSIDPath( song );
  }

  executeEventCallback(SID_NEW_FILE);
  playSongNumber( currentsong );
  //xTaskNotifyGive(SIDTUNESSerialSongPlayerTaskLock);
  //while(1){};
  return true;
}


void SIDTunesPlayer::playNextSongInSid()
{
  //log_v("in net:%d\n",subsongs);
  stop();
  currentsong=(currentsong+1)%subsongs;
  playSongNumber(currentsong);
}


void SIDTunesPlayer::executeEventCallback(sidEvent event)
{
  if (eventCallback) (*eventCallback)(event);
}


void SIDTunesPlayer::playPrevSongInSid()
{
  stop();
  if(currentsong==0)
      currentsong=(subsongs-1);
  else
      currentsong--;
  playSongNumber(currentsong);
}


uint32_t SIDTunesPlayer::getCurrentTrackDuration()
{
  return song_duration;
}


void SIDTunesPlayer::stop()
{
  //log_v("playing stop n:%d/%d\n",1,subsongs);
  sid.soundOff();
  if(SIDTUNESSerialPlayerTaskHandle!=NULL) {
    vTaskDelete(SIDTUNESSerialPlayerTaskHandle);
    SIDTUNESSerialPlayerTaskHandle=NULL;
  }
  executeEventCallback(SID_STOP_TRACK);
  //log_v("playing sop n:%d/%d\n",1,subsongs);
}

void SIDTunesPlayer::stopPlayer()
{
  //log_v("playing stop n:%d/%d\n",1,subsongs);
  sid.soundOff();

  if(SIDTUNESSerialPlayerTaskHandle!=NULL) {
    vTaskDelete(SIDTUNESSerialPlayerTaskHandle);
    SIDTUNESSerialPlayerTaskHandle=NULL;
  }
  if(SIDTUNESSerialLoopPlayerTask!=NULL) {
    vTaskDelete(SIDTUNESSerialLoopPlayerTask);
    SIDTUNESSerialLoopPlayerTask=NULL;
  }
  playerrunning=false;
  executeEventCallback(SID_END_PLAY);
  //log_v("playing sop n:%d/%d\n",1,subsongs);
}

void SIDTunesPlayer::pausePlay()
{
  if(SIDTUNESSerialPlayerTaskHandle!=NULL) {
    if(!paused) {
      paused=true;
      executeEventCallback(SID_PAUSE_PLAY);
    } else {
      sid.soundOn();
      paused=false;
      executeEventCallback(SID_RESUME_PLAY);
      if(PausePlayTaskLocker!=NULL)
        xTaskNotifyGive(PausePlayTaskLocker);

    }
  }
}

bool SIDTunesPlayer::begin(int clock_pin,int data_pin, int latch,int sid_clock_pin)
{
  volume=15;
  numberOfSongs=0;
  currentfile=0;
  return sid.begin(clock_pin,data_pin,latch,sid_clock_pin);
}

bool SIDTunesPlayer::begin(int clock_pin,int data_pin, int latch )
{
  volume=15;
  numberOfSongs=0;
  currentfile=0;
  return sid.begin(clock_pin,data_pin,latch);;
}

bool SIDTunesPlayer::getPlayerStatus()
{
  return playerrunning;
}

void SIDTunesPlayer::setLoopMode(loopmode mode)
{
  currentloopmode=mode;
}

loopmode SIDTunesPlayer::getLoopMode()
{
  return currentloopmode;
}

void SIDTunesPlayer::setDefaultDuration(uint32_t duration)
{
  default_song_duration=duration;
}

uint32_t SIDTunesPlayer::getDefaultDuration()
{
  return default_song_duration;
}

uint32_t SIDTunesPlayer::getElapseTime()
{
  return delta_song_duration;
}

void SIDTunesPlayer::playSongNumber(int songnumber)
{
  //sid.soundOff();
  sid.resetsid();
  buff=0;
  buffold=0;
  currentsong=songnumber;
  if(SIDTUNESSerialPlayerTaskHandle!=NULL) {
    vTaskDelete(SIDTUNESSerialPlayerTaskHandle);
    SIDTUNESSerialPlayerTaskHandle=NULL;
  }
  if(songnumber<0 || songnumber>=subsongs) {
    log_e("Wrong song number");
    return;
  }
  log_i("playing song n:%d/%d\n",(songnumber+1),subsongs);

  cpuReset();
  //log_v("playing song n:%d/%d\n",(songnumber+1),subsongs);
  //_sidtunes_voicesQueues= xQueueCreate( 26000, sizeof( serial_command ) );

  if(play_addr==0) {
    // log_v("init %x\n",init_addr);
    // for(int g=0;g<10;g++)
    // {
    //     log_v("d %x\n",mem[init_addr+g]);
    // }
    cpuJSR(init_addr, 0); //0
    play_addr = (mem[0x0315] << 8) + mem[0x0314];
    cpuJSR(init_addr, songnumber);
    if(play_addr==0) {
        play_addr=(mem[0xffff] << 8) + mem[0xfffe];
        mem[1]=0x37;
    }
    // log_v("new play address %x %x %x\n",play_addr,(mem[0xffff] << 8) + mem[0xfffe],(mem[0x0315] << 8) + mem[0x0314]);
  } else {
    // log_v("playing song n:%d/%d\n",(songnumber+1),subsongs);
    cpuJSR(init_addr, songnumber);
  }

  //sid.soundOn();
  xTaskCreatePinnedToCore( SIDTunesPlayer::SIDTUNESSerialPlayerTask, "SIDTUNESSerialPlayerTask",  4096,  this, SID_CPU_TASK_PRIORITY, &SIDTUNESSerialPlayerTaskHandle, SID_CPU_CORE);
  //log_v("tas %d core %d\n",SID_TASK_PRIORITY,SID_CPU_CORE);
  delay(200);

  if(listsongs[getcurrentfile]->durations[currentsong]==0) {
    song_duration = default_song_duration;
    log_w("Playing with default song duration %d ms\n",song_duration);
  } else {
    song_duration = listsongs[getcurrentfile]->durations[currentsong];
    log_d("Playing track %d with md5 database song duration %d ms", currentsong, song_duration);
  }
  delta_song_duration=0;

  executeEventCallback(SID_NEW_TRACK);
  if(SIDTUNESSerialPlayerTaskLock!=NULL)
    xTaskNotifyGive(SIDTUNESSerialPlayerTaskLock);
}


void  SIDTunesPlayer::SIDTUNESSerialPlayerTask(void * parameters)
{
  for(;;) {
    //serial_command element;
    SIDTUNESSerialPlayerTaskLock  = xTaskGetCurrentTaskHandle();
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    SIDTunesPlayer * cpu= (SIDTunesPlayer *)parameters;
    cpu->delta_song_duration=0;

    //cpu->delta_song_duration=0;
    cpu->stop_song=false;
    uint32_t start_time=millis();
    uint32_t now_time=0;
    uint32_t pause_time=0;
    while(1 && !cpu->stop_song) {
      cpu->totalinframe+=cpu->cpuJSR(cpu->play_addr,0);
      cpu->wait+=cpu->waitframe;
      cpu->frame=true;
      int nRefreshCIA = (int)( ((19650 * (cpu->getmem(0xdc04) | (cpu->getmem(0xdc05) << 8)) / 0x4c00) + (cpu->getmem(0xdc04) | (cpu->getmem(0xdc05) << 8))  )  /2 );
      if ((nRefreshCIA == 0) /*|| (cpu->speedsong[cpu->currentsong]==0)*/)
        nRefreshCIA = 19650;
      cpu->waitframe=nRefreshCIA;

      if(cpu->song_duration>0) {
        now_time=millis();
        cpu->delta_song_duration+=now_time-start_time;
        start_time=now_time;
        if(cpu->delta_song_duration>=cpu->song_duration) {
          cpu->stop_song=true;
          break;
        }
      }

      if(cpu->paused) {
        sid.soundOff();
        PausePlayTaskLocker  = xTaskGetCurrentTaskHandle();
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        PausePlayTaskLocker=NULL;
        sid.soundOn();
        start_time=millis();
      }
      vTaskDelay(1);

    }
    sid.soundOff();
    cpu->executeEventCallback(SID_END_TRACK);

    if(SIDTUNESSerialSongPlayerTaskLock!=NULL)
      xTaskNotifyGive(SIDTUNESSerialSongPlayerTaskLock);
    vTaskDelay(1);
  }
}


int SIDTunesPlayer::addSong(fs::FS &fs,  const char * path)
{
  if(numberOfSongs >= maxSongs) {
    log_e("Playlist full (%d heap free)", ESP.getFreeHeap() );
    return -1;
  }
  songstruct * song;
  song = (songstruct*)sid_calloc( 1, sizeof(songstruct)+1 );

  if(song==NULL) {
    log_e("Not enough memory to add %s file", path);
    maxSongs = numberOfSongs;
    return -1;
  }

  song->fs=(fs::FS *)&fs;
  sprintf( song->filename,"%s", path );

  if( getInfoFromFile( fs, path, song ) ) {
    listsongs[numberOfSongs] = song;
    numberOfSongs++;
    Serial.printf("[addSong# %04d] md5( %16s ) = %s, heap free=%6d\n", numberOfSongs, (const char*)song->name, (const char*)song->md5, ESP.getFreeHeap() );
    log_d("Added file %s to playlist", path);
    return 1;
  } else {
    free( song ); // don't waste precious bytes on unplayable song
    ignoredSongs++;
    log_e("Ignoring unhandled file %s", path);
    return 0;
  }
  //log_v("nb song:%d\n",numberOfSongs);
}

void SIDTunesPlayer::loopPlayer(void *param)
{
  SIDTunesPlayer * cpu= (SIDTunesPlayer *)param;

  songstruct p1= *(cpu->listsongs[cpu->currentfile]);
  // log_v("currentfile %d %s\n",currentfile,p1.filename);
  if(!cpu->playSidFile( &p1 )) {
    if(cpu->is_error) {
      cpu->is_error=false;
      return;
    }
    cpu->playNext();
  }

  cpu->executeEventCallback(SID_START_PLAY);

  while(1) {
    //log_v("%s", "we do enter the while");
    log_v("we do enter the while");
    SIDTUNESSerialSongPlayerTaskLock  = xTaskGetCurrentTaskHandle();
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    log_v("we do exit the take");
    //log_v("%s", "we do exit the take");
    SIDTUNESSerialSongPlayerTaskLock=NULL;
    if(!cpu->playNextSong()) {
      break;
    }
  }

  cpu->stop();
  SIDTUNESSerialLoopPlayerTask=NULL;
  cpu->playerrunning=false;

  cpu->executeEventCallback(SID_END_PLAY);

  vTaskDelete(NULL); //we delete the task

}


bool  SIDTunesPlayer::playNextSong()
{
  switch(currentloopmode) {
    case MODE_SINGLE_TRACK: stop(); return false;
    case MODE_LOOP_PLAYLIST_RANDOM: currentfile=rand()%numberOfSongs; playNext(); return true;
    case MODE_LOOP_PLAYLIST_SINGLE_TRACK: playNext(); return true;
    case MODE_SINGLE_SID:
      if(currentsong+1<subsongs) {
        playNextSongInSid();
        return true;
      } else {
        stop();
        return false;
      }
    break;
    case MODE_LOOP_SID: playNextSongInSid(); return true;
    case MODE_LOOP_PLAYLIST_SINGLE_SID:
      if(currentsong+1<subsongs) {
        playNextSongInSid();
        return true;
      } else {
        playNext();
        return true;
      }
    break;
    default: stop(); return false;
  }
}


bool SIDTunesPlayer::play()
{
  if(playerrunning) {
    log_w("player already running");
    return false;
  }
  if(SIDTUNESSerialLoopPlayerTask!=NULL) {
    //sid.soundOff();
    stop();
    vTaskDelete(SIDTUNESSerialLoopPlayerTask);
  }
  xTaskCreatePinnedToCore( SIDTunesPlayer::loopPlayer, "loopPlayer", 4096, this, 1, & SIDTUNESSerialLoopPlayerTask, SID_PLAYER_CORE);
  vTaskDelay(200);
  playerrunning=true;
  return true;
}


bool SIDTunesPlayer::playSongAtPosition(int position)
{
  if(position>=0 && position<numberOfSongs) {
    stop();
    currentfile=position;
    songstruct * p1=listsongs[currentfile];
    if(!playSidFile( p1 )) {
      if(is_error) {
        is_error=false;
        return false;
      }
      return playNext();
    }
  }
  return true;
}


bool SIDTunesPlayer::playNext()
{
  //log_v("%s", "in next");
  //if(SIDTUNESSerialSongPlayerTaskLock!=NULL)
  //   xTaskNotifyGive(SIDTUNESSerialSongPlayerTaskLock);
  //sid.soundOff();
  stop();
  currentfile=(currentfile+1)%numberOfSongs;
  songstruct * p1=listsongs[currentfile];
  if(!playSidFile( p1 )) {
    if(is_error) {
      is_error =false;
      return false;
    }
    return playNext();
  }
  return true;
}


bool SIDTunesPlayer::playPrev()
{
  stop();
  if(currentfile==0)
      currentfile=numberOfSongs-1;
  else
      currentfile--;
  songstruct * p1=listsongs[currentfile];
  //executeEventCallback(SID_NEW_FILE);
  if(!playSidFile( p1 )) {
    if(is_error) {
      is_error =false;
      return false;
    }
    return playPrev();
  }
  return true;
}


char * SIDTunesPlayer::getName()
{
  return (char *)name;
}

char * SIDTunesPlayer::getPublished()
{
  return (char *)published;
}

char * SIDTunesPlayer::getAuthor()
{
  return (char *)author;
}

int SIDTunesPlayer::getNumberOfTunesInSid()
{
  return subsongs;
}

int SIDTunesPlayer::getCurrentTuneInSid()
{
  return currentsong+1;
}

int SIDTunesPlayer::getDefaultTuneInSid()
{
  return startsong;
}

char * SIDTunesPlayer::getFilename()
{
  return currentfilename;
}

int SIDTunesPlayer::getPlaylistSize()
{
  return numberOfSongs;
}

int SIDTunesPlayer::getPositionInPlaylist()
{
    return getcurrentfile+1;
}
