// host_keyboard.h
//
// Low-level USB keyboard output for the Leonardo (the device that emulates a
// keyboard towards the host computer).
//
// The modal-keys engine works directly on raw 8-byte HID boot-keyboard
// reports and needs *positional* write access to that buffer:
//
//     buf[0]     modifier bitmask (LCtrl, LShift, ... RGui)
//     buf[1]     reserved (always 0)
//     buf[2..7]  up to six simultaneously-pressed key scan codes
//
// Historically the sketch sent these reports with the Arduino core's
// `HID_SendReport(2, buf, 8)` free function. That function was removed from
// the AVR core in 2015 (the PluggableUSB rework, core >= 1.6.6): the modern
// `Keyboard` library only exposes press()/release()/write() over its own
// private report buffer and gives no positional access to the wire bytes.
//
// This module restores exactly that capability. On a modern core it registers
// its own boot-keyboard report descriptor (report id 2, byte-for-byte the same
// descriptor the old core hard-coded) with the PluggableUSB `HID()` singleton
// and sends raw reports through `HID().SendReport(2, buf, 8)` -- producing
// identical USB traffic to the legacy call. On an old core it falls back to
// the original `HID_SendReport` (define USE_LEGACY_HID_API to select it).

#if !defined(__HOST_KEYBOARD_H_)
#define __HOST_KEYBOARD_H_

#include <Arduino.h>

// Register the keyboard HID interface with the USB stack. Call once from
// setup(). No-op on the legacy code path.
void HostKeyboardBegin();

// Send a raw 8-byte boot-keyboard report to the host. This is the positional
// low-level buffer write the engine depends on.
void SendKeyReport(uint8_t buf[8]);

#endif // __HOST_KEYBOARD_H_
