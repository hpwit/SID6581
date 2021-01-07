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

#include "MOS_CPU_Controls.h"


void MOS_CPU_Controls::setflags( flag_enum flag, int cond )
{
  if (cond!=0) {
    p |= flag;
  } else {
    p &= ~flag & 0xff;
  }
}


void MOS_CPU_Controls::push( uint8_t val )
{
  setmem(0x100 + s, val);
  if (s) s--;
}


uint8_t MOS_CPU_Controls::pop ()
{
  if (s < 0xff) s++;
  return getmem(0x100 + s);
}


void MOS_CPU_Controls::setSpeed( uint32_t speed )
{
  int_speed = 100-(speed%101);
}



uint8_t MOS_CPU_Controls::getmem( uint16_t addr )
{
  return mem[addr];
}


void MOS_CPU_Controls::setmem( uint16_t addr,uint8_t value )
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
      //reset = 0;
      wait = 0;
      totalinframe = 0;
    }

    uint16_t decal = t+currentoffset-previousoffset;
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
    previousoffset=currentoffset;
  } else {
    mem[addr] = value;
  }
}


uint16_t MOS_CPU_Controls::pcinc()
{
  uint16_t tmp=pc++;
  pc &=0xffff;
  return tmp;
}


void MOS_CPU_Controls::cpuReset()
{
  a    = 0;
  x    = 0;
  y    = 0;
  p    = 0;
  s    = 255;
  pc   = getmem(0xfffc);
  pc  |= 256 * getmem(0xfffd);
}


void MOS_CPU_Controls::cpuResetTo( uint16_t npc, uint16_t na )
{
  a    = na | 0;
  x    = 0;
  y    = 0;
  p    = 0;
  s    = 255;
  pc   = npc;
}


uint8_t MOS_CPU_Controls::getaddr( mode_enum mode )
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



void MOS_CPU_Controls::setaddr( mode_enum mode, uint8_t val )
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


void MOS_CPU_Controls::putaddr( mode_enum mode, uint8_t val )
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




void MOS_CPU_Controls::branch( bool flag )
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


uint16_t MOS_CPU_Controls::cpuParse()
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


uint16_t MOS_CPU_Controls::cpuJSR( uint16_t npc, uint8_t na )
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
    currentoffset+=g;
    ccl +=g;
    //sid.feedTheDog ();
    //printf("cycles %d\n",currentoffset);
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


void MOS_CPU_Controls::getNextFrame( uint16_t npc, uint8_t na )
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




