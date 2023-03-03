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

#include "SID_Player.hpp"

// task lockers
static TaskHandle_t xPausePlayTaskLocker = NULL;
static TaskHandle_t xTunePlayerTaskLocker = NULL;
static TaskHandle_t xPlayerLoopTaskLocker = NULL;
// task handles
static TaskHandle_t xTunePlayerTaskHandle = NULL;
//static TaskHandle_t xPlayerLoopTaskHandle = NULL;

static xSemaphoreHandle mux = NULL;
#define takeMuxSemaphore() if( mux ) { xSemaphoreTake(mux, portMAX_DELAY); log_v("Took Semaphore"); }
#define giveMuxSemaphore() if( mux ) { xSemaphoreGive(mux); log_v("Gave Semaphore"); }


// ******************************************
// Class controls


SIDTunesPlayer::~SIDTunesPlayer()
{
  //freeListsongsMem();
  if( mem!=NULL ) free( mem );
  #ifdef _SID_HVSC_FILE_INDEXER_H_
  if( MD5Parser != NULL ) delete( MD5Parser );
  #endif
  //TODO: free/delete more stuff
  //currenttrack = (SID_Meta_t*)sid_calloc( 1, sizeof( SID_Meta_t* ) );
  // if( currenttrack != nullptr ) {
  //   if( currenttrack->durations != nullptr ) free( currenttrack->durations );
  //   free( currenttrack );
  // }
  if( currenttune != nullptr ) delete currenttune;
}


