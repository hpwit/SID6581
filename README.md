This library is to control SID 6581 chips from the 1980s era with an Esp32.
the program allows you to push directly the register to the  SID chip. hence you can program like in the good all times :)
it should work with other mcu as it uses SPI but not tested.
NB: playSIDTunes will only work with WROOVER because I use PSRAM for the moment. all the rest will run with all esp32.
I intend on writing a MIDI translation to SID chip. Hence a Play midi will be availble soon.

look at the schematics for the setup of the shift registers and  MOS 6581

Example to play a SIDtunes
```

#include "SID6581.h"
#include <SD.h>
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27
#include "SPIFFS.h"
SID6581 sid;

void setup() {
// put your setup code here, to run once:
Serial.begin(115200);

sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);

/// if you are using an SD card
//this pin set up is for MY Board
SPI.begin(14,2,15,13); 
if (!SD.begin(13,SPI,80000000)) {
Serial.println("initialization failed!");
//return;
}

//if your file is stored in SPIFF

if(!SPIFFS.begin(true)){
Serial.println("SPIFFS Mount Failed");
return;
}


}

void loop() {

// put your main code here, to run repeatedly:
sid.playSIDTunes(SD,"/yourtunes"); //or sid.playSIDTunes(SPIFF,"/yourtunes");
}
```



PS: to transform the .sid into register commands 

1) I use the fantastic program of Ken Händel
https://haendel.ddns.net/~ken/#_latest_beta_version
```
java -jar jsidplay2_console-4.1.jar --audio LIVE_SID_REG --recordingFilename export_file sid_file

use SID_REG instead of LIVE_SID_REG to keep you latptop quiet


```
2) I use the program traduct_2.c
to compile it for your platform:
```
>gcc traduct_2.c -o traduct
```
to use it
```
./traduct export_file > final_file

```
Put the final_file on a SD card or in your SPIFF

here it goes

Updated 6 March 2020
