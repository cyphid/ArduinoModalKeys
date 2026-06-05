ArduinoModalKeys
================

This is a project to implement programmable keyboard behavior in hardware: a small board sits
between your USB keyboard and your computer, reads every keystroke, and re-emits remapped ones.
The default logic turns the Alt keys into special modifiers that make ASDF and ;LKJ and QWER and POIU 
into sets of (Shift, Alt, Ctrl, Gui/Super/Command/Win) modifiers, each of which put the keyboard in different modes.

The sketch supports two hardware targets, selected automatically at compile time from the
board you have chosen in the Arduino IDE:

1. **Arduino Leonardo + USB Host Shield 2.0** — the original combo.
2. **[Adafruit Feather RP2040 with USB Type A Host](https://www.adafruit.com/product/5723)** —
   a single board with a built-in USB-A host port, no shield required.

The board-independent remapping engine (`keymap.cpp`, `helpers.cpp`) is shared; only the USB
host (reading the attached keyboard) and USB device (presenting a keyboard to the PC) layers
differ between targets. See [Hardware Targets](#hardware-targets) below for the per-board details.

## Hardware Prerequisites

Pick **one** of the two supported targets.

### Option A — Arduino Leonardo + USB Host Shield

* [Arduino Leonardo](http://arduino.cc/en/Main/arduinoBoardLeonardo)
* [USB Host Shield](http://arduino.cc/en/Main/ArduinoUSBHostShield)
* [Micro USB to USB cable](http://www.amazon.com/AmazonBasics-USB-2-0-Micro-Cable/dp/B00C28L5UW)
* USB Keyboard with at least [6KRO](https://en.wikipedia.org/wiki/Rollover_%28key%29)

### Option B — Adafruit Feather RP2040 with USB Type A Host

* [Adafruit Feather RP2040 with USB Type A Host](https://www.adafruit.com/product/5723) (product 5723)
* USB-C cable (Feather to your computer)
* USB Keyboard with at least [6KRO](https://en.wikipedia.org/wiki/Rollover_%28key%29), plugged into
  the board's USB-A host port

## Software Prerequisites

* [Arduino IDE](http://arduino.cc/en/main/software)
* The contents of this repository (yes, really :P)

Then, depending on your target:

### Option A — Leonardo libraries

* [USB Host Shield library](https://github.com/felis/USB_Host_Shield_2.0)

### Option B — Feather RP2040 USB Host libraries

* The [Earle Philhower arduino-pico](https://github.com/earlephilhower/arduino-pico) board package
  (Tools → Board → Boards Manager → "Raspberry Pi Pico/RP2040").
* The **Adafruit TinyUSB Library** (Library Manager → search "Adafruit TinyUSB").
* The **Pico PIO USB** library (Library Manager → search "Pico PIO USB").

In the Arduino IDE select the board **"Adafruit Feather RP2040 USB Host"**, and crucially set
**Tools → USB Stack → "Adafruit TinyUSB"** (the default "Pico SDK" stack will not work). Leave the
CPU speed at a 120 MHz or 240 MHz option — Pico-PIO-USB bit-banging requires one of those clocks.
The board variant already defines the host pins (D+ = GPIO16, D- = GPIO17, 5V enable = GPIO18);
the sketch falls back to those same defaults if they are ever missing.

## Setup Instructions

### Option A — Arduino Leonardo + USB Host Shield
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

### Option B — Adafruit Feather RP2040 with USB Type A Host
* Install the board package and the two libraries listed under
  [Software Prerequisites](#option-b--feather-rp2040-usb-host-libraries).
* Open `modal_keys/modal_keys.ino` in the Arduino IDE.
* Select Tools → Board → **"Adafruit Feather RP2040 USB Host"**.
* Select Tools → **USB Stack → "Adafruit TinyUSB"**.
* Connect the Feather to your computer with a USB-C cable.
* Upload the sketch: File → Upload. (If the board does not appear, double-tap the reset button to
  enter the UF2 bootloader, then re-select the port.)
* Plug your USB keyboard into the Feather's USB-A **host** port.
* Keystrokes typed into that keyboard are now remapped and sent to your computer through the Feather.

## Hardware Targets

The sketch auto-detects the target at compile time using the `ARDUINO_ARCH_RP2040` macro:
an RP2040 board builds the Feather path, anything else builds the Leonardo path (with the legacy
`LEONARDO` switch defined for backward compatibility). Both targets feed identical 8-byte
boot-keyboard reports into the shared engine through a single `HandleKeyboardReport()` entry
point, so the remapping behaviour is the same on either board.

| Concern | Leonardo + USB Host Shield | Feather RP2040 USB Host |
| --- | --- | --- |
| Read attached keyboard | USB Host Shield 2.0 `HIDBoot` / `KeyboardReportParser` | TinyUSB host (`tuh_hid_report_received_cb`) over Pico-PIO-USB |
| Present keyboard to PC | PluggableUSB `HID().SendReport(2, …)` | TinyUSB device `usb_hid.sendReport(0, …)` |
| Where the USB host runs | `Usb.Task()` in `loop()` | `USBHost.task()` in `loop1()` (second core) |
| Config storage | AVR `EEPROM` | `EEPROM` emulated in flash (`begin()`/`commit()`) |

## Library Compatibility

This sketch works directly on raw 8-byte HID boot-keyboard reports and needs
*positional* write access to the wire buffer (modifier byte, reserved byte,
six key slots). On the Leonardo, two libraries it depended on have since
reshaped their APIs, so the original 2017 code no longer compiles on current
versions. The code now targets the **current** libraries, with a fallback for
old ones.

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

### Feather RP2040 USB Host (Adafruit TinyUSB + Pico-PIO-USB)

On the RP2040 the same engine is wired to a completely different USB stack:

* **Device side** ([`host_keyboard.cpp`](modal_keys/host_keyboard.cpp)): an
  `Adafruit_USBD_HID` keyboard interface built from `TUD_HID_REPORT_DESC_KEYBOARD()`.
  That descriptor has the boot-keyboard layout (1 modifier byte, 1 reserved byte, six key codes),
  so the engine's `buf[8]` maps onto it verbatim and goes out with `usb_hid.sendReport(0, buf, 8)`.
* **Host side** (`modal_keys.ino`): the attached keyboard is read over the bit-banged
  Pico-PIO-USB port. `tuh_hid_mount_cb` requests reports from keyboard-protocol interfaces and
  `tuh_hid_report_received_cb` forwards each 8-byte report into `HandleKeyboardReport()`. Because
  the RP2040 dedicates its **second core** and one PIO block to USB host duties, the host runs in
  `setup1()` / `loop1()` while the TinyUSB device is serviced on core 0.
* **Config storage** (`keymap.cpp`): the arduino-pico `EEPROM` library is flash-backed, so it is
  initialised with `EEPROM.begin()` and writes are flushed with `EEPROM.commit()`. Both are
  compiled out on the AVR target, which needs neither.

### Verifying a build

The **Leonardo** firmware compiles and links against the latest Arduino AVR core and the latest
USB Host Shield 2.0 using only `avr-gcc` — no Arduino IDE required:

```
avr-g++ -mmcu=atmega32u4 -DF_CPU=16000000L -DARDUINO_AVR_LEONARDO \
        -DUSB_VID=0x2341 -DUSB_PID=0x8036 ... \
        -I<core>/cores/arduino -I<core>/variants/leonardo \
        -I<core>/libraries/HID/src -I<USB_Host_Shield_2.0> \
        -c modal_keys/*.cpp
```

The **Feather RP2040 USB Host** firmware is most easily built with `arduino-cli` once the board
package and libraries above are installed (the `--build-property` ensures the TinyUSB stack is
selected, matching the IDE's "USB Stack: Adafruit TinyUSB" menu):

```
arduino-cli compile \
  --fqbn rp2040:rp2040:adafruit_feather_usb_host \
  --build-property "build.usbstack_flags=-DUSE_TINYUSB -I{runtime.platform.path}/libraries/Adafruit_TinyUSB_Arduino/src/arduino" \
  modal_keys
```
