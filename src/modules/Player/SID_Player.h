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
#include "SID6581.h"

#ifndef NO_HVSC_INDEXER
#include "../HVSC/SID_HVSC_Indexer.h"
#endif

#include "../MOS/MOS_CPU_Controls.h"
#include "SID_Track.h"

/*
#ifndef SID_CPU_CORE
  #define SID_CPU_CORE SID_QUEUE_CORE
#else
  #warning "User defined CPU CORE"
#endif
*/
/*
#ifndef SID_PLAYER_CORE
  #define SID_PLAYER_CORE 1-SID_QUEUE_CORE
#else
  #warning "User defined PLAYER CORE"
#endif
*/
#ifndef SID_CPU_TASK_PRIORITY
  #define SID_CPU_TASK_PRIORITY 5
#else
  #warning "User defined CPU TASK PRIORITY"
#endif

#ifndef SID_PLAYER_TASK_PRIORITY
  #define SID_PLAYER_TASK_PRIORITY 0
#else
  #warning "User defined PLAYER TASK PRIORITY"
#endif


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

    void reset();
    void kill();

    loopmode   currentloopmode = SID_LOOP_OFF;
    playmode   currentplaymode = SID_ALL_SONGS;
    //SID_Meta_t *currenttrack = nullptr; // the current track
    SIDTune    *currenttune  = nullptr;
    fs::FS     *fs;
    Sid_md5    md5; // for calculating the hash of SID Files on the filesystem

    uint8_t  subsongs;
    uint8_t  startsong;
    uint8_t  currentsong;

    uint32_t default_song_duration = 180000;
    //uint32_t song_duration;
    //uint32_t delta_song_duration = 0;
    //uint32_t start_time = 0;
    //uint32_t pause_duration = 0;

    bool playerrunning = false;
    bool paused = false;
    bool begun = false;

    //void loadSID( SID_Meta_t* song ) { currenttrack = song; };
    //void loadFile( const char* filename );
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

    //SID_Meta_t* getInfoFromSIDFile( const char * path );
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
    //uint8_t  published[32];
    //uint8_t  author[32];
    //uint8_t  name[32];
    uint8_t  data_offset;

    bool TunePlayerTaskRunning = false;
    bool TunePlayerTaskPlaying = false;
    bool ExitTunePlayerLoop    = false;
    bool LoopPlayerTaskRunning = false;
    bool is_error = false;


};



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

    int      speedsong[255];
    //int      nRefreshCIAbase; // what is it used for ??
    uint16_t init_addr;
    uint16_t play_addr;
    uint16_t load_addr;

    uint8_t  sidtype[5]; // file format
    uint8_t  data_offset;

    uint8_t  currentsong; // zero-indexed song position
    uint32_t duration;
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



static void printspan( const char*title, const char*val )
{
  Serial.printf("│ %-18s%-42s │\n", title, val );
}

static void printspan( const char*t1, const char*v1, const char*t2, const char*v2 )
{
  Serial.printf("│ %-18s%-12s%-18s%-12s │\n", t1, v1, t2, v2 );
}


//80 ────────────────────────────────────────────────────────────────────────────────
//60 ────────────────────────────────────────────────────────────

static void printspanln( bool openclose=false, bool is_hr=false )
{
  static bool opening = true;
  const char* fillLine = "────────────────────────────────────────────────────────────"; // std::string(80, 0xc4).c_str(); // 0xc4 = horizontal dash
  if( openclose ) {
    if( opening  ) {
      Serial.printf("┌─%s─┐\n", fillLine);
    } else {
      Serial.printf("└─%s─┘\n", fillLine);
    }
    opening = ! opening;
  } else {
    if( is_hr ) {
      Serial.printf("├─%s─┤\n", fillLine);
    } else {
      printspan("", "", "", "");
    }
  }
}


static char toHexBuf[255] = {0};

static const char* hexString16( uint16_t hexval )
{
  snprintf(toHexBuf, 255, "$%04x", hexval );
  return toHexBuf;
}

static const char* hexString32( uint32_t hexval )
{
  snprintf(toHexBuf, 255, "$%08x", hexval );
  return toHexBuf;
}

__attribute__((unused))
static void tunedebug( SIDTune* tune )
{
  printspanln(true);
  Serial.printf("│ %-60s │\n", (const char*)tune->trackinfo->filename );
  printspanln(false, true);
  printspanln();
  printspan("  Title", (const char*)tune->trackinfo->name );
  printspan("  Author", (const char*)tune->trackinfo->author );
  printspan("  Released", (const char*)tune->trackinfo->published );
  printspanln();
  printspan("  Load Address", hexString16(tune->load_addr),        "Number of tunes", String(tune->trackinfo->subsongs).c_str() );
  printspan("  Init Address", hexString16(tune->init_addr),        "Default tune", String(tune->trackinfo->startsong).c_str() );
  printspan("  Play Address", hexString16(tune->play_addr),        "Speed", hexString32(tune->speed) );
  printspan("  RelocStartPage", hexString16(tune->relocStartPage), "RelocPages" , String(tune->relocPages).c_str() );
  printspanln();
  printspan("  SID Model", tune->getSidModelStr(),                 "Clock", tune->getClockStr() ); // TODO: name model (1=6581, 2=8580) and clock (1=PAL, 2=NTSC)
  printspanln();
  printspan("  File Format", (const char*)tune->sidtype,           "BASIC", tune->musPlay?"true":"false" );
  printspan("  Format Version", String(tune->version).c_str(),     "PlaySID Specific", tune->psidSpecific?"true":"false" );
  printspanln();
  printspan("  MD5", (const char*)tune->trackinfo->md5 );
  printspanln();
  printspanln(false, true);
  //printspan("  Default Duration (ms)", String(tune->default_song_duration).c_str() );
  //Serial.printf("│ %-60s │\n", "Speed/Duration" );
  //printspan("  Durations", "" );

  // │ ──────────────────────────────────────────────────────────── │
  // │   Song#           Speed       Duration                       │
  // │ ──────────────────────────────────────────────────────────── │
  // │ #0                0            6mn 08s                       │


  if( tune->trackinfo->subsongs>0 ) {
    size_t found = 0;
    printspan("  Song#", "Speed", "Duration", "" );
    Serial.println("│ ──────────────────────────────────────────────────────────── │");
    //printspan("  ─────", "─────", "────────", "" );
    for( int i=0; i < tune->trackinfo->subsongs; i++ ) {
      //String coltitle1 = "    Song#" + String(i) + " ";
      //String coltitle2 = "Duration";
      char songduration[32] = {'0', 0 };
      if( tune->has_durations && tune->trackinfo->durations != nullptr ) {
        if( tune->trackinfo->durations[i] > 0 ) {
          uint16_t mm,ss,SSS;
          SSS = tune->trackinfo->durations[i]%1000;
          ss  = (tune->trackinfo->durations[i]/1000)%60;
          mm  = (tune->trackinfo->durations[i]/60000);
          if( SSS == 0 ) {
            snprintf(songduration, 32, "%2dmn %02ds", mm, ss );
          } else {
            snprintf(songduration, 32, "%2dmn %02d.%ds", mm, ss, SSS );
          }
        }
      }
      printspan( String("  #"+String(i)).c_str(), String(tune->speedsong[i]).c_str(), (const char*)songduration, "" );
    }
  }
  printspanln();
  printspanln(true);
}


#endif // _SID_PLAYER_H_
