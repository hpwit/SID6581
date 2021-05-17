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

#include "SID_Player.h"

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
  if( currenttrack != nullptr ) {
    if( currenttrack->durations != nullptr ) free( currenttrack->durations );
    free( currenttrack );
  }
}


SIDTunesPlayer::SIDTunesPlayer( fs::FS *_fs ) : fs(_fs)
{
  // init(); // dafuq is this ?
  #ifdef BOARD_HAS_PSRAM
    psramInit();
  #endif
  mux = xSemaphoreCreateMutex();
  currenttrack = (SID_Meta_t*)sid_calloc( 1, sizeof( SID_Meta_t ) );
  currenttrack->durations = nullptr;
  volume = 15;
  if( mem == NULL ) {
    log_d("We create the memory buffer for C64 memory");
    log_d("[%d] largest free block=%d bytes)", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
    mem = (uint8_t*)sid_calloc( 0x10000, 1 );
    if( mem == NULL ) {
      log_e("Failed to allocate %d bytes for SID memory, the app WILL crash", 0x10000 );
      return;
    }
  }
  log_w("SID Player Init done");
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
  return sid.begin( clock_pin, data_pin, latch, sid_clock_pin );
}


bool SIDTunesPlayer::begin(int clock_pin,int data_pin, int latch )
{
  return sid.begin( clock_pin, data_pin, latch );
}


void SIDTunesPlayer::kill()
{
  stop();
  if( xTunePlayerTaskHandle     != NULL ) vTaskDelete( xTunePlayerTaskHandle );
  if( xTunePlayerTaskLocker     != NULL ) vTaskDelete( xTunePlayerTaskLocker );
  if( xPlayerLoopTaskLocker != NULL ) vTaskDelete( xPlayerLoopTaskLocker );
  if( xPlayerLoopTaskHandle != NULL ) vTaskDelete( xPlayerLoopTaskHandle );
}


void SIDTunesPlayer::reset()
{
  kill(); // kill tasks first
  delay(100);

  if( currenttrack != nullptr ) {
    if( currenttrack->durations != nullptr ) free( currenttrack->durations );
    free( currenttrack );
  }

  currenttrack = (SID_Meta_t*)sid_calloc( 1, sizeof( SID_Meta_t ) );
  currenttrack->durations = nullptr;

  if( mem!=NULL ) {
    free( mem );
    mem = (uint8_t*)sid_calloc( 0x10000, 1 );
  } else {
    memset( mem, 0, 0x10000 );
  }
}



// *****************************
// Getters/Setters

void     SIDTunesPlayer::fireEvent( SIDTunesPlayer* player, sidEvent event) { if (eventCallback) (*eventCallback)(player, event); }
void     SIDTunesPlayer::setMaxVolume( uint8_t vol ) { volume=vol;  sid.setMaxVolume(vol); }
void     SIDTunesPlayer::setDefaultDuration(uint32_t duration) { default_song_duration=duration; }
void     SIDTunesPlayer::setLoopMode(loopmode mode) { currentloopmode=mode; }
void     SIDTunesPlayer::setPlayMode(playmode mode) { currentplaymode=mode; }

bool     SIDTunesPlayer::isPlaying() { return TunePlayerTaskPlaying; }
bool     SIDTunesPlayer::isRunning() { return playerrunning; }

loopmode SIDTunesPlayer::getLoopMode() { return currentloopmode; }
playmode SIDTunesPlayer::getPlayMode() { return currentplaymode; }

uint32_t SIDTunesPlayer::getDefaultDuration() {      return default_song_duration; }
uint32_t SIDTunesPlayer::getElapseTime() {           return delta_song_duration; }
uint32_t SIDTunesPlayer::getCurrentTrackDuration() { return song_duration; }

uint8_t  *SIDTunesPlayer::getSidType() {   return (uint8_t*)sidtype; }
char     *SIDTunesPlayer::getName() {      return (char*)currenttrack->name; }
char     *SIDTunesPlayer::getPublished() { return (char*)currenttrack->published; }
char     *SIDTunesPlayer::getAuthor() {    return (char*)currenttrack->author; }
char     *SIDTunesPlayer::getFilename() {  return (char*)currenttrack->filename; }

uint8_t  SIDTunesPlayer::getNumberOfTunesInSid() { return subsongs; }
uint8_t  SIDTunesPlayer::getCurrentTuneInSid() {   return currentsong; }
uint8_t  SIDTunesPlayer::getDefaultTuneInSid() {   return startsong; }


// *********************************************************
// Player controls

bool SIDTunesPlayer::getInfoFromSIDFile( const char * path )
{
  return getInfoFromSIDFile( path, currenttrack );
}


bool SIDTunesPlayer::getInfoFromSIDFile( const char * path, SID_Meta_t *songinfo )
{
  uint8_t stype[5];
  if( !fs->exists( path ) ) {
    log_e("File %s does not exist", path);
    return false;
  }
  //log_e("Opening FS from core #%d", xPortGetCoreID() );
  File file = fs->open( path );
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
    /*
    if( !MD5Parser->getDurationsFromSIDPath( songinfo ) ) {
      if( MD5Parser->getDurationsFastFromMD5Hash( songinfo ) ) {
        log_d("[%d] Hash lookup success after SID Path lookup failed for '%s' ", ESP.getFreeHeap(), songinfo->name );
      } else {
        log_e("[%d] Both Hash lookup and SID Path lookup failed for '%s' / '%s' ", ESP.getFreeHeap(), songinfo->name, songinfo->md5 );
      }
    }*/
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
}



bool SIDTunesPlayer::playSidFile()
{

  if( TunePlayerTaskRunning ) {
    if( paused || TunePlayerTaskPlaying ) {
      log_w("Stopping the current song");
      stop();
    }
  }

  if( currenttrack == nullptr ) {
    log_e("Load a track first !!");
    return false;
  }

  log_d("playing file:%s", currenttrack->filename );

  if(mem == NULL) {
    log_e("Error mem init was missed");
    return false;
    /*
    log_w("[%d] allocating %d bytes", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
    mem = (uint8_t*)sid_calloc( 0x10000, 1 );
    if(mem == NULL) {
      log_e("not enough memory");
      is_error = true;
      //error_type = NOT_ENOUGH_MEMORY;
      return false;
    }
    */
  } else {
    memset( mem, 0x0, 0x10000 );
  }

  /*
  memset( name, 0, 32 );
  memset( author, 0, 32 );
  memset( published, 0, 32 );
  memset( currentfilename, 0, sizeof(currentfilename) );
  */
  memset( sidtype, 0, 5 );

  // TODO check semaphore before using SPI

  //log_e("Accessing FS from core #%d", xPortGetCoreID() );

  if( !fs->exists( currenttrack->filename ) ) {
    log_e("File %s does not exist", currenttrack->filename );
    //getcurrentfile = currentfile;
    return false;
  }

  fs::File file = fs->open( currenttrack->filename );
  if(!file) {
    log_e("Unable to open file %s", currenttrack->filename );
    //getcurrentfile = currentfile;
    return false;
  }

  file.read( sidtype, 4 );

  if(sidtype[0]!=0x50 || sidtype[1]!=0x53 || sidtype[2]!=0x49 || sidtype[3]!=0x44) {
    log_e("File type:%s not supported yet", sidtype );
    //getcurrentfile = currentfile;
    file.close();
    return false;
  }

  file.seek(7);
  data_offset = file.read();

  uint8_t not_load_addr[2];
  file.read(not_load_addr,2);

  init_addr  = file.read()*256;
  init_addr += file.read();

  play_addr  = file.read()*256;
  play_addr += file.read();

  file.seek( 15);
  subsongs = file.read();

  file.seek(17);
  startsong = file.read();

  file.seek(21);
  speed = file.read();

  /*
  file.seek(22);
  file.read(name,32);

  file.seek(0x36);
  file.read(author,32);

  file.seek(0x56);
  file.read(published,32);
  */

  if( speed == 0 )
    nRefreshCIAbase = 38000;
  else
    nRefreshCIAbase = 19950;

  file.seek(data_offset);
  load_addr = file.read()+(file.read()<<8);

  // CODE SMELL: should read file.size() - current offset
  __attribute__((unused))
  size_t g = file.read(&mem[load_addr], file.size()/*-(data_offset+2)*/ );
  log_d("Read %d bytes when %d were expected (max=%d, current offset=%d)", g, file.size()-(data_offset+2), file.size(), data_offset );
  //memset(&mem[load_addr+g],0xea,0x10000-load_addr+g);
  for(int i=0;i<32;i++) {
    uint8_t fm;
    file.seek( 0x12+(i>>3) );
    fm = file.read();
    speedsong[31-i] = (fm & (byte)pow(2,7-i%8))?1:0;
    if( speedsong[31-i] != 0 ) {
      log_d("Speedsong[%d] = %d", 31-i, speedsong[31-i] );
    }
  }
  //snprintf( currentfilename, 256, "%s", currenttrack->filename );
  //getcurrentfile = currentfile;
  //fireEvent(SID_START_PLAY);
  if( startsong > 0 ) {
    currentsong = startsong - 1;
  } else {
    log_e("Invalid startsong value: 0 (should be at least 1), forcing to 0");
    currentsong = 0;
  }

  file.close();
  #ifdef _SID_HVSC_FILE_INDEXER_H_
  if( MD5Parser != NULL ) {
    if( currenttrack->durations == nullptr ) {
      log_d("[%d] Fetching durations", ESP.getFreeHeap() );
      if( !MD5Parser->getDuration( currenttrack ) ) {
        log_e("Can't get duration for this song (md5:%s):-(", currenttrack->md5 );
        free( currenttrack->durations );
        currenttrack->durations = (uint32_t*)sid_calloc(1, sizeof(uint32_t));
        currenttrack->durations[0] = 0;
      }
    } else {
      log_d("[%d] Durations don't need parsing", ESP.getFreeHeap() );
    }
  } else {
    log_v("[%d] No MD5 Parser", ESP.getFreeHeap() );
    if( currenttrack->durations == nullptr ) {
      currenttrack->durations = (uint32_t*)sid_calloc(1, sizeof(uint32_t));
    }
    currenttrack->durations[0] = 0;
  }
  #endif

  //fireEvent(SID_NEW_FILE);

  return playSongNumber( currentsong );
}



bool SIDTunesPlayer::playNextSongInSid()
{
  //stop();
  currentsong = int(currentsong+1)%subsongs;
  return playSongNumber( currentsong );
}



bool SIDTunesPlayer::playPrevSongInSid()
{
  //stop();
  if(currentsong == 0) {
    currentsong = ( subsongs-1 );
  } else {
    currentsong--;
  }
  return playSongNumber(currentsong);
}





void SIDTunesPlayer::stop()
{
  if( xTunePlayerTaskHandle != NULL ) {

    if( TunePlayerTaskPlaying ) {
      if( paused ) {
        togglePause();
      }
      ExitTunePlayerLoop = true;
      while( ExitTunePlayerLoop ) {
        vTaskDelay(1);
      }
      sid.soundOff();
      sid.resetsid();

      fireEvent( this, SID_STOP_TRACK );
    } else {
      log_e("[Cannot stop] Player task is not playing");
    }
  } else {
    log_e("[Cannot stop] Player task is not running");
  }
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
    if( xPausePlayTaskLocker != NULL ) xTaskNotifyGive( xPausePlayTaskLocker ); // send "unpause" notification
  }
}