SIDTunesPlayer::SIDTunesPlayer( fs::FS *_fs ) : fs(_fs)
{
  // init(); // dafuq is this ?
  #ifdef BOARD_HAS_PSRAM
    psramInit();
  #endif
  mux = xSemaphoreCreateMutex();
  //currenttrack = (SID_Meta_t*)sid_calloc( 1, sizeof( SID_Meta_t ) );
  //currenttrack->durations = nullptr;
  volume = 15;
  if( mem == NULL ) {
    log_i("Creating C64 memory buffer (%d bytes)", 0x10000);
    //
    mem = (uint8_t*)heap_caps_malloc( 0x10000, MALLOC_CAP_8BIT );
    //mem = (uint8_t*)sid_calloc( 0x10000, 1 );
    if( mem == NULL ) {
      log_e("Failed to allocate %d bytes for SID memory, the app WILL crash", 0x10000 );
      log_e("[freeheap=%d] largest free block=%d bytes)", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
      return;
    }
  }

  //

  log_i("SID Player Ready, play/pause core=%d, audio core=%d", SID_PLAYER_CORE, SID_AUDIO_CORE );
}


#ifdef _SID_HVSC_FILE_INDEXER_H_

  SIDTunesPlayer::SIDTunesPlayer( MD5FileConfig *cfg )
  {
    setMD5Parser( cfg );
    fs = cfg->fs;
  }


  void SIDTunesPlayer::setMD5Parser( MD5FileConfig *cfg )
  {
    MD5Parser = new MD5FileParser( cfg );
  }

#endif



void SIDTunesPlayer::beginTask()
{
  xTaskCreatePinnedToCore( SIDTunesPlayer::SIDTunePlayerTask, "SIDTunePlayerTask", 4096, this, SID_AUDIO_PRIORITY,  &xTunePlayerTaskHandle, SID_AUDIO_CORE );
  xTaskCreatePinnedToCore( SIDTunesPlayer::SIDLoopPlayerTask, "SIDLoopPlayerTask", 4096, this, SID_PLAYER_PRIORITY, &xPlayerLoopTaskLocker, SID_PLAYER_CORE );
}


bool SIDTunesPlayer::begin(int clock_pin,int data_pin, int latch,int sid_clock_pin)
{
  if( begun ) return true;
  begun = sid.begin( clock_pin, data_pin, latch, sid_clock_pin );
  beginTask();
  return begun;
}


bool SIDTunesPlayer::begin(int clock_pin,int data_pin, int latch )
{
  if( begun ) return true;
  begun = sid.begin( clock_pin, data_pin, latch );
  beginTask();
  return begun;
}


void SIDTunesPlayer::kill()
{
  stop();
  vTaskDelay(10);
  if( xTunePlayerTaskHandle != NULL ) { vTaskDelete( xTunePlayerTaskHandle ); xTunePlayerTaskHandle = NULL; }
  if( xTunePlayerTaskLocker != NULL ) { vTaskDelete( xTunePlayerTaskLocker ); xTunePlayerTaskLocker = NULL; }
  if( xPlayerLoopTaskLocker != NULL ) { vTaskDelete( xPlayerLoopTaskLocker ); xPlayerLoopTaskLocker = NULL; }
  //if( xPlayerLoopTaskHandle != NULL ) { vTaskDelete( xPlayerLoopTaskHandle ); xPlayerLoopTaskHandle = NULL; }
  sid.end();
  begun = false;
}


void SIDTunesPlayer::reset()
{
  kill(); // kill tasks first
  delay(100);

  // if( currenttrack != nullptr ) {
  //   if( currenttrack->durations != nullptr ) free( currenttrack->durations );
  //   free( currenttrack );
  // }
  //
  // currenttrack = (SID_Meta_t*)sid_calloc( 1, sizeof( SID_Meta_t ) );
  // currenttrack->durations = nullptr;

  if( mem!=NULL ) {
    free( mem );
    //mem = (uint8_t*)sid_calloc( 0x10000, 1 );
    mem = (uint8_t*)heap_caps_malloc( 0x10000, MALLOC_CAP_8BIT );
  } else {
    //memset( mem, 0, 0x10000 );
  }
}



// *****************************
// Getters/Setters

void     SIDTunesPlayer::fireEvent( SIDTunesPlayer* player, sidEvent event) { if (eventCallback) (*eventCallback)(player, event); vTaskDelay(1); }
void     SIDTunesPlayer::setMaxVolume( uint8_t vol ) { volume=vol;  sid.setMaxVolume(vol); }
void     SIDTunesPlayer::setDefaultDuration(uint32_t duration) { default_song_duration=duration; }
void     SIDTunesPlayer::setLoopMode(loopmode mode) { currentloopmode=mode; }
void     SIDTunesPlayer::setPlayMode(playmode mode) { currentplaymode=mode; }

bool     SIDTunesPlayer::isPlaying() { return TunePlayerTaskPlaying; }
bool     SIDTunesPlayer::isRunning() { return playerrunning; }

loopmode SIDTunesPlayer::getLoopMode() { return currentloopmode; }
playmode SIDTunesPlayer::getPlayMode() { return currentplaymode; }


uint32_t SIDTunesPlayer::getDefaultDuration() {      if( currenttune ) return currenttune->getDefaultDuration(); else return 0; }
uint32_t SIDTunesPlayer::getElapseTime() {           if( currenttune ) return currenttune->getElapseTime(); else return 0; }
uint32_t SIDTunesPlayer::getCurrentTrackDuration() { if( currenttune ) return currenttune->getDuration(); else return 0; }

uint8_t  *SIDTunesPlayer::getSidType() {   if( currenttune ) return currenttune->getSidType(); else return nullptr; }
char     *SIDTunesPlayer::getName() {      if( currenttune ) return currenttune->getName(); else return nullptr; }
char     *SIDTunesPlayer::getPublished() { if( currenttune ) return currenttune->getPublished(); else return nullptr; }
char     *SIDTunesPlayer::getAuthor() {    if( currenttune ) return currenttune->getAuthor(); else return nullptr; }
char     *SIDTunesPlayer::getFilename() {  if( currenttune ) return currenttune->getFilename(); else return nullptr; }

uint8_t  SIDTunesPlayer::getSongsCount() {  return currenttune->getSongsCount(); }
uint8_t  SIDTunesPlayer::getCurrentSong() { return currenttune->getCurrentSong(); }
uint8_t  SIDTunesPlayer::getStartSong() {   return currenttune->getStartSong(); }
uint8_t  SIDTunesPlayer::getMaxVolume() { return sid.getVolume(); }



bool SIDTunesPlayer::getInfoFromSIDFile( const char * path, SID_Meta_t *songinfo )
{
  if( !fs->exists( path ) ) {
    log_e("File %s does not exist", path);
    return false;
  }
  File file = fs->open( path );
  if( !file ) {
    log_e("nothing to open" );
    return false;
  }
  if( currenttune != nullptr ) {
    log_v("Deleting old SIDTune object");
    delete currenttune;
  }

  log_v("Fetching meta into %s pointer for path %s", songinfo==nullptr ? "unallocated" : "preallocated", path );
  currenttune = new SIDTune( this, path, songinfo );
  bool ret = currenttune->getMetaFromFile( &file );
  return ret;
}



bool SIDTunesPlayer::playSidFile()
{
  if(mem == NULL) {
    log_e("Error mem init was missed");
    return false;
  }

  if( TunePlayerTaskRunning ) {
    if( paused || TunePlayerTaskPlaying ) {
      log_w("Stopping the current song");
      stop();
    }
  }

  if( currenttune->trackinfo == nullptr ) {
    log_e("Load a track first !!");
    return false;
  }

  if( !currenttune->loaded ) {
    if( !currenttune->loadFromFile( fs ) ) {
      log_e("loadFromFile first!");
      return false;
    }
    fireEvent(this, SID_NEW_FILE);
  }
  bool playable = playSongNumber( currenttune->trackinfo->startsong > 0 ? currenttune->trackinfo->startsong - 1 : 0 );
  return playable;
}



void SIDTunesPlayer::stop()
{
  if( !isPlaying() ) return; // nothing to stop
  if( xTunePlayerTaskHandle == NULL ) return; // no task to hear the stop signal

  if( paused ) {
    togglePause();
  }

  bool was_playing = TunePlayerTaskPlaying;

  sid.clearQueue();

  ExitTunePlayerLoop = true;
  uint32_t max_wait_ms = 100;
  uint32_t timed_out  = millis() + max_wait_ms;
  while( ExitTunePlayerLoop ) {
    vTaskDelay(1);
    if( millis() > timed_out ) {
      log_e("[FATAL] Timed out while waiting for tune to exit (a song %s being played), halting", was_playing ? "was":"was NOT");
      while(1) vTaskDelay(1);
      break;
    }
  }

  sid.soundOff();

  if( !was_playing ) {
    log_e("[NOEVENT] Stop signal sent but player task was not playing!");
  } else {
    fireEvent( this, SID_STOP_TRACK );
  }
  log_d("stopped");
}


void SIDTunesPlayer::togglePause()
{
  if( xTunePlayerTaskHandle == NULL ) {
    log_w("No song is playing, can't toggle pause");
    return;
  }
  if(!paused) {
    paused = true;
    fireEvent( this, SID_PAUSE_PLAY );
  } else {
    sid.soundOn();
    paused = false;
    fireEvent( this, SID_RESUME_PLAY );
    if( xPausePlayTaskLocker != NULL ) {
      log_d("[NOTIFYING] PlayerLoopTask that pause ended (core #%d", xPortGetCoreID() );
      xTaskNotifyGive( xPausePlayTaskLocker ); // send "unpause" notification
      vTaskDelay(1);
    }
  }
}


bool SIDTunesPlayer::playNextSongInSid()
{
  log_d("Current song = %d, next song = %d", currenttune->currentsong, (currenttune->currentsong+1)%currenttune->trackinfo->subsongs );
  currenttune->currentsong = (currenttune->currentsong+1)%currenttune->trackinfo->subsongs;
  return playSongNumber( currenttune->currentsong );
}



bool SIDTunesPlayer::playPrevSongInSid()
{
  log_d("Current song = %d, prev song = %d", currenttune->currentsong, (currenttune->currentsong == 0) ? currenttune->trackinfo->subsongs-1 : currenttune->currentsong-1 );
  currenttune->currentsong = (currenttune->currentsong == 0) ? currenttune->trackinfo->subsongs-1 : currenttune->currentsong-1;
  return playSongNumber( currenttune->currentsong );
}



bool SIDTunesPlayer::playSongNumber( int songnumber )
{
  if( xTunePlayerTaskHandle == NULL ) {
    log_d("[%d] no SIDTunePlayerTask found, halting", ESP.getFreeHeap() );
    while(1) vTaskDelay(1);
  }

  //sid.soundOff();
  //sid.resetsid();

  if( isPlaying() ) stop();

  if( songnumber<0 || songnumber >= getSongsCount() ) {
    log_e("Wrong song number: %d (max=%d)", songnumber, getSongsCount());
    return false;
  }

  if( currenttune->currentsong != songnumber ) {
    currenttune->currentsong = songnumber;
  }

  log_d("[NOTIFYING] Sending TunePlay notification to SIDTunePlayerTask (core #%d, songnumber=%d)", xPortGetCoreID(), songnumber );
  xTaskNotifyGive( xTunePlayerTaskHandle );

  unsigned long startwait = millis();
  unsigned long timeout = 1000;
  while( !isPlaying() && millis()-startwait<timeout ) vTaskDelay(1);
  if( millis()-startwait>=timeout ) {
    log_e("[FATAL] Song still not playing after %s ms timeout, halting", timeout );
    while(1) vTaskDelay(1);
  }

  fireEvent( this, SID_NEW_TRACK );
  return true;
}



void SIDTunesPlayer::SIDTunePlayerTask(void * parameters)
{
  log_d("[freeheap=%d] Entering SID*Tune*PlayerTask on core %d width priority %d", ESP.getFreeHeap(), xPortGetCoreID(), uxTaskPriorityGet(NULL) );

  SIDTunesPlayer * _player = (SIDTunesPlayer *)parameters;
  _player->TunePlayerTaskRunning = true;
  _player->TunePlayerTaskPlaying = false; // hold until track info is loaded

  for(;;) {
    log_d("[WAIT] Blocking core #%d while waiting for tune start notification on xTunePlayerTaskHandle (#%d)...", xPortGetCoreID(), xTunePlayerTaskHandle );
    _player->ExitTunePlayerLoop = false;
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    log_d("[NOTIFIED] Releasing core #%d : got TunePlay notification (songnumber=%d), now resetting delta duration and looping tune", xPortGetCoreID(), _player->currenttune->currentsong );

    SIDTune* tune = _player->currenttune;

    // for cpu controls
    //_player->currentoffset = 0;
    //_player->previousoffset = 0;

    _player->speed = tune->speedsong[tune->currentsong]; // _player->currenttune->speed;
    //_player->setSpeed(tune->speedsong[tune->currentsong]==0?0:1); // force speed 100% (TODO: remove this when the speed bug is found)
    // for song control
    int songnumber = tune->currentsong;

    _player->cpuReset();
    _player->sid.soundOn();

    if( tune->play_addr == 0 ) { // no song selected or first song
      _player->cpuJSR( tune->init_addr, 0 );
      tune->play_addr = (_player->mem[0x0315] << 8) + _player->mem[0x0314];
      _player->cpuJSR( tune->init_addr, songnumber );
      if( tune->play_addr == 0 ) { // one song only ?
        tune->play_addr = (_player->mem[0xffff] << 8) + _player->mem[0xfffe];
        _player->mem[1]=0x37;
        log_i("play_addr speculated from 0xffff/0xfffe : $%04x", tune->play_addr);
      } else {
        log_i("play_addr speculated from 0x314/0x315 : $%04x", tune->play_addr);
      }
    } else {
      _player->cpuJSR( tune->init_addr, songnumber );
      log_i("play_addr preselected: $%04x", tune->play_addr );
    }

    if((int)tune->trackinfo->durations[songnumber]<=0 /*|| _player->MD5Parser == NULL*/ ) {
      tune->duration = tune->default_song_duration;
      log_i("Addrs: load=$%04x init=$%04x play=$%04x (song %d/%d) duration=%dms (default)",
            tune->load_addr, tune->init_addr, tune->play_addr, songnumber+1, tune->getSongsCount(), tune->duration);
    } else {
      tune->duration = tune->trackinfo->durations[songnumber];
      log_i("Addrs: load=$%04x init=$%04x play=$%04x (song %d/%d) duration=%dms",
            tune->load_addr, tune->init_addr, tune->play_addr, songnumber+1, tune->getSongsCount(), tune->duration);
    }
    log_i("Attrs: tunespeed=$%08x songspeed[%d]=%d", tune->speed, songnumber, tune->speedsong[songnumber] );
    log_i("Attrs: clock=$%02x sidModel=$%02x relocStartPage=$%02x relocPages=%-2d", tune->clock, tune->sidModel, tune->relocStartPage, tune->relocPages );

    tune->delta_song_duration = 0;
    tune->start_time = millis();
    //uint32_t now_time = 0;

    _player->TunePlayerTaskPlaying = true;
    bool end_of_track = false;

    _player->fireEvent( _player, SID_START_PLAY );

    while( _player->TunePlayerTaskPlaying == true && _player->ExitTunePlayerLoop==false ) {

      _player->totalinframe += _player->cpuJSR( tune->play_addr, 0 );
      _player->wait += _player->waitframe;
      _player->frame = true;
      int nRefreshCIA = (int)( ((19954 * (_player->getmem(0xdc04) | (_player->getmem(0xdc05) << 8)) / 0x4c00) + (_player->getmem(0xdc04) | (_player->getmem(0xdc05) << 8))  )  /2 );
      if ((nRefreshCIA == 0) /*|| (_player->speedsong[_player->currentsong]==0)*/)
        nRefreshCIA = 19954;
      _player->waitframe = nRefreshCIA;

      if( tune->duration > 0 ) {
        uint32_t now_time = millis();
        tune->delta_song_duration += now_time - tune->start_time;
        tune->start_time = now_time;
        if( tune->delta_song_duration >= tune->duration ) {
          end_of_track = true;
          break;
        }
      }

      if( _player->paused ) {
        _player->sid.soundOff();
        _player->TunePlayerTaskPlaying = false;
        xPausePlayTaskLocker = xTaskGetCurrentTaskHandle();
        log_d("[WAIT] Blocking core#%d while waiting for pause notification", xPortGetCoreID() );
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY ); // wait for a 'unpause' notification
        log_d("[NOTIFIED] Releasing core#%d after unpause", xPortGetCoreID() );
        xPausePlayTaskLocker = NULL;
        _player->sid.soundOn();
        tune->start_time = millis();
        _player->TunePlayerTaskPlaying = true;
      }
      //vTaskDelay(1);
    }

    log_i("Playback stopped");
    _player->sid.soundOff();
    _player->cpuReset();
    vTaskDelay(1);

    _player->sid.clearQueue();
    _player->TunePlayerTaskPlaying = false;
    _player->fireEvent( _player, SID_END_PLAY );

    if( end_of_track ) {
      _player->fireEvent( _player, SID_END_TRACK );
      if( xPlayerLoopTaskLocker != NULL ) {
        log_d("[NOTIFYING] PlayerLoopTask that end of track was reached (core #%d", xPortGetCoreID() );
        xTaskNotifyGive( xPlayerLoopTaskLocker );
        vTaskDelay(1);
      } else {
        log_e("LoopExit after reaching end of track but no player loop task notifier exists !");
      }
    }

  } // for(;;)
}


