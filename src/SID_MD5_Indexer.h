/*\


  MD5 File indexer for SID Collection (e.g. HVSC)
  The purpose of this indexer is to speed up lookups
  by pre-indexing the file zones by path names.
  Lookup indexes can also be built for md5 hash search.

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

// MD5 file lookup methods
enum MD5LookupMethod
{
  MD5_RAW_READ,      // parse the whole md5 file if necessary, very slow
  MD5_INDEX_LOOKUP,  // SD REQUIRED: use an alphabetical index for faster seek in md5 file, fastest
  MD5_RAINBOW_LOOKUP // SD REQUIRED: reverse md5 lookup files, useful for custom playlists
};

// HVSC download/unpack info
struct MD5Archive
{
  const char* name; // example: "HVSC 74", can't be empty
  const char* url;  // example: "https://phpsecu.re/HVSC-74.tar.gz", must be a URL to a .tar.gz file mirroring the contents of /C64Music folder in HVSC archive
  const char* path; // example: "HVSC-74", unique name ([a-z0-9_-]+) for dotfile creation, used to mark the archive as successfully unpacked
};

// overall config holder
struct MD5FileConfig
{
  fs::FS *fs;              // example: SD strongly recommended
  MD5Archive* archive;     // HVSC Archive previously declared
  const char* sidfolder;   // example: /sid
  const char* md5folder;   // example: /md5
  const char* md5filepath; // example: /md5/Songlengths.full.md5
  const char* md5idxpath;  // example: /md5/Songlengths.full.md5.idx
  const char* md5url;      // example: https://phpsecu.re/Songlengths.md5
  MD5LookupMethod lookupMethod; // preferred lookup method although not exclusive
  void (*progressCb)( size_t progress, size_t total ); // callback function for init, this may be handy since the operation can be very long
};


// items in the BufferedIndex storage
struct FileIndex
{
  size_t  offset  = 0; // real offset in the MD5 File
  uint8_t pathlen = 0; // bytes to seek() before the next item in the buffer
  char*   path    = nullptr;
};


// creates and stores an index of "folder name" => position
// in the md5file for faster "search by path" lookups
// only useful with HVSC folder structures
struct BufferedIndex
{
  fs::FS *fs;
  fs::File indexFile;
  size_t outBufferSize = 4096;
  size_t bufferPos = 0;
  char*  outBuffer = nullptr;
  FileIndex *IndexItem = nullptr;

  void (*progressCb)( size_t progress, size_t total ) = nullptr;

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
    // debugItem();
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

  // build an index based on FILE PATH in order to provide
  // a database of [PATH => line number] in the md5 file for faster seek
  // this only applies to HVSC folder structure
  bool buildSIDPathIndex( fs::FS *_fs, const char* md5Path, const char* idxpath )
  {
    fs::File md5File = _fs->open( md5Path );
    if( ! md5File ) {
      log_e("Fatal: could not open md5 file");
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

    if( progressCb != nullptr ) progressCb( 0, md5Size );

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
        foldersCount++;
        addItem( offset, folderName );
        if( progressCb != nullptr ) progressCb( offset, md5Size );
      }
    }
    if( progressCb != nullptr ) progressCb( md5Size, md5Size );
    md5File.close();
    close();

    free( folderName );
    free( tmpName    );
    free( bufferstr  );

    Serial.printf("Total folders: %d\n", foldersCount );
    return true;
  }


  // build folders structure and store md5 hashes in different
  // files for faster lookup, only useful with custom-made
  // folders or non-HVSC SID collection
  bool buildSIDHashIndex(  fs::FS *_fs, const char* md5Path, const char* folderpath, bool purge = false )
  {

    char path[255] = {0};
    sprintf( path, "%s/.HashIndexDone", folderpath );

    if( _fs->exists( path ) ) purge = true;

    if( purge ) {
      // recursively delete files
      char folders[] = "0123456789abcdef";

      String fname = gnu_basename( md5Path );
      size_t pathlen = strlen( md5Path ) - fname.length();
      memcpy( path, md5Path, pathlen );
      path[pathlen] = '\0';
      for( int i=0;i<16;i++ ) {
        char folder[255] = {0};
        char sub[2] = { folders[i], '\0' };
        sprintf( folder, "%s%s", path, sub );
        File root = _fs->open( folder );
        if(!root){
          log_d("Skipping directory %s\n", folder);
          continue;
        }
        if(!root.isDirectory()){
          log_w("%s is not a directory\b", folder);
          continue;
        }
        File file = root.openNextFile();
        while( file ) {
          log_w("Deleting %s", file.name() );
          _fs->remove( file.name() );
          file = root.openNextFile();
        }
        root.close();
        _fs->remove( folder );
      }
    }

    fs::File md5File = _fs->open( md5Path );
    if( ! md5File ) {
      log_e("Could not open md5 file");
      return false;
    }

    size_t md5Size = md5File.size();
    String buffer;
    buffer.reserve(512);

    char md5hash[32];
    char fullidxpath[255];

    if( progressCb != nullptr ) progressCb( 0, md5Size );

    while( md5File.available() ) {
      size_t offset = md5File.position();
      buffer = md5File.readStringUntil('\n');
      if( buffer.length() <2 ) break; // EOF
      if( buffer[0] == '[' ) continue; // first line
      if( progressCb != nullptr ) progressCb( offset, md5Size );
      if( buffer.c_str()[0] == ';' ) { // that's a path+filename
        // now read next line to get the md5 hash+song durations
        buffer = md5File.readStringUntil('\n');
        // store two first letters of the hash then create subfolders freebsd style
        // this will store hashes into 255 files spread across 16 folders
        // e.g. hash abcdef01abcdef01abcdef01abcdef01 will be stored in "/md5/a/ab.md5idx"
        char c1[2] = { buffer.c_str()[0], '\0' };
        char c3[3] = { buffer.c_str()[0], buffer.c_str()[1], '\0' };
        snprintf( fullidxpath, 255, "%s/%s", folderpath, c1 );
        if( !_fs->exists( fullidxpath ) ) _fs->mkdir( fullidxpath );
        snprintf( fullidxpath, 255, "%s/%s/%s.md5idx", folderpath, c1, c3 );
        fs::File md5GroupFile = _fs->open( fullidxpath, "a+" ); // append or create
        if(! md5GroupFile ) {
          log_e("Could not create/update group file %s, aborting", fullidxpath );
          break;
        }
        md5GroupFile.write( (uint8_t*)buffer.c_str(), buffer.length() ); // write md5 line+lengths
        md5GroupFile.write('\n'); // add line separator
        md5GroupFile.close();
      }
    }
    md5File.close();
    close();
    fs::File indexDoneFile;
    sprintf( path, "%s/.HashIndexDone", folderpath );
    indexDoneFile = _fs->open( path, FILE_WRITE );
    indexDoneFile.write( 64 );
    indexDoneFile.close();
    if( progressCb != nullptr ) progressCb( md5Size, md5Size );
    return true;
  }

};



struct MD5FileParser
{
  public:

    MD5FileConfig *cfg;
    BufferedIndex MD5Index;

    MD5FileParser( MD5FileConfig *config ) : cfg( config) {
      if( cfg->progressCb != nullptr ) {
        MD5Index.progressCb = cfg->progressCb;
      }

      if( !cfg->fs->exists( cfg->md5idxpath ) ) {
        Serial.printf("Now indexing MD5 paths into %s file, this is a one-time operation\n", cfg->md5idxpath );
        if( ! MD5Index.buildSIDPathIndex( cfg->fs, cfg->md5filepath, cfg->md5idxpath ) ) {
          log_e("Fatal error, aborting");
          // no need to go any further
          return;
        }
      }

      char path[255] = {0};
      sprintf( path, "%s/.HashIndexDone", cfg->md5folder );

      if( !cfg->fs->exists( path ) ) {
        // hash index is missing, rebuild it
        Serial.printf("Now spreading MD5 hashes across %s folder, this is a one-time operation\n", cfg->md5folder );
        MD5Index.buildSIDHashIndex( cfg->fs, cfg->md5filepath, cfg->md5folder, true );
      }

    }


    bool getDuration( songstruct *song)
    {
      switch( cfg->lookupMethod )
      {
        case MD5_RAW_READ       : return getDurationsFromMD5Hash( song); break;     // parse the whole md5 file if necessary, very slow
        case MD5_INDEX_LOOKUP   : return getDurationsFromSIDPath( song); break;     // SD REQUIRED: search index by SID path fastest
        case MD5_RAINBOW_LOOKUP : return getDurationsFastFromMD5Hash( song); break; // SD REQUIRED: reverse md5 lookup files, useful for custom playlists
        default: return false;
      }
    }


    bool reset()
    {
      switch( cfg->lookupMethod )
      {
        case MD5_RAW_READ       : return cfg->fs->exists( cfg->md5filepath ); break;
        case MD5_INDEX_LOOKUP   : return MD5Index.buildSIDPathIndex( cfg->fs, cfg->md5filepath, cfg->md5idxpath );      break;
        case MD5_RAINBOW_LOOKUP : return MD5Index.buildSIDHashIndex( cfg->fs, cfg->md5filepath, cfg->md5folder, true ); break;
        default: return false;
      }
    }


    // fastest lookup for HVSC
    bool getDurationsFromSIDPath( songstruct *song )
    {
      if( ! cfg->fs->exists( cfg->md5idxpath ) ) {
        log_e("MD5 Index file is missing, aborting");
        return false;
      }
      bool found = false;

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
        MD5File.close();
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
      return found;
    }


    // faster lookup for custom playlists
    bool getDurationsFastFromMD5Hash( songstruct *song )
    {
      if( strcmp( song->md5, "00000000000000000000000000000000" ) == 0 ) {
        log_e("Invalid MD5");
        return false;
      }
      char c1[2] = { song->md5[0], '\0' };
      char c3[3] = { song->md5[0], song->md5[1], '\0' };
      char fullidxpath[255];

      snprintf( fullidxpath, 255, "%s/%s/%s.md5idx", cfg->md5folder, c1, c3 );
      if( !cfg->fs->exists( fullidxpath ) ) {
        // build hash index ?
        log_e("No index exists for hash %s (expected path: %s)", song->md5, fullidxpath );
        return false;
      }
      fs::File md5GroupFile = cfg->fs->open( fullidxpath ); // readonly
      if(! md5GroupFile ) {
        log_e("Could not access group file %s, aborting", fullidxpath );
        return false;
      }
      bool found = false;
      while( md5GroupFile.available() ) {
        String md5Line = md5GroupFile.readStringUntil( '\n' );
        if( md5Line.startsWith( song->md5 ) ) {
          found = true;
          log_d("Found song lengths for hash %s => %s", song->md5, md5line.c_str() );
          if( song->durations != nullptr ) free( song->durations );
          song->durations = (uint32_t*)sid_calloc( song->subsongs, sizeof(uint32_t) );
          int subsongs = getDurationsFromMd5String( md5Line, song );
          if( subsongs == song->subsongs ) {
            for( int i=0; i<subsongs; i++ ) {
              log_d("Subsong #%d: %d millis", i, int( song->durations[i] ) );
            }
          } else {
            log_e("Subsongs count mismatch %d vs %d", subsongs, song->subsongs );
          }
          break;
        }
      }
      return found;
    }


    // sluggish as hell, no hash lookup, legacy failsafe method
    bool getDurationsFromMD5Hash( songstruct *song )
    {
      if( strcmp( song->md5, "00000000000000000000000000000000" ) == 0 ) {
        log_e("Invalid MD5");
        return false;
      }
      fs::File md5File = cfg->fs->open( cfg->md5filepath );
      if( ! md5File ) {
        log_e("Could not open md5 file");
        return false;
      }

      size_t md5Size = md5File.size();
      String buffer;
      buffer.reserve(512);

      char md5hash[32];
      char fullidxpath[255];
      bool found = false;

      while( md5File.available() ) {
        size_t offset = md5File.position();
        buffer = md5File.readStringUntil('\n');
        if( buffer.c_str()[0] == '[' ) continue; // skip header
        if( buffer.c_str()[0] == ';' ) continue; // skip filename
        if( buffer.startsWith( song->md5 ) ) {
          found = true;
          if( song->durations != nullptr ) free( song->durations );
          song->durations = (uint32_t*)sid_calloc( song->subsongs, sizeof(uint32_t) );
          int subsongs = getDurationsFromMd5String( buffer, song );
          if( subsongs == song->subsongs ) {
            for( int i=0; i<subsongs; i++ ) {
              log_d("Subsong #%d: %d millis", i, int( song->durations[i] ) );
            }
          } else {
            log_e("Subsongs count mismatch %d vs %d", subsongs, song->subsongs );
          }
          break;
        }
      }
      md5File.close();
      return found;
    }

  private:

    int getDurationsFromMd5String( String md5line, songstruct *song )
    {
      if( !md5line.endsWith(" ") ) md5line += " "; // append a space as a terminator
      const char* md5linecstr = md5line.c_str();
      size_t lineLen = strlen( md5linecstr );
      if( song->subsongs == 0 ) {
        log_e("Invalid song has zero subsongs, aborting");
        return -1;
      }
      memset( song->durations, 0, song->subsongs ); // blank the array
      int res = md5line.indexOf('='); // where do the timings start in the string ?
      if (res == -1 || res >= lineLen ) { // the md5 file is malformed or an i/o error occured
        log_e("[ERROR] bad md5 line: %s", md5linecstr );
        return -1;
      }
      log_d("Scanning string (%d bytes)", lineLen);

      char parsedTime[11]     = {0}; // for storing mm.ss.SSS
      uint8_t parsedTimeCount = 0;   // how many durations/subsongs
      uint8_t parsedIndex     = 0;   // substring parser index
      for( int i=res; i<lineLen; i++ ) {
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
        if( parsedTimeCount >= song->subsongs ) { // prevent stack smashing
          log_d("Found enough subsongs (max=%d), stopping at index %d", song->subsongs, parsedTimeCount-1);
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