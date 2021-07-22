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

#ifndef _SID_MD5_H_
#define _SID_MD5_H_

#include <FS.h>


//#include "esp32/rom/md5_hash.h"
#include <MD5Builder.h>

static MD5Builder sid_md5;
static char sid_md5result[33];
static uint8_t sid_fbuf[256];

class Sid_md5 {

  public:

    static char* calcMd5(fs::File *file ) {

      int len = file->size();
      //if( file->position() != 0 )
      file->seek(0); // make sure to read from the start

      sid_md5.begin();

      int bufSize = len > 256 ? 256 : len;
      //uint8_t *sid_fbuf = (uint8_t*)malloc(bufSize+1);

      size_t bytes_read = file->read( sid_fbuf, bufSize );

      do {
        len -= bytes_read;
        if( bufSize > len ) bufSize = len;
        sid_md5.add( sid_fbuf, bytes_read );
        if( len == 0 ) break;
        bytes_read = file->read( sid_fbuf, bufSize );
      } while( bytes_read > 0 );

      sid_md5.calculate();

      snprintf( sid_md5result, 33, "%s", sid_md5.toString().c_str() );

      return sid_md5result;

    }
};




#endif