bool SIDTunesPlayer::playSID(SID_Meta_t *songinfo )
{
  assert(songinfo);

  if( currenttune != nullptr ) {
    log_d("Deleting old SIDTune object");
    delete currenttune;
  }
  currenttune = new SIDTune( this, songinfo->filename, songinfo );
  return playSID();
}


bool SIDTunesPlayer::playSID()
{
  if( !playSidFile() ) {
    log_e("playSID failed (asked #%d out of %d subsongs)", currenttune->currentsong+1, currenttune->trackinfo->subsongs );
    return false;
  }

  if( xPlayerLoopTaskLocker == NULL ) {
    log_e("No SIDLoopPlayerTask found, halting");
    while(1) vTaskDelay(1);
  }

  playerrunning = true;
  vTaskDelay(10); // breathe other core task
  return true;
}



// play one or all songs from a SID file in different modes
void SIDTunesPlayer::SIDLoopPlayerTask(void *param)
{
  bool playerstop = false;
  SIDTunesPlayer *_player = (SIDTunesPlayer *)param;
  log_d("Entering Player controls task on core #%d with priority %d", xPortGetCoreID(), uxTaskPriorityGet(NULL));

  _player->LoopPlayerTaskRunning = true;

  while( true ) {

    log_d("[WAIT] Blocking core #%d while waiting for a notification", xPortGetCoreID() );
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    log_d("[NOTIFIED] Releasing core#%d", xPortGetCoreID());

    SIDTune *tune = _player->currenttune;

    switch( _player->currentplaymode ) {
      case SID_ALL_SONGS:

        switch( _player->currentloopmode ) {
          case SID_LOOP_ON:
            if( !_player->playNextSongInSid() ) {
              log_e("playNextSongInSid failed (asked #%d out of %d subsongs)", tune->currentsong+1, tune->trackinfo->subsongs );
              playerstop = true;
            }
          break;
          case SID_LOOP_RANDOM:
          {
            uint8_t randomSongNumber = rand()%tune->trackinfo->subsongs;
            // TODO: avoid playing the same song twice
            if( !_player->playSongNumber( randomSongNumber ) ) {
              log_e("Random failed (diced %d out of %d subsongs)", randomSongNumber, tune->trackinfo->subsongs );
              playerstop = true;
            }
          }
          break;
          case SID_LOOP_OFF:
            if ( int(tune->currentsong+1) >= tune->trackinfo->subsongs ) {
              log_d("loop mode = SID_LOOP_OFF, stopping at `this track");
              playerstop = true;
            } else {
              if( !_player->playNextSongInSid() ) {
                log_e("playNextSongInSid failed (asked #%d out of %d subsongs)", tune->currentsong+1, tune->trackinfo->subsongs );
                playerstop = true;
              }
            }
          break;
        }
      break;
      case SID_ONE_SONG:
        log_d("loop mode = SID_ONE_SONG, stopping");
        playerstop = true;
      break;
    }

    if( playerstop ) {
      _player->stop();
      //_player->sid.soundOff();
      //_player->sid.resetsid();
      _player->playerrunning = false;
      _player->LoopPlayerTaskRunning = false;
      _player->fireEvent( _player, SID_END_FILE );
      playerstop = false;
    }

    vTaskDelay(1);

  } // end while(true)

}






