/*\


  MD5 File indexer for SID Collection (e.g. HVSC)
  The purpose of this indexer is to speed up lookups
  by pre-indexing the file zones by path names.

  MIT License

  Copyright (c) 2020 tobozo

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

\*/


#ifndef _SID_MD5_FILE_INDEXER_H_
#define _SID_MD5_FILE_INDEXER_H_

// depends on SID651.h ( songtruct )
#include "SID6581.h"


static String gnu_basename( String path ) {
  char *base = strrchr(path.c_str(), '/');
  return base ? String( base+1) : path;
}


struct MD5FileConfig
{
  fs::FS *fs;
  const char* sidfolder;   // /sid"
  const char* md5filepath; // /md5/Songlengths.full.md5
  const char* md5idxpath;  // /md5/Songlengths.full.md5.idx
  const char* md5url;      // https://www.prg.dtu.dk/HVSC/C64Music/DOCUMENTS/Songlengths.md5
};


struct FileIndex
{
  size_t  offset  = 0;
  uint8_t pathlen = 0;
  char*   path    = nullptr;
};


struct BufferedIndex
{
  fs::FS *fs;
  fs::File indexFile;
  size_t outBufferSize = 4096;
  size_t bufferPos = 0;
  char*  outBuffer = nullptr;
  FileIndex *IndexItem = nullptr;

  bool open( fs::FS *_fs, const char *name, bool readonly=true )
  {
    fs = _fs;
    if( readonly ) {
      indexFile = fs->open( name );
      if(! indexFile ) {
        log_e("Unable to access file %s", name );
        return false;
      }
      log_d("Opened %s as readonly", name );
    } else {
      indexFile = fs->open( name, FILE_WRITE );
      if(! indexFile ) {
        log_e("Unable to create file %s", name );
        return false;
      }
      log_d("Opened %s as r/w", name );
    }
    if( outBuffer == nullptr ) {
      log_d("Allocating buffer");
      outBuffer = (char*)sid_calloc( outBufferSize, sizeof(char) );
      if( outBuffer == NULL ) {
        log_e("Failed to allocate %d bytes", outBufferSize );
        return false;
      }
    }
    if( IndexItem == nullptr ) {
      log_d("Allocating item");
      IndexItem = (FileIndex*)sid_calloc( 1, sizeof( IndexItem )+1 );
      if( IndexItem == NULL ) {
        log_e("Failed to allocate %d bytes", sizeof( IndexItem )+1 );
        return false;
      }
      IndexItem->path = (char*)sid_calloc( 1, 256 );
      if( IndexItem->path == NULL ) {
        log_e("Failed to allocate %d bytes", 256 );
        return false;
      }
    }
    bufferPos = 0;
    return true;
  }

  void close()
  {
    if( bufferPos > 0 ) {
      writeIndexBuffer();
    }
    if( indexFile) {
      indexFile.close();
    }
    if( outBuffer != nullptr ) {
      log_w("Clearing outBuffer");
      free( outBuffer );
      outBuffer = nullptr;
    }
    log_w("BufferedIndexWriter closed!");
  }

  void addItem( size_t offset, const char *folderName )
  {
    IndexItem->offset = offset;
    IndexItem->pathlen = strlen( folderName ) + 1;
    snprintf( IndexItem->path, 256, "%s", folderName );
    write();
  }

  void write()
  {
    size_t objsize = sizeof( FileIndex ) + strlen( IndexItem->path ) + 1;
    if( bufferPos + objsize > outBufferSize ) {
      writeIndexBuffer();
    }
    memcpy( outBuffer+bufferPos,                                &IndexItem->offset,  sizeof(size_t) );
    memcpy( outBuffer+bufferPos+sizeof(size_t),                 &IndexItem->pathlen, sizeof(uint8_t) );
    memcpy( outBuffer+bufferPos+sizeof(size_t)+sizeof(uint8_t), IndexItem->path,     IndexItem->pathlen );

    bufferPos += sizeof(size_t)+sizeof(uint8_t)+IndexItem->pathlen;
    debugItem();
  }

  void writeIndexBuffer()
  {
    log_w("Writing %d bytes from buffer", bufferPos );
    indexFile.write( (uint8_t*)outBuffer, bufferPos );
    memset( outBuffer, 0, outBufferSize );
    bufferPos = 0;
  }

  void debugItem()
  {
    log_d("[Heap:%6d][Buff pos:%d][Offset: %d][objsize:%3d][pathlen:%03d] %s",
      ESP.getFreeHeap(),
      bufferPos,
      IndexItem->offset,
      sizeof( FileIndex ) + strlen( IndexItem->path ) + 1,
      IndexItem->pathlen,
      IndexItem->path
    );
  }

