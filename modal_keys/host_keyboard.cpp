#include "host_keyboard.h"

#if defined(ARDUINO_ARCH_RP2040)

// ===========================================================================
// Raw USB keyboard output for the Adafruit Feather RP2040 USB Host (TinyUSB).
// ---------------------------------------------------------------------------
// The modal-keys engine works on raw 8-byte HID boot-keyboard reports. The
// Adafruit TinyUSB device stack lets us present a standard keyboard interface
// and push those raw bytes straight onto the wire. The report descriptor
// produced by TUD_HID_REPORT_DESC_KEYBOARD() has the boot-keyboard layout
// (1 modifier byte, 1 reserved byte, six key codes), so our buf[8] maps onto
// it verbatim, exactly as it did for the Leonardo.
//
// Requires the Arduino IDE "USB Stack" to be set to "Adafruit TinyUSB".
// ===========================================================================

#include "Adafruit_TinyUSB.h"

static uint8_t const _bootKeyboardDescriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// Report-id 0 (no report id in the descriptor), polled like a normal keyboard.
static Adafruit_USBD_HID usb_hid(_bootKeyboardDescriptor,
                                 sizeof(_bootKeyboardDescriptor),
                                 HID_ITF_PROTOCOL_KEYBOARD, 2, false);

void HostKeyboardBegin() {
    usb_hid.begin();
}

void SendKeyReport(uint8_t buf[8]) {
    // The engine calls this from core1 (inside the USB-host report callback);
    // usb_hid lives on core0. Wait until the device endpoint is ready, then
    // hand over the raw 8-byte boot report (report id 0).
    while (!usb_hid.ready()) {
        yield();
    }
    usb_hid.sendReport(0, buf, 8);
}

#else

// ===========================================================================
// Raw USB keyboard output for the Arduino AVR core (PluggableUSB).
// ---------------------------------------------------------------------------
// The modal-keys engine works directly on raw 8-byte HID boot-keyboard reports
// and needs positional write access to the wire buffer. The core's
// PluggableUSB HID() singleton can send raw reports via SendReport(id, ...),
// but it ships with no descriptor, so the host would not recognise us as a
// keyboard. We register our own boot-keyboard descriptor (report id 2) and then
// send raw reports through HID().SendReport(2, buf, 8).
// ===========================================================================

#include <HID.h>

// Standard 8-byte boot-keyboard report descriptor, report id 2.
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