////////////////////////////////////////////////// SidTune methods



SIDTune::SIDTune( SIDTunesPlayer *_player )
{
  assert(_player);

  player    = _player;
  trackinfo = (SID_Meta_t*)sid_calloc(1, sizeof(SID_Meta_t) );
  default_song_duration = player->default_song_duration;
  memset( sidtype, 0, 5 );
}


SIDTune::SIDTune( SIDTunesPlayer *_player, const char* _path )
{
  assert(_player);
  assert(_path);

  player    = _player;
  path      = _path;
  trackinfo = (SID_Meta_t*)sid_calloc(1, sizeof(SID_Meta_t) );
  default_song_duration = player->default_song_duration;
  memset( sidtype, 0, 5 );
}


SIDTune::SIDTune( SIDTunesPlayer *_player, const char* _path, SID_Meta_t *_trackinfo )
{
  assert(_player);
  assert(_path);

  player = _player;
  path   = _path;

  if( _trackinfo == nullptr ) {
    log_v("Allocating %d bytes for song %s", sizeof(SID_Meta_t), path );
    _trackinfo = (SID_Meta_t*)sid_calloc(1, sizeof(SID_Meta_t) );
    needs_free = true;
  } else {
    log_v("Re-using %d bytes for song %s", sizeof(SID_Meta_t), path );
    // trackinfo ptr is provided from outside the class, don't free on destruct
    needs_free = false;
  }
  trackinfo = _trackinfo;

  //if( _trackinfo->durations != nullptr ) {
  //  default_song_duration = _trackinfo->durations[0];
  //} else {
    default_song_duration = player->default_song_duration;
  //}
  memset( sidtype, 0, 5 );
}


