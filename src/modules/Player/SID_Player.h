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

#ifndef SID_CPU_CORE
  #define SID_CPU_CORE 0
#endif
#ifndef SID_PLAYER_CORE
  #define SID_PLAYER_CORE 1
#endif

#ifndef SID_CPU_TASK_PRIORITY
  #define SID_CPU_TASK_PRIORITY 8
#endif


enum sidEvent
{
  SID_NEW_TRACK,
  SID_NEW_FILE,
  SID_START_PLAY,
  SID_END_PLAY,
  SID_PAUSE_PLAY,
  SID_RESUME_PLAY,
  SID_END_TRACK,
  SID_STOP_TRACK
};


enum loopmode
{
  SID_LOOP_ON,
  SID_LOOP_RANDOM,
  SID_LOOP_OFF,
};


enum playmode
{
  SID_ALL_SONGS,
  SID_ONE_SONG,
};





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
    SID_Meta_t *currenttrack = nullptr; // the current track
    fs::FS     *fs;
    Sid_md5    md5; // for calculating the hash of SID Files on the filesystem

    uint8_t  subsongs;
    uint8_t  startsong;
    uint8_t  currentsong;

    uint32_t default_song_duration = 180000;
    uint32_t song_duration;
    uint32_t delta_song_duration = 0;
    uint32_t start_time = 0;
    uint32_t pause_duration = 0;

    bool playerrunning = false;
    bool paused = false;

    //void loadSID( SID_Meta_t* song ) { currenttrack = song; };
    //void loadFile( const char* filename );
    void togglePause();
    void stop();

    void setMaxVolume( uint8_t volume );
    inline void setEventCallback(void (*fptr)(SIDTunesPlayer* player, sidEvent event)) { eventCallback = fptr; }

    bool playSID(); // play subsongs inside a SID file
    bool playSongNumber( int songnumber ); // play a subsong from the current SID track slot
    bool playPrevSongInSid();
    bool playNextSongInSid();
    bool isRunning(); // player looptask
    bool isPlaying(); // sid registers looptask

    bool getInfoFromSIDFile( const char * path );
    bool getInfoFromSIDFile( const char * path, SID_Meta_t *songinfo );

    void setLoopMode( loopmode mode );
    void setPlayMode( playmode mode );

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

    uint8_t getNumberOfTunesInSid();
    uint8_t getCurrentTuneInSid();
    uint8_t getDefaultTuneInSid();

  private:

    static void SIDTunePlayerTask(void * parameters); // sends song registers to SID
    static void SIDLoopPlayerTask(void *param);       // handles subsong start/end events

    bool playSidFile(); // play currently loaded SID file (and subsongs if any)
    void (*eventCallback)( SIDTunesPlayer* player, sidEvent event ) = NULL;
    void fireEvent( SIDTunesPlayer* player, sidEvent event);
    void setDefaultDuration( uint32_t duration );

    int      speedsong[32]; // TODO: actually use this
    int      nRefreshCIAbase; // what is it used for ??
    uint16_t init_addr;
    uint16_t play_addr;
    uint16_t load_addr;
    uint8_t  sidtype[5];
    //uint8_t  published[32];
    //uint8_t  author[32];
    //uint8_t  name[32];
    uint8_t  data_offset;

    bool stop_song = true;
    bool is_error = false;


};


#endif // _SID_PLAYER_H_