bool SIDTunesPlayer::playSongNumber( int songnumber )
{
  //sid.soundOff();
  //sid.resetsid();

  if( TunePlayerTaskPlaying ) {
    //log_w("Marking the current song as stopped");
    //stop();
    //TunePlayerTaskPlaying = false;
    //paused = false;
    //vTaskDelay(10);
  }

  currentoffset = 0;
  previousoffset = 0;
  currentsong = songnumber;
  delta_song_duration = 0;

  if( songnumber<0 || songnumber>=subsongs ) {
    log_e("Wrong song number");
    return false;
  }

  cpuReset();

  if( play_addr == 0 ) { // no song selected or first song
    cpuJSR( init_addr, 0 );
    play_addr = (mem[0x0315] << 8) + mem[0x0314];
    cpuJSR( init_addr, songnumber );
    if(play_addr==0) { // one song only ?
      play_addr=(mem[0xffff] << 8) + mem[0xfffe];
      mem[1]=0x37;
    }
  } else {
    cpuJSR( init_addr, songnumber );
  }
  log_d("Play_addr = $%04x, Init_addr = $%04x (song %d/%d), clockspeed=%d, songspeed=%d", play_addr, init_addr, songnumber+1, subsongs, speed, speedsong[songnumber]);

  //sid.soundOn();
  if( xTunePlayerTaskHandle != NULL ) {
    //     log_d("[%d] Killing SIDTunePlayerTask", ESP.getFreeHeap() );
    //     vTaskDelete( xTunePlayerTaskHandle );
    //     xTunePlayerTaskLocker = NULL;
    //     xTunePlayerTaskHandle = NULL;
    //     TunePlayerTaskPlaying = false;
    //     TunePlayerTaskRunning = false;
    //     vTaskDelay(10);
  } else {

    log_d("[%d] Firing SIDTunePlayerTask (will wait for incoming notification)", ESP.getFreeHeap() );
    xTaskCreatePinnedToCore( SIDTunesPlayer::SIDTunePlayerTask, "SIDTunePlayerTask",  2048,  this, SID_CPU_TASK_PRIORITY, &xTunePlayerTaskHandle, SID_CPU_CORE);
    //delay(200);
  }

  if((int)currenttrack->durations[currentsong]<=0 || MD5Parser == NULL ) {
    song_duration = default_song_duration;
    log_w("[%d] Playing task (4Kb) with default song duration %d ms", ESP.getFreeHeap(), song_duration);
  } else {
    song_duration = currenttrack->durations[currentsong];
    log_d("[%d] Playing track %d with md5 database song duration %d ms", ESP.getFreeHeap(), currentsong, song_duration);
  }
  //delta_song_duration=0;

  fireEvent( this, SID_NEW_TRACK );

  if( xTunePlayerTaskLocker != NULL ) {
    log_d("[%d] Sending notification to xTunePlayerTaskLocker", ESP.getFreeHeap() );
    xTaskNotifyGive( xTunePlayerTaskLocker );
  } else {
    log_e("No player to notify, halting");
    while(1) { vTaskDelay(1); }
  }
  return true;
}