SIDTune::~SIDTune()
{
  if( player->isPlaying() ) {
    player->stop();
  }
  if( needs_free ) {
    if( trackinfo == nullptr ) {
      log_e("trackinfo is already free!");
      return;
    }
    if(has_durations && trackinfo->durations != nullptr ) {
      log_d("Freeing durations");
      free(trackinfo->durations);
    }
    free( trackinfo );
    trackinfo = nullptr;
  }
}





// 80 chars -> ────────────────────────────────────────────────────────────────────────────────
// 60 chars -> ────────────────────────────────────────────────────────────

void printspanln( bool openclose, bool is_hr )
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
      printspan4("", "", "", "");
    }
  }
}

void printspan2( const char*title, const char*val )
{
  Serial.printf("│ %-18s%-42s │\n", title, val );
}

void printspan4( const char*t1, const char*v1, const char*t2, const char*v2 )
{
  Serial.printf("│ %-18s%-12s%-18s%-12s │\n", t1, v1, t2, v2 );
}

const char* hexString16( uint16_t hexval, char *buf )
{
  snprintf(buf, 8, "$%04x", hexval );
  return buf;
}

const char* hexString32( uint32_t hexval, char *buf  )
{
  snprintf(buf, 10, "$%08x", hexval );
  return buf;
}



