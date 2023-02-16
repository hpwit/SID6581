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

#ifndef _MOS_CPU_Controls_H_
#define _MOS_CPU_Controls_H_

#include "SID6581.hpp"
#include "MOS_Registers.h"


class MOS_CPU_Controls
{

  public:

    SID6581 sid;
    uint8_t *mem = NULL;

    long int currentoffset;
    long int previousoffset;
    long int totalinframe;
    long int waitframe;
    long int wait;
    int      volume;
    uint8_t  speed;
    bool     frame;

    uint8_t  getmem( uint16_t addr );
    void     cpuReset();
    uint16_t cpuJSR( uint16_t npc, uint8_t na );
    void     setSpeed( uint32_t _speed );

  private:

    long int waitframeold;
    uint32_t int_speed = 100; // percent
    uint16_t cycles;
    uint16_t wval;
    uint16_t pc;
    uint16_t plmo;
    uint8_t a, x, y, p, s, bval; // wut ?

    uint8_t  getaddr( mode_enum mode );
    uint8_t  pop();
    uint16_t pcinc();
    uint16_t cpuParse();

    void cpuResetTo( uint16_t npc, uint16_t na );
    void setmem( uint16_t addr, uint8_t value );
    void setaddr( mode_enum mode, uint8_t val );
    void putaddr( mode_enum mode, uint8_t val );
    void setflags( flag_enum flag, int cond );
    void push( uint8_t val );
    void getNextFrame( uint16_t npc, uint8_t na );
    void branch( bool flag );
    void feedTheDog();

};



#endif