void SIDTunesPlayer::SIDTunePlayerTask(void * parameters)
{
  log_d("[%d] Entering SID*Tune*PlayerTask", ESP.getFreeHeap() );
  xTunePlayerTaskLocker = xTaskGetCurrentTaskHandle();
  SIDTunesPlayer * cpu = (SIDTunesPlayer *)parameters;
  cpu->TunePlayerTaskRunning = true;

  for(;;) {
    log_d("Waiting for xTaskNotifyGive on xTunePlayerTaskLocker (#%d)...", xTunePlayerTaskLocker );
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    log_d("Got TunePlay notification, now resetting delta duration and looping tune");
    //cpu->sid.soundOn();
    cpu->delta_song_duration = 0;
    cpu->start_time = millis();
    uint32_t now_time = 0;
    //uint32_t pause_time = 0;
    cpu->TunePlayerTaskPlaying = true;
    cpu->ExitTunePlayerLoop = false;

    while( cpu->TunePlayerTaskPlaying == true && cpu->ExitTunePlayerLoop==false ) {

      cpu->totalinframe += cpu->cpuJSR( cpu->play_addr, 0 );
      cpu->wait += cpu->waitframe;
      cpu->frame = true;
      int nRefreshCIA = (int)( ((19954 * (cpu->getmem(0xdc04) | (cpu->getmem(0xdc05) << 8)) / 0x4c00) + (cpu->getmem(0xdc04) | (cpu->getmem(0xdc05) << 8))  )  /2 );
      if ((nRefreshCIA == 0) /*|| (cpu->speedsong[cpu->currentsong]==0)*/)
        nRefreshCIA = 19954;
      cpu->waitframe = nRefreshCIA;

      if( cpu->song_duration > 0 ) {
        now_time = millis();
        cpu->delta_song_duration += now_time - cpu->start_time;
        cpu->start_time = now_time;
        if( cpu->delta_song_duration >= cpu->song_duration ) {
          break;
        }
      }

      if( cpu->paused ) {
        cpu->sid.soundOff();
        cpu->TunePlayerTaskPlaying = false;
        xPausePlayTaskLocker = xTaskGetCurrentTaskHandle();
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY ); // wait for a 'unpause' notification
        xPausePlayTaskLocker = NULL;
        cpu->sid.soundOn();
        cpu->start_time = millis();
        cpu->TunePlayerTaskPlaying = true;
      }
      vTaskDelay(1);
    }

    log_d("Track ended");
    cpu->sid.soundOff();
    cpu->TunePlayerTaskPlaying = false;
    cpu->ExitTunePlayerLoop    = false;
    cpu->fireEvent( cpu, SID_END_TRACK );

    //xTaskNotifyGive( xTunePlayerTaskLocker );

    if( xPlayerLoopTaskLocker != NULL ) {
      log_d("Notifying PlayerLoopTask");
      xTaskNotifyGive( xPlayerLoopTaskLocker );
    } else {
      //log_e("No xPlayerLoopTaskLocker to notify, halting");
      //while(1) vTaskDelay(1);
      //xTunePlayerTaskLocker = NULL;
      //cpu->TunePlayerTaskRunning = false;
      //vTaskDelete(NULL);
    }
  }
}