void SIDTune::tunedebug( SIDTune* tune )
{
  char toHexBuf[12] = {0};
  printspanln(true);
  Serial.printf("│ %-60s │\n", (const char*)tune->trackinfo->filename );
  printspanln(false, true);
  printspanln();
  printspan2("  Title", (const char*)tune->trackinfo->name );
  printspan2("  Author", (const char*)tune->trackinfo->author );
  printspan2("  Released", (const char*)tune->trackinfo->published );
  printspanln();
  printspan4("  Load Address", hexString16(tune->load_addr, toHexBuf),        "Number of tunes", String(tune->trackinfo->subsongs).c_str() );
  printspan4("  Init Address", hexString16(tune->init_addr, toHexBuf),        "Default tune", String(tune->trackinfo->startsong).c_str() );
  printspan4("  Play Address", hexString16(tune->play_addr, toHexBuf),        "Speed", hexString32(tune->speed, toHexBuf) );
  printspan4("  RelocStartPage", hexString16(tune->relocStartPage, toHexBuf), "RelocPages" , String(tune->relocPages).c_str() );
  printspanln();
  printspan4("  SID Model", tune->getSidModelStr(),                 "Clock", tune->getClockStr() ); // TODO: name model (1=6581, 2=8580) and clock (1=PAL, 2=NTSC)
  printspanln();
  printspan4("  File Format", (const char*)tune->sidtype,           "BASIC", tune->musPlay?"true":"false" );
  printspan4("  Format Version", String(tune->version).c_str(),     "PlaySID Specific", tune->psidSpecific?"true":"false" );
  printspanln();
  printspan2("  MD5", (const char*)tune->trackinfo->md5 );
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
    //size_t found = 0;
    printspan4("  Song#", "Speed", "Duration", "" );
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
      printspan4( String("  #"+String(i)).c_str(), String(tune->speedsong[i]).c_str(), (const char*)songduration, "" );
    }
  }
  printspanln();
  printspanln(true);
}



bool SIDTune::loadFromFile( fs::FS *fs )
{
  if( loaded ) return true;

  fs::File file = fs->open( trackinfo->filename );

  if( !file ) {
    log_e("Unable to open file %s for reading", trackinfo->filename);
    return false;
  }
  if( !loadFromFile( &file ) ) {
    log_e("Could not load tune from file");
    return false;
  }
  return true;
}



