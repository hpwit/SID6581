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

#ifndef _SID_TRACK_H_
#define _SID_TRACK_H_

#include "SID6581.hpp"



struct SID_Meta_t
{
  ~SID_Meta_t() { if(durations) { free(durations); durations=nullptr; } }
  char     filename[256]; // WARNING: SPIFFS which is limited to 32 chars
  uint8_t  name[33];      // Song title
  uint8_t  author[33];    // Original author
  char     md5[33];       // The MD5 hash according to HVSC MD5 File
  uint8_t  published[33]; // This is NOT a copyright
  uint8_t  subsongs{0};      // How many subsongs in this track (up to 255)
  uint8_t  startsong{0};     // Which song should be played first (0=invalid)
  uint32_t *durations{nullptr}; // up to 255 durations in a song

} /*SID_Meta_t*/;



#ifndef SID_SONG_DEBUG_PATTERN
  #define SID_SONG_DEBUG_PATTERN "Fname: %s\nName: %s\nAuthor: %s\nMD5: %s\npub: %s\nsongs: %d\nstart: %d\n"
#endif


__attribute__((unused))
static void songdebug( SID_Meta_t* SIDMeta )
{
  Serial.printf(SID_SONG_DEBUG_PATTERN,
    SIDMeta->filename,
    (const char*)SIDMeta->name,
    (const char*)SIDMeta->author,
    SIDMeta->md5,
    (const char*)SIDMeta->published,
    SIDMeta->subsongs,
    SIDMeta->startsong
  );
  if( SIDMeta->subsongs>0 ) {
    if( SIDMeta->durations != nullptr ) {
      size_t found = 0;
      Serial.print("Durations: ");
      for( int i=0; i < SIDMeta->subsongs; i++ ) {
        uint32_t duration = SIDMeta->durations[i];
        if( duration > 0 ) {
          uint16_t mm,ss,SSS;
          SSS = (duration%1000);
          ss  = ((duration/1000)%60);
          mm  = (duration/60000);
          //Serial.printf("%lu ms ", duration);
          Serial.printf("%d:%d.%d ", mm, ss, SSS );
          found++;
        }
      }
      if( found == 0 ) {
        Serial.print("[unpopulated]");
      }
    } else {
      Serial.print("[no durations]");
    }
  } else {
    Serial.print("[no subsongs]");
  }
  Serial.println();
}


#endif
