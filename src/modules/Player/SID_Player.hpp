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

#ifndef _SID_PLAYER_H_
#define _SID_PLAYER_H_

#include <FS.h>
#include "SID6581.hpp"

#ifndef NO_HVSC_INDEXER
#include "../HVSC/SID_HVSC_Indexer.hpp"
#endif

#ifndef NO_DEEPSID
#include "../Deepsid/Deepsid.hpp"
#endif

#include "../MOS/MOS_CPU_Controls.hpp"
#include "SID_Track.h"


enum sidEvent
{
  SID_NEW_FILE,
  SID_END_FILE,
  SID_NEW_TRACK,
  SID_END_TRACK,
  SID_STOP_TRACK,
  SID_START_PLAY,
  SID_END_PLAY,
  SID_PAUSE_PLAY,
  SID_RESUME_PLAY
};

// applies to one SID
enum loopmode
{
  SID_LOOP_ON,
  SID_LOOP_RANDOM,
  SID_LOOP_OFF,
};

// applies to one SID
enum playmode
{
  SID_ALL_SONGS,
  SID_ONE_SONG,
};


class SIDTune;


class SIDTunesPlayer : public MOS_CPU_Controls
{
  public:

    ~SIDTunesPlayer();
    SIDTunesPlayer( fs::FS *_fs );
    #ifdef _SID_HVSC_FILE_INDEXER_H_
      MD5FileParser *MD5Parser = NULL; // for parsing the MD5 Database file
      SIDTunesPlayer( MD5FileConfig *cfg );
      void setMD5Parser( MD5FileConfig *cfg );
    #endif

    bool begin(int clock_pin,int data_pin, int latch );
    bool begin(int clock_pin,int data_pin, int latch,int sid_clock_pin);
    void beginTask();

    BaseType_t  SID_AUDIO_CORE  = 0; // task sending SID cpu instructions
    BaseType_t  SID_PLAYER_CORE = 0; // task handling play/pause controls

    UBaseType_t SID_AUDIO_PRIORITY  = 5;
    UBaseType_t SID_PLAYER_PRIORITY = 1;

    void reset();
    void kill();

    loopmode   currentloopmode = SID_LOOP_OFF;
    playmode   currentplaymode = SID_ALL_SONGS;

    SIDTune    *currenttune  = nullptr;
    fs::FS     *fs;
    Sid_md5    md5; // for calculating the hash of SID Files on the filesystem

    uint8_t  subsongs;
    uint8_t  startsong;
    uint8_t  currentsong;

    uint32_t default_song_duration = 180000;

    bool playerrunning = false;
    bool paused = false;
    bool begun = false;

    void togglePause();
    void stop();

    void setMaxVolume( uint8_t volume );
    inline void setEventCallback(void (*fptr)(SIDTunesPlayer* player, sidEvent event)) { eventCallback = fptr; }

    bool playSID(); // play subsongs inside a SID file
    bool playSID(SID_Meta_t *songinfo );
    bool playSongNumber( int songnumber ); // play a subsong from the current SID track slot
    bool playPrevSongInSid();
    bool playNextSongInSid();
    bool isRunning(); // player looptask (does NOT imply a track is being played)
    bool isPlaying(); // sid registers looptask (returns true if a track is being played)

    bool getInfoFromSIDFile( const char * path, SID_Meta_t *songinfo );

    void setLoopMode( loopmode mode );
    void setPlayMode( playmode mode );
    void setDefaultDuration( uint32_t duration );

    loopmode getLoopMode();
    playmode getPlayMode();

    uint32_t getElapseTime();
    uint32_t getCurrentTrackDuration();
    uint32_t getDefaultDuration();

    char    *getName();
    char    *getPublished();
    char    *getAuthor();
    char    *getFilename();
    uint8_t *getSidType();

    uint8_t getSongsCount();
    uint8_t getCurrentSong();
    uint8_t getStartSong();

    uint8_t getMaxVolume();

  private:

    static void SIDTunePlayerTask(void *parameters); // sends song registers to SID
    static void SIDLoopPlayerTask(void *param);       // handles subsong start/end events

    bool playSidFile(); // play currently loaded SID file (and subsongs if any)
    void (*eventCallback)( SIDTunesPlayer* player, sidEvent event ) = NULL;
    void fireEvent( SIDTunesPlayer* player, sidEvent event);