bool SIDTune::loadFromFile( fs::File *file, bool close )
{
  if( file==nullptr ) {
    log_e("nothing to open" );
    return false;
  }

  player->stop();

  memset( player->mem, 0, 0x10000 );

  uint16_t flags;
  uint32_t offset;

  file->seek(0); // rewind
  file->read( sidtype, 4 );

  if(sidtype[0]!=0x50 || sidtype[1]!=0x53 || sidtype[2]!=0x49 || sidtype[3]!=0x44) {
    log_e("File type:%s not supported yet", sidtype );
    if( close ) file->close();
    return false;
  }

  file->seek(4);
  version  = (file->read()<<8) + file->read();
  offset   = (file->read()<<8) + file->read();

  file->seek(7);
  data_offset = file->read();

  if( offset != data_offset ) {
    log_e("Bad offset calculation %d vs %d", data_offset, offset);
    if( close ) file->close();
    return false;
  }

  load_addr  = (file->read()<<8) + file->read(); // this one isn't trusted yet
  init_addr  = (file->read()<<8) + file->read();
  play_addr  = (file->read()<<8) + file->read();

  file->seek( 15);
  trackinfo->subsongs = file->read();
  file->seek(17);
  trackinfo->startsong = file->read();

  speed = (file->read()<<24) | (file->read()<<16) | (file->read()<<8) | file->read();

  file->seek(22);
  file->read(trackinfo->name,32);
  file->seek(0x36);
  file->read(trackinfo->author,32);
  file->seek(0x56);
  file->read(trackinfo->published,32);

  flags  = (file->read()<<8) + file->read();

  musPlay      = false;
  psidSpecific = false;

  if (version >= 2) {
    if (flags & 0x01)
      musPlay = true;
    if (flags & 0x02)
      psidSpecific = true;
    clock          = (flags & 0x0c) >> 2;
    sidModel       = (flags & 0x30) >> 4;
    relocStartPage = file->read();
    relocPages     = file->read();
    //log_w("Clock: $%02x, sidModel: $%02x, relocStartPage: $%02x, relocPages: %2d", clock, sidModel, relocStartPage, relocPages );
  } else {
    clock          = 0;//SIDTUNE_CLOCK_UNKNOWN;
    sidModel       = 0;//SIDTUNE_SIDMODEL_UNKNOWN;
    relocStartPage = 0;
    relocPages     = 0;
    //log_w("Clock/sidModel to defaults");
  }

  size_t modLen = file->size() - offset;

  // Now adjust MUS songs
  if (musPlay) {
    load_addr = 0x1000;
    init_addr = 0xc7b0;
    play_addr = 0x0000;
  } else {
    if (load_addr == 0) {
      file->seek(offset);
      load_addr = file->read() + (file->read()<<8);
      modLen -= 2;
    }
  }

  if (init_addr == 0)
    init_addr = load_addr;

  if (trackinfo->startsong == 0)
    trackinfo->startsong = 1;

  file->read(&player->mem[load_addr], modLen );
/*
  for(int i=0;i<32;i++) {
    uint8_t fm;
    file->seek( 0x12+(i>>3) );
    fm = file->read();
    speedsong[31-i] = (fm & (byte)pow(2,7-i%8))?1:0;
    if( speedsong[31-i] != 0 ) {
      log_d("Speedsong[%d] = %d", 31-i, speedsong[31-i] );
    }
  }
*/

  for (int32_t s = 0; s < trackinfo->subsongs; s++) {
    if (((speed >> (s & 0x1f)) & 0x01) == 0) {
      //~tune->speedsong[i]
      speedsong[s] = 0; // SIDTUNE_SPEED_VBI;		// 50 Hz
    } else {
      speedsong[s] = 60; // SIDTUNE_SPEED_CIA_1A;	// CIA 1 Timer A
    }
  }



  /*
  if( speed == 0 )
    nRefreshCIAbase = 38000;
  else
    nRefreshCIAbase = 19950;
*/

//   file->seek(data_offset);
//
//   // CODE SMELL: should read file->size() - current offset
//   __attribute__((unused))
//   size_t g = file->read(&player->mem[load_addr], file->size()/*-(data_offset+2)*/ );
//   log_d("Read %d bytes when %d were expected (max=%d, current offset=%d)", g, file->size()-(data_offset+2), file->size(), data_offset );
//   //memset(&mem[load_addr+g],0xea,0x10000-load_addr+g);
//   for(int i=0;i<32;i++) {
//     uint8_t fm;
//     file->seek( 0x12+(i>>3) );
//     fm = file->read();
//     speedsong[31-i] = (fm & (byte)pow(2,7-i%8))?1:0;
//     if( speedsong[31-i] != 0 ) {
//       log_d("Speedsong[%d] = %d", 31-i, speedsong[31-i] );
//     }
//   }
  //snprintf( currentfilename, 256, "%s", trackinfo->filename );
  //getcurrentfile = currentfile;
  //fireEvent(SID_START_PLAY);
  if( trackinfo->startsong > 0 ) {
    currentsong = trackinfo->startsong - 1;
  } else {
    log_e("Invalid startsong value: 0 (should be at least 1), forcing to 0");
    currentsong = 0;
  }

  //songdebug( trackinfo );

  if( close) file->close();

  #ifdef _SID_HVSC_FILE_INDEXER_H_
    if( !has_durations && player->MD5Parser != NULL ) {
      if( trackinfo->durations == nullptr ) {
        log_d("[%d] Fetching durations from HVSC", ESP.getFreeHeap() );
        if( !player->MD5Parser->getDurations( trackinfo ) ) {
          log_e("Can't get duration for this song (md5:%s), assigning default length to all", trackinfo->md5 );
          free( trackinfo->durations );
          trackinfo->durations = (uint32_t*)sid_calloc(trackinfo->subsongs, sizeof(uint32_t));
          trackinfo->durations[0] = 0;
          for( int i=0; i<trackinfo->subsongs; i++ ) {
            trackinfo->durations[i] = default_song_duration;
          }
        } else {
          // just populated
          //log_i("Extracted durations" );
          //songdebug( trackinfo );
        }
        has_durations = true;
      } else {
        log_e("[%d] Durations don't need parsing", ESP.getFreeHeap() );
        for( int i=0; i<trackinfo->subsongs; i++ ) {
          log_d("  durations[%2d] = %8d", i, trackinfo->durations[i] );
        }
      }
    } else {
      log_v("[%d] No MD5 Parser", ESP.getFreeHeap() );
      if( trackinfo->durations == nullptr ) {
        trackinfo->durations = (uint32_t*)sid_calloc(1, sizeof(uint32_t));
        trackinfo->durations[0] = 0;
      } else {
        // already populated
        has_durations = true;
      }
    }
  #endif


  loaded = true;

  SIDTune::tunedebug(this);

  return true;
}






