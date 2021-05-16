
SIDTunesPlayer
--------------


Interesting methods:

```C
// Constructors
SIDTunesPlayer( fs::FS *_fs );
SIDTunesPlayer( MD5FileConfig *cfg );
// optional MD5 File parser with its setter
MD5FileParser *MD5Parser = NULL; // for parsing the MD5 Database file
void setMD5Parser( MD5FileConfig *cfg );
// start the SID
bool begin(int clock_pin,int data_pin, int latch );
bool begin(int clock_pin,int data_pin, int latch,int sid_clock_pin);
// load a file
bool getInfoFromSIDFile( const char * path );
// control the player
void setMaxVolume( uint8_t volume );
bool playSID(); // play subsongs inside a SID file
bool playSongNumber( int songnumber ); // play a subsong from the current SID track slot
bool playPrevSongInSid();
bool playNextSongInSid();
void togglePause();
void stop();
// get notifications
inline void setEventCallback(void (*fptr)(SIDTunesPlayer* player, sidEvent event)) { eventCallback = fptr; }
// getters for the current song
uint32_t getElapseTime();
uint32_t getCurrentTrackDuration();
char    *getName();
char    *getPublished();
char    *getAuthor();
char    *getFilename();
uint8_t *getSidType();
```



Usage example:
--------------



```C

#include <SD.h>
#include <FS.h>

#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27
#define SID_PLAYER
#include <SID6581.h>

SIDRegisterPlayer * player;

void myCallback(  sidEvent event )
{
  switch( event ) {
    case SID_NEW_TRACK:
      Serial.printf( "New track: %s\n",player->getFilename() );
    break;
    case SID_START_PLAY:
      Serial.printf( "Start play: %s\n",player->getFilename() );
    break;
    case SID_END_PLAY:
      Serial.printf( "stopping play: %s\n",player->getFilename() );
    break;
    case SID_PAUSE_PLAY:
      Serial.printf( "pausing play: %s\n",player->getFilename() );
    break;
    case SID_RESUME_PLAY:
      Serial.printf( "resume play: %s\n",player->getFilename() );
    break;
    case SID_END_SONG:
      Serial.println("End of track");
    break;
  }
}

void setup()
{

  Serial.begin(115200);

  if( !SD.begin() ) {
    Serial.println("SD Card failed");
    while(1);
  }

  player = new SIDTunesPlayer( &SD );

  player->begin(SID_CLOCK,SID_DATA,SID_LATCH);

  player->setEventCallback(myCallback);

  if( !player->getInfoFromSIDFile( "/C64Music/MUSICIANS/H/Hubbard_Rob/Synth_Sample_III.sid" ) ) {
    Serial.println("SID File is not readable!");
    while(1);
  }

  player->setMaxVolume(7); //value between 0 and 15
  player->playSID(); //it will play all songs in loop
}

void loop()
{

}

```
