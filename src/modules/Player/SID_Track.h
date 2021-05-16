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

#include "SID6581.h"



typedef struct
{

  char     filename[255]; // This eliminates SPIFFS which is limited to 32 chars
  uint8_t  name[32];      // Song title
  uint8_t  author[32];    // Original author
  char     md5[32];       // The MD5 hash according to HVSC MD5 File
  uint8_t  published[32]; // This is NOT a copyright
  uint8_t  subsongs;      // How many subsongs in this track (up to 255)
  uint8_t  startsong;     // Which song should be played first
  uint32_t *durations = nullptr; // up to 255 durations in a song

} SID_Meta_t;



__attribute__((unused))
static void songdebug( SID_Meta_t* SIDMeta )
{
  Serial.printf("Fname: %s, name: %s, author: %s, md5: %s, pub: %s, sub/start: %d/%d ",
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
      for( int i=0; i < SIDMeta->subsongs; i++ ) {
        if( SIDMeta->durations[i] > 0 ) {
          Serial.printf("%d ms ", int(SIDMeta->durations[i]));
          found++;
        }
      }
      if( found == 0 ) {
        Serial.print("[unpopulated durations]");
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
