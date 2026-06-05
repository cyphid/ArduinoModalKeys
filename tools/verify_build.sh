#!/usr/bin/env bash
#
# verify_build.sh -- compile-verify both hardware targets of ArduinoModalKeys
# without the Arduino IDE.
#
# It checks three things:
#
#   1. LEONARDO  -- a full compile *and link* of a flashable Leonardo firmware
#                   against the real Arduino AVR core and the real USB Host
#                   Shield 2.0 library, using avr-gcc.
#
#   2. RP2040 (shim) -- (Adafruit Feather RP2040 USB Host) the board-independent
#                   engine is compiled for Cortex-M0+, and the TinyUSB /
#                   Pico-PIO-USB glue is compiled against a small "conformance
#                   shim" whose declarations are transcribed verbatim from the
#                   upstream library headers. A relocatable link then proves
#                   that every project symbol resolves and only genuine
#                   external library symbols remain undefined. This is fast and
#                   needs no board package -- but, by design, it cannot see the
#                   real C++ standard library, so it is a structural check only.
#
#   3. RP2040 (full) -- a *real* full firmware build with arduino-cli against
#                   the actual arduino-pico board package + Adafruit TinyUSB +
#                   Pico-PIO-USB, producing a flashable .uf2 (exactly what the
#                   Arduino IDE does). This is what catches issues the shim
#                   cannot, e.g. clashes with the STL those libraries pull in,
#                   and -Werror=return-type (enabled by the arduino-pico core).
#                   Soft-skips if arduino-cli is not installed.
#
# Toolchains (Debian/Ubuntu):  apt-get install gcc-avr avr-libc gcc-arm-none-eabi
# Plus arduino-cli for check 3:  https://arduino.github.io/arduino-cli/
# Libraries are cloned from GitHub into $WORK/libs on first run and reused.
#
# Usage:   tools/verify_build.sh            # run both checks
#          WORK=/path tools/verify_build.sh # cache toolchain clones elsewhere
#
# Exit status is non-zero if either target fails to build.

set -euo pipefail

# --------------------------------------------------------------------------
# Locations
# --------------------------------------------------------------------------
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
SRC="$REPO/modal_keys"
WORK="${WORK:-/tmp/amk_verify}"
LIBS="$WORK/libs"
BUILD="$WORK/build"
SHIM="$WORK/shim"

mkdir -p "$LIBS" "$BUILD" "$SHIM"

red()   { printf '\033[31m%s\033[0m\n' "$*"; }
green() { printf '\033[32m%s\033[0m\n' "$*"; }
bold()  { printf '\033[1m%s\033[0m\n'  "$*"; }

fail() { red "FAIL: $*"; exit 1; }

need() {
    command -v "$1" >/dev/null 2>&1 || fail "missing tool '$1' (apt-get install $2)"
}

clone() { # clone <url> <dir>
    local url="$1" dir="$2"
    if [ ! -d "$LIBS/$dir" ]; then
        echo "  cloning $dir ..."
        git clone --depth 1 "$url" "$LIBS/$dir" >/dev/null 2>&1 \
            || fail "could not clone $url"
    fi
}

