//
//  mos6501b.cpp
//
//
//  Created by Yves BAZIN on 25/03/2020.
//

#include "SidPlayer.h"
#include "SID6581.h"

uint8_t SIDTunesPlayer::getmem(uint16_t addr)
{
    return mem[addr];
}

void SIDTunesPlayer::setSpeed(uint32_t speed)
{
    int_speed=100-(speed%101);
}
void SIDTunesPlayer::setmem(uint16_t addr,uint8_t value)
{
    if (addr >= 0xd400 and addr <=0xd620) {
        mem[addr] = value;
        int based=(addr>>8)-0xd4;
        
        long int t=0;
        if(frame)
        {
            
            if(wait==0)
                t=20000;
            else
                t=wait;
            t=t-totalinframe;
            if(t<0)
                t=20000;
            frame=false;
            reset=0;
            wait=0;
            totalinframe=0;
        }
        
        
        uint16_t decal=t+buff-buffold;
        
        addr=(addr&0xFF)+32*based;
        //Serial.printf("%x %x %d\n",addr,value,decal);
        //Serial.printf("%d %d %ld")
        if((addr%32)%24==0 and (addr%32)>0) //we deal with the sound
        {
            //Serial.printf("sound :%x %x\n",addr,value&0xf);
            //sidReg->save24=*(uint8_t*)(d+1);
            value=value& 0xf0 +( ((value& 0x0f)*volume)/15)  ;
            
        }
//        if((addr%32)%7==4)
//        {
//            int d=value&0xf0;
//            if(d!=128 && d!=64 && d!=32 && d!=16 && d!=0)
//            Serial.printf("waveform:%d %d\n",value&0xf0,addr);
//        }
        _sid->pushRegister(addr/32,addr%32,value);
        decal=(decal*int_speed)/100;
        //Serial.printf("ff %d\n",decal);
        //vTaskDelay only takes milliseconds and the delays are in microseconds but the frame switch can be up to 19ms hence to avoid blocking the cpu
        // we do a delayMicroseconds for the microseconds part and and vTaskDelay for the millisecond part
        //maybe a minus to be added in oder to cope with the time to push and stuff
        delayMicroseconds(decal%1000);
        vTaskDelay(decal/1000);
        
        // sid.feedTheDog();
        //mem[addr] = value;
        buffold=buff;
    }
    else
    {
        
        mem[addr] = value;
    }
}

uint16_t SIDTunesPlayer::pcinc(){
    uint16_t tmp=pc++;
    pc &=0xffff;
    return tmp;
}

void SIDTunesPlayer::cpuReset() {
    a    = 0;
    x    = 0;
    y    = 0;
    p    = 0;
    s    = 255;
    pc    = getmem(0xfffc);
    pc |= 256 * getmem(0xfffd);
}

void SIDTunesPlayer::cpuResetTo(uint16_t npc, uint16_t na) {
    a    = na | 0;
    x    = 0;
    y    = 0;
    p    = 0;
    s    = 255;
    pc    = npc;
}


uint8_t SIDTunesPlayer::getaddr(mode_enum mode) {
    
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
    }
    
    return 0;
    
}


void SIDTunesPlayer::SetMaxVolume( uint8_t vol)
{
    volume=vol;
}

void SIDTunesPlayer::setaddr(mode_enum mode,uint8_t val){
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
    }
    
}

void SIDTunesPlayer::putaddr(mode_enum mode,uint8_t val) {
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
    }
    //console.log("putaddr: attempted unhandled mode");
}

void SIDTunesPlayer::setflags(flag_enum flag, int cond) {
    if (cond!=0) {
        p |= flag;
    } else {
        p &= ~flag & 0xff;
    }
}

void SIDTunesPlayer::push(uint8_t val) {
    setmem(0x100 + s, val);
    if (s) s--;
}

uint8_t SIDTunesPlayer::pop () {
    if (s < 0xff) s++;
    return getmem(0x100 + s);
}

