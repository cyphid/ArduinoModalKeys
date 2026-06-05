// ===========================================================================
// Target selection
// ---------------------------------------------------------------------------
// The modal-keys engine (keymap.cpp / helpers.cpp) is board independent: it
// only manipulates raw 8-byte boot-keyboard buffers. The two pieces that *are*
// board specific are reading the attached keyboard (USB host) and presenting a
// keyboard to the PC (USB device). Both supported targets are detected here:
//
//   * MODAL_TARGET_RP2040  -- Adafruit Feather RP2040 with USB Type A Host.
//                             USB host via Pico-PIO-USB, device via TinyUSB.
//   * MODAL_TARGET_LEONARDO -- Arduino Leonardo + USB Host Shield 2.0.
//                              USB host via the shield, device via PluggableUSB.
//
// LEONARDO is kept as a legacy alias for the original manual switch.
// ===========================================================================
#if defined(ARDUINO_ARCH_RP2040)
    #define MODAL_TARGET_RP2040
#else
    #define MODAL_TARGET_LEONARDO
    #define LEONARDO
#endif

#include "modal_keys.h"
#include "keymap.h"
#include "helpers.h"
#include "host_keyboard.h"

#if defined(MODAL_TARGET_RP2040)
// Adafruit Feather RP2040 USB Host: USB host runs on the second core through
// the Pico-PIO-USB library; the keyboard we present to the PC is TinyUSB
// (see host_keyboard.cpp). Requires "USB Stack: Adafruit TinyUSB".
#include "pio_usb.h"
#include "Adafruit_TinyUSB.h"
#else
#include <SoftwareSerial.h>
#include <USBAPI.h>
#include <hidboot.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#include <SPI.h>
#include <EEPROM.h>
#endif
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
void HandleKeyboardReport(uint8_t buf[8]);

// *******************************************************************************************
// Variables
// *******************************************************************************************

bool WriteToLog = true;
bool SendOutput = true;

uint8_t InputBuffer[8] = { 0 };
uint8_t OutputBuffer[8] = { 0 };

// *******************************************************************************************
// Engine entry point (shared by both targets)
// *******************************************************************************************

// Feed one raw 8-byte boot-keyboard report from the attached keyboard into the
// modal-keys engine. Both the USB Host Shield parser (Leonardo) and the TinyUSB
// host callback (RP2040) call this with the same buffer layout.
void HandleKeyboardReport(uint8_t buf[8]) {
    // On error (phantom / rollover) - return
    if (buf[2] == 1) return;

    uint8_t outbuf[8] = { 0 };
    TransformBuffer(buf, outbuf);

    CopyBuf(buf, InputBuffer);
    TransitionToState(outbuf);
}

// *******************************************************************************************
// Per-target USB host (reading the attached keyboard)
// *******************************************************************************************

#if defined(MODAL_TARGET_LEONARDO)

// USB Host Shield 2.0 renamed its HID class to USBHID (to stop it colliding
// with the Arduino core's HID object). Match the current
// KeyboardReportParser::Parse signature so this still overrides it.
class KbdRptParser : public KeyboardReportParser
{
protected:
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);
KbdRptParser Prs;

void KbdRptParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
    HandleKeyboardReport(buf);
}

#elif defined(MODAL_TARGET_RP2040)

// USB host object, served by the Pico-PIO-USB bit-banged port on core1.
Adafruit_USBH_Host USBHost;

// The Feather RP2040 USB Host board variant defines these; provide the
// board's documented defaults as a fallback (D+ = GPIO16, D- = GPIO17,
// 5V enable = GPIO18, active high).
#ifndef PIN_USB_HOST_DP
#define PIN_USB_HOST_DP 16
#endif
#ifndef PIN_5V_EN
#define PIN_5V_EN 18
#endif
#ifndef PIN_5V_EN_STATE
#define PIN_5V_EN_STATE 1
#endif

static void rp2040_configure_pio_usb() {
    // Power the USB-A port.
    pinMode(PIN_5V_EN, OUTPUT);
    digitalWrite(PIN_5V_EN, PIN_5V_EN_STATE);

    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = PIN_USB_HOST_DP;
    USBHost.configure_pio_usb(1, &pio_cfg);
}

// TinyUSB host callbacks. These run on core1; they hand each raw report to the
// shared engine, which forwards remapped reports to the device side on core0.
extern "C" {

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const *desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;
    // Only drive boot-style keyboards into the engine.
    if (tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD) {
        if (WriteToLog) Serial.println("Keyboard attached.");
        tuh_hid_receive_report(dev_addr, instance);
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;
    if (WriteToLog) Serial.println("Keyboard detached.");
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const *report, uint16_t len) {
    // The engine speaks the 8-byte boot-keyboard report exclusively.
    if (len == 8) {
        uint8_t buf[8];
        CopyBuf((uint8_t *)report, buf);
        HandleKeyboardReport(buf);
    }
    // Re-arm: ask for the next report.
    tuh_hid_receive_report(dev_addr, instance);
}

} // extern "C"

#endif

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
    // The boot-keyboard modifier byte uses the same bit order as the LCtrl..RGui
    // masks in keys.h, so decode it directly rather than via the USB Host
    // Shield's MODIFIERKEYS union (which is unavailable on the RP2040 target).
    String str = "<";
    str += (mods & LCtrl)  ? "C" : "-";
    str += (mods & LShift) ? "S" : "-";
    str += (mods & LAlt)   ? "A" : "-";
    str += (mods & LGui)   ? "G" : "-";
    str += ".";
    str += (mods & RCtrl)  ? "C" : "-";
    str += (mods & RShift) ? "S" : "-";
    str += (mods & RAlt)   ? "A" : "-";
    str += (mods & RGui)   ? "G" : "-";
    str += ">";
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
#if defined(MODAL_TARGET_RP2040) || defined(LEONARDO)
    SendKeyReport(buf);
#else
    Serial.write(buf, 8);
#endif
}

// *******************************************************************************************
// Arduino main functions
// *******************************************************************************************

#if defined(MODAL_TARGET_LEONARDO)

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

#elif defined(MODAL_TARGET_RP2040)

// On the RP2040 the USB device (the keyboard we present to the PC) is serviced
// on core0, while the USB host (the attached keyboard) is bit-banged on core1.

void setup()
{
    Serial.begin( 115200 );

    InitializeState();

    HostKeyboardBegin();   // bring up the TinyUSB keyboard device
}

void loop()
{
    // The TinyUSB device stack is serviced in the background by the core; the
    // engine is driven entirely by the host report callbacks on core1.
}

void setup1()
{
    // Pico-PIO-USB requires a 120 MHz or 240 MHz CPU clock; the Feather RP2040
    // USB Host board package selects a compatible clock by default.
    rp2040_configure_pio_usb();
    USBHost.begin(1);
}

void loop1()
{
    USBHost.task();
}

#endif

