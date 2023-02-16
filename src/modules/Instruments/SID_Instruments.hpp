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
#ifndef _SID_INSTRUMENTS_H_
#define _SID_INSTRUMENTS_H_

#include "SID6581.hpp"
#include "samples.h"


//#define SID_INSTRUMENTS_CORE 1-SID_QUEUE_CORE


typedef struct
{
  int note;
  int velocity;
  uint16_t duration;
} SID_Command_t;


static QueueHandle_t SID_VoicesQueues[15];
static TaskHandle_t  SID_xTaskHandles[15];

static uint16_t SID_PlayNotes[96];
static uint8_t  keyboardnbofvoices;

static bool SID_VoicesBusy[15];
static bool SID_TaskReadyBusy[15];

static int SID_SD_to_duration_ms [16] = { 6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000 };


class sid_instrument
{

  public:

    // allow destructor in inheritance
    virtual ~sid_instrument() { }
    SID6581 *sid;

    int32_t  attack;
    int32_t  sustain;
    int32_t  decay;
    int32_t  release;
    int32_t  pulse;
    int32_t  waveform;
    int32_t  type_instrument;
    int32_t  current_note;
    int32_t  start;
    int32_t  loop;

    uint32_t sustain_time;

    virtual void start_sample(int voice,int note) { }
    virtual void next_instruction(int voice,int note) { }
    virtual void after_off(int voice,int note) { }
    virtual void after_offinstrument(int voice, int note) { }

};


static sid_instrument *current_instruments[15];

static void assertInstrument( int voice )
{
  while( current_instruments[voice] == nullptr ) {
    vTaskDelay(1);
  }
}


template<int voice>
class VoiceTask
{

  public:

    static void  vTaskCode( void * pvParameters )
    {

      if( pvParameters == NULL ) {
        log_e("NO SID OBJECT PASSED TO TASK, ABORTING");
        vTaskDelete( NULL );
        return;
      }
      SID_TaskReadyBusy[voice]=true;
      SID6581 *sid = (SID6581*) pvParameters;
      uint32_t start_time;
      uint32_t sustain_time;
      SID_Command_t element;
      current_instruments[voice]->sid = sid;


      for( ;; ) {
        xQueueReceive( SID_VoicesQueues[voice], &element, portMAX_DELAY );
        assertInstrument( voice );
        if( element.velocity > 0 ) {
          current_instruments[voice]->release=0;
          current_instruments[voice]->current_note=element.note;
          current_instruments[voice]->start_sample(voice,element.note);
          current_instruments[voice]->release = sid->voices[voice].release;
        }
        start_time=millis();
        while( uxQueueMessagesWaiting( SID_VoicesQueues[voice] )==0 ) {
          assertInstrument( voice );
          if( element.duration>0 ) {
            if( millis()-start_time >= element.duration ) {
              sustain_time=millis();
              sid->setGate(voice,0);
              while( uxQueueMessagesWaiting( SID_VoicesQueues[voice] )==0) {
                assertInstrument( voice );
                if( millis()-sustain_time >= SID_SD_to_duration_ms[current_instruments[voice]->release] ) {
                  sid->setFrequency(voice,0);
                  SID_VoicesBusy[voice]=false;
                  break;
                } else {
                  current_instruments[voice]->after_off(voice,current_instruments[voice]->current_note);
                  vTaskDelay(1);
                }
              }
              SID_VoicesBusy[voice]=false;
              break;
            }
          }
          assertInstrument( voice );
          if( element.velocity > 0 ) {
            current_instruments[voice]->next_instruction(voice,element.note);
            vTaskDelay(1);
          } else {
            sustain_time=millis();
            sid->setGate(voice,0);
            SID_VoicesBusy[voice]=false;
            while( uxQueueMessagesWaiting( SID_VoicesQueues[voice] )==0 ) {
              assertInstrument( voice );
              if( millis()-sustain_time >= SID_SD_to_duration_ms[current_instruments[voice]->release] ) {
                sid->setFrequency(voice,0);
                SID_VoicesBusy[voice]=false;
                break;
              } else {
                current_instruments[voice]->after_off(voice,current_instruments[voice]->current_note);
                vTaskDelay(1);
              }
            }
            SID_VoicesBusy[voice]=false;
            break;
          }
        }
      }
    }
};


class sid_piano5 : public sid_instrument
{
  public:

    int i;
    int flo, fhi, plo, phi;
    const uint16_t *df2;

    sid_piano5() { df2=sample2; }
    virtual void start_sample(int voice,int note) { i=30; }
    virtual void after_off(int voice,int note) { i=30; }

    virtual void next_instruction(int voice,int note)
    {
      i=(i+1)%187;
      if(i==0)
          i=43;
      switch(df2[i*3]) {
        case 0:
          flo=df2[i*3+1];
        break;
        case 1:
          fhi=df2[i*3+1];
          sid->setFrequency(voice,((fhi*256+flo)*note)/3747);
        break;
        default:
          if(df2[i*3]<7)
            sid->pushRegister(voice/3,df2[i*3]+(voice%3)*7,df2[i*3+1]);
        break;
      }

      vTaskDelay(df2[i*3+2]/1000+2);
    }
};


