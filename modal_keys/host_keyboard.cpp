#include "host_keyboard.h"

#if defined(USE_LEGACY_HID_API)

// ===========================================================================
// Legacy path: Arduino AVR core <= 1.6.5
// ---------------------------------------------------------------------------
// These cores exposed a global raw-report sender and shipped a built-in HID
// report descriptor in which report id 2 is the boot keyboard, so there is
// nothing to set up -- we just forward to the original function.
// ===========================================================================

#include <USBAPI.h>

void HostKeyboardBegin() {}

void SendKeyReport(uint8_t buf[8]) {
    HID_SendReport(2, buf, 8);
}

#else

// ===========================================================================
// Modern path: Arduino AVR core >= 1.6.6 (PluggableUSB)
// ---------------------------------------------------------------------------
// HID_SendReport() is gone. Raw reports now go through the HID() singleton,
// but unlike the old core it ships with NO descriptor, so the host would not
// recognise us as a keyboard. We register our own boot-keyboard descriptor
// (report id 2 -- identical to the one the old core hard-coded) and then send
// raw reports through HID().SendReport(2, ...). Both the old free function and
// SendReport() prepend the report-id byte and then emit the payload verbatim,
// so this is wire-for-wire identical to the original code.
// ===========================================================================

#include <HID.h>

// Standard 8-byte boot-keyboard report descriptor, report id 2. Copied from
// the keyboard collection the pre-1.6.6 AVR core hard-coded in HID.cpp.
static const uint8_t _bootKeyboardDescriptor[] PROGMEM = {
    0x05, 0x01,  // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,  // USAGE (Keyboard)
    0xa1, 0x01,  // COLLECTION (Application)
    0x85, 0x02,  //   REPORT_ID (2)
    0x05, 0x07,  //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,  //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,  //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,  //   LOGICAL_MINIMUM (0)
    0x25, 0x01,  //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,  //   REPORT_SIZE (1)
    0x95, 0x08,  //   REPORT_COUNT (8)
    0x81, 0x02,  //   INPUT (Data,Var,Abs)   -- buf[0]: 8 modifier bits
    0x95, 0x01,  //   REPORT_COUNT (1)
    0x75, 0x08,  //   REPORT_SIZE (8)
    0x81, 0x03,  //   INPUT (Cnst,Var,Abs)   -- buf[1]: reserved byte
    0x95, 0x06,  //   REPORT_COUNT (6)
    0x75, 0x08,  //   REPORT_SIZE (8)
    0x15, 0x00,  //   LOGICAL_MINIMUM (0)
    0x25, 0x65,  //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,  //   USAGE_PAGE (Keyboard)
    0x19, 0x00,  //   USAGE_MINIMUM (no event)
    0x29, 0x65,  //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,  //   INPUT (Data,Ary,Abs)   -- buf[2..7]: six key codes
    0xc0         // END_COLLECTION
};

void HostKeyboardBegin() {
    static HIDSubDescriptor node(_bootKeyboardDescriptor, sizeof(_bootKeyboardDescriptor));
    HID().AppendDescriptor(&node);
}

void SendKeyReport(uint8_t buf[8]) {
    HID().SendReport(2, buf, 8);
}

#endif