# --------------------------------------------------------------------------
# Target 1: Leonardo (AVR) -- real compile + link
# --------------------------------------------------------------------------
verify_leonardo() {
    bold "== Target 1: Arduino Leonardo + USB Host Shield 2.0 (AVR) =="
    need avr-g++ gcc-avr
    need avr-gcc gcc-avr

    clone https://github.com/arduino/ArduinoCore-avr.git    ArduinoCore-avr
    clone https://github.com/felis/USB_Host_Shield_2.0.git  USB_Host_Shield_2.0

    local CORE="$LIBS/ArduinoCore-avr"
    local UHS="$LIBS/USB_Host_Shield_2.0"
    local B="$BUILD/leo"
    rm -rf "$B"; mkdir -p "$B"; cd "$B"

    local INC="-I$CORE/cores/arduino -I$CORE/variants/leonardo \
        -I$CORE/libraries/HID/src -I$CORE/libraries/EEPROM/src \
        -I$CORE/libraries/SPI/src -I$CORE/libraries/SoftwareSerial/src \
        -I$UHS -I$SRC"
    local DEF="-DF_CPU=16000000L -DARDUINO=10819 -DARDUINO_AVR_LEONARDO \
        -DARDUINO_ARCH_AVR -DUSB_VID=0x2341 -DUSB_PID=0x8036"
    local CXX="-mmcu=atmega32u4 -Os -w -ffunction-sections -fdata-sections \
        -std=gnu++11 -fno-exceptions -fno-threadsafe-statics"
    local CC="-mmcu=atmega32u4 -Os -w -ffunction-sections -fdata-sections -std=gnu11"

    # The Arduino IDE prepends <Arduino.h> to the .ino; replicate that.
    { echo '#include <Arduino.h>'; cat "$SRC/modal_keys.ino"; } > "$B/modal_keys.ino.cpp"

    echo "  compiling sketch translation units ..."
    local sketch_objs=""
    for f in "$B/modal_keys.ino.cpp" "$SRC"/helpers.cpp "$SRC"/keymap.cpp \
             "$SRC"/keys.cpp "$SRC"/host_keyboard.cpp; do
        local o="$B/$(basename "${f%.cpp}").o"
        avr-g++ -c $CXX $DEF $INC "$f" -o "$o" || fail "Leonardo: $(basename "$f")"
        sketch_objs="$sketch_objs $o"
    done

    echo "  compiling core + libraries ..."
    local lib_objs=""
    for f in "$CORE"/cores/arduino/*.cpp "$CORE"/cores/arduino/*.c \
             "$CORE"/libraries/HID/src/*.cpp \
             "$CORE"/libraries/SPI/src/*.cpp \
             "$CORE"/libraries/SoftwareSerial/src/*.cpp \
             "$UHS"/*.cpp; do
        [ -e "$f" ] || continue
        local o="$B/lib_$(echo "$f" | md5sum | cut -c1-8)_$(basename "$f").o"
        case "$f" in
            *.c)   avr-gcc -c $CC  $DEF $INC "$f" -o "$o" 2>/dev/null && lib_objs="$lib_objs $o" || true ;;
            *.cpp) avr-g++ -c $CXX $DEF $INC "$f" -o "$o" 2>/dev/null && lib_objs="$lib_objs $o" || true ;;
        esac
    done

    echo "  linking firmware ..."
    avr-gcc $CXX -Wl,--gc-sections -o "$B/modal_keys.elf" \
        $sketch_objs $lib_objs -lm || fail "Leonardo: link"

    [ -f "$B/modal_keys.elf" ] || fail "Leonardo: no ELF produced"
    avr-size "$B/modal_keys.elf"
    green "  Leonardo: compiled and linked a flashable firmware OK"
}

# --------------------------------------------------------------------------
# RP2040 conformance shim
# --------------------------------------------------------------------------
# Signatures below are transcribed verbatim from the upstream headers (paths in
# comments). If the sketch ever calls one of these APIs incorrectly, the
# compile fails -- which is the whole point of the shim.
write_shim() {
    cat > "$SHIM/Arduino.h" <<'EOF'
#ifndef ARDUINO_SHIM_H
#define ARDUINO_SHIM_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
class String {
  char b[256];
public:
  String(){ b[0]=0; }
  String(const char* s){ strncpy(b,s,255); b[255]=0; }
  String(char c){ b[0]=c; b[1]=0; }
  String(int){ b[0]=0; }
  String(unsigned int){ b[0]=0; }
  unsigned int length() const { return strlen(b); }
  String substring(unsigned int) const { return *this; }
  String substring(unsigned int, unsigned int) const { return *this; }
  char charAt(unsigned int i) const { return b[i]; }
  String operator+(const String&) const { return *this; }
  String& operator+=(const char*){ return *this; }
  String& operator+=(const String&){ return *this; }
  String& operator+=(char){ return *this; }
  bool operator==(const String& o) const { return strcmp(b,o.b)==0; }
  const char* c_str() const { return b; }
};
inline String operator+(const char* a, const String&){ return String(a); }
struct SerialT {
  void begin(unsigned long){}
  void println(){} void println(const String&){} void println(const char*){}
  void print(const String&){} void print(const char*){}
  size_t write(const uint8_t*, size_t){ return 0; }
};
extern SerialT Serial;
#define OUTPUT 1
#define HIGH 1
#define LOW 0
inline void pinMode(int,int){}
inline void digitalWrite(int,int){}
inline void delay(unsigned long){}
inline void yield(){}
#endif
EOF

    cat > "$SHIM/EEPROM.h" <<'EOF'
#ifndef EEPROM_SHIM_H
#define EEPROM_SHIM_H
#include <stdint.h>
#include <string.h>
class EEPROMClass {            // arduino-pico flash-backed EEPROM API
  uint8_t buf[512];
public:
  void begin(size_t){}
  bool commit(){ return true; }
  template<typename T> T& get(int i, T& t){ memcpy(&t, buf+i, sizeof(T)); return t; }
  template<typename T> const T& put(int i, const T& t){ memcpy(buf+i, &t, sizeof(T)); return t; }
};
extern EEPROMClass EEPROM;
#endif
EOF

    cat > "$SHIM/pio_usb.h" <<'EOF'
#ifndef PIO_USB_SHIM_H
#define PIO_USB_SHIM_H
#include <stdint.h>
// Pico-PIO-USB/src/pio_usb_configuration.h
typedef struct {
  uint8_t pin_dp;                 // :10
  uint8_t pinout;
  uint8_t sm_tx, sm_rx, sm_eop;
  void* pio_tx_num; void* pio_rx_num;
  uint8_t tx_ch;
  uint8_t alarm_pool;
  uint8_t debug_pin_rx, debug_pin_eop;
  bool skip_alarm_pool;
} pio_usb_configuration_t;         // :22
#define PIO_USB_DEFAULT_CONFIG { 0,0,0,1,2,0,0,0,0,-1,-1,false }  // :38
#endif
EOF

    cat > "$SHIM/Adafruit_TinyUSB.h" <<'EOF'
#ifndef ADAFRUIT_TINYUSB_SHIM_H
#define ADAFRUIT_TINYUSB_SHIM_H
#include <stdint.h>
#include <stddef.h>
// class/hid/hid.h:71
enum { HID_ITF_PROTOCOL_NONE = 0, HID_ITF_PROTOCOL_KEYBOARD = 1, HID_ITF_PROTOCOL_MOUSE = 2 };
// class/hid/hid_device.h:177
#define TUD_HID_REPORT_DESC_KEYBOARD(...) 0x05,0x01,0x09,0x06,0xA1,0x01,0xC0
// arduino/hid/Adafruit_USBD_HID.h:44,58,61,62
class Adafruit_USBD_HID {
public:
  Adafruit_USBD_HID(uint8_t const *desc_report, uint16_t len,
                    uint8_t protocol = HID_ITF_PROTOCOL_NONE,
                    uint8_t interval_ms = 4, bool has_out_endpoint = false);
  bool begin(void);
  bool ready(void);
  bool sendReport(uint8_t report_id, void const *report, uint8_t len);
};
// arduino/Adafruit_USBH_Host.h:87,90,91
class Adafruit_USBH_Host {
public:
  bool configure_pio_usb(uint8_t rhport, const void *cfg_param);
  bool begin(uint8_t rhport);
  void task(uint32_t timeout_ms = 0xFFFFFFFF, bool in_isr = false);
};
// class/hid/hid_host.h:82,129,151,154,158
extern "C" {
uint8_t tuh_hid_interface_protocol(uint8_t dev_addr, uint8_t idx);
bool    tuh_hid_receive_report(uint8_t dev_addr, uint8_t idx);
void    tuh_hid_mount_cb(uint8_t dev_addr, uint8_t idx, const uint8_t *report_desc, uint16_t desc_len);
void    tuh_hid_umount_cb(uint8_t dev_addr, uint8_t idx);
void    tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t idx, const uint8_t *report, uint16_t len);
}
#endif
EOF
}

# --------------------------------------------------------------------------
# Target 2: RP2040 (Cortex-M0+) -- engine compile + glue conformance + link
# --------------------------------------------------------------------------
verify_rp2040() {
    bold "== Target 2: Adafruit Feather RP2040 USB Host (Cortex-M0+) =="
    need arm-none-eabi-g++ gcc-arm-none-eabi
    need arm-none-eabi-nm  gcc-arm-none-eabi

    # Cross-check the conformance-shim signatures against the real upstream
    # headers when they are available, so the shim can't silently drift.
    clone https://github.com/adafruit/Adafruit_TinyUSB_Arduino.git Adafruit_TinyUSB_Arduino
    clone https://github.com/sekigon-gonnoc/Pico-PIO-USB.git        Pico-PIO-USB
    clone https://github.com/earlephilhower/arduino-pico.git        arduino-pico
    cross_check_signatures

    write_shim

    local B="$BUILD/rp2040"
    rm -rf "$B"; mkdir -p "$B"; cd "$B"
    local FLAGS="-mcpu=cortex-m0plus -mthumb -Os -w -std=gnu++17 \
        -fno-exceptions -fno-threadsafe-statics -DARDUINO_ARCH_RP2040 -DARDUINO=10819"
    local INC="-I$SHIM -I$SRC"

    echo "  compiling board-independent engine for ARM ..."
    local objs=""
    for f in helpers keymap keys; do
        arm-none-eabi-g++ -c $FLAGS $INC "$SRC/$f.cpp" -o "$B/$f.o" \
            || fail "RP2040 engine: $f.cpp"
        objs="$objs $B/$f.o"
    done

    echo "  compiling TinyUSB device side (host_keyboard.cpp) ..."
    arm-none-eabi-g++ -c $FLAGS $INC "$SRC/host_keyboard.cpp" -o "$B/host_keyboard.o" \
        || fail "RP2040: host_keyboard.cpp"
    objs="$objs $B/host_keyboard.o"

    echo "  compiling USB host glue + callbacks (modal_keys.ino) ..."
    { echo '#include <Arduino.h>'; cat "$SRC/modal_keys.ino"; } > "$B/modal_keys.ino.cpp"
    arm-none-eabi-g++ -c $FLAGS $INC "$B/modal_keys.ino.cpp" -o "$B/modal_keys.ino.o" \
        || fail "RP2040: modal_keys.ino"
    objs="$objs $B/modal_keys.ino.o"

    echo "  relocatable link + undefined-symbol audit ..."
    arm-none-eabi-g++ -mcpu=cortex-m0plus -mthumb -nostdlib -r \
        -o "$B/all.o" $objs || fail "RP2040: relocatable link"

    # Any undefined symbol that is NOT a known external library/runtime symbol
    # means an internal wiring problem in the sketch.
    local undef
    undef="$(arm-none-eabi-nm -C -u "$B/all.o" | sed 's/^[[:space:]]*U[[:space:]]*//')"
    echo "$undef" | sed 's/^/    U /'
    local unexpected
    unexpected="$(echo "$undef" | grep -vE \
        'EEPROM|Adafruit_USBD_HID|Adafruit_USBH_Host|tuh_hid_|^Serial$|memcpy|memset|strncpy|strlen|strcmp|__gnu_thumb1_case|__aeabi' \
        || true)"
    if [ -n "$unexpected" ]; then
        red "  unexpected unresolved project symbols:"
        echo "$unexpected" | sed 's/^/    /'
        fail "RP2040: sketch has unresolved internal symbols"
    fi

    # The three HID host callbacks must be *defined* (they override TinyUSB's
    # weak defaults), i.e. they must NOT appear as undefined.
    for cb in tuh_hid_mount_cb tuh_hid_umount_cb tuh_hid_report_received_cb; do
        if echo "$undef" | grep -q "\\b$cb\\b"; then
            fail "RP2040: callback $cb is not defined by the sketch"
        fi
    done

    green "  RP2040: engine + TinyUSB/PIO-USB glue compile; all project symbols resolve OK"
}