    int      speedsong[255]; // TODO: actually use this
    int      nRefreshCIAbase; // what is it used for ??
    uint16_t init_addr;
    uint16_t play_addr;
    uint16_t load_addr;
    uint8_t  sidtype[5]; // adding an extra byte for debug
    uint8_t  data_offset;

    bool TunePlayerTaskRunning = false;
    bool TunePlayerTaskPlaying = false;
    bool ExitTunePlayerLoop    = false;
    bool LoopPlayerTaskRunning = false;
    bool is_error = false;


};


void printspan4( const char*t1, const char*v1, const char*t2, const char*v2 );
void printspan2( const char*title, const char*val );
void printspanln( bool openclose=false, bool is_hr=false );
const char* hexString16( uint16_t hexval, char *buf );
const char* hexString32( uint32_t hexval, char *buf  );


class SIDTune
{
  public:

    /*

    Title            Proteus
    Author           Rob Hubbard
    Released         1986 Firebird

    Load Address     $0820       Number of tunes    1
    Init Address     $0820       Default tune       1
    Play Address     $0826       Speed              $00000000

    SID Model        6581        Clock              PAL

    File Format      PSID        BASIC              false
    Format Version   2           PlaySID Specific   false

    Free Pages       Auto Detected
    MD5              6aed23276526c45dc0901f9070847015

    */

    SIDTune( SIDTunesPlayer *_player );
    SIDTune( SIDTunesPlayer *_player, const char* _path );
    SIDTune( SIDTunesPlayer *_player, const char* _path, SID_Meta_t *_trackinfo );
    ~SIDTune();

    const char     *path;
    SIDTunesPlayer *player;
    SID_Meta_t     *trackinfo;

    static void tunedebug( SIDTune* tune );

    int      speedsong[255];
    //int      nRefreshCIAbase; // what is it used for ??
    uint16_t init_addr;
    uint16_t play_addr;
    uint16_t load_addr;

    uint8_t  sidtype[5]; // file format
    uint8_t  data_offset;

    uint8_t  currentsong; // zero-indexed song position
    uint32_t duration; // song duration
    uint32_t speed;

    uint8_t clock  = 0x00;
    uint8_t sidModel = 0x00;
    uint8_t relocStartPage;
    uint8_t relocPages;
    uint16_t SIDModel[3];
    uint16_t version;

    bool musPlay      = false;
    bool psidSpecific = false;

    uint32_t default_song_duration = 180000;

    uint32_t delta_song_duration = 0;
    uint32_t start_time = 0;
    uint32_t pause_duration = 0;

    bool loaded = false; // enabled by loadFromFile()
    bool has_durations = false;

    bool getMetaFromFile( fs::File *file, bool close=true ); // same as loadFromFile without loading the songs into mem buffer
    bool loadFromFile( fs::File *file, bool close=true );
    bool loadFromFile( fs::FS *fs );

    // some getters
    uint32_t getDefaultDuration() { return default_song_duration; }
    uint32_t getElapseTime() {      return delta_song_duration; }
    uint32_t getDuration() {        return duration; }

    uint8_t* getSidType() {   return (uint8_t*)sidtype; }
    char*    getName() {      return (char*)trackinfo->name; }
    char*    getPublished() { return (char*)trackinfo->published; }
    char*    getAuthor() {    return (char*)trackinfo->author; }
    char*    getFilename() {  return (char*)trackinfo->filename; }

    uint8_t  getSongsCount() {  return trackinfo->subsongs; }
    uint8_t  getStartSong() {   return trackinfo->startsong; }
    uint8_t  getCurrentSong() { return currentsong; }


    const char* getSidModelStr();
    const char* getClockStr();

    // TODO: name model (1=6581, 2=8580) and clock (1=PAL, 2=NTSC)

  private:
    bool needs_free = false; // free trackinfo on destruct
    const char* fs_file_path( fs::File *file ) {
      #if defined ESP_IDF_VERSION_MAJOR && ESP_IDF_VERSION_MAJOR >= 4
        return file->path();
      #else
        return file->name();
      #endif
    }


};// SID_Tune;





#endif // _SID_PLAYER_H_
