#define LEONARDO

#include "modal_keys.h"
#include "keymap.h"
#include "helpers.h"
#include "host_keyboard.h"

#include <SoftwareSerial.h>
#include <USBAPI.h>
#include <hidboot.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#include <SPI.h>
#include <EEPROM.h>
#endif


// *******************************************************************************************
// Forward declarations
// (The IDE's automatic prototype generation misses these, so declare them explicitly.)
// *******************************************************************************************

bool TransitionToState(uint8_t newbuf[8]);
void SendState(uint8_t buf[8]);
void PrintState(uint8_t inBuf[8], uint8_t outBuf[8], bool outputChanged);
void PressKey(RichKey key);
void SendKeysToHost(uint8_t buf[8]);

// *******************************************************************************************
// Types
// *******************************************************************************************

class KbdRptParser : public KeyboardReportParser
{
protected:
    // USB Host Shield 2.0 renamed its HID class to USBHID (to stop it
    // colliding with the Arduino core's HID object). Match the current
    // KeyboardReportParser::Parse signature so this still overrides it.
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

// *******************************************************************************************
// Variables
// *******************************************************************************************

bool WriteToLog = true;
bool SendOutput = true;

USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);
KbdRptParser Prs;

uint8_t InputBuffer[8] = { 0 };
uint8_t OutputBuffer[8] = { 0 };

// *******************************************************************************************
// Parse
// *******************************************************************************************

void KbdRptParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
    // On error - return
    if (buf[2] == 1) return;

    uint8_t outbuf[8] = { 0 };
    TransformBuffer(buf, outbuf);

    CopyBuf(buf, prevState.bInfo);
    CopyBuf(buf, InputBuffer);
    TransitionToState(outbuf);
}

// *******************************************************************************************
// Helper Functions
// *******************************************************************************************

// returns true if a new state was transmitted
bool TransitionToState(uint8_t newbuf[8]) {
    if (EqualBuffers(newbuf, OutputBuffer)) { // no need to run transition if states are already equal
        PrintState(InputBuffer, OutputBuffer, false);
        return false;
    }

    uint8_t oldbuf[8] = { 0 };
    CopyBuf(OutputBuffer, oldbuf);

    // do released keys
    uint8_t releaseKeys[8] = { 0 };
    if (KeyIntersection(oldbuf, newbuf, releaseKeys)){
        releaseKeys[0] = oldbuf[0];
        SendState(releaseKeys);
    }

    // do released mods
    uint8_t releaseMods[8] = { 0 };
    if (ModIntersection(oldbuf, newbuf, releaseMods)){
        CopyKeys(releaseKeys, releaseMods);
        SendState(releaseMods);
    }

    // do pressed mods
    uint8_t pressMods[8] = { 0 };
    if (newbuf[0] != releaseMods[0]){
        pressMods[0] = newbuf[0];
        CopyKeys(releaseKeys, pressMods);
        SendState(pressMods);
    }

    // do pressed keys
    if (!EqualKeys(newbuf, releaseKeys)){
        SendState(newbuf);
    }
    return true;
}

void SendState(uint8_t buf[8]) {
    CopyBuf(buf, OutputBuffer);
    PrintState(InputBuffer, OutputBuffer, true);
    if (SendOutput){
        delay(1);
        SendKeysToHost(OutputBuffer);
    }
}



// ****************************************************************************
// Logging
// ****************************************************************************

String KeyToHexString(uint8_t key) {
  int num_nibbles = 2;
  String out = "";
  do {
          char v = 48 + (((key >> (num_nibbles - 1) * 4)) & 0x0f);
          if(v > 57) v += 7;
          out += v;
  } while(--num_nibbles);
  return out;
}

String ModifiersToString(uint8_t mods) {
    MODIFIERKEYS mod;
    *((uint8_t*)&mod) = mods;
    String str = "<" +
    String((mod.bmLeftCtrl   == 1) ? "C" : "-") +
    String((mod.bmLeftShift  == 1) ? "S" : "-") +
    String((mod.bmLeftAlt    == 1) ? "A" : "-") +
    String((mod.bmLeftGUI    == 1) ? "G" : "-") +
    "." +
    String((mod.bmRightCtrl   == 1) ? "C" : "-") +
    String((mod.bmRightShift  == 1) ? "S" : "-") +
    String((mod.bmRightAlt    == 1) ? "A" : "-") +
    String((mod.bmRightGUI    == 1) ? "G" : "-") +
    ">";
    return str;
}

String KeyToString(uint8_t key) {
    if (key) {
        return KeyToHexString(key);
    } else {
        return "__";
    }
}

/* shared */ String RichKeyToString(RichKey key) {
    String modStr = ModifiersToString(key.mods);
    String keyStr = KeyToString(key.key);
    return modStr + keyStr;
}

/* shared */ void Log(String text){
    if (!WriteToLog) return;
    Serial.println(text);
}

String BufferToString(uint8_t buf[8]) {
    String out = "";
    out += ModifiersToString(buf[0]);
    out += ModifiersToString(buf[1]);
    for (uint8_t i = 2; i < 8; i++) {
        out += (" " + KeyToString(buf[i]));
    }
    return out;
}

void PrintState(uint8_t inBuf[8], uint8_t outBuf[8], bool outputChanged) {
    if (!WriteToLog) return;
    Serial.print(GetStateString());
    Serial.print(BufferToString(inBuf));
    if (outputChanged){
        Serial.print("  ==>  ");
        Serial.print(BufferToString(outBuf));
    }
    Serial.println();
}

void PressKey(RichKey key){
    uint8_t buf[8];
    CopyBuf(OutputBuffer, buf);
    MergeKeyIntoBuffer(key, buf, true);
    TransitionToState(buf);
}

/* shared */ void PressAndReleaseKey(RichKey key){
    uint8_t current_buf[8];
    CopyBuf(OutputBuffer, current_buf);

    PressKey(key);
    TransitionToState(current_buf);
}

inline void SendKeysToHost (uint8_t buf[8])
{
#ifdef LEONARDO
    SendKeyReport(buf);
#else
    Serial.write(buf, 8);
#endif
}

// *******************************************************************************************
// Arduino main functions
// *******************************************************************************************

void setup()
{
    InitializeState();

    HostKeyboardBegin();

    Serial.begin( 115200 );

    if (Usb.Init() == -1 && WriteToLog)
        Serial.println("OSC did not start.");

    delay( 200 );

    HidKeyboard.SetReportParser(0, (HIDReportParser*)&Prs);
}

void loop()
{
    Usb.Task();
}

