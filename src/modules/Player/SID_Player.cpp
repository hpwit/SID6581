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
static TaskHandle_t xPlayerLoopTaskHandle = NULL;

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
    mem = (uint8_t*)sid_calloc( 0x10000, 1 );
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


bool SIDTunesPlayer::begin(int clock_pin,int data_pin, int latch,int sid_clock_pin)
{
  if( begun ) return true;
  begun = sid.begin( clock_pin, data_pin, latch, sid_clock_pin );
  return begun;
}


bool SIDTunesPlayer::begin(int clock_pin,int data_pin, int latch )
{
  if( begun ) return true;
  begun = sid.begin( clock_pin, data_pin, latch );
  return begun;
}


void SIDTunesPlayer::kill()
{
  stop();
  vTaskDelay(10);
  if( xTunePlayerTaskHandle != NULL ) { vTaskDelete( xTunePlayerTaskHandle ); xTunePlayerTaskHandle = NULL; }
  if( xTunePlayerTaskLocker != NULL ) { vTaskDelete( xTunePlayerTaskLocker ); xTunePlayerTaskLocker = NULL; }
  if( xPlayerLoopTaskLocker != NULL ) { vTaskDelete( xPlayerLoopTaskLocker ); xPlayerLoopTaskLocker = NULL; }
  if( xPlayerLoopTaskHandle != NULL ) { vTaskDelete( xPlayerLoopTaskHandle ); xPlayerLoopTaskHandle = NULL; }
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
    mem = (uint8_t*)sid_calloc( 0x10000, 1 );
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


// *********************************************************
// Player controls
/*
SID_Meta_t* SIDTunesPlayer::getInfoFromSIDFile( const char * path )
{

  SID_Meta_t* songinfo =(SID_Meta_t*)sid_calloc(1, sizeof(SID_Meta_t));
  SIDTune tmptune = new SIDTune( this, path, songinfo );



  //return getInfoFromSIDFile( path, currenttrack );
  return nullptr;
}
*/

bool SIDTunesPlayer::getInfoFromSIDFile( const char * path, SID_Meta_t *songinfo )
{
  //uint8_t stype[5];
  if( !fs->exists( path ) ) {
    log_e("File %s does not exist", path);
    return false;
  }
  //log_e("Opening FS from core #%d", xPortGetCoreID() );
  File file = fs->open( path );
  if( !file ) {
    log_e("nothing to open" );
    return false;
  }
  if( currenttune != nullptr ) {
    log_d("Deleting old SIDTune object");
    delete currenttune;
  }

  log_d("Fetching meta into %s pointer for path %s", songinfo==nullptr ? "unallocated" : "preallocated", path );
  currenttune = new SIDTune( this, path, songinfo );
  bool ret = currenttune->getMetaFromFile( &file );
  //currenttrack = currenttune->trackinfo;
  return ret;

/*
  if(!file) {
    log_e("Unable to open %s", path);
    return false;
  }

  if( songinfo == nullptr ) {
    log_v("[%d] Allocating %d bytes for songinfo", ESP.getFreeHeap(), sizeof(SID_Meta_t) );
    songinfo = (SID_Meta_t*)sid_calloc(1, sizeof(SID_Meta_t) );
  } else {
    if( songinfo->durations != nullptr ) {
      log_v("songinfo has durations, freeing");
      free( songinfo->durations  );
    }
  }

  memset( songinfo, 0, sizeof(SID_Meta_t) );
  songinfo->durations = nullptr;
  //setCleanFileName( songinfo->filename, 255, "%s", path );
  snprintf( songinfo->filename, 255, "%s", path );

  memset(stype, 0, 5);
  file.read(stype, 4);
  if( stype[0]!=0x50 || stype[1]!=0x53 || stype[2]!=0x49 || stype[3]!=0x44 ) {
    log_d("Unsupported SID Type: '%s' in file %s", stype, path);
    file.close();
    return false;
  }
  file.seek( 15 );
  songinfo->subsongs = file.read();
  if( songinfo->subsongs == 0 ) songinfo->subsongs = 1; // prevent malformed SID files from zero-ing song lengths
  file.seek( 17 );
  songinfo->startsong = file.read();
  file.seek( 22 );
  file.read( songinfo->name, 32 );
  file.seek( 0x36 );
  file.read( songinfo->author, 32 );
  file.seek( 0x56 );
  file.read( songinfo->published, 32 );

  uint16_t preferred_SID_model[3];
  file.seek( 0x77 );
  preferred_SID_model[0] = ( file.read() & 0x30 ) >= 0x20 ? 8580 : 6581;
  file.seek( 0x77 );
  preferred_SID_model[1] = ( file.read() & 0xC0 ) >= 0x80 ? 8580 : 6581;
  file.seek( 0x76 );
  preferred_SID_model[2] = ( file.read() & 3 ) >= 3 ? 8580 : 6581;

  bool playable = true;

  if( preferred_SID_model[0]!= 6581 || preferred_SID_model[1]!= 6581 || preferred_SID_model[2]!= 6581 ) {
    log_v("Unsupported SID Model?:  0: %d  1: %d  2: %d  in file %s",
      preferred_SID_model[0],
      preferred_SID_model[1],
      preferred_SID_model[2],
      path
    );
  }


  #ifdef _SID_HVSC_FILE_INDEXER_H_
  if( MD5Parser != NULL ) {

    snprintf( songinfo->md5, 33, "%s", Sid_md5::calcMd5( file ) );
    file.close();
    if( !MD5Parser->getDuration( songinfo ) ) {
      log_d("[%d] lookup failed for '%s' / '%s' ", ESP.getFreeHeap(), songinfo->name, songinfo->md5 );
    }
//     if( !MD5Parser->getDurationsFromSIDPath( songinfo ) ) {
//       if( MD5Parser->getDurationsFastFromMD5Hash( songinfo ) ) {
//         log_d("[%d] Hash lookup success after SID Path lookup failed for '%s' ", ESP.getFreeHeap(), songinfo->name );
//       } else {
//         log_e("[%d] Both Hash lookup and SID Path lookup failed for '%s' / '%s' ", ESP.getFreeHeap(), songinfo->name, songinfo->md5 );
//       }
//     }
    else {
      log_d("[%d] SID Path lookup success for song '%s' ", ESP.getFreeHeap(), songinfo->name );
    }
  } else {
    log_v("[%d] NOT Collecting/Checking md5" );
    snprintf( songinfo->md5, 33, "%s", "00000000000000000000000000000000" );
  }
  #else
    snprintf( songinfo->md5, 33, "%s", Sid_md5::calcMd5( file ) );
    file.close();
  #endif
  //if( playable ) songdebug( songinfo );
  return playable;
  */
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

  //memset( mem, 0x0, 0x10000 );
  /*
  memset( name, 0, 32 );
  memset( author, 0, 32 );
  memset( published, 0, 32 );
  memset( currentfilename, 0, sizeof(currentfilename) );
  */
  //memset( sidtype, 0, 5 );
  //log_e("Accessing FS from core #%d", xPortGetCoreID() );
  /*
  bool hastune = currenttune != nullptr && strcmp( currenttune->trackinfo->filename, currenttrack->filename ) == 0;

  if( !fs->exists( currenttrack->filename ) ) {
    log_e("File %s does not exist", currenttrack->filename );
    return false;
  }

  if( !hastune ) {
    log_d("Allocating new SIDTune slot");
    if( currenttune != nullptr ) delete currenttune;
    currenttune = new SIDTune(this, currenttrack->filename, currenttrack );
  } else {
    log_d("Reusing SIDTune slot");
  }

  //log_d("playing file:%s", currenttrack->filename );

  fs::File file = fs->open( currenttrack->filename );

  if( !file ) {
    log_e("Unable to open file %s for reading", currenttrack->filename);
    return false;
  }

  //log_d("Loading from file");

  if( !currenttune->loadFromFile( &file ) ) {
    log_e("Could not load tune from file");
    return false;
  }
  fireEvent(this, SID_NEW_FILE);
  bool playable = playSongNumber( currenttune->currentsong );
  return playable;
  */
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
  log_d("wait");
  uint32_t max_wait_ms = 100;
  uint32_t timed_out  = millis() + max_wait_ms;
  while( ExitTunePlayerLoop ) {
    vTaskDelay(1);
    if( millis() > timed_out ) {
      log_e("Timed out while waiting for tune to exit (a song %s being played)", was_playing ? "was":"was NOT");
      //fireEvent( this, SID_STOP_TRACK );
      break;
    }
  }
  //log_d("stopped");

//   // now purge task queue
//   timed_out  = millis() + max_wait_ms;
//   while(!sid.xQueueIsQueueEmpty()) {
//     sid.clearQueue();
//     if( millis() > timed_out ) {
//       log_e("Timed out while waiting for SID Queue to clear, queue core busy?");
//       fireEvent( this, SID_STOP_TRACK );
//       return;
//     }
//   }

  //sid.clearQueue();

  sid.soundOff();

  if( !was_playing ) {
    log_e("[Ignore event emit] Stop signal sent but player task was not playing!");
  } else {
    fireEvent( this, SID_STOP_TRACK );
    //log_d("stopped");
  }

  //} else {
    //log_v("[Ignore stop] Player task is not running");
  //}
}


/*
void SIDTunesPlayer::stopPlayer()
{
  //log_v("playing stop n:%d/%d\n",1,subsongs);
  sid.soundOff();
  sid.resetsid();
  if(xTunePlayerTaskHandle!=NULL) {
    vTaskDelete(xTunePlayerTaskHandle);
    xTunePlayerTaskHandle=NULL;
    stop_song = true;
  }
  if(xPlayerLoopTaskHandle!=NULL) {
    vTaskDelete(xPlayerLoopTaskHandle);
    xPlayerLoopTaskHandle=NULL;
  }
  vTaskDelay(100);
  playerrunning=false;
  fireEvent(SID_END_PLAY);
  //log_v("playing sop n:%d/%d\n",1,subsongs);
}
*/


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
  __attribute__((unused)) auto csong = currenttune->currentsong;
  currenttune->currentsong = (currenttune->currentsong+1)%currenttune->trackinfo->subsongs;
  __attribute__((unused)) auto nsong = currenttune->currentsong;
  log_d("Current song = %d, next song = %d", csong, nsong );
  return playSongNumber( currenttune->currentsong );
}



