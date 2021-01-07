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
static TaskHandle_t xPlayerTaskLocker = NULL;
static TaskHandle_t xPlayerLoopTaskLocker = NULL;
// task handles
static TaskHandle_t xPlayerTaskHandle = NULL;
static TaskHandle_t xPlayerLoopTaskHandle = NULL;




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
  init();
  #ifdef BOARD_HAS_PSRAM
    psramInit();
  #endif
  currenttrack = (SID_Meta_t*)sid_calloc( 1, sizeof( SID_Meta_t ) );
  currenttrack->durations = nullptr;
  volume = 15;
  if( mem == NULL ) {
    log_d("We create the memory buffer for C64 memory");
    log_d("[%d] largest free block=%d bytes)\n", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
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
  if( xPlayerTaskHandle     != NULL ) vTaskDelete( xPlayerTaskHandle );
  if( xPlayerTaskLocker     != NULL ) vTaskDelete( xPlayerTaskLocker );
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
void     SIDTunesPlayer::setMaxVolume( uint8_t vol ) { volume=vol; }
void     SIDTunesPlayer::setDefaultDuration(uint32_t duration) { default_song_duration=duration; }
void     SIDTunesPlayer::setLoopMode(loopmode mode) { currentloopmode=mode; }
void     SIDTunesPlayer::setPlayMode(playmode mode) { currentplaymode=mode; }

bool     SIDTunesPlayer::isPlaying() { return !stop_song; }
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
    log_e("File %s does not exist\n", path);
    return false;
  }
  File file = fs->open( path );
  if(!file) {
    log_e("Unable to open %s\n", path);
    return false;
  }

  if( songinfo == nullptr ) {
    log_w("[%d] Allocating %d bytes for songinfo", ESP.getFreeHeap(), sizeof(SID_Meta_t) );
    songinfo = (SID_Meta_t*)sid_calloc(1, sizeof(SID_Meta_t) );
  } else {
    if( songinfo->durations != nullptr ) {
      log_d("songinfo has durations, freeing");
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
    log_d("Unsupported SID Model?:  0: %d  1: %d  2: %d  in file %s",
      preferred_SID_model[0],
      preferred_SID_model[1],
      preferred_SID_model[2],
      path
    );
  }

  snprintf( songinfo->md5, 33, "%s", Sid_md5::calcMd5( file ) );

  file.close();

  #ifdef _SID_HVSC_FILE_INDEXER_H_
  if( MD5Parser != NULL ) {
    if( !MD5Parser->getDurationsFromSIDPath( songinfo ) ) {
      if( MD5Parser->getDurationsFastFromMD5Hash( songinfo ) ) {
        log_d("[%d] Hash lookup success after SID Path lookup failed for '%s' ", ESP.getFreeHeap(), songinfo->name );
      } else {
        log_e("[%d] Both Hash lookup and SID Path lookup failed for '%s' / '%s' ", ESP.getFreeHeap(), songinfo->name, songinfo->md5 );
      }
    } else {
      log_d("[%d] SID Path lookup success for song '%s' ", ESP.getFreeHeap(), songinfo->name );
    }
  }
  #endif
  //if( playable ) songdebug( songinfo );
  return playable;
}



bool SIDTunesPlayer::playSidFile()
{
  if( currenttrack == nullptr ) {
    log_e("Load a track first !!");
    return false;
  }

  log_i("playing file:%s\n", song->filename );

  if(mem == NULL) {
    log_w("[%d] allocating %d bytes\n", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
    mem = (uint8_t*)sid_calloc( 0x10000, 1 );
    if(mem == NULL) {
      log_e("not enough memory\n");
      is_error = true;
      //error_type = NOT_ENOUGH_MEMORY;
      return false;
    }
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
  size_t g = file.read(&mem[load_addr], file.size()-(data_offset+2) );
  log_d("Read %d bytes when %d were expected (max=%d, current offset=%d)", g, file.size()-(data_offset+2), file.size(), data_offset );
  //memset(&mem[load_addr+g],0xea,0x10000-load_addr+g);
  for(int i=0;i<32;i++) {
    uint8_t fm;
    file.seek( 0x12+(i>>3) );
    fm = file.read();
    speedsong[31-i]= (fm & (byte)pow(2,7-i%8))?1:0;
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
    //if( currenttrack->durations == nullptr ) {
      log_w("[%d] Fetching durations", ESP.getFreeHeap() );
      if( !MD5Parser->getDuration( currenttrack ) ) {
        log_e("Can't get duration for this song (md5:%s):-(", currenttrack->md5 );
        currenttrack->durations[0] = 0;
      }
    //} else {
    //  log_w("[%d] Durations don't need parsing", ESP.getFreeHeap() );
    //}
  } else {
    log_w("[%d] No MD5 Parser", ESP.getFreeHeap() );
  }
  #endif

  //fireEvent(SID_NEW_FILE);

  return playSongNumber( currentsong );
}



bool SIDTunesPlayer::playNextSongInSid()
{
  stop();
  currentsong = int(currentsong+1)%subsongs;
  return playSongNumber(currentsong);
}



bool SIDTunesPlayer::playPrevSongInSid()
{
  stop();
  if(currentsong == 0) {
    currentsong = ( subsongs-1 );
  } else {
    currentsong--;
  }
  return playSongNumber(currentsong);
}





void SIDTunesPlayer::stop()
{
  sid.soundOff();
  sid.resetsid();
  if(xPlayerTaskHandle!=NULL) {
    vTaskDelete(xPlayerTaskHandle);
    stop_song = true;
    xPlayerTaskHandle=NULL;
  }
  vTaskDelay(100);
  fireEvent( this, SID_STOP_TRACK );
}


/*
void SIDTunesPlayer::stopPlayer()
{
  //log_v("playing stop n:%d/%d\n",1,subsongs);
  sid.soundOff();
  sid.resetsid();
  if(xPlayerTaskHandle!=NULL) {
    vTaskDelete(xPlayerTaskHandle);
    xPlayerTaskHandle=NULL;
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
  if( xPlayerTaskHandle ==NULL ) {
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
    if( xPausePlayTaskLocker != NULL ) xTaskNotifyGive( xPausePlayTaskLocker );
  }
}



bool SIDTunesPlayer::playSongNumber( int songnumber )
{
  //sid.soundOff();
  sid.resetsid();
  currentoffset = 0;
  previousoffset = 0;
  currentsong = songnumber;
  if( xPlayerTaskHandle != NULL ) {
    vTaskDelete( xPlayerTaskHandle );
    xPlayerTaskHandle = NULL;
    stop_song = true;
  }
  if( songnumber<0 || songnumber>=subsongs ) {
    log_e("Wrong song number");
    return false;
  }
  log_i( "playing song n:%d/%d\n", (songnumber+1), subsongs );

  cpuReset();

  if( play_addr == 0 ) {
    cpuJSR( init_addr, 0 ); //0
    play_addr = (mem[0x0315] << 8) + mem[0x0314];
    cpuJSR( init_addr, songnumber );
    if(play_addr==0) {
      play_addr=(mem[0xffff] << 8) + mem[0xfffe];
      mem[1]=0x37;
    }
  } else {
    cpuJSR( init_addr, songnumber );
  }

  //sid.soundOn();
  xTaskCreatePinnedToCore( SIDTunesPlayer::SIDTunePlayerTask, "SIDTunePlayerTask",  4096,  this, SID_CPU_TASK_PRIORITY, &xPlayerTaskHandle, SID_CPU_CORE);
  delay(200);

  if(currenttrack->durations[currentsong]==0) {
    song_duration = default_song_duration;
    log_w("[%d] Playing task (4Kb) with default song duration %d ms", ESP.getFreeHeap(), song_duration);
  } else {
    song_duration = currenttrack->durations[currentsong];
    log_d("[%d] Playing track %d with md5 database song duration %d ms", ESP.getFreeHeap(), currentsong, song_duration);
  }
  delta_song_duration=0;

  fireEvent( this, SID_NEW_TRACK );
  if( xPlayerTaskLocker != NULL ) xTaskNotifyGive( xPlayerTaskLocker );
  return true;
}



void SIDTunesPlayer::SIDTunePlayerTask(void * parameters)
{
  for(;;) {
    //serial_command element;
    xPlayerTaskLocker  = xTaskGetCurrentTaskHandle();
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    SIDTunesPlayer * cpu = (SIDTunesPlayer *)parameters;
    cpu->delta_song_duration = 0;
    cpu->stop_song=false;
    cpu->start_time=millis();
    uint32_t now_time = 0;
    //uint32_t pause_time = 0;
    while( 1 && !cpu->stop_song ) {
      cpu->totalinframe += cpu->cpuJSR( cpu->play_addr, 0 );
      cpu->wait += cpu->waitframe;
      cpu->frame = true;
      int nRefreshCIA = (int)( ((19650 * (cpu->getmem(0xdc04) | (cpu->getmem(0xdc05) << 8)) / 0x4c00) + (cpu->getmem(0xdc04) | (cpu->getmem(0xdc05) << 8))  )  /2 );
      if ((nRefreshCIA == 0) /*|| (cpu->speedsong[cpu->currentsong]==0)*/)
        nRefreshCIA = 19650;
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
        xPausePlayTaskLocker = xTaskGetCurrentTaskHandle();
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        xPausePlayTaskLocker=NULL;
        cpu->sid.soundOn();
        cpu->start_time = millis();
      }
      vTaskDelay(1);

    }

    cpu->stop_song = true;
    cpu->sid.soundOff();
    cpu->fireEvent( cpu, SID_END_TRACK );

    if( xPlayerLoopTaskLocker != NULL ) xTaskNotifyGive( xPlayerLoopTaskLocker );
    vTaskDelay(1);
  }
}