bool SIDTunesPlayer::playSID(SID_Meta_t *songinfo )
{
  if( currenttrack == nullptr ) {
    log_e("No dest holder, halting");
    while(1) vTaskDelay(1);
  }
  if( songinfo == nullptr ) {
    log_e("No source holder, halting");
    while(1) vTaskDelay(1);
  }
  currenttrack = songinfo;
  //memcpy( currenttrack, songinfo, sizeof( SID_Meta_t ) );
  return playSID();
}


bool SIDTunesPlayer::playSID()
{

  if( !playSidFile() ) {
    log_e("playSID failed (asked #%d out of %d subsongs)", currentsong, subsongs );
    return false;
  }

  if( xPlayerLoopTaskLocker != NULL ) {
    log_d("Notifying existing SIDLoopPlayerTask");
    xTaskNotifyGive( xPlayerLoopTaskLocker );
    vTaskDelay(1);
  } else {
    log_d("Firing SIDLoopPlayerTask task");
    xTaskCreatePinnedToCore( SIDTunesPlayer::SIDLoopPlayerTask, "SIDLoopPlayerTask", 4096, this, SID_PLAYER_TASK_PRIORITY, & xPlayerLoopTaskHandle, SID_PLAYER_CORE );
    vTaskDelay(1);
  }

  playerrunning = true;
  return true;
}