bool SIDTunesPlayer::playPrevSongInSid()
{
  __attribute__((unused)) auto csong = currenttune->currentsong;
  if(currenttune->currentsong == 0) {
    currenttune->currentsong = ( currenttune->trackinfo->subsongs-1 );
  } else {
    currenttune->currentsong--;
  }
  __attribute__((unused)) auto psong = currenttune->currentsong;
  log_d("Current song = %d, prev song = %d", csong, psong );
  return playSongNumber( currenttune->currentsong );
}




bool SIDTunesPlayer::playSongNumber( int songnumber )
{
  //sid.soundOff();
  //sid.resetsid();
  stop();

  if( songnumber<0 || songnumber >= getSongsCount() ) {
    log_e("Wrong song number: %d (max=%d)", songnumber, getSongsCount());
    return false;
  }

  if( currenttune->currentsong != songnumber ) {
    currenttune->currentsong = songnumber;
  }

  if( xTunePlayerTaskHandle == NULL ) {
    log_d("[%d] Creating SIDTunePlayerTask (will wait for incoming 'song/track ended' notification)", ESP.getFreeHeap() );
    xTaskCreatePinnedToCore( SIDTunesPlayer::SIDTunePlayerTask, "SIDTunePlayerTask",  4096,  this, SID_AUDIO_PRIORITY, &xTunePlayerTaskHandle, SID_AUDIO_CORE );
    vTaskDelay(100);
  }

  if( xTunePlayerTaskHandle == NULL ) {
    log_e("No player to notify, waiting until player found...");
    while( xTunePlayerTaskHandle == NULL ) { vTaskDelay(1); }
    log_e("Player found!");
  }
  log_d("[NOTIFYING] Sending TunePlay notification to SIDTunePlayerTask (core #%d", xPortGetCoreID() );
  xTaskNotifyGive( xTunePlayerTaskHandle );

  unsigned long startwait = millis();
  unsigned long timeout = 1000;
  while( !isPlaying() && millis()-startwait<timeout ) vTaskDelay(1);
  if( millis()-startwait>=timeout ) log_e("song still not playing after %s ms timeout, returning anyway", timeout );
  fireEvent( this, SID_NEW_TRACK );

  return true;
}