bool SIDTunesPlayer::playSID()
{
  if( xPlayerLoopTaskHandle != NULL ) {
    log_w("player loop task already running");
    vTaskDelete( xPlayerLoopTaskHandle );
    playerrunning = false;
  }
  xTaskCreatePinnedToCore( SIDTunesPlayer::SIDLoopPlayerTask, "SIDLoopPlayerTask", 4096, this, 1, & xPlayerLoopTaskHandle, SID_PLAYER_CORE);
  vTaskDelay(200);
  playerrunning = true;
  return true;
}




void SIDTunesPlayer::SIDLoopPlayerTask(void *param)
{
  bool playerloop = true;
  SIDTunesPlayer *cpu = (SIDTunesPlayer *)param;

  if( !cpu->playSidFile() ) {
    log_w("playSidFile failed (asked #%d out of %d subsongs)", cpu->currentsong, cpu->subsongs );
    playerloop = false;
  }

  cpu->fireEvent( cpu, SID_START_PLAY );

  while( playerloop ) {

    xPlayerLoopTaskLocker = xTaskGetCurrentTaskHandle();
    log_d("wait for a mux");
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

    switch( cpu->currentplaymode ) {
      case SID_ALL_SONGS:

        switch( cpu->currentloopmode ) {
          case SID_LOOP_ON:
            if( ! cpu->playNextSongInSid() ) {
              log_w("playNextSongInSid failed (asked #%d out of %d subsongs)", cpu->currentsong+1, cpu->subsongs );
              playerloop = false;
            }
          break;
          case SID_LOOP_RANDOM:
          {
            uint8_t randomSongNumber = rand()%cpu->subsongs;
            // TODO: avoid playing the same song twice
            cpu->stop();
            if( !cpu->playSongNumber( randomSongNumber ) ) {
              log_w("Random failed (diced %d out of %d subsongs)", randomSongNumber, cpu->subsongs );
              playerloop = false;
            }
          }
          break;
          case SID_LOOP_OFF:
            if ( int(cpu->currentsong+1) >= cpu->subsongs ) {
              playerloop = false;
            } else {
              if( ! cpu->playNextSongInSid() ) {
                log_w("playNextSongInSid failed (asked #%d out of %d subsongs)", cpu->currentsong+1, cpu->subsongs );
                playerloop = false;
              }
            }
          break;
        }
      break;
      case SID_ONE_SONG:
        log_w("loop mode = SID_ONE_SONG, stopping");
        playerloop = false;
      break;
    }

    log_d("got the mux !");
    //log_v("%s", "we do exit the take");
    xPlayerLoopTaskLocker = NULL;

  }

  cpu->stop();
  cpu->playerrunning = false;
  cpu->fireEvent( cpu, SID_END_PLAY );
  log_e("Leaving SIDLoopPlayerTask");
  xPlayerLoopTaskHandle = NULL;
  vTaskDelete( NULL );

}




