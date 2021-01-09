

MD5FileParser
-------------

This class is a MD5 File indexer for High Voltage SID Collection (HVSC).

[The HVSC MD5 file](https://hvsc.c64.org/download/C64Music/DOCUMENTS/Songlengths.md5) contains
the song lengths and a MD5 digest for every SID file in the collection.

The purpose of this class is to provide three means of searching in this file.


1) `MD5_INDEX_LOOKUP` : Path-based lookups (by building a folder=>offset database)
2) `MD5_RAINBOW_LOOKUP` : Hash-based lookups (by spreading MD5 hashes into folder/files)
3) `MD5_RAW_READ` : Raw-based loopup (by reading the file until a match is found)

Raw-based lookup is the slowest of all but requires no index building, use only on reduced
version of the MD5 File, may be useful for small SPIFF based appliances.
Average lookup time (*): 3000ms

Path-based lookup is especially useful with the complete SID Collection from HVSC.
Average lookup time (*): 180ms

Hash-based lookup is probably the best choice for custom playlist that do not follow the HVSC
folder structure, but keep in mind the index building process is very slow for this one.
Average lookup time (*): 120ms

(*) The average lookup time is based on readings made on a fast Micro SD (SanDisk Ultra) using
the latest Songlengths.md5 file from HVSC, which contains ~50000 songs and weighs 4.3Mb:



Usage
-----


- Get the latest [HCSC archive](https://hvsc.c64.org) and unzip it on the SD Card.
- Check that the "C64Music" folder is at the top level.
- Open the Serial console
- Run this sketch


```C

#include <FS.h>
#include <SD.h>

#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27

#define SID_PLAYER
#include <SID6581.h>


static MD5Archive HSVC =
{
  "HVSC 74",    // just a fancy name
  "HVSC-74"     // !! a dotfile name will be derivated from that
};

static MD5FileConfig MD5Config =
{
  &SD,                                   // The filesystem (reminder: SPIFFS is limited to 32 chars paths)
  &HVSC,                                 // High Voltage SID Collection meta info
  "/C64Music",                           // Folder where the HVSC unzipped contents can be found
  "/md5",                                // Folder where MD5 pre-parsed files will be spreaded (SD only)
  "/C64Music/DOCUMENTS/Songlengths.md5", // Where the MD5 file can be found (a custom/shorter file may be specified)
  "/md5/Songlengths.md5.idx",            // Where the Songlengths file will be indexed (SD only)
  MD5_INDEX_LOOKUP,                      // one of MD5_RAW_READ (SPIFFS), MD5_INDEX_LOOKUP (SD), or MD5_RAINBOW_LOOKUP (SD)
  nullptr                                // callback function for progress when the cache init happens, can be overloaded later
};


SIDTunesPlayer *player;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  player = new SIDTunesPlayer( &MD5Config );

  player->begin( SID_CLOCK, SID_DATA, SID_LATCH );

  if(!SD.begin()){
    Serial.println("SD Mount Failed");
    while(1);
  }

  if( !player->getInfoFromSIDFile( "/C64Music/MUSICIANS/H/Hubbard_Rob/Synth_Sample_III.sid" ) ) {
    Serial.println("SID File is not readable!");
    while(1);
  }

  player->play();

}

void loop()
{

}



```