void SIDTunesPlayer::SIDTunePlayerTask(void * parameters)
{
  log_d("[freeheap=%d] Entering SID*Tune*PlayerTask on core %d", ESP.getFreeHeap(), xPortGetCoreID() );
  //xTunePlayerTaskLocker = xTaskGetCurrentTaskHandle();
  SIDTunesPlayer * _player = (SIDTunesPlayer *)parameters;
  _player->TunePlayerTaskRunning = true;
  _player->TunePlayerTaskPlaying = false;

  for(;;) {
    log_d("[WAIT] Blocking core #%d while waiting for tune start notification on xTunePlayerTaskHandle (#%d)...", xPortGetCoreID(), xTunePlayerTaskHandle );
    _player->ExitTunePlayerLoop = false;
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    log_d("[NOTIFIED] Releasing core #%d : got TunePlay notification, now resetting delta duration and looping tune", xPortGetCoreID() );

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
        log_d("play_addr speculated from 0xffff/0xfffe : $%04x", tune->play_addr);
      } else {
        log_d("play_addr speculated from 0x314/0x315 : $%04x", tune->play_addr);
      }
    } else {
      _player->cpuJSR( tune->init_addr, songnumber );
      log_d("play_addr preselected: $%04x", tune->play_addr );
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
    uint32_t now_time = 0;

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
        now_time = millis();
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
    // // wait until task queue is purged
    // uint32_t startWaitQueue = millis();
    // uint32_t notifyEvery = 1000;
    // uint32_t notifyNext = startWaitQueue+notifyEvery;
    // while( !_player->sid.xQueueIsQueueEmpty() ) {
    //   _player->sid.clearQueue();
    //   vTaskDelay(1);
    //   if( millis() < notifyNext ) {
    //     notifyNext = millis()+notifyEvery;
    //     log_d("[wait] _player->queue isn't empty yet (waited %d ms)", notifyEvery);
    //   }
    // }

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
  /*
  if( currenttrack == nullptr ) {
    log_e("No dest holder, halting");
    while(1) vTaskDelay(1);
  }
  if( songinfo == nullptr ) {
    log_e("No source holder, halting");
    while(1) vTaskDelay(1);
  }
  */
  assert(songinfo);

  //currenttrack = songinfo;
  if( currenttune != nullptr ) {
    log_d("Deleting old SIDTune object");
    delete currenttune;
  }

  currenttune = new SIDTune( this, songinfo->filename, songinfo );
  //bool ret = currenttune->getMetaFromFile( &file );
  //currenttrack = currenttune->trackinfo;
  //memcpy( currenttrack, songinfo, sizeof( SID_Meta_t ) );
  return playSID();
}


