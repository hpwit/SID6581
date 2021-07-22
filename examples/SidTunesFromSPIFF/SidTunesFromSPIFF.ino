#define SID_CLOCK 25
#define SID_DATA 15
#define SID_LATCH 2
#define SID_CLOCK_PIN 26 // optional if the SID already has an oscillator
#define SID_PLAYER // load the sidPlayer module over the SID library

#include <SID6581.h> // https://github.com/hpwit/SID6581

// use SPIFFS
#include <SPIFFS.h>
#define SID_FS SPIFFS

// or SD
//#include <SD.h>
//#define SID_FS SD

// or LittleFS
//#include <LITTLEFS.h> // https://github.com/lorol/LITTLEFS
//#define SID_FS LITTLEFS

static SIDTunesPlayer *sidPlayer = nullptr;

#include <vector>
std::vector<String> songListStr; // for storing SID filenames
std::vector<SID_Meta_t> songList; // for storing SID tracks
size_t songIndex = 0; // the current song being played

// Arduino 2.0.0-alpha brutally changed the behaviour of fs::File->name()
const char* fs_file_path( fs::File *file ) {
  #if defined ESP_IDF_VERSION_MAJOR && ESP_IDF_VERSION_MAJOR >= 4
    return file->path();
  #else
    return file->name();
  #endif
}


bool playNextTrack()
{
  if( songIndex+1 < songListStr.size() ) {
    songIndex++;
  } else {
    songIndex = 0;
  }

  bool playable = false;

  if( songList[songIndex].filename[0] == '\0' ) {
    // store song info in songList cache
    playable = sidPlayer->getInfoFromSIDFile( songListStr[songIndex].c_str(), &songList[songIndex] );
  } else {
    playable = true;
  }

  if( !playable ) {
    log_e("Track %s is NOT playable", songListStr[songIndex].c_str() );
    return playNextTrack();
  } else {
    songdebug( &songList[songIndex] );
    return sidPlayer->playSID( &songList[songIndex] );
  }
}


static void eventCallback( SIDTunesPlayer* player, sidEvent event )
{
  switch( event ) {
    case SID_NEW_FILE:
      log_n( "[%d] SID_NEW_FILE: %s (%d songs)\n", ESP.getFreeHeap(), player->getFilename(), player->getSongsCount() );
    break;
    case SID_NEW_TRACK:
      log_n( "[%d] SID_NEW_TRACK: %s (%02dm %02ds) %d/%d subsongs\n",
        ESP.getFreeHeap(),
        player->getName(),
        player->getCurrentTrackDuration()/60000,
        (player->getCurrentTrackDuration()/1000)%60,
        player->getCurrentSong()+1,
        player->getSongsCount()
      );
    break;
    case SID_START_PLAY:
      log_n( "[%d] SID_START_PLAY: #%d from %s\n", ESP.getFreeHeap(), player->getCurrentSong()+1, player->getFilename() );
    break;
    case SID_END_FILE:
      log_n( "[%d] SID_END_FILE: %s\n", ESP.getFreeHeap(), player->getFilename() );
      //if( autoplay ) {
        playNextTrack();
      // } else {
      //   log_n("All %d tracks have been played\n", songList.size() );
      //   while(1) vTaskDelay(1);
      //   play_ended = true;
      // }
    break;
    case SID_END_PLAY:
      log_n( "[%d] SID_END_PLAY: %s\n", ESP.getFreeHeap(), player->getFilename() );
    break;
    case SID_PAUSE_PLAY:
      log_n( "[%d] SID_PAUSE_PLAY: %s\n", ESP.getFreeHeap(), player->getFilename() );
    break;
    case SID_RESUME_PLAY:
      Serial.printf( "[%d] SID_RESUME_PLAY: %s\n", ESP.getFreeHeap(), player->getFilename() );
    break;
    case SID_END_TRACK:
      log_n("[%d] SID_END_TRACK\n", ESP.getFreeHeap());
    break;
    case SID_STOP_TRACK:
      log_n("[%d] SID_STOP_TRACK\n", ESP.getFreeHeap());
    break;
  }
}



void setup()
{
  Serial.begin(115200);

  if( !SID_FS.begin() ) {
    Serial.println("Filesystem mount failed");
    while(1);
  }

  sidPlayer = new SIDTunesPlayer( &SID_FS );
  sidPlayer->setEventCallback( eventCallback );

  if( sidPlayer->begin( SID_CLOCK, SID_DATA, SID_LATCH, SID_CLOCK_PIN ) ) {
    Serial.println("SID Player Begin OK");
  } else {
    Serial.println("SID Player failed to start ... duh !");
    while(1);
  }

  File root = SID_FS.open("/");
  if(!root){
    Serial.println("- failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file) {
    if(file.isDirectory()) {
      Serial.printf("Ignoring DIR: %s\n", file.path() );
    } else {
      if( String( file.path() ).endsWith(".sid" ) ) {
        songListStr.push_back( file.path() );
        SID_Meta_t songinfo;
        songList.push_back( songinfo );
        Serial.printf("[+] FILE: %-32s %6d bytes\n", file.path(), file.size() );
      }
    }
    file = root.openNextFile();
  }

  if( songListStr.size() == 0 ) {
    Serial.println("No SID files have been found, go to 'Tools-> ESP32 Scketch data upload'");
    Serial.println("Halting");
    while(1) vTaskDelay(1);
  }

  Serial.printf("Tracks list has %d items\n", songList.size() );

  sidPlayer->setMaxVolume(15); //value between 0 and 15

  //sidPlayer->setDefaultDuration( 180000 ); // 3mn per song max for this example, comment this out to get full songs
  sidPlayer->setDefaultDuration( 10000 ); // 10s per song max for this example, comment this out to get full songs

  sidPlayer->setPlayMode( SID_ALL_SONGS ); // applies to subsongs in a track, values = SID_ONE_SONG or SID_ALL_SONGS
  sidPlayer->setLoopMode( SID_LOOP_OFF );  // applies to subsongs in a track, values = SID_LOOP_ON, SID_LOOP_RANDOM or SID_LOOP_OFF

  srand(time(NULL));
  songIndex = random(songList.size());

  playNextTrack();

}


void loop()
{

}
