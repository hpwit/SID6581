#define SID_CLOCK 25
#define SID_DATA 15
#define SID_LATCH 2
#define SID_CLOCK_PIN 26 // optional if the SID already has an oscillator
#define SID_PLAYER // load the sidPlayer module over the SID library
#define SID_INSTRUMENTS // load the instruments along with the sidPlayer
// pattern used by songdebug()
#define SID_SONG_DEBUG_PATTERN "Fname: %s\nName: %s\nauthor: %s\nmd5: %s\npub: %s\nsub/start: %d/%d\nDurations: "


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


static MD5Archive HSVC =
{
  "local", // just a fancy name
  "local"  // dotfile name will be derivated from that
};

static MD5FileConfig MD5Config =
{
  &SID_FS,          // SD, SPIFFS, LittleFS, Whatever filesystem
  &HSVC,            // High Voltage SID Collection info, not needed for this demo but required by the object
  "/s",             // path to the folder containing SID Files
  "/md5",           // path to the folder containing md5 stuff (where the indexes are created)
  "/md5/soundlength.md5", // full path to the md5 file
  "/md5/sl.idx",    // path to the indexed version of the md5 file
  MD5_RAW_READ,     // other possible values if using a big md5 file and a SD Card: MD5_INDEX_LOOKUP, MD5_RAINBOW_LOOKUP
  nullptr           // callback function for progress when the init happens, overloaded later
};

static SIDTunesPlayer *sidPlayer = nullptr;

// It's up to the app to maintain the files list
std::vector<String> songListStr; // for storing SID filenames
std::vector<SID_Meta_t> songList; // for storing SID tracks
size_t songIndex = 0; // the current song being played
bool autoplay = false;
bool play_ended = false;

bool playNextSong()
{
  if( songIndex+1 < songListStr.size() ) {
    songIndex++;

    bool playable = false;

    if( songList[songIndex].filename[0] == '\0' ) {
      // store song info in songList cache
      playable = sidPlayer->getInfoFromSIDFile( songListStr[songIndex].c_str(), &songList[songIndex] );
    } else {
      playable = true;
    }

    if( !playable ) {
      return false;
    } else {
      songdebug( &songList[songIndex] );
      return sidPlayer->playSID( &songList[songIndex] );
    }
  }
  songIndex = 0;
  return false;
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
      if( autoplay ) {
        playNextSong();
      } else {
        play_ended = true;
      }
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



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  if(!SID_FS.begin()) {
    Serial.println("Filesystem Mount Failed");
    return;
  }

  sidPlayer = new SIDTunesPlayer( &SID_FS );
  sidPlayer->setEventCallback( eventCallback );

  //sidPlayer->begin(SID_CLOCK,SID_DATA,SID_LATCH);
  sidPlayer->begin( SID_CLOCK, SID_DATA, SID_LATCH, SID_CLOCK_PIN ); //(if you do not have a external clock cicuit)

  sidPlayer->setMD5Parser( &MD5Config ); // warning: using big soundlength.md5 on spiffs can be very slow !

  File root = SID_FS.open("/s");
  if(!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if(!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }
  File file = root.openNextFile();
  // The following loop will go through all the files in the filesystem
  // Do not forget to do "Tools-> ESP32 Scketch data upload"
  while(file) {
    if(file.isDirectory()) {
      Serial.printf("Ignoring DIR: %s\n", file.name() );
    } else {
      if( String( file.name() ).endsWith(".sid" ) ) {
        songListStr.push_back( file.name() );
        SID_Meta_t songinfo;
        songList.push_back( songinfo );
        Serial.printf("[+] FILE: %-32s %6d bytes\n", file.name(), file.size() );
      }
    }
    file = root.openNextFile();
  }

  if( songListStr.size() == 0 ) {
    Serial.println("No SID files have been found, go to 'Tools-> ESP32 Scketch data upload'");
    Serial.println("Halting");
    while(1) vTaskDelay(1);
  }


  // The Player only loads one SID at a time, so play mode and loop mode only apply to the subsongs.
  sidPlayer->setPlayMode( SID_ALL_SONGS ); // applies to subsongs in a track, values = SID_ONE_SONG or SID_ALL_SONGS
  sidPlayer->setLoopMode( SID_LOOP_OFF );  // applies to subsongs in a track, values = SID_LOOP_ON, SID_LOOP_RANDOM or SID_LOOP_OFF

  if( !playNextSong() ) {
    Serial.printf("[Error, %s is not playable ?", songListStr[songIndex].c_str() );
    Serial.println("Halting");
    while(1) vTaskDelay(1);
  }

  Serial.println();
  Serial.printf("author:%s\n", sidPlayer->getAuthor());
  Serial.printf("published:%s\n", sidPlayer->getPublished());
  Serial.printf("name:%s\n", sidPlayer->getName());
  Serial.printf("nb tunes:%d default tunes:%d\n", sidPlayer->getNumberOfTunesInSid(), sidPlayer->getDefaultTuneInSid() );

  unsigned long now = millis();
  do {
    Serial.printf("Waveforms: [ 0x%02x, 0x%02x, 0x%02x ]\n",
      sidPlayer->sid.getWaveForm(0),
      sidPlayer->sid.getWaveForm(1),
      sidPlayer->sid.getWaveForm(2)
    );
  } while( now+5000 > millis() );


  delay(5000);
  Serial.println("Playing next song in SID");
  sidPlayer->playNextSongInSid();
  delay(5000);

  playNextSong();
  delay(5000);
  Serial.println();
  Serial.printf("author:%s\n", sidPlayer->getAuthor());
  Serial.printf("published:%s\n", sidPlayer->getPublished());
  Serial.printf("name:%s\n", sidPlayer->getName());
  Serial.printf("nb tunes:%d default tunes:%d\n", sidPlayer->getNumberOfTunesInSid(), sidPlayer->getDefaultTuneInSid() );

  //Serial.println("Halting");
  while(!play_ended) {
    vTaskDelay(1);
  }

  //sidPlayer->setDefaultDuration( 10000 ); // 10s per song max
  autoplay = true;
  playNextSong();

  //delay(5000);
  Serial.println("Pause the song");
  sidPlayer->togglePause();
  delay(4000);
  Serial.println("restart the song");
  sidPlayer->togglePause();
  delay(3000);
  Serial.println("low volume ");
  sidPlayer->sid.setMaxVolume(3);
  delay(3000);
  Serial.println("medium");
  sidPlayer->sid.setMaxVolume(7);
  delay(3000);
  Serial.println("hi volume");
  sidPlayer->sid.setMaxVolume(15);
  delay(6000);

}


void loop()
{


}