bool SIDTunesPlayer::playSID()
{

  if( !playSidFile() ) {
    log_e("playSID failed (asked #%d out of %d subsongs)", currenttune->currentsong+1, currenttune->trackinfo->subsongs );
    return false;
  }

  if( xPlayerLoopTaskLocker != NULL ) {
    log_d("SID Playing \\o/");
    //xTaskNotifyGive( xPlayerLoopTaskLocker );
    //vTaskDelay(1);
    vTaskDelay(10);
  } else {
    log_d("SID Playing \\o/ now firing SIDLoopPlayerTask task");
    xTaskCreatePinnedToCore( SIDTunesPlayer::SIDLoopPlayerTask, "SIDLoopPlayerTask", 4096, this, SID_PLAYER_PRIORITY, & xPlayerLoopTaskHandle, SID_PLAYER_CORE );
    vTaskDelay(100);
  }

  playerrunning = true;
  return true;
}



// play one or all songs from a SID file in different modes
void SIDTunesPlayer::SIDLoopPlayerTask(void *param)
{
  bool exitloop = false;
  SIDTunesPlayer *cpu = (SIDTunesPlayer *)param;
  log_d("Entering SID*Loop*PlayerTask");

  cpu->LoopPlayerTaskRunning = true;

  log_d("creating task locker");
  xPlayerLoopTaskLocker = xTaskGetCurrentTaskHandle();

  //cpu->fireEvent( cpu, SID_START_PLAY );

  while( !exitloop ) {

    log_d("[WAIT] Blocking core #%d while waiting for a notification", xPortGetCoreID() );
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    log_d("[NOTIFIED] Releasing core#%d", xPortGetCoreID());

    SIDTune *tune = cpu->currenttune;

    switch( cpu->currentplaymode ) {
      case SID_ALL_SONGS:

        switch( cpu->currentloopmode ) {
          case SID_LOOP_ON:
            if( !cpu->playNextSongInSid() ) {
              log_e("playNextSongInSid failed (asked #%d out of %d subsongs)", tune->currentsong+1, tune->trackinfo->subsongs );
              exitloop = true;
            }
          break;
          case SID_LOOP_RANDOM:
          {
            uint8_t randomSongNumber = rand()%tune->trackinfo->subsongs;
            // TODO: avoid playing the same song twice
            if( !cpu->playSongNumber( randomSongNumber ) ) {
              log_e("Random failed (diced %d out of %d subsongs)", randomSongNumber, tune->trackinfo->subsongs );
              exitloop = true;
            }
          }
          break;
          case SID_LOOP_OFF:
            if ( int(tune->currentsong+1) >= tune->trackinfo->subsongs ) {
              log_d("Exiting playerloop");
              exitloop = true;
            } else {
              if( !cpu->playNextSongInSid() ) {
                log_e("playNextSongInSid failed (asked #%d out of %d subsongs)", tune->currentsong+1, tune->trackinfo->subsongs );
                exitloop = true;
              }
            }
          break;
        }
      break;
      case SID_ONE_SONG:
        log_d("loop mode = SID_ONE_SONG, stopping");
        exitloop = true;
      break;
    }

    if( exitloop ) {
      //cpu->stop();
      cpu->sid.soundOff();
      //cpu->sid.resetsid();
      cpu->playerrunning = false;
      cpu->LoopPlayerTaskRunning = false;
      xPlayerLoopTaskLocker = NULL;
      //cpu->fireEvent( cpu, SID_END_PLAY );
      //log_d("Leaving SID*Loop*PlayerTask");
      cpu->fireEvent( cpu, SID_END_FILE );
      vTaskDelete( NULL );
      return;
    }

    vTaskDelay(1);

  } // end while(!exitloop)

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
    log_d("Allocating %d bytes for song %s", sizeof(SID_Meta_t), path );
    _trackinfo = (SID_Meta_t*)sid_calloc(1, sizeof(SID_Meta_t) );
    needs_free = true;
  } else {
    log_d("Re-using %d bytes for song %s", sizeof(SID_Meta_t), path );
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
          log_i("Extracted durations" );
          songdebug( trackinfo );
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

  tunedebug(this);

  return true;
}