void SIDTunesPlayer::SIDLoopPlayerTask(void *param)
{
  bool exitloop = false;
  SIDTunesPlayer *cpu = (SIDTunesPlayer *)param;
  log_d("Entering SID*Loop*PlayerTask");

  cpu->LoopPlayerTaskRunning = true;

  log_d("creating task locker");
  xPlayerLoopTaskLocker = xTaskGetCurrentTaskHandle();

  cpu->fireEvent( cpu, SID_START_PLAY );

  while( !exitloop ) {

    log_d("LoopPlayerTask waiting for a notification");
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

    switch( cpu->currentplaymode ) {
      case SID_ALL_SONGS:

        switch( cpu->currentloopmode ) {
          case SID_LOOP_ON:
            if( !cpu->playNextSongInSid() ) {
              log_e("playNextSongInSid failed (asked #%d out of %d subsongs)", cpu->currentsong+1, cpu->subsongs );
              exitloop = true;
            }
          break;
          case SID_LOOP_RANDOM:
          {
            uint8_t randomSongNumber = rand()%cpu->subsongs;
            // TODO: avoid playing the same song twice
            if( !cpu->playSongNumber( randomSongNumber ) ) {
              log_e("Random failed (diced %d out of %d subsongs)", randomSongNumber, cpu->subsongs );
              exitloop = true;
            }
          }
          break;
          case SID_LOOP_OFF:
            if ( int(cpu->currentsong+1) >= cpu->subsongs ) {
              log_d("Exiting playerloop");
              exitloop = true;
            } else {
              if( !cpu->playNextSongInSid() ) {
                log_e("playNextSongInSid failed (asked #%d out of %d subsongs)", cpu->currentsong+1, cpu->subsongs );
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
      cpu->stop();
      cpu->sid.soundOff();
      cpu->playerrunning = false;
      cpu->LoopPlayerTaskRunning = false;
      xPlayerLoopTaskLocker = NULL;
      cpu->fireEvent( cpu, SID_END_PLAY );
      log_d("Leaving SID*Loop*PlayerTask");
      vTaskDelete( NULL );
    }

  } // end while(!exitloop)

}




