/*\
 *

  SID6581 + USB Soft Host Demo

  A demo brought to you by tobozo

  MIT License

  Copyright (c) 2021 tobozo

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  -----------------------------------------------------------------------------

  USB Soft Host is for ESP32 Wroom/Wrover only!
  This demo require 2 *non shared* pins.

  // some pins tested successfully on ESP32-Wroom
  #define DP_P0  16
  #define DM_P0  17
  #define DP_P1  22
  #define DM_P1  23
  #define DP_P2  18
  #define DM_P2  19
  #define DP_P3  13
  #define DM_P3  15

 *
\*/

#define SID_CLOCK 25
#define SID_DATA 15
#define SID_LATCH 2
#define SID_CLOCK_PIN 26 // optional if the SID already has an oscillator
#define SID_INSTRUMENTS // load the instruments module from the SID6581 library
#define NUMBER_OF_VOICES 3

// Only one USB port is necessary in this demo, but all pins must be declared.
// /!\ Not all pins can be used, see the USB_Soft_Host examples for full pins list
#define DP_P0  SCL  // always enabled
#define DM_P0  SDA  // always enabled
#define DP_P1  -1
#define DM_P1  -1
#define DP_P2  -1
#define DM_P2  -1
#define DP_P3  -1
#define DM_P3  -1


// uncomment this if you have a display supported by LovyanGFX
#define USE_DISPLAY

#ifdef USE_DISPLAY

  #include <SID6581.h> // http://librarymanager#SID6581
  #include <ESP32-Chimera-Core.h> // http://librarymanager#esp32-chimera-core + http://librarymanager#lovyangfx
  #include <ESP32-USBSoftHost.hpp> // http://librarymanager#ESP32-USB-Soft-Host
  #include "usbkbd.h" // HID Keyboard Report Parser
  #include "kbdparser.h" // USB-HID layer
  #include "C64_UI.h" // C64 styled UI

#else

  #include <SID6581.h> // http://librarymanager#SID6581
  #include <ESP32-USBSoftHost.hpp> // http://librarymanager#ESP32-USB-Soft-Host
  #include "usbkbd.h" // HID Keyboard Report Parser
  #include "kbdparser.h" // USB-HID layer
  void usbTicker() { }

#endif


static SID6581* sid;
KbdRptParser Prs;


int voices[NUMBER_OF_VOICES];
int lastvoice=0;
uint8_t octave = 1;
int max_octaves = 4;
uint8_t pressedKeys[NUMBER_OF_VOICES];
uint8_t pressedKeysCache[NUMBER_OF_VOICES];
int selectedIntrument = 0;


// keyboard data parser to pass to the USB driver as a callback
static void m5_USB_PrintCB(uint8_t usbNum, uint8_t byte_depth, uint8_t* data, uint8_t data_len)
{
  Prs.Parse( data_len, data );
}


// USB detection callback
static void my_USB_DetectCB( uint8_t usbNum, void * dev )
{
  sDevDesc *device = (sDevDesc*)dev;
  printf("New device detected on USB#%d\n", usbNum);
  printf("desc.idVendor           = 0x%04x\n", device->idVendor);
  printf("desc.idProduct          = 0x%04x\n", device->idProduct);
}


void  change_instrument()
{
  selectedIntrument = selectedIntrument%5;

  switch(selectedIntrument) {
    case 0:
      SIDKeyBoardPlayer::changeAllInstruments<sid_piano2>( sid );
    break;
    case 1:
       SIDKeyBoardPlayer::changeAllInstruments<sid_piano5>( sid );
    break;
    case 2:
      SIDKeyBoardPlayer::changeAllInstruments<sid_piano>( sid );
    break;
    case 3:
      SIDKeyBoardPlayer::changeAllInstruments<sid_piano3>( sid );
    break;
    case 4:
      SIDKeyBoardPlayer::changeAllInstruments<sid_piano4>( sid );
    break;
  }
  vTaskDelay(20);
  Serial.printf("Changed instrument to #%d\n", selectedIntrument);
  selectedIntrument++;
}


void playKeyNote(uint8_t key, int note)
{
  Serial.printf("Play note #%d\n", note );
  for(int i=0;i<NUMBER_OF_VOICES;i++) {
    int new_voice=(lastvoice+1+i)%NUMBER_OF_VOICES;
    if(voices[new_voice]==0) {
      voices[new_voice]=note;
      pressedKeys[new_voice] = key;
      SIDKeyBoardPlayer::playNote(new_voice,SID_PlayNotes[note],0);
      lastvoice=new_voice;
      #ifdef USE_DISPLAY
        drawKey( key, true );
        kbChanged = true;
      #endif
      break;
    }
    Serial.printf("voice %d not free\n",i);
  }
}


void stopKeyNote(uint8_t key, int note)
{
  for(int i=0;i<NUMBER_OF_VOICES;i++) {
    if(voices[i]==note) {
      SIDKeyBoardPlayer::stopNote(i);
      voices[i]=0;
      pressedKeys[i] = 0;
      #ifdef USE_DISPLAY
        drawKey( key, false );
        kbChanged = true;
      #endif
      break;
    }
  }
}


void HIDEventHandler( uint8_t mod, uint8_t key, keyToMidiEvents evt )
{
  //                      key/notes position
  //  2 3   5 6 7   9 0
  // q w e r t y u i o p
  //                      => q2w3er5t6y7ui9o0p
  //  s d   g h j
  // z x c v b n m
  //                      => zsxdcvgbhnjm
  char keyNotes[] = "zsxdcvgbhnjmq2w3er5t6y7ui9o0p";
  char keyNotesLen = strlen(keyNotes);
  int note = -1;

  if( evt == onKeyReleased || evt == onKeyPressed ) {
    // usb event to printable char
    for(int i=0;i<keyNotesLen;i++) {
      // printable char to note
      if( keyNotes[i] ==key ) {
        note = i + (octave*24);
      }
    }
  }
  if( evt == onKeyPressed ) {
    if( key == '+' ) {
      octave = (octave+1)%max_octaves;
      Serial.printf("New Octave: %d\n", octave);
    }
    if( key == '-' ) {
      octave = (octave+max_octaves-1)%max_octaves;
      Serial.printf("New Octave: %d\n", octave);
    }
    if( key == '`' ) {
      change_instrument();
      Serial.printf("New instrument\n");
    }
    if( note > -1 ) {
      playKeyNote(key, note);
    }
  }
  if( evt == onKeyReleased ) {
    if( note > -1 ) {
      stopKeyNote(key, note);
    }
  }
}

static const float deg2rad = M_PI / 180.0;


void setup()
{
  #ifdef USE_DISPLAY
    M5.begin();
    // draw music keyboard
    setupUI();
  #else
    Serial.begin(115200);
  #endif

  sid = new SID6581();

  // start the SID
  sid->begin(SID_CLOCK,SID_DATA,SID_LATCH);
  SIDKeyBoardPlayer::KeyBoardPlayer( sid, NUMBER_OF_VOICES);

  sid->sidSetVolume(0,15);
  sid->sidSetVolume(1,15);
  sid->sidSetVolume(2,15);

  // attach to USB event handler
  Prs.setEventHandler( HIDEventHandler );
  USH.setTaskCore(0);
  // init the USB Host
  USH.init(
    (usb_pins_config_t){
      DP_P0, DM_P0,
      DP_P1, DM_P1,
      DP_P2, DM_P2,
      DP_P3, DM_P3
    },
    my_USB_DetectCB,
    m5_USB_PrintCB,
    usbTicker
  );


}



void loop()
{
  vTaskDelete(NULL);
}
