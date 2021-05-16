#define SID_CLOCK 25
#define SID_DATA 15
#define SID_LATCH 2
#define SID_CLOCK_PIN 26 // optional if the SID already has an oscillator
#define SID_PLAYER // load the sidPlayer module over the SID library

#include <SID6581.h> // https://github.com/hpwit/SID6581

// use SPIFFS
//#include <SPIFFS.h>
//#define SID_FS SPIFFS

// or SD
//#include <SD.h>
//#define SID_FS SD

// or LittleFS
#include <LITTLEFS.h> // https://github.com/lorol/LITTLEFS
#define SID_FS LITTLEFS

static SIDTunesPlayer *sidPlayer = nullptr;
static SID_Meta_t tmpsonginfo;

std::vector<SID_Meta_t> songList;
size_t songIndex = 0;



void playNextSong( SIDTunesPlayer* player )
{
  if( songIndex < songList.size()-1 ) {
    songIndex++;
  } else {
    songIndex = 0;
  }

  bool playable = player->getInfoFromSIDFile( songList[songIndex].filename/*, &tmpsonginfo*/ );

  if( !playable ) {
    playNextSong( player );
  } else {
    player->playSID( /*&tmpsonginfo*/ );
  }
}


static void eventCallback( SIDTunesPlayer* player, sidEvent event )
{
  switch( event ) {
    case SID_NEW_FILE:
      Serial.printf( "[%d] SID_NEW_FILE: %s\n", ESP.getFreeHeap(), player->getFilename() );
    break;
    case SID_NEW_TRACK:
      Serial.printf( "[%d] SID_NEW_TRACK: %s (%02d:%02d) %d/%d subsongs\n",
        ESP.getFreeHeap(),
        player->getName(),
        player->getCurrentTrackDuration()/60000,
        (player->getCurrentTrackDuration()/1000)%60,
        player->currentsong+1,
        player->subsongs
      );
    break;
    case SID_START_PLAY:
      Serial.printf( "[%d] SID_START_PLAY: %s\n", ESP.getFreeHeap(), player->getFilename() );
    break;
    case SID_END_PLAY:
      Serial.printf( "[%d] SID_END_PLAY: %s\n", ESP.getFreeHeap(), player->getFilename() );
      // eternal loop on all SID tracks
      vTaskDelay(100);
      playNextSong( player );
    break;
    case SID_PAUSE_PLAY:
      Serial.printf( "[%d] SID_PAUSE_PLAY: %s\n", ESP.getFreeHeap(), player->getFilename() );
    break;
    case SID_RESUME_PLAY:
      Serial.printf( "[%d] SID_RESUME_PLAY: %s\n", ESP.getFreeHeap(), player->getFilename() );
    break;
    case SID_END_TRACK:
      Serial.printf("[%d] SID_END_TRACK\n", ESP.getFreeHeap());
    break;
    case SID_STOP_TRACK:
      Serial.printf("[%d] SID_STOP_TRACK\n", ESP.getFreeHeap());
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

  File root = SID_FS.open("/i"); // "/i" for SID with illegal instructions, and "/f" for failing to play
  if(!root){
    Serial.println("- failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.printf("Ignoring DIR: %s\n", file.name() );
    } else {
      if( String( file.name() ).endsWith(".sid" ) ) {
        SID_Meta_t songinfo;
        if( sidPlayer->getInfoFromSIDFile( file.name(), &songinfo ) ) {
          Serial.printf("[+] FILE: %-32s %6d bytes\n", file.name(), file.size() );
          songList.push_back( songinfo );
        } else {
          Serial.printf("[!] FILE: %-32s %6d bytes\n", file.name(), file.size() );
        }
      }
    }
    file = root.openNextFile();
  }

  sidPlayer->setMaxVolume(15); //value between 0 and 15
  sidPlayer->setDefaultDuration( 10000 ); // 10 seconds per song max for this example, comment this out to get full songs

  sidPlayer->setPlayMode( SID_ALL_SONGS ); // applies to subsongs in a track, values = SID_ONE_SONG or SID_ALL_SONGS
  sidPlayer->setLoopMode( SID_LOOP_OFF );  // applies to subsongs in a track, values = SID_LOOP_ON, SID_LOOP_RANDOM or SID_LOOP_OFF

  int randIndex = 0;
  srand(time(NULL));
  int attempts = micros()%47 + 53;

  while( attempts-- > 0 ) {
    srand(time(NULL));
    randIndex = random(songList.size());
  }

  bool playable = sidPlayer->getInfoFromSIDFile( songList[randIndex].filename/*, &tmpsonginfo*/ );

  if( !playable ) {
    Serial.println("SID File is not readable!");
    while(1);
  }

  //sidPlayer->playSID(); //it will play all songs in loop
  sidPlayer->playSID( /*&tmpsonginfo*/ ); //it will play all songs in loop

}


void loop()
{

}