class sid_piano : public sid_instrument
{
  public:

    int i;
    int flo, fhi, plo, phi;
    const uint16_t *df2;

    sid_piano() { df2 = sample1; }
    virtual void start_sample(int voice,int note) { i=0; }

    virtual void next_instruction(int voice,int note)
    {
      i=(i+1)%1918;
      if(i==0)
        i=1651;
      switch(df2[i*3]) {
        case 0:
          flo=df2[i*3+1];
        break;
        case 1:
          fhi=df2[i*3+1];
          sid->setFrequency(voice,((fhi*256+flo)*note/7977));
        break;
        default:
          if(df2[i*3]<7)
            sid->pushRegister(voice/3,df2[i*3]+(voice%3)*7,df2[i*3+1]);
        break;
      }
      vTaskDelay(df2[i*3+2]/1000);
    }
};



class sid_piano4 : public sid_instrument
{
  public:

    int i;

    virtual void start_sample(int voice,int note)
    {
      sid->setAttack(voice,3);
      sid->setSustain(voice,9);
      sid->setDecay(voice,9);
      sid->setRelease(voice,10);
      sid->setPulse(voice,3000);
      sid->setWaveForm(voice,SID_WAVEFORM_SAWTOOTH);
      sid->setFrequency(voice,note);
      sid->setGate(voice,1);
      i=0;
    }

    virtual void next_instruction(int voice,int note)
    {
      sid->setFrequency(voice,note+100*(cos(2*3.14*i/10)));
      //sid->setGate(voice,(i/3)%2);
      //x
      i=(i+1)%50;
      vTaskDelay(20);
    }

    virtual void after_off(int voice,int note)
    {
      sid->setFrequency(voice,note+100*(cos(2*3.14*i/10)));
      i=(i+1)%50;
      vTaskDelay(20);
    }
};


class sid_piano3 : public sid_instrument
{
  public:

    int i;

    virtual void start_sample(int voice,int note)
    {
      sid->setAttack(voice,0);
      sid->setSustain(voice,1);
      sid->setDecay(voice,10);
      sid->setRelease(voice,10);
      sid->setPulse(voice,3000);
      sid->setWaveForm(voice,SID_WAVEFORM_SAWTOOTH);
      sid->setFrequency(voice,note);
      sid->setGate(voice,1);
    }
};


class sid_piano2 : public sid_instrument
{
  public:

    int i;

    virtual void start_sample(int voice,int note)
    {
      sid->setAttack(voice,0);
      sid->setSustain(voice,15);
      sid->setDecay(voice,2);
      sid->setRelease(voice,10);
      sid->setPulse(voice,1000);
      sid->setWaveForm(voice,SID_WAVEFORM_PULSE);
      sid->setGate(voice,1);
      i=0;
    }

    virtual void next_instruction(int voice,int note)
    {
      //log_v(note+10*(cos(2*3.14*i/100)));
      sid->setFrequency(voice,note+20*(1+i/50)*(cos(2*3.14*i/10)));
      //x
      i++;
      vTaskDelay(20);
    }

    virtual void after_off(int voice,int note)
    {
      sid->setFrequency(voice,note+20*(cos(2*3.14*i/10)));
      //x
      i++;
      vTaskDelay(20);
    }
};


class SIDKeyBoardPlayer
{

  public:

    static void KeyBoardPlayer( SID6581 *sid, int nbofvoices)
    {
      sid->resetsid();
      createnot();
      keyboardnbofvoices=(nbofvoices%16);
      for( int i=0; i<nbofvoices; i++ ) {
        SID_TaskReadyBusy[i]=false;
        current_instruments[i] = nullptr;
      }
      for( int i=0; i<nbofvoices; i++ ) {
        SID_VoicesQueues[i]= xQueueCreate( 10, sizeof( SID_Command_t ) );
        SID_VoicesBusy[i]=false;
      }
      if( nbofvoices > 0 ) {
        for( int i=0;i<nbofvoices;i++ ) {
          //XTASK_LAUNCH(i);
          //taskName(I);
        }
      }
      BaseType_t SID_INSTRUMENTS_CORE = 1-sid->SID_QUEUE_CORE;

      if(nbofvoices>=1) {
        xTaskCreatePinnedToCore( VoiceTask<0>::vTaskCode, "VoiceTask1",  2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[0], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=2) {
        xTaskCreatePinnedToCore( VoiceTask<1>::vTaskCode, "VoiceTask2", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[1], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=3) {
        xTaskCreatePinnedToCore( VoiceTask<2>::vTaskCode, "VoiceTask3", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[2], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=4) {
        xTaskCreatePinnedToCore( VoiceTask<3>::vTaskCode, "VoiceTask4", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[3], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=5) {
        xTaskCreatePinnedToCore( VoiceTask<4>::vTaskCode, "VoiceTask5", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[4], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=6) {
        xTaskCreatePinnedToCore( VoiceTask<5>::vTaskCode, "VoiceTask6", 2048,(void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[5], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=7) {
        xTaskCreatePinnedToCore( VoiceTask<6>::vTaskCode, "VoiceTask7", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[6], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=8) {
        xTaskCreatePinnedToCore( VoiceTask<7>::vTaskCode, "VoiceTask8", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[7], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=9) {
        xTaskCreatePinnedToCore( VoiceTask<8>::vTaskCode, "VoiceTask9", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[8], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=10) {
        xTaskCreatePinnedToCore( VoiceTask<9>::vTaskCode, "VoiceTask10", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[9], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=11) {
        xTaskCreatePinnedToCore( VoiceTask<10>::vTaskCode, "VoiceTask11", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[10], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=12) {
        xTaskCreatePinnedToCore( VoiceTask<11>::vTaskCode, "VoiceTask12", 2048,(void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[11], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=13) {
        xTaskCreatePinnedToCore( VoiceTask<12>::vTaskCode, "VoiceTask13", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[12], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices>=14) {
        xTaskCreatePinnedToCore( VoiceTask<13>::vTaskCode, "VoiceTask14", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[13], SID_INSTRUMENTS_CORE );
      }
      if(nbofvoices==15) {
        xTaskCreatePinnedToCore( VoiceTask<14>::vTaskCode, "VoiceTask15", 2048, (void*)sid, tskIDLE_PRIORITY, & SID_xTaskHandles[14], SID_INSTRUMENTS_CORE );
      }
      changeAllInstruments<sid_piano4>( sid );
      while(allTaskReady());
      log_i("Keyboard intiated");
    }

    template <typename instrument>static void changeAllInstruments( SID6581 *sid )
    {
      for(int i=0;i<keyboardnbofvoices;i++) {
        sid->voices[i].release=0;
        if( current_instruments[i] != nullptr ) {
          delete current_instruments[i];
          current_instruments[i] = nullptr;
          vTaskDelay(1);
        }
        current_instruments[i] = new instrument;
        current_instruments[i]->sid = sid;
      }
      log_w("Free mem after instruments change: %d", ESP.getFreeHeap() );
    }

    template <typename instrument>static void changeInstrumentOnVoice( SID6581 *sid, int voice)
    {
      if(voice<15) {
        sid->voices[voice].release=0;
        if( current_instruments[voice] != nullptr ) {
          delete current_instruments[voice];
          current_instruments[voice] = nullptr;
          vTaskDelay(1);
        }
        current_instruments[voice] = new instrument;
        current_instruments[voice]->sid = sid;
      }
    }

    static void createnot() {
      double f=274.3352163421902;
      double  g=powf((double)2,(double)1/12);
      for(int i=0;i<95;i++) {
        uint16_t d=floor(f);
        SID_PlayNotes[i]=d;
        f=f*g;
      }
    }

    static void playNoteVelocity(int voice,uint16_t note,int velocity,uint32_t duration)
    {
      SID_Command_t c;
      c.note=note;
      c.duration=duration;
      c.velocity=velocity;
      SID_VoicesBusy[voice]=true;
      xQueueSend(SID_VoicesQueues[voice], &c, portMAX_DELAY);
    }

    static void playNote(int voice,uint16_t note,uint32_t duration)
    {
      SID_Command_t c;
      c.note=note;
      c.duration=duration;
      c.velocity=120;
      SID_VoicesBusy[voice]=true;
      xQueueSend(SID_VoicesQueues[voice], &c, portMAX_DELAY);
    }

    static void stopNote(int voice)
    {
      SID_Command_t c;
      c.note=0;
      c.duration=0;
      c.velocity=0;
      SID_VoicesBusy[voice]=true;
      xQueueSend(SID_VoicesQueues[voice], &c, portMAX_DELAY);
    }

    static void playNoteHz(int voice,int frequencyHz,uint32_t duration)
    {
      double fout=frequencyHz*16.777216;
      playNote(voice,(uint16_t)fout,duration);
    }

    static bool isVoiceBusy(int voice)
    {
      vTaskDelay(1);
      return SID_VoicesBusy[voice];
    }

    static bool areAllVoiceBusy()
    {
      bool res=false;
      vTaskDelay(1);
      for(int i=0;i<keyboardnbofvoices;i++) {
        if( SID_VoicesBusy[i]) {
          res=true;
          break;
        }
      }
      return res;
    }

    static bool allTaskReady()
    {
      vTaskDelay(1);
      for(int i=0;i<keyboardnbofvoices;i++) {
        if( SID_TaskReadyBusy[i]==false)
          return true;
      }
      return false;
    }
};


#endif