bool SIDTune::getMetaFromFile( fs::File *file, bool close )
{
  if(file == nullptr) {
    log_e("Not a valid file");
    return false;
  } else {
    //log_d("Valid file");
  }

  //log_e("SAFEGUARD");
  //while(1) vTaskDelay(1);

  //assert(file);
  uint8_t stype[5];
  uint16_t flags, version;
  __attribute__((unused)) uint32_t offset;

  if( trackinfo == nullptr ) {
    log_d("[%d] Allocating %d bytes for trackinfo", ESP.getFreeHeap(), sizeof(SID_Meta_t) );
    trackinfo = (SID_Meta_t*)sid_calloc(1, sizeof(SID_Meta_t) );
  } else {
    if( trackinfo->durations != nullptr ) {
      if( needs_free ) {
        log_d("songinfo has durations, freeing");
        free( trackinfo->durations  );
        trackinfo->durations = nullptr;
      } else {
        log_d("songinfo has durations, keeping");
      }
    } else {
      log_d("songinfo has no durations as expected");
    }
  }

  if( needs_free ) {
    log_d("Zeroing trackinfo");
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

  log_d("Will check whether HVSC info needs populating");

  #ifdef _SID_HVSC_FILE_INDEXER_H_
    if( !has_durations && player->MD5Parser != NULL ) {
      //log_d("Fetching HVSC Info");
      snprintf( trackinfo->md5, 33, "%s", Sid_md5::calcMd5( file ) );
      if( close ) file->close();
      if( !player->MD5Parser->getDurations( trackinfo ) ) {
        log_d("[%d] lookup failed for '%s' / '%s' ", ESP.getFreeHeap(), trackinfo->name, trackinfo->md5 );
        free( trackinfo->durations );
        trackinfo->durations = (uint32_t*)sid_calloc(trackinfo->subsongs, sizeof(uint32_t));
        trackinfo->durations[0] = 0;
        for( int i=0; i<trackinfo->subsongs; i++ ) {
          trackinfo->durations[i] = default_song_duration;
        }
        has_durations = true;
      } else {
        log_d("[%d] SID Path lookup success for song '%s' : %d durations extracted", ESP.getFreeHeap(), trackinfo->name, trackinfo->subsongs );
        has_durations = true;
      }
    } else {
      log_d("[%d] NOT Collecting/Checking HVSC Database as per player configuration (see SIDTunesPlayer::MD5Parser)", ESP.getFreeHeap() );
      snprintf( trackinfo->md5, 33, "%s", Sid_md5::calcMd5( file ) );
      if( close ) file->close();
      //snprintf( trackinfo->md5, 33, "%s", "00000000000000000000000000000000" );
    }
  #else
    log_d("NOT Fetching HVSC Info");
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
