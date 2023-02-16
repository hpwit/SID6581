/*\


  MD5 File indexer for High Voltage SID Collection (HVSC).

  HVSC MD5 file contains the song lengths and a MD5 digest for every SID file
  in the collection.

  The purpose of this indexer is to speed up lookups by pre-indexing the file
  zones by path names.

  Secondary lookup indexes can also be built for MD5 digest search, when the
  paths don't match but the MD5 digest does.

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


#ifndef _SID_HVSC_FILE_INDEXER_H_
#define _SID_HVSC_FILE_INDEXER_H_

// depends on SID651.h ( songtruct )
//#include "SID6581.h"
#include "../Player/SID_Track.h"


// MD5 file lookup methods
typedef enum
{
  MD5_RAW_READ,      // parse the whole md5 file if necessary, very slow but fit for curated MD5 File and SPIFFS
  MD5_INDEX_LOOKUP,  // SD REQUIRED: use an alphabetical index for faster seek in md5 file, fastest
  MD5_RAINBOW_LOOKUP // SD REQUIRED: reverse md5 lookup files, useful for custom playlists
} MD5LookupMethod;


// HVSC download/unpack info
typedef struct
{
  const char* name; // example: "HVSC 74", fancy name for the archive, can't be empty
  const char* path; // example: "HVSC-74", unique name ([a-z0-9_-]+) for dotfile creation, used to mark the archive as successfully unpacked
} MD5Archive;


// overall config holder
typedef struct
{
  fs::FS *fs;              // example: SD strongly recommended
  MD5Archive* archive;     // HVSC Archive previously declared
  const char* sidfolder;   // example: /C64Music
  const char* md5folder;   // example: /md5 (for storing spreaded MD5 files)
  const char* md5filepath; // example: /C64Music/DOCUMENTS/Songlengths.full.md5
  const char* md5idxpath;  // example: /md5/Songlengths.full.md5.idx
  MD5LookupMethod lookupMethod; // preferred lookup method although not exclusive
  void (*progressCb)( size_t progress, size_t total ); // callback function for init, this may be handy since the operation can be very long
} MD5FileConfig;


// items in the BufferedIndex storage
typedef struct
{
  size_t  offset  = 0; // real offset in the MD5 File
  uint8_t pathlen = 0; // bytes to seek() before the next item in the buffer
  char*   path    = nullptr;
} fileIndex_t;


#ifdef BOARD_HAS_PSRAM

typedef struct
{
  const char*    name = nullptr;
  size_t         size = 0;
  uint8_t*       data = nullptr;

} RamDiskItem;

#endif


// creates and stores an index of "folder name" => position
// in the md5file for faster "search by path" lookups
// only useful with HVSC folder structures
class BufferedIndex
{

  public:

    BufferedIndex( fs::FS *_fs );

    void (*progressCb)( size_t progress, size_t total ) = nullptr;

    bool    open( const char *name, bool readonly=true );
    int64_t find( const char* haystack, const char* needle );
    void    close();

    // build an index based on FILE PATH in order to provide
    // a database of [PATH => line number] in the md5 file for faster seek
    // this only applies to HVSC folder structure
    bool buildSIDPathIndex( const char* md5Path, const char* idxpath );
    // build folders structure and shard md5 hashes in different
    // files for faster lookup, only useful with custom-made
    // folders or non-HVSC SID collection
    bool buildSIDHashIndex( const char* md5Path, const char* folderpath, bool purge = false );
    // some string helper, made static so it can be shared
    static String gnu_basename( String path )
    {
      char *base = strrchr(path.c_str(), '/');
      return base ? String( base+1) : path;
    }

   #ifdef BOARD_HAS_PSRAM
      RamDiskItem ramDiskFile;
      bool openRamDiskFile( const char* name );
   #endif

  private:

    fs::FS      *fs;
    fs::File    indexFile;
    // Arduino 2.0.0-alpha brutally changed the behaviour of fs::File->name()
    const char* fs_file_path( fs::File *file ) {
      #if defined ESP_IDF_VERSION_MAJOR && ESP_IDF_VERSION_MAJOR >= 4
        return file->path();
      #else
        return file->name();
      #endif
    }

    char        *outBuffer = nullptr;
    char        *lastSearch = nullptr;
    fileIndex_t *IndexItem = nullptr;
    size_t      outBufferSize = 4096;
    size_t      bufferPos = 0;
    size_t      lastOffset = -1;

    void addItem( size_t offset, const char *folderName );
    void indexItemWords( size_t offset, const char *folderName );
    void write();
    void writeIndexBuffer();
    void debugItem();

};



struct MD5FileParser
{
  public:

    BufferedIndex *MD5Index = nullptr;

    MD5FileParser( MD5FileConfig *config );

    bool reset();
    bool getDurations( SID_Meta_t *song);
    int64_t getOffsetFromSIDPath( SID_Meta_t *song, bool populate = false );
    void progressCb( size_t progress, size_t total ) { if( cfg && cfg->progressCb ) cfg->progressCb(progress, total); };

  private:

    MD5FileConfig *cfg      = nullptr;

    // fastest lookup for HVSC
    bool getDurationsFromSIDPath( SID_Meta_t *song );
    // faster lookup for custom playlists
    bool getDurationsFastFromMD5Hash( SID_Meta_t *song );
    // sluggish as hell, no hash lookup, legacy failsafe method
    bool getDurationsFromMD5Hash( SID_Meta_t *song );
    // helper for SongLengths entry parser
    int  getDurationsFromMd5String( String md5line, SID_Meta_t *song );

};

#endif