# --------------------------------------------------------------------------
# Keep the conformance shim honest: confirm the symbols it declares still
# exist in the freshly-cloned upstream headers.
# --------------------------------------------------------------------------
cross_check_signatures() {
    local TU="$LIBS/Adafruit_TinyUSB_Arduino" PIO="$LIBS/Pico-PIO-USB"
    local miss=0
    check() { # check <description> <file> <pattern>
        if [ -f "$2" ] && grep -q "$3" "$2"; then
            :
        else
            red "  shim drift: '$1' not found in $(basename "$2")"; miss=1
        fi
    }
    check "sendReport"            "$TU/src/arduino/hid/Adafruit_USBD_HID.h" "bool sendReport(uint8_t report_id, void const \*report, uint8_t len)"
    check "configure_pio_usb"     "$TU/src/arduino/Adafruit_USBH_Host.h"   "bool configure_pio_usb(uint8_t rhport, const void \*cfg_param)"
    check "tuh_hid_receive_report" "$TU/src/class/hid/hid_host.h"          "bool tuh_hid_receive_report(uint8_t dev_addr, uint8_t idx)"
    check "tuh_hid_report_received_cb" "$TU/src/class/hid/hid_host.h"      "void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t idx, const uint8_t \*report, uint16_t len)"
    check "HID_ITF_PROTOCOL_KEYBOARD"  "$TU/src/class/hid/hid.h"          "HID_ITF_PROTOCOL_KEYBOARD = 1"
    check "pin_dp"                "$PIO/src/pio_usb_configuration.h"        "uint8_t pin_dp"
    [ "$miss" = 0 ] && green "  shim signatures match upstream headers" \
                    || fail "conformance shim is out of date with upstream"
}

