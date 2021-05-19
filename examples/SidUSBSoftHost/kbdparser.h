
enum keyToMidiEvents
{
  onKeyDown,
  onKeyUp,
  onControlKeysChanged,
  onKeyPressed,
  onKeyReleased
};


typedef void (*eventEmitter)(uint8_t mod, uint8_t key, keyToMidiEvents evt );


class KbdRptParser : public KeyboardReportParser
{
  public:
    void setEventHandler( eventEmitter cb ) { sendEvent = cb; };
    void PrintKey(uint8_t mod, uint8_t key);
    char lastkey[2] = {0,0};

  protected:
    void OnControlKeysChanged(uint8_t before, uint8_t after);
    void OnKeyDown	(uint8_t mod, uint8_t key);
    void OnKeyUp	(uint8_t mod, uint8_t key);
    void OnKeyPressed(uint8_t key);
    void OnKeyReleased(uint8_t key);
    eventEmitter sendEvent = nullptr;
};


void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
  if( sendEvent ) sendEvent( mod, key, onKeyUp );
  uint8_t c = OemToAscii(mod, key);
  if (c)
    OnKeyReleased(c);
}


void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
  if( sendEvent ) sendEvent( mod, key, onKeyDown );
  uint8_t c = OemToAscii(mod, key);

  if (c)
    OnKeyPressed(c);
}


void KbdRptParser::OnKeyReleased(uint8_t key)
{
  if( sendEvent ) sendEvent( 0, key, onKeyReleased );
}


void KbdRptParser::OnKeyPressed(uint8_t key)
{
  Serial.printf("ASCII(0x%02x): ", key);
  Serial.println((char)key);
  if( sendEvent ) sendEvent( 0, key, onKeyPressed );
  lastkey[0] = (char)key;
};


void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after)
{
  if( sendEvent ) sendEvent( before, after, onControlKeysChanged );

  MODIFIERKEYS beforeMod;
  *((uint8_t*)&beforeMod) = before;

  MODIFIERKEYS afterMod;
  *((uint8_t*)&afterMod) = after;

  if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl) {
    Serial.println("LeftCtrl changed");
  }
  if (beforeMod.bmLeftShift != afterMod.bmLeftShift) {
    Serial.println("LeftShift changed");
  }
  if (beforeMod.bmLeftAlt != afterMod.bmLeftAlt) {
    Serial.println("LeftAlt changed");
  }
  if (beforeMod.bmLeftGUI != afterMod.bmLeftGUI) {
    Serial.println("LeftGUI changed");
  }

  if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl) {
    Serial.println("RightCtrl changed");
  }
  if (beforeMod.bmRightShift != afterMod.bmRightShift) {
    Serial.println("RightShift changed");
  }
  if (beforeMod.bmRightAlt != afterMod.bmRightAlt) {
    Serial.println("RightAlt changed");
  }
  if (beforeMod.bmRightGUI != afterMod.bmRightGUI) {
    Serial.println("RightGUI changed");
  }
}


void KbdRptParser::PrintKey(uint8_t m, uint8_t key)
{
  MODIFIERKEYS mod;
  *((uint8_t*)&mod) = m;
  Serial.print((mod.bmLeftCtrl   == 1) ? "C" : " ");
  Serial.print((mod.bmLeftShift  == 1) ? "S" : " ");
  Serial.print((mod.bmLeftAlt    == 1) ? "A" : " ");
  Serial.print((mod.bmLeftGUI    == 1) ? "G" : " ");

  Serial.print(" >");
  Serial.printf("0x%02x", key );
  Serial.print("< ");

  Serial.print((mod.bmRightCtrl   == 1) ? "C" : " ");
  Serial.print((mod.bmRightShift  == 1) ? "S" : " ");
  Serial.print((mod.bmRightAlt    == 1) ? "A" : " ");
  Serial.println((mod.bmRightGUI    == 1) ? "G" : " ");
};