void SIDTunesPlayer::branch(bool flag) {
    uint16_t dist = getaddr(mode_imm);
    // FIXME: while this was checked out, it still seems too complicated
    // make signed
    //Serial.printf("dist:%d\n",dist);
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



uint16_t SIDTunesPlayer::cpuParse() {
    uint16_t c;
    cycles = 0;
    //Serial.printf("core:%d\n",xPortGetCoreID());
    uint8_t opc = getmem(pcinc());
    //Serial.printf("ops:%d %d\n",opc,opcodes[opc].inst);
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
            // console.log("cpuParse: attempted unhandled instruction, opcode: ", opc);
    }
    return cycles;
    
}

uint16_t SIDTunesPlayer::cpuJSR(uint16_t npc, uint8_t na) {
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
        //Serial.printf("pc: %d net instr:%d\n",pc,mem[pc+1]);
    }
    //plmo++;
    sid.feedTheDog();
    //Serial.printf("time:%d %d %d\n",timeout,g,pc);
    //Serial.printf("after parse:%d %d %x %x %x %ld\n",pc,wval ,mem[pc],mem[pc+1],mem[pc+2],plmo);
    return ccl;
}


void SIDTunesPlayer::getNextFrame(uint16_t npc, uint8_t na)
{
    for(int i=0;i<15000;i++)
    {
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


bool SIDTunesPlayer::getInfoFromFile(fs::FS &fs, const char * path,songstruct * songinfo)
{
    uint8_t stype[5];
    if( &fs==NULL || path==NULL)
    {
        Serial.println("Invalid parameter");
        
        getcurrentfile=currentfile;
        return false;
    }
    if(!fs.exists(path))
    {
        Serial.printf("file %s unknown\n",path);
        getcurrentfile=currentfile;
        return false;
    }
    File file=fs.open(path);
    if(!file)
    {
        Serial.printf("Unable to open %s\n",path);
        getcurrentfile=currentfile;
        return false;
    }
    memset(stype,0,5);
    file.read(stype,4);
    if(stype[0]!=0x50 || stype[1]!=0x53 || stype[2]!=0x49 || stype[3]!=0x44)
    {
        Serial.printf("File type:%s not handle yet\n",stype);
        //getcurrentfile=currentfile;
        file.close();
        return false;
    }
    
    file.seek( 15);
    songinfo->subsongs=0;
    file.read(&songinfo->subsongs,1);
    //Serial.printf("subsongs :%d\n",subsongs );
    
    file.seek(17);
    songinfo->startsong=0;
    file.read(&songinfo->startsong,1);
    //Serial.printf("startsong :%d\n",startsong -1);
    
    //Serial.printf("speed :%d\n",speed );
    
    file.seek(22);
    
    file.read(songinfo->name,32);
    //Serial.printf("name :%s\n",name );
    
    file.seek(0x36);
    
    file.read(songinfo->author,32);
    //Serial.printf("author :%s\n",author );
    
    file.seek(0x56);
    
    file.read(songinfo->published,32);
    file.close();
    sprintf(songinfo->md5,"%s",md5.calcMd5(fs,path));
    return true;
}


void SIDTunesPlayer::getSongslengthfromMd5(fs::FS &fs, const char * path)
{
    
    char lom[320+35];
    char bu[320];
    char parsestr[32*10+35];
    int f,jd,k;
    Serial.printf("Opening songslength file :%s ...\n",path);
    File md5File=fs.open(path);
    if(!md5File)
    {
        Serial.println("File not found");
        return;
    }
    Serial.println("File open. Matching MD5 ...");
    while (md5File.available())
    {
        String list = md5File.readStringUntil('\n');
        //Serial.println(list.length());
        
        
        String stringTwo;
        
        if(list[0]!=59 && list[0]!=91)
        {
            for(int s=0;s<numberOfSongs;s++)
            {
                //Serial.printf("dd %s %s\n",listsongs[s].filename,listsongs[s].md5);
                stringTwo=String(listsongs[s]->md5);
                if(list.startsWith(stringTwo))
                {
                    Serial.printf("md5 info found for file %s\n",listsongs[s]->filename);
                    memset(lom,0,320+35);
                    list.toCharArray(lom,list.length()+1);
                    //Serial.println(lom);
                    //Serial.println(list);
                    memset(parsestr,0,32*10+35);
                    sprintf(parsestr,"%s=%%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s %%s",listsongs[s]->md5);
                    //Serial.printf("parse :%s\n",parsestr);
                    memset(bu,0,320);            sscanf(lom,parsestr,&bu[0],&bu[10],&bu[20],&bu[30],&bu[40],&bu[50],&bu[60],&bu[70],&bu[80],&bu[90],&bu[100],&bu[110],&bu[120],&bu[130],&bu[140],&bu[150],&bu[160],&bu[170],&bu[180],&bu[190],&bu[200],&bu[210],&bu[220],&bu[230],&bu[240],&bu[250],&bu[260],&bu[270],&bu[280],&bu[290],&bu[300],&bu[310]);
                    for(int m=0;m<32;m++)
                    {
                        if(bu[m*10]!=0)
                        {
                            f=jd=k=0;
                            sscanf(&bu[m*10],"%d:%d.%d",&f,&jd,&k);
                            //Serial.printf("%s %d\n",&bu[10*m],(f*60*1000+jd*1000+k));
                            listsongs[s]->durations[m]=f*60*1000+jd*1000+k;
                        }
                    }
                }
            }
        }
    }
    md5File.close();
    Serial.println("Matching done.");
    //free(lom);
    //free(bu);
    //free(parsestr);
}

//The following is a function from tobozo
//thank to him

void SIDTunesPlayer::addSongsFromFolder( fs::FS &fs, const char* foldername, const char* filetype, bool recursive ) {
    
    File root = fs.open( foldername );
    if(!root){
        Serial.printf("[ERROR] Failed to open %s directory\n", foldername);
        return;
    }
    if(!root.isDirectory()){
        Serial.printf("[ERROR] %s is not a directory\b", foldername);
        return;
    }
    File file = root.openNextFile();
    while(file){
        if( file.isDirectory() || !String( file.name() ).endsWith( filetype ) ) {
            if( recursive ) {
                addSongsFromFolder( fs, file.name(), filetype, true );
            } else {
                Serial.printf("[INFO] Ignored [%s]\n", file.name() );
            }
        } else {
            addSong(fs, file.name() );
            // Serial.printf("[INFO] Added [%s] ( %d bytes )\n", file.name(), file.size() );
        }
        file = root.openNextFile();
    }
}


songstruct  SIDTunesPlayer::getSidFileInfo(int songnumber)
{
    if(songnumber<numberOfSongs && songnumber>=0)
    {
        return *listsongs[songnumber];
    }
}

bool SIDTunesPlayer::playSidFile(fs::FS &fs, const char * path)
{
    
    Serial.printf("playing file:%s\n",path);
    if(mem==NULL)
    {
        Serial.println("we create the memory buffer");
        Serial.printf("available mem:%d %d\n",ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
        mem=(uint8_t*)calloc(0x10000,1);
        if(mem==NULL)
        {
            Serial.println("not enough memory\n");
            return false;
        }
    }
    else
    {
 
        memset(mem,0x0,0x10000);
    }
    memset(name,0,32);
    memset(author,0,32);
    memset(published,0,32);
    memset(currentfilename,0,50);
    memset(sidtype,0,5);
    if( &fs==NULL || path==NULL)
    {
        Serial.println("Invalid parameter");
        
        getcurrentfile=currentfile;
        return false;
    }
    if(!fs.exists(path))
    {
        Serial.printf("file %s unknown\n",path);
        getcurrentfile=currentfile;
        return false;
    }
    File file=fs.open(path);
    if(!file)
    {
        Serial.printf("Unable to open %s\n",path);
        getcurrentfile=currentfile;
        return false;
    }
    file.read(sidtype,4);
    if(sidtype[0]!=0x50 || sidtype[1]!=0x53 || sidtype[2]!=0x49 || sidtype[3]!=0x44)
    {
        Serial.printf("File type:%s not handle yet\n",sidtype);
        getcurrentfile=currentfile;
        return false;
    }
    
    file.seek(7);
    
    data_offset=0;
    file.read(&data_offset,1);
    
    //Serial.printf("data_offset:%d\n",data_offset);
    uint8_t not_load_addr[2];
    file.read(not_load_addr,2);
    
    init_addr=0;
    uint8_t f=0;
    file.read(&f,1);
    init_addr  = f*256;
    file.read(&f,1);
    init_addr+=f;
    //Serial.printf("init_addr:%d\n",init_addr);
    
    
    play_addr=0;
    f=0;
    file.read(&f,1);
    play_addr  = f*256;
    file.read(&f,1);
    play_addr+=f;
    //Serial.printf("play_addr:%d\n",play_addr);
    
    file.seek( 15);
    subsongs=0;
    file.read(&subsongs,1);
    //Serial.printf("subsongs :%d\n",subsongs );
    
    file.seek(17);
    startsong=0;
    file.read(&startsong,1);
    //Serial.printf("startsong :%d\n",startsong -1);
    
    file.seek(21);
    speed=0;
    file.read(&speed,1);
    //Serial.printf("speed :%d\n",speed );
    
    file.seek(22);
    
    file.read(name,32);
    //Serial.printf("name :%s\n",name );
    
    file.seek(0x36);
    
    file.read(author,32);
    //Serial.printf("author :%s\n",author );
    
    file.seek(0x56);
    
    file.read(published,32);
    //Serial.printf("published :%s\n",published );
    
    
    
    
    if(speed==0)
        nRefreshCIAbase=38000;
    else
        nRefreshCIAbase=19950;
    
    file.seek(data_offset);
    load_addr;
    uint8_t d1,d2;
    file.read(&d1,1);
    file.read(&d2,1);
    load_addr=d1+(d2<<8);
    //Serial.printf("load_addr :%d\n",load_addr );
    size_t g=file.read(&mem[load_addr],file.size());
    //memset(&mem[load_addr+g],0xea,0x10000-load_addr+g);
    //Serial.printf("read %d\n",(int)g);
    for(int i=0;i<32;i++)
    {
        uint8_t fm;
        file.seek(0x12+(i>>3));
        file.read(&fm,1);
        speedsong[31-i]= (fm & (byte)pow(2,7-i%8))?1:0;
        //Serial.printf("%d %1d\n",31-i, (fm & (byte)pow(2,7-i%8))?1:0);
    }
    sprintf(currentfilename,"%s",path);
    getcurrentfile=currentfile;
    //executeEventCallback(SID_START_PLAY);
    currentsong=startsong -1;
    executeEventCallback(SID_NEW_FILE);
    file.close();
    _playSongNumber(startsong -1);
    
    //xTaskNotifyGive(SIDTUNESSerialSongPlayerTaskLock);
    //while(1){};
    return true;
}

void SIDTunesPlayer::playNextSongInSid()
{
    
    //Serial.printf("in net:%d\n",subsongs);
    stop();
    currentsong=(currentsong+1)%subsongs;
    _playSongNumber(currentsong);
    
}

void SIDTunesPlayer::executeEventCallback(sidEvent event){
    if (eventCallback) (*eventCallback)(event);
}
void SIDTunesPlayer::playPrevSongInSid()
{
    if(currentsong==0)
        currentsong=(subsongs-1);
    else
        currentsong--;
    
    _playSongNumber(currentsong);
    
}

uint32_t SIDTunesPlayer::getCurrentTrackDuration()
{
    return song_duration;
}
void SIDTunesPlayer::stop()
{
    
    executeEventCallback(SID_STOP_TRACK);
    //Serial.printf("playing stop n:%d/%d\n",1,subsongs);
    sid.soundOff();
    if(SIDTUNESSerialPlayerTaskHandle!=NULL)
    {
        
        vTaskDelete(SIDTUNESSerialPlayerTaskHandle);
        SIDTUNESSerialPlayerTaskHandle=NULL;
    }
    //Serial.printf("playing sop n:%d/%d\n",1,subsongs);
}

void SIDTunesPlayer::stopPlayer()
{
    
    executeEventCallback(SID_END_PLAY);
    //Serial.printf("playing stop n:%d/%d\n",1,subsongs);
    sid.soundOff();
    if(SIDTUNESSerialPlayerTaskHandle!=NULL)
    {
        
        vTaskDelete(SIDTUNESSerialPlayerTaskHandle);
        SIDTUNESSerialPlayerTaskHandle=NULL;
    }
    if(SIDTUNESSerialLoopPlayerTask!=NULL)
    {
        
        vTaskDelete(SIDTUNESSerialLoopPlayerTask);
        SIDTUNESSerialLoopPlayerTask=NULL;
    }
    //SIDTUNESSerialLoopPlayerTask=NULL;
    playerrunning=false;
    //Serial.printf("playing sop n:%d/%d\n",1,subsongs);
}


void SIDTunesPlayer::pausePlay()
{
    
    if(SIDTUNESSerialPlayerTaskHandle!=NULL)
    {
        
        if(!paused)
        {
            
            paused=true;
            executeEventCallback(SID_PAUSE_PLAY);
        }
        else
        {
            sid.soundOn();
            executeEventCallback(SID_RESUME_PLAY);
            paused=false;
            if(PausePlayTaskLocker!=NULL)
                xTaskNotifyGive(PausePlayTaskLocker);
        }
    }
    
}
bool SIDTunesPlayer::begin(int clock_pin,int data_pin, int latch,int sid_clock_pin)
{
    sid.begin(clock_pin,data_pin,latch,sid_clock_pin);
    volume=15;
    numberOfSongs=0;
    currentfile=0;
    
    
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
bool SIDTunesPlayer::begin(int clock_pin,int data_pin, int latch )
{
    sid.begin(clock_pin,data_pin,latch);
    volume=15;
    numberOfSongs=0;
    currentfile=0;
}
void SIDTunesPlayer::_playSongNumber(int songnumber)
{
    sid.soundOff();
    sid.resetsid();
    buff=0;
    buffold=0;
    currentsong=songnumber;
    if(SIDTUNESSerialPlayerTaskHandle!=NULL)
    {
        
        vTaskDelete(SIDTUNESSerialPlayerTaskHandle);
        SIDTUNESSerialPlayerTaskHandle==NULL;
    }
    if(songnumber<0 || songnumber>=subsongs)
    {
        Serial.println("Wrong song number");
        return;
    }
    Serial.printf("playing song n:%d/%d\n",(songnumber+1),subsongs);
    
    cpuReset();
    //Serial.printf("playing song n:%d/%d\n",(songnumber+1),subsongs);
    //_sidtunes_voicesQueues= xQueueCreate( 26000, sizeof( serial_command ) );
    
    if(play_addr==0)
    {
        
        //                Serial.printf("init %x\n",init_addr);
        //        for(int g=0;g<10;g++)
        //        {
        //            Serial.printf("d %x\n",mem[init_addr+g]);
        //        }
        //songnumber=2;
        cpuJSR(init_addr, 0); //0
        
        play_addr = (mem[0x0315] << 8) + mem[0x0314];
        if(play_addr==0)
            
        {
            cpuJSR(init_addr, songnumber);
            play_addr=(mem[0xffff] << 8) + mem[0xfffe];
            mem[1]=0x37;
        }
        else
        {
            cpuJSR(init_addr, songnumber);
        }
        
        // Serial.printf("new play address %x %x %x\n",play_addr,(mem[0xffff] << 8) + mem[0xfffe],(mem[0x0315] << 8) + mem[0x0314]);
        
    }
    else
    {
        cpuJSR(init_addr, songnumber);
    }
    // Serial.printf("playing song n:%d/%d\n",(songnumber+1),subsongs);
    //cpuJSR(init_addr, songnumber);
    //sid.soundOn();
    xTaskCreatePinnedToCore(
                            SIDTunesPlayer::SIDTUNESSerialPlayerTask,      /* Function that implements the task. */
                            "SIDTUNESSerialPlayerTask",          /* Text name for the task. */
                            4096,      /* Stack size in words, not bytes. */
                            this,    /* Parameter passed into the task. */
                            SID_CPU_TASK_PRIORITY,/* Priority at which the task is created. */
                            & SIDTUNESSerialPlayerTaskHandle,SID_CPU_CORE);
    
    //Serial.printf("tas %d core %d\n",SID_TASK_PRIORITY,SID_CPU_CORE);
    
    delay(200);
    songstruct * p=listsongs[getcurrentfile];
    if(p->durations[currentsong]==0)
    {
        song_duration=default_song_duration;
        Serial.printf("Playing with default song duration %d ms\n",song_duration);
    }
    else
    {
        song_duration=p->durations[currentsong];
        Serial.printf("Playing with md5 database song duration %d ms\n",song_duration);
    }
    executeEventCallback(SID_NEW_TRACK);
    if(SIDTUNESSerialPlayerTaskLock!=NULL)
        xTaskNotifyGive(SIDTUNESSerialPlayerTaskLock);
    
    
}

void  SIDTunesPlayer::SIDTUNESSerialPlayerTask(void * parameters)
{
    
    for(;;)
    {
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
        while(1 && !cpu->stop_song)
        {
            
            cpu->totalinframe+=cpu->cpuJSR(cpu->play_addr,0);
            
            cpu->wait+=cpu->waitframe;
            cpu->frame=true;
            
            int nRefreshCIA = (int)( ((19650 * (cpu->getmem(0xdc04) | (cpu->getmem(0xdc05) << 8)) / 0x4c00) + (cpu->getmem(0xdc04) | (cpu->getmem(0xdc05) << 8))  )  /2 )    ;
            
            if ((nRefreshCIA == 0) )//|| (cpu->speedsong[cpu->currentsong]==0))
                nRefreshCIA = 19650;
            
            cpu->waitframe=nRefreshCIA;
            
            if(cpu->song_duration>0)
            {
                now_time=millis();
                cpu->delta_song_duration+=now_time-start_time;
                start_time=now_time;
                if(cpu->delta_song_duration>=cpu->song_duration)
                {
                    
                    cpu->stop_song=true;
                    break;
                }
            }
            
            if(cpu->paused)
            {
                
                sid.soundOff();
                PausePlayTaskLocker  = xTaskGetCurrentTaskHandle();
                //yield();
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                PausePlayTaskLocker=NULL;
                start_time=millis();
            }
            
        }
         sid.soundOff();
        cpu->executeEventCallback(SID_END_TRACK);
       
        if(SIDTUNESSerialSongPlayerTaskLock!=NULL)
            xTaskNotifyGive(SIDTUNESSerialSongPlayerTaskLock);
    }
}



void SIDTunesPlayer::addSong(fs::FS &fs,  const char * path)
{
 
    if(numberOfSongs==255)
    {
        Serial.println("Play List full");
        return;
    }
    songstruct * p1;
    p1=(songstruct *)calloc(1,sizeof(songstruct));
    if(p1==NULL)
    {
        Serial.println("no more memory to add a song");
        return;
    }
    p1->fs=(fs::FS *)&fs;
    //char h[250];
    sprintf(p1->filename,"%s",path);
    if(getInfoFromFile(fs,path,p1))
    {
        //p1.filename=h;
        
        memset(p1->durations,0,32*4);
        listsongs[numberOfSongs]=p1;
        numberOfSongs++;
    }
    else
    {
        Serial.printf("file %s not handle yet\n",path);
    }
    //Serial.printf("nb song:%d\n",numberOfSongs);
}

void SIDTunesPlayer::loopPlayer(void *param)
{
    SIDTunesPlayer * cpu= (SIDTunesPlayer *)param;
    cpu->executeEventCallback(SID_START_PLAY);
    for(;;)
    {
        
        
        songstruct p1=*(cpu->listsongs[cpu->currentfile]);
        // Serial.printf("currentfile %d %s\n",currentfile,p1.filename);
        
        if(!cpu->playSidFile(*p1.fs,p1.filename))
        {
            cpu->playNext();
        }
        while(1)
        {
            //Serial.println("we do enter the while");
            log_v("we do enter the while");
            SIDTUNESSerialSongPlayerTaskLock  = xTaskGetCurrentTaskHandle();
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            log_v("we do exit the take");
             //Serial.println("we do exit the take");
            
            SIDTUNESSerialSongPlayerTaskLock=NULL;
            if(!cpu->playNextSong())
            {
                break;
            }
        }
       
        //sid.soundOff();
        cpu->stop();
        cpu->executeEventCallback(SID_END_PLAY);
        SIDTUNESSerialLoopPlayerTask=NULL;
        cpu->playerrunning=false;
        vTaskDelete(NULL); //we delete the task
    }
}

bool  SIDTunesPlayer::playNextSong()
{
    switch(currentloopmode)
    {
        case MODE_SINGLE_TRACK:
            //sid.soundOff();
            stop();
            return false;
            break;
            
        case MODE_LOOP_PLAYLIST_SINGLE_TRACK:
            playNext();
            return true;
            break;
        case MODE_SINGLE_SID:
            if(currentsong+1<subsongs)
            {
                playNextSongInSid();
                return true;
            }
            else
            {
                //sid.soundOff();
                stop();
                return false;
            }
            break;
        case MODE_LOOP_SID:
            playNextSongInSid();
            return true;
            break;
        case MODE_LOOP_PLAYLIST_SINGLE_SID:
            if(currentsong+1<subsongs)
            {
                playNextSongInSid();
                return true;
            }
            else
            {
                playNext();
                return true;
                
            }
            break;
        default:
            //sid.soundOff();
            stop();
            return false;
            break;
    }
}

bool SIDTunesPlayer::play()
{
    //stop();
    if(playerrunning)
    {
        Serial.println("player already running");
        return false;
    }
    if(SIDTUNESSerialLoopPlayerTask!=NULL)
    {
        //sid.soundOff();
        stop();
        vTaskDelete(SIDTUNESSerialLoopPlayerTask);
    }
    xTaskCreatePinnedToCore(
                            SIDTunesPlayer::loopPlayer,      /* Function that implements the task. */
                            "loopPlayer",          /* Text name for the task. */
                            4096,      /* Stack size in words, not bytes. */
                            this,    /* Parameter passed into the task. */
                            1,/* Priority at which the task is created. */
                            & SIDTUNESSerialLoopPlayerTask,1);
    delay(200);
    playerrunning=true;
    return true;
}

bool SIDTunesPlayer::playSongAtPosition(int position)
{
    
    if(position>=0 && position<numberOfSongs)
        {
            stop();
            currentfile=position;
            songstruct * p1=listsongs[currentfile];
            
            if(!playSidFile(*p1->fs,p1->filename))
            {
                return playNext();
            }
    }
    return true;
}

bool SIDTunesPlayer::playNext()
{
    //Serial.println("in next");
    //if(SIDTUNESSerialSongPlayerTaskLock!=NULL)
    //   xTaskNotifyGive(SIDTUNESSerialSongPlayerTaskLock);
    
    //sid.soundOff();
    stop();
    
    currentfile=(currentfile+1)%numberOfSongs;
    songstruct * p1=listsongs[currentfile];
    
    if(!playSidFile(*p1->fs,p1->filename))
    {
        return playNext();
    }
    return true;
}

bool SIDTunesPlayer::playPrev()
{
    //sid.soundOff();
    stop();
    if(currentfile==0)
        currentfile=numberOfSongs-1;
    else
        currentfile--;
    songstruct * p1=listsongs[currentfile];
    //executeEventCallback(SID_NEW_FILE);
    
    if(!playSidFile(*p1->fs,p1->filename))
    {
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

char * SIDTunesPlayer::getFilename(){
    
    return currentfilename;
}
int SIDTunesPlayer::getPlaylistSize(){
    return numberOfSongs;
}
int SIDTunesPlayer::getPositionInPlaylist(){
    return getcurrentfile+1;
    
}