  int64_t find( fs::FS *_fs, const char* haystack, const char* needle )
  {
    if( !open( _fs, haystack ) ) return -1;
    while( indexFile.available() ) {
      indexFile.readBytes( (char*)&IndexItem->offset, sizeof( size_t ) );
      IndexItem->pathlen = indexFile.read();
      indexFile.readBytes( (char*)IndexItem->path, IndexItem->pathlen );
      IndexItem->path[IndexItem->pathlen] = '\0';
      if( strcmp( needle, IndexItem->path ) == 0 ) {
        return IndexItem->offset;
      } // else debugItem();
    }
    close();
    return -1;
  }

  bool build( fs::FS *_fs, const char* md5Path, const char* idxpath )
  {
    fs::File md5File = _fs->open( md5Path );
    if( ! md5File ) {
      log_e("Could not open md5 file");
      return false;
    }

    size_t md5Size = md5File.size();
    String buffer;
    buffer.reserve(512);

    size_t bfrsize = 256;

    char *folderName = (char*)sid_calloc( bfrsize, sizeof( char ) );
    char *tmpName    = (char*)sid_calloc( bfrsize, sizeof( char ) );
    char *bufferstr  = (char*)sid_calloc( bfrsize, sizeof( char ) );

    if( folderName == NULL || tmpName == NULL || bufferstr == NULL ) {
      log_e("Unable to alloc %d bytes form MD5Indexer, aborting", bfrsize*3 );
      return false;
    }

    size_t foldersCount = 0;

    if( !open( _fs, idxpath, false ) ) return false; // can't create index file

    while( md5File.available() ) {
      size_t offset = md5File.position();
      buffer = md5File.readStringUntil('\n');
      if( buffer.length() <2 ) break; // EOF
      if( buffer.c_str()[0] != ';' ) continue; // md5 string
      memset( bufferstr, 0, bfrsize );
      snprintf( bufferstr, bfrsize, "%s", buffer.c_str() );
      size_t buflen = strlen( bufferstr );
      memmove( bufferstr, bufferstr+2, buflen ); // remove leading "; "
      String basename = gnu_basename( bufferstr ); // extract filename
      size_t flen = (buflen-2)-basename.length(); // get foldername length
      memset( tmpName, 0, bfrsize );
      memcpy( tmpName, bufferstr, flen );
      if( strcmp( tmpName, folderName )!=0 ) {
        memset( folderName, 0, strlen( folderName) );
        snprintf( folderName, bfrsize, "%s", tmpName );
        //Serial.printf("[Offset: %d] %s\n", offset, folderName );
        foldersCount++;
        addItem( offset, folderName );
      }
    }
    md5File.close();
    close();

    free( folderName );
    free( tmpName    );
    free( bufferstr  );

    Serial.printf("Total folders: %d\n", foldersCount );
    return true;
  }

};



struct MD5FileParser
{

  MD5FileConfig *cfg;
  BufferedIndex MD5Index;

  MD5FileParser( MD5FileConfig *config ) : cfg( config) {
    if( ! cfg->fs->exists( cfg->md5idxpath ) ) {
      Serial.printf("Indexing MD5 file %s, this is a one-time operation\n", cfg->md5idxpath );
      if( ! MD5Index.build( cfg->fs, cfg->md5filepath, cfg->md5idxpath ) ) {
        log_e("Fatal error, aborting");
      }
    }
  }