bool SIDTune::getMetaFromFile( fs::File *file, bool close )
{
  assert( file );

  uint8_t stype[5];
  uint16_t flags, version;
  __attribute__((unused)) uint32_t offset;

  if( trackinfo == nullptr ) {
    log_v("[%d] Allocating %d bytes for trackinfo", ESP.getFreeHeap(), sizeof(SID_Meta_t) );
    trackinfo = (SID_Meta_t*)sid_calloc(1, sizeof(SID_Meta_t) );
  } else {
    if( trackinfo->durations != nullptr ) {
      if( needs_free ) {
        log_v("songinfo has durations, freeing");
        free( trackinfo->durations  );
        trackinfo->durations = nullptr;
      } else {
        log_d("keeping songinfo durations");
      }
    } else {
      log_v("songinfo has no durations as expected");
    }
  }

  if( needs_free ) {
    //log_d("Zeroing trackinfo");
    //memset( trackinfo, 0, sizeof(SID_Meta_t) );
    *trackinfo = {};
  }

  log_d("Fetching trackinfo for %s", path );

  //setCleanFileName( songinfo->filename, 255, "%s", path );
  snprintf( trackinfo->filename, 255, "%s", path );

  memset(stype, 0, 5);

  file->seek(0); // rewind
  file->read(stype, 4);
  if( stype[0]!=0x50 || stype[1]!=0x53 || stype[2]!=0x49 || stype[3]!=0x44 ) {
    log_e("Unsupported SID Type: '%s' in file %s", stype, path );
    if( close ) file->close();
    return false;
  }

  file->seek(4);
  version  = (file->read()<<8) + file->read();
  offset   = (file->read()<<8) + file->read();

  file->seek( 15);
  trackinfo->subsongs = file->read();
  file->seek(17);
  trackinfo->startsong = file->read();

  speed  = (file->read()<<24) | (file->read()<<16) | (file->read()<<8) | file->read();

  file->seek(22);
  file->read(trackinfo->name,32);
  file->seek(0x36);
  file->read(trackinfo->author,32);
  file->seek(0x56);
  file->read(trackinfo->published,32);

  flags  = (file->read()<<8) + file->read();

  musPlay      = false;
  psidSpecific = false;

  if (version >= 2) {
    if (flags & 0x01)
      musPlay = true;
    if (flags & 0x02)
      psidSpecific = true;
    clock          = (flags & 0x0c) >> 2;
    sidModel       = (flags & 0x30) >> 4;
    relocStartPage = file->read();
    relocPages     = file->read();
    //log_w("Clock: $%02x, sidModel: $%02x, relocStartPage: $%02x, relocPages: %2d", clock, sidModel, relocStartPage, relocPages );
  } else {
    clock          = 0;//SIDTUNE_CLOCK_UNKNOWN;
    sidModel       = 0;//SIDTUNE_SIDMODEL_UNKNOWN;
    relocStartPage = 0;
    relocPages     = 0;
    //log_w("Clock/sidModel to defaults");
  }

  // Now adjust MUS songs
  if (musPlay) {
    load_addr = 0x1000;
    init_addr = 0xc7b0;
    play_addr = 0x0000;
  }


  //uint16_t SIDModel[3];
  file->seek( 0x77 );
  SIDModel[0] = ( file->read() & 0x30 ) >= 0x20 ? 8580 : 6581;
  file->seek( 0x77 );
  SIDModel[1] = ( file->read() & 0xC0 ) >= 0x80 ? 8580 : 6581;
  file->seek( 0x76 );
  SIDModel[2] = ( file->read() & 3 ) >= 3 ? 8580 : 6581;

  bool playable = true;

  if( SIDModel[0]!= 6581 || SIDModel[1]!= 6581 || SIDModel[2]!= 6581 ) {
    log_v("Unsupported SID Model?:  0: %d  1: %d  2: %d  in file %s",
      SIDModel[0],
      SIDModel[1],
      SIDModel[2],
      path
    );
  }

  //log_d("Will check whether HVSC info needs populating");

  #ifdef _SID_HVSC_FILE_INDEXER_H_
    if( !has_durations && player->MD5Parser != NULL ) {
      //log_d("Fetching HVSC Info");
      snprintf( trackinfo->md5, 33, "%s", Sid_md5::calcMd5( file ) );
      if( close ) file->close();
      if( !player->MD5Parser->getDurations( trackinfo ) ) {
        log_d("[%d] HVSC md5 lookup failed for '%s' / '%s' ", ESP.getFreeHeap(), trackinfo->name, trackinfo->md5 );
        free( trackinfo->durations );
        trackinfo->durations = (uint32_t*)sid_calloc(trackinfo->subsongs, sizeof(uint32_t));
        trackinfo->durations[0] = 0;
        for( int i=0; i<trackinfo->subsongs; i++ ) {
          trackinfo->durations[i] = default_song_duration;
        }
        has_durations = true;
      } else {
        log_d("[%d] HVSC SID Path lookup success for song '%s' : %d durations extracted", ESP.getFreeHeap(), trackinfo->name, trackinfo->subsongs );
        has_durations = true;
      }
    } else {
      log_v("[%d] NOT Collecting/Checking HVSC Database as per player configuration (see SIDTunesPlayer::MD5Parser)", ESP.getFreeHeap() );
      snprintf( trackinfo->md5, 33, "%s", Sid_md5::calcMd5( file ) );
      if( close ) file->close();
      //snprintf( trackinfo->md5, 33, "%s", "00000000000000000000000000000000" );
    }
  #else
    log_v("NOT Fetching HVSC Info");
    snprintf( trackinfo->md5, 33, "%s", Sid_md5::calcMd5( file ) );
    if( close ) file->close();
  #endif
  //if( playable ) songdebug( songinfo );
  return playable;
}



const char* SIDTune::getSidModelStr()
{
  switch( sidModel )
  {
    case 1: return "6581";
    case 2: return "8580";
    default: return "UNKNOWN";
  }
}
const char* SIDTune::getClockStr()
{
  switch( clock ) {
    case 1: return "PAL";
    case 2: return "NTSC";
    default: return "UNKNOWN";
  }
  //tune->clock// TODO: clock (1=PAL, 2=NTSC)
}
