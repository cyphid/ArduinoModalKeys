// host_keyboard.h
//
// Low-level USB keyboard output: the device side that emulates a keyboard
// towards the host computer. This is the one piece of the project that is
// genuinely board specific, so it is abstracted behind two functions that
// every supported target implements.
//
// The modal-keys engine works directly on raw 8-byte HID boot-keyboard
// reports and needs *positional* write access to that buffer:
//
//     buf[0]     modifier bitmask (LCtrl, LShift, ... RGui)
//     buf[1]     reserved (always 0)
//     buf[2..7]  up to six simultaneously-pressed key scan codes
//
// Two targets are supported, selected automatically at compile time:
//
//   * Arduino Leonardo (AVR, ATmega32u4) + USB Host Shield 2.0.
//     Historically the sketch sent reports with the Arduino core's
//     `HID_SendReport(2, buf, 8)` free function. That function was removed
//     from the AVR core in 2015 (the PluggableUSB rework, core >= 1.6.6): the
//     modern `Keyboard` library only exposes press()/release()/write() over
//     its own private report buffer and gives no positional access to the
//     wire bytes. This module restores that capability by registering its own
//     boot-keyboard report descriptor (report id 2) with the PluggableUSB
//     `HID()` singleton and sending raw reports through
//     `HID().SendReport(2, buf, 8)`.
//
//   * Adafruit Feather RP2040 with USB Type A Host (product 5723).
//     Here the device side is provided by the Adafruit TinyUSB library: an
//     `Adafruit_USBD_HID` keyboard interface is created and raw 8-byte reports
//     are pushed with `usb_hid.sendReport(0, buf, 8)`. (The USB *host* side --
//     reading the attached keyboard over the Pico-PIO-USB bit-banged port --
//     lives in modal_keys.ino, since it feeds directly into the engine.)

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
