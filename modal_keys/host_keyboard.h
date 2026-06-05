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
// This module restores that capability on the modern core by registering its
// own boot-keyboard report descriptor (report id 2) with the PluggableUSB
// `HID()` singleton and sending raw reports through `HID().SendReport(2, buf,
// 8)`.

#if !defined(__HOST_KEYBOARD_H_)
#define __HOST_KEYBOARD_H_

#include <Arduino.h>

// Register the keyboard HID interface with the USB stack. Call once from
// setup().
void HostKeyboardBegin();

// Send a raw 8-byte boot-keyboard report to the host. This is the positional
// low-level buffer write the engine depends on.
void SendKeyReport(uint8_t buf[8]);

#endif // __HOST_KEYBOARD_H_