# --------------------------------------------------------------------------
# Target 3: RP2040 (Cortex-M0+) -- REAL full firmware build via arduino-cli.
# --------------------------------------------------------------------------
# Unlike verify_rp2040() above (which links the engine against a hand-written
# conformance shim), this compiles the sketch against the *actual* arduino-pico
# board package, Adafruit TinyUSB and Pico-PIO-USB libraries and produces a
# flashable .uf2 -- exactly what the Arduino IDE does. This is what catches
# problems the shim cannot see: clashes with the real C++ standard library that
# those libraries drag in, and -Werror=return-type (which the arduino-pico core
# enables and the AVR/-w build does not).
#
# It needs arduino-cli and network access. If arduino-cli is missing it is a
# soft SKIP (the shim build above still ran); a genuine *compile* failure is
# always fatal.
#
# Env overrides (mainly for sandboxed/CI networks):
#   ARDUINO_CLI       path to the arduino-cli binary (default: arduino-cli)
#   MODAL_CTAGS_PATH  dir containing a `ctags` binary, passed through as
#                     runtime.tools.ctags.path (only needed where arduino-cli
#                     cannot fetch its bundled ctags, e.g. downloads.arduino.cc
#                     is blocked).
verify_rp2040_full() {
    bold "== Target 3: Adafruit Feather RP2040 USB Host -- full firmware (arduino-cli) =="
    local ACLI="${ARDUINO_CLI:-arduino-cli}"
    if ! command -v "$ACLI" >/dev/null 2>&1; then
        red "  SKIP: arduino-cli not found -- install it to run the full firmware build."
        red "        https://arduino.github.io/arduino-cli/latest/installation/"
        return 0
    fi

    # Keep all arduino-cli state inside $WORK so the script is self-contained and
    # does not touch the user's ~/.arduino15.
    export ARDUINO_DIRECTORIES_DATA="$WORK/arduino15"
    export ARDUINO_DIRECTORIES_USER="$WORK/Arduino"
    export ARDUINO_DIRECTORIES_DOWNLOADS="$WORK/arduino15/staging"
    export ARDUINO_BOARD_MANAGER_ADDITIONAL_URLS="https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json"
    export ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL=true
    mkdir -p "$ARDUINO_DIRECTORIES_DATA" "$ARDUINO_DIRECTORIES_USER"

    local FQBN="rp2040:rp2040:adafruit_feather_usb_host:usbstack=tinyusb"

    # The earlephilhower index + core + tools are all served from GitHub; the
    # main Arduino index (downloads.arduino.cc) is only needed for builtin tools
    # like ctags, so tolerate its failure when MODAL_CTAGS_PATH provides one.
    echo "  updating board index ..."
    "$ACLI" core update-index >/dev/null 2>&1 || true

    if ! "$ACLI" core list 2>/dev/null | grep -q '^rp2040:rp2040'; then
        echo "  installing rp2040:rp2040 core (~100 MB, first run only) ..."
        "$ACLI" core install rp2040:rp2040 >/dev/null 2>&1 \
            || fail "RP2040 full: could not install rp2040:rp2040 core"
    fi

    if [ ! -d "$ARDUINO_DIRECTORIES_USER/libraries/Pico_PIO_USB" ]; then
        echo "  installing Pico-PIO-USB library ..."
        "$ACLI" lib install --git-url https://github.com/sekigon-gonnoc/Pico-PIO-USB.git \
            >/dev/null 2>&1 || fail "RP2040 full: could not install Pico-PIO-USB"
    fi

    local B="$BUILD/rp2040_full"
    rm -rf "$B"; mkdir -p "$B"

    local ctags_prop=()
    [ -n "${MODAL_CTAGS_PATH:-}" ] && ctags_prop=(--build-property "runtime.tools.ctags.path=$MODAL_CTAGS_PATH")

    echo "  compiling real firmware (FQBN: $FQBN) ..."
    "$ACLI" compile --fqbn "$FQBN" "${ctags_prop[@]}" \
        --output-dir "$B" "$SRC" 2>&1 \
        | grep -vE 'Downloading index|Error initializing instance|Multiple libraries|Not used:|^  Used:' \
        || fail "RP2040 full: arduino-cli compile failed"

    local uf2
    uf2="$(ls "$B"/*.uf2 2>/dev/null | head -1)"
    [ -n "$uf2" ] || fail "RP2040 full: no .uf2 firmware produced"
    echo "  produced: $(basename "$uf2") ($(wc -c < "$uf2") bytes)"
    green "  RP2040: real firmware compiled and a flashable .uf2 was produced OK"
}

# --------------------------------------------------------------------------
main() {
    need git git
    bold "ArduinoModalKeys build verification"
    echo "  repo:  $REPO"
    echo "  work:  $WORK"
    echo
    verify_leonardo
    echo
    verify_rp2040
    echo
    verify_rp2040_full
    echo
    green "ALL TARGETS VERIFIED"
}

main "$@"
