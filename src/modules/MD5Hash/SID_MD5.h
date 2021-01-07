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
#include "mbedtls/md5.h"


class Sid_md5
{

  public:

    static char* calcMd5(fs::File &file)
    {

      static char md5result[33];
      mbedtls_md5_context _ctx;

      uint8_t i;
      uint8_t * _buf = (uint8_t*)malloc(16);

      if(_buf == NULL) {
        log_e("Error can't malloc 16 bytes for md5 hashing");
        return md5result;
      }

      int len = file.size();
      file.seek(0);

      memset(_buf, 0x00, 16);
      mbedtls_md5_init(&_ctx);
      mbedtls_md5_starts(&_ctx);

      size_t bufSize = len > 4096 ? 4096 : len;
      uint8_t *fbuf = (uint8_t*)malloc(bufSize+1);
      size_t bytes_read = file.read( fbuf, bufSize );

      do {
        len -= bytes_read;
        if( bufSize > len ) bufSize = len;
        mbedtls_md5_update(&_ctx, (const uint8_t *)fbuf, bytes_read );
        if( len == 0 ) break;
        bytes_read = file.read( fbuf, bufSize );
      } while( bytes_read > 0 );

      mbedtls_md5_finish(&_ctx, _buf);

      for(i = 0; i < 16; i++) {
        sprintf(md5result + (i * 2), "%02x", _buf[i]);
      }

      md5result[32] = 0;

      free(_buf);
      free(fbuf);

      return md5result;
    }

};

#endif
