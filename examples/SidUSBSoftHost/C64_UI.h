#define tft M5.Lcd

#define C64_DARKBLUE      0x4338ccU
#define C64_LIGHTBLUE     0xC9BEFFU

#define bgcolor C64_DARKBLUE
#define fgcolor C64_LIGHTBLUE


static int fontWidth, fontHeight;
bool kbChanged = false;


struct keyItemPos
{
  char key;
  uint8_t x;
  uint8_t y;
  bool pushed;
};

//  2 3   5 6 7   9 0
// q w e r t y u i o p
//                      => q2w3er5t6y7ui9o0p
//  s d   g h j
// z x c v b n m
//                      => zsxdcvgbhnjm

keyItemPos kbbp[] =
{
 { 'z', 0,  3, true },
 { 's', 1,  2, true },
 { 'x', 2,  3, true },
 { 'd', 3,  2, true },
 { 'c', 4,  3, true },
 { 'v', 6,  3, true },
 { 'g', 7,  2, true },
 { 'b', 8,  3, true },
 { 'h', 9,  2, true },
 { 'n', 10, 3, true },
 { 'j', 11, 2, true },
 { 'm', 12, 3, true },
 { 'q', 0,  1, true },
 { '2', 1,  0, true },
 { 'w', 2,  1, true },
 { '3', 3,  0, true },
 { 'e', 4,  1, true },
 { 'r', 6,  1, true },
 { '5', 7,  0, true },
 { 't', 8,  1, true },
 { '6', 9,  0, true },
 { 'y', 10, 1, true },
 { '7', 11, 0, true },
 { 'u', 12, 1, true },
 { 'i', 14, 1, true },
 { '9', 15, 0, true },
 { 'o', 16, 1, true },
 { '0', 17, 0, true },
 { 'p', 18, 1, true }
};

static size_t kbdlen = 0;
static size_t kbdwidth = 0;
static size_t keywidth = 0;
static size_t keyheight = 0;
const size_t keymargin = 1;
const size_t kbmarginx = 6;
const size_t kbmarginy = 50;

static TFT_eSprite *keyNoteSprite = new TFT_eSprite( &tft );


bool drawKey( char key, bool pushed )
{
  bool drawn = false;
  for( int i=0; i<kbdlen; i++ ) {
    if( key == kbbp[i].key ) {
      //if( kbbp[i].pushed == pushed ) return false;
      drawn = true;
      kbbp[i].pushed = pushed;
      int xpos = (kbbp[i].x * keywidth) + kbmarginx;
      int ypos = (kbbp[i].y * keyheight) + kbmarginy;
      if( kbbp[i].y > 1 ) {
        xpos += keywidth*3;
        ypos += keyheight/2;
      }
      if( kbbp[i].y % 2 == 0 ) {
        // black key
      } else {
        // white key
      }
      keyNoteSprite->setTextColor( pushed ? TFT_RED : TFT_WHITE, !pushed ? TFT_BLACK : TFT_WHITE );
      keyNoteSprite->drawString(String(key).c_str(), xpos, ypos );
      keyNoteSprite->drawRect( xpos-1, ypos-1, fontWidth+2, fontHeight+2, TFT_BLACK );
    }
  }
  return drawn;
}


bool drawKeys()
{
  bool drawn = false;
  for( int i=0; i<kbdlen; i++ ) {
    if( drawKey( kbbp[i].key, kbbp[i].pushed ) ) {
      drawn = true;
    }
  }
  return drawn;
}



void setupUI()
{

  tft.setRotation(2);
  tft.fillScreen( bgcolor );
  tft.setTextSize(1);
  tft.setFont( &Font8x8C64 );
  tft.setTextColor( fgcolor, bgcolor );

  // fortunately this is a monotype font :-)
  fontWidth  = tft.fontHeight(&Font8x8C64);
  fontHeight = tft.fontHeight(&Font8x8C64);

  const char* l1 = "** ESP32 SID! **";
  const char* l2 = "READY.";

  // draw the decoration text
  tft.setTextDatum( TC_DATUM );
  tft.drawString(l1, tft.width()/2, fontHeight );
  tft.setTextDatum( TL_DATUM );
  tft.drawString(l2, 0, fontHeight*3 );

  log_w("heap free before sprite [%dx%d] init (core #%d): %d", tft.width(), tft.height(), xPortGetCoreID(), ESP.getFreeHeap() );

  //keyNoteSprite->setPsram(false);
  //keyNoteSprite->setColorDepth( 8 );
  void* sptr = keyNoteSprite->createSprite( tft.width(), tft.height() );
  if( !sptr )  {
    log_e("Can't create sprite, aborting");
    while(1) vTaskDelay(1);
  }
  keyNoteSprite->fillSprite( TFT_ORANGE ); // will be used as transparent color

  keyNoteSprite->setTextSize(1);
  keyNoteSprite->setFont( &Font8x8C64 );
  keyNoteSprite->setTextDatum( TL_DATUM );

  kbdlen = sizeof(kbbp) / sizeof( keyItemPos );
  kbdwidth = strlen( "q2w3er5t6y7ui9o0p" );
  keyheight = kbdwidth*.75;
  keywidth = (tft.width() / kbdwidth)-1;
  Serial.printf("Keyboard has %d keys, will use %d pixels per key\n", kbdlen, keywidth );
  for( int i=0; i<kbdlen; i++ ) {
    Serial.printf("Enabling key %s\n", String(kbbp[i].key).c_str() );
    drawKey( kbbp[i].key, false );
  }
  keyNoteSprite->pushSprite( 0, 0, TFT_ORANGE );
}



void usbTicker()
{
  if( kbChanged ) {
    if( drawKeys() ) {
      keyNoteSprite->pushSprite( 0, 0, TFT_ORANGE );
    }
    kbChanged = false;
  }
}
