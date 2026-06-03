ArduinoModalKeys
================

This is a project to implement programmable keyboard behavior in hardware using an Arduino with USB Host Shield. 
The default logic turns the Alt keys into special modifiers that make ASDF and ;LKJ and QWER and POIU 
into sets of (Shift, Alt, Ctrl, Gui/Super/Command/Win) modifiers, each of which put the keyboard in different modes.

## Hardware Prerequisites

* [Arduino Leonardo](http://arduino.cc/en/Main/arduinoBoardLeonardo)
* [USB Host Shield](http://arduino.cc/en/Main/ArduinoUSBHostShield)
* [Micro USB to USB cable](http://www.amazon.com/AmazonBasics-USB-2-0-Micro-Cable/dp/B00C28L5UW)
* USB Keyboard with at least [6KRO](https://en.wikipedia.org/wiki/Rollover_%28key%29)

## Software Prerequisites

* [Arduino IDE](http://arduino.cc/en/main/software)
* [USB Host Shield library](https://github.com/felis/USB_Host_Shield_2.0)
* The contents of this repository (yes, really :P)

## Setup Instructions
* Connect your Arduino Leonardo to your computer via the micro USB cable
* Open modal_keys/modal_keys.ino in the Arduino IDE
* Download the USB Host Shield library into \<Arduino Sketchbook location\>/library. Arduino Sketchbook location is specified in File -> Preferences.
* Restart Arduino IDE
* In the Arduino IDE, select the Board type to be Leonardo: Tools -> Board
* Check that the Arduino Leonardo is connected. If it is, there should be an entry under the Tools -> Serial Port menu. If it isn't, Google is your friend :-)
* Upload the sketch to your device: File -> Upload
* Unplug your Arduino device
* Connect the USB Host Shield to the Arduino Leonardo
* Plug your USB keyboard into the USB port on the USB Host Shield
* Plug the Arduino back into the computer
* Keystrokes typed into this keyboard should now be sent to your computer through the Arduino Leonardo

## Library Compatibility

This sketch works directly on raw 8-byte HID boot-keyboard reports and needs
*positional* write access to the wire buffer (modifier byte, reserved byte,
six key slots). Two libraries it depended on have since reshaped their APIs,
so the original 2017 code no longer compiles on current versions. The code now
targets the **current** libraries, with a fallback for old ones.

### Output side — Arduino AVR core (the keyboard the Leonardo presents to the PC)

The sketch used to push reports with the core's `HID_SendReport(2, buf, 8)`
free function. That function was **removed** from the AVR core in mid-2015
(core `>= 1.6.6`, the PluggableUSB rework). The modern `Keyboard` library only
offers `press()` / `release()` / `write()` over its own private buffer — no
positional access to the raw bytes this project relies on.

The raw capability is still reachable, just relocated: the modern core exposes
a `HID()` singleton with `SendReport(id, data, len)`, but it ships with **no
report descriptor**. [`host_keyboard.cpp`](modal_keys/host_keyboard.cpp)
bridges the gap — it registers a boot-keyboard descriptor (report id 2) and
sends raw reports through `HID().SendReport(2, buf, 8)`. Because both the old
free function and `SendReport()` prepend the report-id byte and then emit the
payload verbatim, the USB traffic is **identical** to the original.

### Input side — USB Host Shield 2.0 (reading the attached keyboard)

Two renames in the current [USB Host Shield
2.0](https://github.com/felis/USB_Host_Shield_2.0) library:

* the `HID` class became `USBHID` (to stop colliding with the core's `HID`
  object), so `KeyboardReportParser::Parse(...)` now takes a `USBHID *`;
* `HID_PROTOCOL_KEYBOARD` became `USB_HID_PROTOCOL_KEYBOARD`.

Both are reflected in `modal_keys.ino`.

### Verifying a build

The sketch compiles and links to a flashable Leonardo firmware against the
latest Arduino AVR core and the latest USB Host Shield 2.0 using only
`avr-gcc` — no Arduino IDE required:

```
avr-g++ -mmcu=atmega32u4 -DF_CPU=16000000L -DARDUINO_AVR_LEONARDO \
        -DUSB_VID=0x2341 -DUSB_PID=0x8036 ... \
        -I<core>/cores/arduino -I<core>/variants/leonardo \
        -I<core>/libraries/HID/src -I<USB_Host_Shield_2.0> \
        -c modal_keys/*.cpp
```