  void getDurationsFromSIDPath( songstruct *song ) {

    if( ! cfg->fs->exists( cfg->md5idxpath ) ) {
      log_e("Fatal error, aborting");
    }

    auto start_seek = millis();

    const char* track = (const char*)&song->filename;
    char MD5FolderPath[255] = {0};
    char MD5FilePath[255]   = {0};

    memcpy( MD5FilePath, track+strlen(cfg->sidfolder), strlen(track)+1 ); //
    memcpy( MD5FolderPath, track+strlen(cfg->sidfolder), strlen(track)+1 ); // remove SID_FOLDER from 'dirty' path
    MD5FolderPath[strlen(MD5FolderPath)-gnu_basename(MD5FolderPath).length()] = '\0'; // remove filename from path (truncate)

    int64_t respoffset =  MD5Index.find( cfg->fs, cfg->md5idxpath, MD5FolderPath);

    if( respoffset > -1 ) {
      log_d("Offset for %s is %d", MD5FolderPath, int(respoffset) );

      fs::File MD5File = cfg->fs->open( cfg->md5filepath );
      MD5File.seek( respoffset );
      bool found = false;
      String fname, md5line;
      String md5needle = String("; ") + String( MD5FolderPath );
      String sidneedle = String("; ") + String( MD5FilePath );

      while( MD5File.available() ) {
        fname  = MD5File.readStringUntil('\n'); // read filename
        md5line = MD5File.readStringUntil('\n'); // read md5 hash and song lengths
        if( fname[0]!=';') {
          log_w("Invalid read at offset %d while md5needle=%s, sidneedle=%s, fname=%s, md5line=%s", MD5File.position(), md5needle.c_str(), sidneedle.c_str(), fname.c_str(), md5line.c_str() );
          break; // something wrong, not a valid offset
        }
        if( !fname.startsWith( md5needle ) ) {
          log_w("Broke out of folder at offset %d while md5needle=%s, sidneedle=%s, fname=%s, md5line=%s", MD5File.position(), md5needle.c_str(), sidneedle.c_str(), fname.c_str(), md5line.c_str() );
          break; // out of folder
        }
        if( fname.startsWith( sidneedle ) ) {
          // found !
          found = true;
          break;
        } else {
          log_d("[SKIPPING] [%s]!=[%s], md5needle=%s, md5line=%s", fname.c_str(), sidneedle.c_str(), md5needle.c_str(), md5line.c_str() );
        }
      }
      if( found ) {
        log_d("Found hash for file %s ( %s => %s ) : %s", track, MD5FilePath, MD5FolderPath, md5line.c_str() );
        if( song->durations != nullptr ) free( song->durations );
        song->durations = (uint32_t*)sid_calloc( song->subsongs, sizeof(uint32_t) );
        int subsongs = getDurationsFromMd5String( md5line, song );
        if( subsongs == song->subsongs ) {
          for( int i=0; i<subsongs; i++ ) {
            log_d("Subsong #%d: %d millis", i, int( song->durations[i] ) );
          }
        } else {
          log_e("Subsongs count mismatch %d vs %d", subsongs, song->subsongs );
        }
      } else {
        log_w("Hash not found for %s", MD5FilePath );
      }
    } else {
      log_w("Offset not found for %s", MD5FolderPath );
    }
    log_d("Seeking md5 hash info took %d millis", int( millis()-start_seek ) );
  }


  int getDurationsFromMd5String( String md5line, songstruct *song )
  {
    if( !md5line.endsWith(" ") ) md5line += " "; // append a space as a terminator
    const char* md5linecstr = md5line.c_str();
    size_t lineLen = strlen( md5linecstr );
    size_t durationsSize = song->subsongs;
    if( durationsSize == 0 ) {
      log_e("Invalid song has zero subsongs, aborting");
      return -1;
    }
    memset( song->durations, 0, durationsSize ); // blank the array
    int res = md5line.indexOf('='); // where do the timings start in the string ?
    if (res == -1 || res >= lineLen ) { // the md5 file is malformed or an i/o error occured
      log_e("[ERROR] bad md5 line: %s", md5linecstr );
      return -1;
    }
    log_d("Scanning string (%d bytes)", lineLen);

    char parsedTime[11]     = {0}; // for storing mm.ss.SSS
    uint8_t parsedTimeCount = 0; // how many durations/subsongs
    uint8_t parsedIndex     = 0; // substring parser index
    for( int i=res; i<lineLen; i++ ) {
      //char parsedChar = md5linecstr[i];
      switch( md5linecstr[i] ) {
        case '=': // durations begin!
          parsedIndex = 0;
        break;
        case ' ': // next or last duration
          if( parsedIndex > 0 ) {
            parsedTime[parsedIndex+1] = '\0'; // null terminate
            int mm,ss,SSS; // for storing the extracted time values
            sscanf( parsedTime, "%d:%d.%d", &mm, &ss, &SSS );
            song->durations[parsedTimeCount] = (mm*60*1000)+(ss*1000)+SSS;
            log_d("Subsong #%d: %s (mm:ss.SSS) / %d (ms)\n", parsedTimeCount, parsedTime, song->durations[parsedTimeCount] );
            parsedTimeCount++;
          }
          parsedIndex = 0;
        break;
        default:
          if( parsedIndex > 10 ) { // "mm.ss.SSS" pattern can't be more than 10 chars, something went wrong
            log_e("Substring index overflow subsong #%d items", parsedTimeCount );
            parsedIndex = 0;
            goto _end; // break out of switch and parent loop
          }
          parsedTime[parsedIndex] = md5linecstr[i];
          parsedIndex++;
      }
      if( parsedTimeCount >= durationsSize ) { // prevent stack smashing
        log_d("Found enough subsongs (max=%d), stopping at index %d", durationsSize, parsedTimeCount-1);
        //totalTruncated++;
        break;
      }
    }
    if( parsedTimeCount > 0 ) {
      log_d("Found %d subsong length(s)", parsedTimeCount );
    }
    _end:
    return parsedTimeCount;
  }

};

#endif
