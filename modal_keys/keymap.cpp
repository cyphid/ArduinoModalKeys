#include "modal_keys.h"
#include "keys.h"
#include "keymap.h"
#include "helpers.h"
#include "layout_qwerty.h"
#include "layout_dvorak.h"
// #include "layout_dvorak_programmer.h"

#include <EEPROM.h>

// Map of where in EEPROM storage to store each config variable
#define OSModeSlot 0
#define KeyboardLayoutSlot 1

// ****************************************************************************
// Type Declarations
// ****************************************************************************

// Operating System Modes
typedef enum {
    Windows = 0,
    OSX
} OSMode;

// Keyboard Layouts
typedef enum {
    qwerty = 0,
    dvorak,
    // dvorakProgrammer
} KeyboardLayout;

// the available keyboard modes
typedef enum
{
    NormalNoKeysMode = 0,
    ModalNoKeysMode,
    EscapeMode,
    CapsLockMode,
    RightCtrlMode,
    NormalTypingMode,
    ModalTypingMode,
    LeftAltMode,
    LeftModMode,
    RightAltMode,
    RightModMode,
    AltTabMode,
    WindowSnapMode,
    NumPadMode,
    GamingNoKeysMode,
    GamingBacktickMode,
    GamingTabMode,
    GamingCapsLockMode,
    GamingShiftMode,
    GamingCtrlMode,
    GamingAltMode,
    GamingSpaceMode,
    BlackDesertNoKeysMode,
    BlackDesertCapsLockMode,
    BlackDesertSpaceMode,
    BlackDesertAltMode
} Mode;

typedef enum {
    Left = 0,
    Right
} Side;

typedef enum {
    Clean = 0,
    Used
} ModeState;

// specifies action to perform after returning from a call to a KeyMapFunc function with a specific key
typedef enum {
    Continue = 0,
    Stop,
    Restart
} ControlCode;

// typedef for functions that specify a key mapping
typedef ControlCode(*KeyMapFunc)(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);

// ****************************************************************************
// Function Declarations
// ****************************************************************************

// helpers
void LoadOSMode();
ControlCode ChangeOSMode(OSMode osMode);
void SetMode(Mode mode, ModeState modeState);
ControlCode EnterMode(Mode mode, ModeState modeState);
ControlCode ChangeConfiguration(KeyboardLayout layout, Mode entryPointMode);
ControlCode SendKey(uint8_t keycode, uint8_t outbuf[8]);
ControlCode SendModifiers(uint8_t mods, uint8_t outbuf[8]);
ControlCode UnsetModifiers(uint8_t mods, uint8_t outbuf[8]);
ControlCode SendKeyCombo(uint8_t mods, uint8_t keycode, uint8_t outbuf[8]);
ControlCode SendOnlyKey(uint8_t keycode, uint8_t outbuf[8]);
ControlCode SendOnlyKeyCombo(uint8_t mods, uint8_t keycode, uint8_t outbuf[8]);
ControlCode SendRichKey(RichKey key, uint8_t outbuf[8]);
ControlCode InvalidKey();
ControlCode MapKey(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
uint8_t NumKeysPressed(uint8_t buf[8]);
uint8_t NumModsPressed(uint8_t buf[8]);
uint8_t NumKeysOrModsPressed(uint8_t buf[8]);
String GetOSModeString(OSMode osMode);
String GetModeString(Mode mode);
String GetModeStateString(ModeState modeState);
String GetLayoutString(KeyboardLayout layout);

// state changing methods
ControlCode NormalEntryPoint_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode ModalEntryPoint_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode Escape_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode CapsLock_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode RightCtrl_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);

ControlCode NormalTyping_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode ModalTyping_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode LeftAltMode_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode LeftModMode_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode RightAltMode_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode RightModMode_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode AltTab_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode WindowSnap_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode NumPad_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);

ControlCode GamingEntryPoint_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode GamingBacktick_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode GamingTab_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode GamingCapsLock_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode GamingShift_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode GamingCtrl_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode GamingAlt_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode GamingSpace_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);

ControlCode BlackDesertEntryPoint_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode BlackDesertCapsLock_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode BlackDesertSpace_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);
ControlCode BlackDesertAlt_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]);

// state handling callbacks
void HandleLastKeyReleased();

// ****************************************************************************
// Constantseeeeeeeeee
// ****************************************************************************

const RichKey NoKey = { 0, 0, 0 };
const RichKey CustomModifierKey = { 0, 0, _CustomModifier };

// Keymaps
const KeySpec *Keymap[] =
{
    qwertyKeymap,
    dvorakKeymap,
    // dvorakProgrammerKeymap
};

// array of KeyMapFuncs, one for each mode
const KeyMapFunc KeyMaps[] = {
    &NormalEntryPoint_keymap,       /* NormalNoKeysMode */
    &ModalEntryPoint_keymap,        /* ModalNoKeysMode */
    &Escape_keymap,                 /* EscapeMode */
    &CapsLock_keymap,               /* CapsLockMode */
    &RightCtrl_keymap,              /* RightCtrlMode */
    &NormalTyping_keymap,           /* NormalTypingMode */
    &ModalTyping_keymap,            /* ModalTypingMode */
    &LeftAltMode_keymap,            /* LeftAltMode */
    &LeftModMode_keymap,            /* LeftModMode */
    &RightAltMode_keymap,           /* RightAltMode */
    &RightModMode_keymap,           /* RightModMode */
    &AltTab_keymap,                 /* AltTabMode */
    &WindowSnap_keymap,             /* WindowSnapMode */
    &NumPad_keymap,                 /* NumPadMode */
    &GamingEntryPoint_keymap,       /* GamingNoKeysMode */
    &GamingBacktick_keymap,         /* GamingBacktickMode */
    &GamingTab_keymap,              /* GamingTabMode */
    &GamingCapsLock_keymap,         /* GamingCapsLockMode */
    &GamingShift_keymap,            /* GamingShiftMode */
    &GamingCtrl_keymap,             /* GamingCtrlMode */
    &GamingAlt_keymap,              /* GamingAltMode */
    &GamingSpace_keymap,            /* GamingSpaceMode */
    &BlackDesertEntryPoint_keymap,  /* BlackDesertNoKeysMode */
    &BlackDesertCapsLock_keymap,    /* BlackDesertCapsLockMode */
    &BlackDesertSpace_keymap,       /* BlackDesertSpaceMode */
    &BlackDesertAlt_keymap          /* BlackDesertAltMode */
};

// ****************************************************************************
// Variables
// ****************************************************************************

KeyboardLayout CurrentLayout = dvorak;
Mode EntryPointMode = ModalNoKeysMode;
Mode CurrentMode = ModalNoKeysMode;
OSMode CurrentOSMode = Windows;
ModeState CurrentModeState = Clean;

// ****************************************************************************
// State Dependant Values
// ****************************************************************************

RichKey CapsLockMod() {
    switch(CurrentOSMode){
        case Windows: return (RichKey){ LCtrl, 0 };
        case OSX: return (RichKey) { LGui, 0 };
    }
}

RichKey LCtrlMod() {
    switch(CurrentOSMode){
        case Windows: return (RichKey) { LCtrl, 0 };
        case OSX: return (RichKey) { LCtrl, 0 };
    }
}

uint8_t AppSwitchModifierKeycode(Side side) {
    switch (side) {
        case Left:
            switch (CurrentOSMode){
                case Windows: return LAlt;
                case OSX: return LGui;
            }
        case Right:
            switch (CurrentOSMode){
                case Windows: return RAlt;
                case OSX: return RGui;
            }
    }
}

uint8_t WindowSnapModifierKeycode() {
    switch(CurrentOSMode){
        case Windows: return LCtrl | LGui;
        case OSX: return LCtrl | LGui | LShift;
    }
}

// ****************************************************************************
// Mode implementations
// ****************************************************************************

ControlCode Escape_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
     // map modifier
    if (i == 0) switch (inbuf[i]) {
        case LCtrl:
        case RCtrl:
            return ChangeOSMode(Windows);
        case LGui:
        case RGui:
            return ChangeOSMode(OSX);
    }

     // map subsequent keys
    if (i >= 2) switch (inbuf[i]) {
        case _Escape:    return Continue;
        case _F1:        return ChangeConfiguration(qwerty, NormalNoKeysMode);
        case _F2:        return ChangeConfiguration(dvorak, ModalNoKeysMode);
        case _F3:        return ChangeConfiguration(qwerty, GamingNoKeysMode);
        case _F4:        return ChangeConfiguration(qwerty, BlackDesertNoKeysMode);
    }
    // all other keys
    return InvalidKey();
}

ControlCode CapsLock_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) switch (inbuf[i]) {

    }

     // map first key
    if (i == 2) switch (inbuf[i]) {
        case _CapsLock:         return Continue;
    }

    // all other keys
    return EnterMode(NormalTypingMode, Used);
}

ControlCode RightCtrl_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) switch (inbuf[i]) {
        case RCtrl:               return Continue;
        case RCtrl | LCtrl:       return ChangeOSMode(Windows);
        case RCtrl | LGui:        return ChangeOSMode(OSX);
    }

     // map first key
    if (i == 2) switch (inbuf[i]) {
        case _1:        return ChangeConfiguration(qwerty, NormalNoKeysMode);
        case _2:        return ChangeConfiguration(dvorak, ModalNoKeysMode);
        case _3:        return ChangeConfiguration(qwerty, GamingNoKeysMode);
        case _4:        return ChangeConfiguration(qwerty, BlackDesertNoKeysMode);
    }
    // all other keys
    return EnterMode(NormalTypingMode, Used);
}

ControlCode NormalEntryPoint_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // Modifier entry points
    if (i == 0) switch (inbuf[i]) {
        case RCtrl:    return EnterMode(RightCtrlMode, Clean);
    }

    // key entry points
    if (i == 2) switch (inbuf[i]) {
        case _Escape:  return EnterMode(EscapeMode, Clean);
    }

    // No special behavior activated. Apply normalTypingMode keyboard behavior.
    return EnterMode(NormalTypingMode, Used);
}

ControlCode ModalEntryPoint_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // Modifier entry points
    if (i == 0) switch (inbuf[i]) {
        case LAlt:     return EnterMode(LeftAltMode, Clean);
        case RAlt:     return EnterMode(RightAltMode, Clean);
        case RCtrl:    return EnterMode(RightCtrlMode, Clean);
    }

    // key entry points
    if (i == 2) switch (inbuf[i]) {
        case _Escape:   return EnterMode(EscapeMode, Clean);
        case _CapsLock: return EnterMode(CapsLockMode, Clean);
    }

    // No special behavior activated. Apply normalTypingMode keyboard behavior.
    return EnterMode(ModalTypingMode, Used);
}

ControlCode mapNormalKeyToCurrentLayout(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifiers
    if (i == 0) {
        if (inbuf[0] & LCtrl) SendRichKey(LCtrlMod(), outbuf); //map LCtrl to OS-specific key
        return SendModifiers(inbuf[0] & ~LCtrl, outbuf);
    }
    // map key
    if (i >= 2) {
        uint8_t inkey = inbuf[i];
        switch (inkey){
            case _CapsLock:         return SendRichKey(CapsLockMod(), outbuf);
        }
        // lookup key for current keyboard layout
        if (inkey >= _A && inkey <= _CapsLock){
            uint8_t shiftOn = outbuf[1] & (LShift | RShift);
            UnsetModifiers(LShift | RShift, outbuf);
            KeySpec keySpec = Keymap[CurrentLayout][inkey - _A];
            uint8_t mappedShift = shiftOn ? keySpec.shift2 : keySpec.shift1;
            uint8_t mappedKey = shiftOn ? keySpec.key2 : keySpec.key1;
            return SendKeyCombo(mappedShift, mappedKey, outbuf);
        }

        return SendKey(inkey, outbuf);
    }
}

// the keymap that just maps the key to the current keyboard layout
ControlCode NormalTyping_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
}

ControlCode ModalTyping_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // Modifier entry points
    if (i == 0) switch (inbuf[i]) {
        case LAlt:     return EnterMode(LeftAltMode, Clean);
        case RAlt:     return EnterMode(RightAltMode, Clean);
    }

    return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
}

ControlCode LeftAltMode_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) switch (inbuf[i]) {
        case LAlt:           return Continue;
    }
    // map 1st key
    if (i == 2) switch (inbuf[i]) {
        // map secondary modifier
        case _X:             return EnterMode(NumPadMode, Used);
        case _C:             return EnterMode(WindowSnapMode, Used);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]) {
        // alt mode modifiers
        case _Q:             return SendModifiers(LShift, outbuf);
        case _W:             return SendModifiers(LAlt, outbuf);
        case _E:             return SendModifiers(LCtrl, outbuf);
        case _R:             return SendModifiers(LGui, outbuf);
        // normalTypingMode mode modifiers
        case _A:             return EnterMode(LeftModMode, Used);
        case _S:             return EnterMode(LeftModMode, Used);
        case _D:             return EnterMode(LeftModMode, Used);
        case _F:             return EnterMode(LeftModMode, Used);

        // Left Hand keys
        case _Backtick:      return EnterMode(AltTabMode, Used);
        case _Tab:           return EnterMode(AltTabMode, Used);
        case _1:             return SendKey(_F1, outbuf);
        case _2:             return SendKey(_F2, outbuf);
        case _3:             return SendKey(_F3, outbuf);
        case _4:             return SendKey(_F4, outbuf);
        case _5:             return SendKey(_F5, outbuf);
        case _6:             return SendKey(_F6, outbuf);

        // Right Hand keys
        case _Y:             return SendKey(_Escape, outbuf);
        case _U:             return SendKey(_Home, outbuf);
        case _I:             return SendKey(_PgUp, outbuf);
        case _O:             return SendKey(_PgDn, outbuf);
        case _P:             return SendKey(_End, outbuf);
        case _LeftBracket:   return SendKey(_Enter, outbuf);
        case _RightBracket:  return SendKey(_Menu, outbuf);
        case _Backslash:     return EnterMode(AltTabMode, Used);
        case _Backspace:     return SendKey(_CapsLock, outbuf);
        case _H:             return SendKey(_Backspace, outbuf);
        case _J:             return SendKey(_Left, outbuf);
        case _K:             return SendKey(_Up, outbuf);
        case _L:             return SendKey(_Down, outbuf);
        case _Semicolon:     return SendKey(_Right, outbuf);
        case _Apostrophe:    return SendKey(_Delete, outbuf);
        case _7:             return SendKey(_F7, outbuf);
        case _8:             return SendKey(_F8, outbuf);
        case _9:             return SendKey(_F9, outbuf);
        case _0:             return SendKey(_F10, outbuf);
        case _Dash:          return SendKey(_F11, outbuf);
        case _Equals:        return SendKey(_F12, outbuf);
    }
    // all other keys
    return EnterMode(NormalTypingMode, Used);
}

ControlCode LeftModMode_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // return to LeftAltMode Mode when LAlt is the only key pressed
    if (inbuf[0] == LAlt && NumKeysOrModsPressed(inbuf) == 1)
        return EnterMode(LeftAltMode, Used);

    // map modifier
    if (i == 0) switch (inbuf[i]) {
        case LAlt:           return Continue;
    }
    // map any key
    if (i >= 2) switch (inbuf[i]) {
        // normalTypingMode mode modifiers
        case _A:             return SendModifiers(LShift, outbuf);
        case _S:             return SendModifiers(LAlt, outbuf);
        case _D:             return SendModifiers(LCtrl, outbuf);
        case _F:             return SendModifiers(LGui, outbuf);
        // Right hand keys
        case _7:             return SendKey(_7, outbuf);
        case _8:             return SendKey(_8, outbuf);
        case _9:             return SendKey(_9, outbuf);
        case _0:             return SendKey(_0, outbuf);
        case _Dash:          return SendKey(_LeftBracket, outbuf);
        case _Equals:        return SendKey(_RightBracket, outbuf);
        case _LeftBracket:   return SendKey(_ForwardSlash, outbuf);
        case _RightBracket:  return SendKey(_Equals, outbuf);
    }
    // all other keys
    return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
}

ControlCode RightAltMode_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) switch (inbuf[i]) {
        case RAlt:           return Continue;
    }
    // map key
    if (i >= 2) switch (inbuf[i]) {
         // alt mode modifiers
        case _U:             return SendModifiers(RGui, outbuf);
        case _I:             return SendModifiers(RCtrl, outbuf);
        case _O:             return SendModifiers(LAlt, outbuf); // RAlt is treated as Alt Grave and doesn't work as Meta key sometimes on Linux
        case _P:             return SendModifiers(RShift, outbuf);
        // normalTypingMode mode modifiers
        case _J:             return EnterMode(RightModMode, Used);
        case _K:             return EnterMode(RightModMode, Used);
        case _L:             return EnterMode(RightModMode, Used);
        case _Semicolon:     return EnterMode(RightModMode, Used);
        // Left Hand keys
        case _Tab:           return EnterMode(AltTabMode, Used);
        case _1:             return SendKey(_F1, outbuf);
        case _2:             return SendKey(_F2, outbuf);
        case _3:             return SendKey(_F3, outbuf);
        case _4:             return SendKey(_F4, outbuf);
        case _5:             return SendKey(_F5, outbuf);
        case _6:             return SendKey(_F6, outbuf);
        // left numpad
        case _Q:             return SendKeyCombo(RShift, _Semicolon, outbuf);
        case _W:             return SendKey(_1, outbuf);
        case _E:             return SendKey(_2, outbuf);
        case _R:             return SendKey(_3, outbuf);
        case _T:             return SendKey(_NumpadTimes, outbuf);

        case _A:             return SendKey(_Backspace, outbuf);
        case _S:             return SendKey(_4, outbuf);
        case _D:             return SendKey(_5, outbuf);
        case _F:             return SendKey(_6, outbuf);
        case _G:             return SendKey(_NumpadMinus, outbuf);

        case _Z:             return SendKey(_7, outbuf);
        case _X:             return SendKey(_8, outbuf);
        case _C:             return SendKey(_9, outbuf);
        case _V:             return SendKey(_NumpadDivide, outbuf);
        case _B:             return SendKey(_NumpadPlus, outbuf);
        case _Space:         return SendKey(_0, outbuf);
        // right hand numpad helpers
        case _Enter:         return SendKey(_Enter, outbuf);
        case _Fullstop:      return SendKey(_Fullstop, outbuf);
        case _Comma:         return SendKey(_Comma, outbuf);

        // Right Hand keys
        case _Backslash:     return EnterMode(AltTabMode, Used);
    }
    // all other keys
    return EnterMode(NormalTypingMode, Used);
}

ControlCode RightModMode_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // return to RightAltMode Mode when RAlt is the only key pressed
    if (inbuf[0] == RAlt && NumKeysOrModsPressed(inbuf) == 1)
        return EnterMode(RightAltMode, Used);

    // map modifier
    if (i == 0) switch (inbuf[i]){
        case RAlt:           return Continue;
    }
    // map any key
    if (i >= 2) switch (inbuf[i]) {
        // normalTypingMode mode modifiers
        case _J:             return SendModifiers(RGui, outbuf);
        case _K:             return SendModifiers(RCtrl, outbuf);
        case _L:             return SendModifiers(LAlt, outbuf); // RAlt is treated as Alt Grave and doesn't work as Meta key sometimes on Linux
        case _Semicolon:     return SendModifiers(RShift, outbuf);
        // Left Hand keys
        case _Backtick:      return SendKey(_Backtick, outbuf);
        case _1:             return SendKey(_1, outbuf);
        case _2:             return SendKey(_2, outbuf);
        case _3:             return SendKey(_3, outbuf);
        case _4:             return SendKey(_4, outbuf);
        case _5:             return SendKey(_5, outbuf);
        case _6:             return SendKey(_6, outbuf);
    }
    // all other keys
    return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
}

ControlCode AltTab_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        uint8_t Mods = 0;
        if (inbuf[i] & LAlt) Mods |= AppSwitchModifierKeycode(Left);
        if (inbuf[i] & RAlt) Mods |= AppSwitchModifierKeycode(Right);
        if (inbuf[i] & LShift) Mods |= LShift;
        if (inbuf[i] & RShift) Mods |= RShift;
        return SendModifiers(Mods, outbuf);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]) {
        // Tilde
        case _Backtick:      return SendKey(_Backtick, outbuf);
        // Tab
        case _Tab:           return SendKey(_Tab, outbuf);
        case _Backslash:     return SendKey(_Tab, outbuf);
        // Shift
        case _Q:             return SendModifiers(LShift, outbuf);
        case _P:             return SendModifiers(RShift, outbuf);
        // Escape
        case _Escape:        return SendKey(_Escape, outbuf);
        case _Y:             return SendKey(_Escape, outbuf);
        // arrow keys
        case _Left:          return SendKey(_Left, outbuf);
        case _Up:            return SendKey(_Up, outbuf);
        case _Down:          return SendKey(_Down, outbuf);
        case _Right:         return SendKey(_Right, outbuf);
        case _J:             return SendKey(_Left, outbuf);
        case _K:             return SendKey(_Up, outbuf);
        case _L:             return SendKey(_Down, outbuf);
        case _Semicolon:     return SendKey(_Right, outbuf);
    }
    // all other keys
    return InvalidKey();
}

ControlCode WindowSnap_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // exit condition: first key pressed is no longer _C
    if (inbuf[2] != _C) return EnterMode(LeftAltMode, Used);

    // map modifier
    if (i == 0) switch (inbuf[i]) {
        case LAlt:         return Continue;
    }
    // map first key
    if (i == 2) switch (inbuf[i]) { // must be _C because of exit guard
        default:           return SendModifiers(WindowSnapModifierKeycode(), outbuf);
    }
    // all other keys
    return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
}

ControlCode NumPad_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // exit condition: first key pressed is no longer _X
    if (inbuf[2] != _X)             return EnterMode(LeftAltMode, Used);

    // map modifier
    if (i == 0) switch (inbuf[i]) {
        case LAlt:                  return Continue;
        default:                    return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
    }
    // map first key
    if (i == 2) switch (inbuf[i]) { // must be _X because of exit guard
        default:                    return Continue;
    }
    // map subsequent keys
    if (i > 2)  switch (inbuf[i]) {
        case _7:                    return SendKey(_Numpad7, outbuf);
        case _8:                    return SendKey(_Numpad8, outbuf);
        case _9:                    return SendKey(_Numpad9, outbuf);
        case _0:                    return SendKey(_NumpadTimes, outbuf);
        case _Dash:                 return SendKey(_VolumeDown, outbuf);
        case _Equals:               return SendKey(_VolumeUp, outbuf);
        case _U:                    return SendKey(_Numpad4, outbuf);
        case _I:                    return SendKey(_Numpad5, outbuf);
        case _O:                    return SendKey(_Numpad6, outbuf);
        case _P:                    return SendKey(_NumpadMinus, outbuf);
        case _LeftBracket:          return SendKey(_NumpadEnter, outbuf);
        case _RightBracket:         return SendKey(_NumLock, outbuf);
        case _Backslash:            return SendKey(_NumLock, outbuf);
        case _H:                    return SendKey(_Backspace, outbuf);
        case _J:                    return SendKey(_Numpad1, outbuf);
        case _K:                    return SendKey(_Numpad2, outbuf);
        case _L:                    return SendKey(_Numpad3, outbuf);
        case _Semicolon:            return SendKey(_NumpadPlus, outbuf);
        case _Apostrophe:           return SendKeyCombo(LShift, _Dash, outbuf); // Underscore
        case _N:                    return SendKeyCombo(LShift, _Semicolon, outbuf); // Colon
        case _M:                    return SendKey(_Numpad0, outbuf);
        case _Comma:                return SendKey(_Comma, outbuf);
        case _Fullstop:             return SendKey(_NumpadDot, outbuf);
        case _ForwardSlash:         return SendKey(_NumpadDivide, outbuf);
        case _Space:                return SendKey(_Space, outbuf);
        case _Enter:                return SendKey(_Enter, outbuf);
    }
    // all other keys
    return InvalidKey();
}

// ================== Gaming Mode ===================

ControlCode GamingEntryPoint_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) switch (inbuf[i]) {
        case LShift:      return EnterMode(GamingShiftMode, Clean);
        case LCtrl:       return EnterMode(GamingCtrlMode, Clean);
        case LGui:        return SendKey(_Backspace, outbuf);
        case LAlt:        return EnterMode(GamingAltMode, Clean);
        case RCtrl:       return EnterMode(RightCtrlMode, Clean);
        default:          return Stop;
    }
    // map first key
    if (i == 2) switch (inbuf[i]) {
        case _Escape:     return EnterMode(EscapeMode, Clean);
        // LH function keys ==> RH function keys
        case _F1:             return SendKey(_F7, outbuf);
        case _F2:             return SendKey(_F8, outbuf);
        case _F3:             return SendKey(_F9, outbuf);
        case _F4:             return SendKey(_F10, outbuf);
        case _F5:             return SendKey(_F11, outbuf);
        case _F6:             return SendKey(_F12, outbuf);
        // LH numbers ==> LH function keys
        case _1:          return SendKey(_F1, outbuf);
        case _2:          return SendKey(_F2, outbuf);
        case _3:          return SendKey(_F3, outbuf);
        case _4:          return SendKey(_F4, outbuf);
        case _5:          return SendKey(_F5, outbuf);
        case _6:          return SendKey(_F6, outbuf);
        // custom modifiers
        case _Backtick:   return EnterMode(GamingBacktickMode, Clean);
        case _Tab:        return EnterMode(GamingTabMode, Clean);
        case _CapsLock:   return EnterMode(GamingCapsLockMode, Clean);
        case _Space:      return EnterMode(GamingSpaceMode, Clean);
    }
    // all other keys
    return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
}

ControlCode GamingBacktick_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]) {
        case _Backtick:      return Continue;
        case _Space:         return SendModifiers(LShift, outbuf);
        // backtick + row0 number ==> ctrl + LH function key
        case _1:             return SendKeyCombo(LCtrl, _F1, outbuf);
        case _2:             return SendKeyCombo(LCtrl, _F2, outbuf);
        case _3:             return SendKeyCombo(LCtrl, _F3, outbuf);
        case _4:             return SendKeyCombo(LCtrl, _F4, outbuf);
        case _5:             return SendKeyCombo(LCtrl, _F5, outbuf);
        case _6:             return SendKeyCombo(LCtrl, _F6, outbuf);
    }
    // all other keys
    return InvalidKey();
}

ControlCode GamingTab_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]){
        case _Tab:           return Continue;
        case _Space:         return SendModifiers(LShift, outbuf);
        // Tab + row1 letter ==> Alt + LH number
        case _Q:             return SendKeyCombo(LAlt, _1, outbuf);
        case _W:             return SendKeyCombo(LAlt, _2, outbuf);
        case _E:             return SendKeyCombo(LAlt, _3, outbuf);
        case _R:             return SendKeyCombo(LAlt, _4, outbuf);
        case _T:             return SendKeyCombo(LAlt, _5, outbuf);
        // Tab + row2 letter ==> Alt + RH number
        case _A:             return SendKeyCombo(LAlt, _6, outbuf);
        case _S:             return SendKeyCombo(LAlt, _7, outbuf);
        case _D:             return SendKeyCombo(LAlt, _8, outbuf);
        case _F:             return SendKeyCombo(LAlt, _9, outbuf);
        case _G:             return SendKeyCombo(LAlt, _0, outbuf);
    }
    // all other keys
    return InvalidKey();
}

ControlCode GamingCapsLock_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]){
        case _CapsLock:      return SendModifiers(LCtrl, outbuf);
        case _Space:         return SendModifiers(LShift, outbuf);
        // CapsLock + row1 letter ==> Ctrl + LH number
        case _Q:             return SendKeyCombo(LCtrl, _1, outbuf);
        case _W:             return SendKeyCombo(LCtrl, _2, outbuf);
        case _E:             return SendKeyCombo(LCtrl, _3, outbuf);
        case _R:             return SendKeyCombo(LCtrl, _4, outbuf);
        case _T:             return SendKeyCombo(LCtrl, _5, outbuf);
        // Capslock + row2 letter => Ctrl + RH number
        case _A:             return SendKeyCombo(LCtrl, _6, outbuf);
        case _S:             return SendKeyCombo(LCtrl, _7, outbuf);
        case _D:             return SendKeyCombo(LCtrl, _8, outbuf);
        case _F:             return SendKeyCombo(LCtrl, _9, outbuf);
        case _G:             return SendKeyCombo(LCtrl, _0, outbuf);
    }
    // all other keys
    return InvalidKey();
}

ControlCode GamingShift_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        uint8_t key = 0;
        if (inbuf[i] & LGui) {
            key = _Backspace;
        }

        uint8_t mods = inbuf[i] & ~LGui;

        return SendKeyCombo(mods, key, outbuf);
    }
    // map first key
    if (i == 2) switch (inbuf[i]) {
        case _CapsLock:   return EnterMode(GamingCapsLockMode, Clean);
        // Shift + row1 letter ==> Shift + LH number
        case _Q:             return SendKey(_1, outbuf);
        case _W:             return SendKey(_2, outbuf);
        case _E:             return SendKey(_3, outbuf);
        case _R:             return SendKey(_4, outbuf);
        case _T:             return SendKey(_5, outbuf);
        // Shift + row2 letter ==> Shift + RH number
        case _A:             return SendKey(_6, outbuf);
        case _S:             return SendKey(_7, outbuf);
        case _D:             return SendKey(_8, outbuf);
        case _F:             return SendKey(_9, outbuf);
        case _G:             return SendKey(_0, outbuf);
    }
    // all other keys
    return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
}

ControlCode GamingCtrl_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        if (inbuf[i] == LCtrl) {
            return Continue;
        }

        uint8_t key = 0;
        if (inbuf[i] & LGui) {
            key = _Backspace;
        }

        uint8_t mods = inbuf[i] & ~LGui;

        return SendKeyCombo(mods, key, outbuf);
    }
    // map subsequent keys
    if (i >= 2) {
        uint8_t mods = inbuf[0] & ~LGui;
        uint8_t key = inbuf[i];
        return SendKeyCombo(mods, key, outbuf);
    }
}

ControlCode GamingAlt_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        uint8_t key = 0;
        if (inbuf[i] & LGui) {
            key = _Backspace;
        }

        uint8_t mods = inbuf[i] & ~LGui & ~LAlt;

        return SendKeyCombo(mods, key, outbuf);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]){
        case _Backtick:      return EnterMode(AltTabMode, Used);
        case _Tab:           return EnterMode(AltTabMode, Used);
        case _CapsLock:      return SendModifiers(LCtrl, outbuf);
        // Space + R1,R2 letter keys ==> navigation keys
        case _Q:             return SendKey(_Home, outbuf);
        case _W:             return SendKey(_PgUp, outbuf);
        case _E:             return SendKey(_Up, outbuf);
        case _R:             return SendKey(_PgDn, outbuf);
        case _T:             return SendKey(_End, outbuf);

        case _A:             return SendKey(_Backspace, outbuf);
        case _S:             return SendKey(_Left, outbuf);
        case _D:             return SendKey(_Down, outbuf);
        case _F:             return SendKey(_Right, outbuf);
        case _G:             return SendKey(_Space, outbuf);
        // Space + R3 letter keys ==> misc extras
        case _Z:             return SendKey(_Insert, outbuf);
        case _X:             return SendKey(_Backslash, outbuf);
        case _C:             return SendKey(_Delete, outbuf);
        case _V:             return SendKey(_LeftBracket, outbuf);
        case _B:             return SendKey(_RightBracket, outbuf);
    }
    // all other keys
    return InvalidKey();
}

ControlCode GamingSpace_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]){
        case _Space:         return Continue;
        case _Backtick:      return EnterMode(GamingBacktickMode, Used);
        case _Tab:           return EnterMode(GamingTabMode, Used);
        case _CapsLock:      return EnterMode(GamingCapsLockMode, Used);
         // Space + row0 number ==> ctrl + LH function key
        case _1:             return SendKeyCombo(LCtrl, _F1, outbuf);
        case _2:             return SendKeyCombo(LCtrl, _F2, outbuf);
        case _3:             return SendKeyCombo(LCtrl, _F3, outbuf);
        case _4:             return SendKeyCombo(LCtrl, _F4, outbuf);
        case _5:             return SendKeyCombo(LCtrl, _F5, outbuf);
        case _6:             return SendKeyCombo(LCtrl, _F6, outbuf);
        // Space + R1 letter keys ==> LH number
        case _Q:             return SendKey(_1, outbuf);
        case _W:             return SendKey(_2, outbuf);
        case _E:             return SendKey(_3, outbuf);
        case _R:             return SendKey(_4, outbuf);
        case _T:             return SendKey(_5, outbuf);
        // Space + R2 letter keys ==> RH number
        case _A:             return SendKey(_6, outbuf);
        case _S:             return SendKey(_7, outbuf);
        case _D:             return SendKey(_8, outbuf);
        case _F:             return SendKey(_9, outbuf);
        case _G:             return SendKey(_0, outbuf);
        // Space + R3 letter keys ==> misc extras
        case _Z:             return SendOnlyKey(_NumpadMinus, outbuf);
        case _X:             return SendOnlyKey(_NumpadPlus, outbuf);
        case _C:             return SendOnlyKey(_Pause, outbuf);
        case _V:             return SendOnlyKey(_Pause, outbuf);
        case _B:             return SendOnlyKey(_Pause, outbuf);
    }
    // all other keys
    return InvalidKey();
}


// ================== BlackDesert Mode ===================

ControlCode BlackDesertEntryPoint_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) switch (inbuf[i]) {
        case LCtrl:       return SendKey(_Escape, outbuf);
        case LGui:        return SendKey(_Enter, outbuf);
        case LAlt:        return EnterMode(BlackDesertAltMode, Clean);
        case RCtrl:       return EnterMode(RightCtrlMode, Clean);
        default:          return EnterMode(NormalTypingMode, Used);
    }
    // map first key
    if (i == 2) switch (inbuf[i]) {
        case _Escape:     return EnterMode(EscapeMode, Clean);
        // LH function keys ==> RH function keys
        case _F1:             return SendKey(_F7, outbuf);
        case _F2:             return SendKey(_F8, outbuf);
        case _F3:             return SendKey(_F9, outbuf);
        case _F4:             return SendKey(_F10, outbuf);
        case _F5:             return SendKey(_F11, outbuf);
        case _F6:             return SendKey(_F12, outbuf);
        // LH numbers ==> LH function keys
        case _Backtick:   return SendKey(_Insert, outbuf);
        case _1:          return SendKey(_F1, outbuf);
        case _2:          return SendKey(_F2, outbuf);
        case _3:          return SendKey(_F3, outbuf);
        case _4:          return SendKey(_F4, outbuf);
        case _5:          return SendKey(_F5, outbuf);
        case _6:          return SendKey(_F6, outbuf);
        // custom modifiers
        case _CapsLock:   return EnterMode(BlackDesertCapsLockMode, Clean);
        case _Space:      return EnterMode(BlackDesertSpaceMode, Clean);
    }
    // all other keys
    return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
}

ControlCode BlackDesertCapsLock_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]){
        case _CapsLock:      return Continue;
        case _Space:         return SendModifiers(LCtrl, outbuf);
        // CapsLock + R1 letter keys ==> LH number
        case _Q:             return SendKey(_P, outbuf);
        case _W:             return SendKey(_O, outbuf);
        case _E:             return SendKey(_I, outbuf);
        case _R:             return SendKey(_U, outbuf);
        case _T:             return SendKey(_Y, outbuf);
        // CapsLock + R2 letter keys ==> RH number
        case _A:             return SendKey(_Semicolon, outbuf);
        case _S:             return SendKey(_L, outbuf);
        case _D:             return SendKey(_K, outbuf);
        case _F:             return SendKey(_J, outbuf);
        case _G:             return SendKey(_H, outbuf);
        // CapsLock + R3 letter keys ==> misc extras
        case _Z:             return SendKey(_Fullstop, outbuf);
        case _X:             return SendKey(_Comma, outbuf);
        case _C:             return SendKey(_M, outbuf);
        case _V:             return SendKey(_N, outbuf);
        case _B:             return SendKey(_B, outbuf);
    }
    // all other keys
    return InvalidKey();
}

ControlCode BlackDesertSpace_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]){
        case _Space:         return Continue;
         // Space + row0 number ==> ctrl + LH function key
        case _1:             return SendKey(_F7, outbuf);
        case _2:             return SendKey(_F8, outbuf);
        case _3:             return SendKey(_F9, outbuf);
        case _4:             return SendKey(_F10, outbuf);
        case _5:             return SendKey(_F11, outbuf);
        case _6:             return SendKey(_F12, outbuf);
        // Space + R1 letter keys ==> LH number
        case _Q:             return SendKey(_1, outbuf);
        case _W:             return SendKey(_2, outbuf);
        case _E:             return SendKey(_3, outbuf);
        case _R:             return SendKey(_4, outbuf);
        case _T:             return SendKey(_5, outbuf);
        // Space + R2 letter keys ==> RH number
        case _A:             return SendKey(_6, outbuf);
        case _S:             return SendKey(_7, outbuf);
        case _D:             return SendKey(_8, outbuf);
        case _F:             return SendKey(_9, outbuf);
        case _G:             return SendKey(_0, outbuf);
        // Space + R3 letter keys ==> misc extras
        case _Z:             return SendOnlyKey(_Left, outbuf);
        case _X:             return SendOnlyKey(_Up, outbuf);
        case _C:             return SendOnlyKey(_Down, outbuf);
        case _V:             return SendOnlyKey(_Right, outbuf);
        case _B:             return SendOnlyKey(_CapsLock, outbuf);
    }
    // all other keys
    return InvalidKey();
}

ControlCode BlackDesertAlt_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]){
        case _Backtick:      return EnterMode(AltTabMode, Used);
        case _Tab:           return EnterMode(AltTabMode, Used);
    }
    // all other keys
    return EnterMode(NormalTypingMode, Used);
}

// ****************************************************************************
// State Handling Callbacks
// ****************************************************************************

void HandleLastKeyReleased() {
    // send normalTypingMode keys on release of custom modifier if no other keys were pressed while it was held down
    if (CurrentModeState == Clean) switch (CurrentMode) {
        case EscapeMode: // send Escape on release
            PressAndReleaseKey((RichKey){ 0, _Escape } );
            break;
        case CapsLockMode: // send Escape on release
            PressAndReleaseKey((RichKey){ 0, _Escape } );
            break;
        case RightCtrlMode: // send RCtrl on releasecase RightCtrlMode: // send RCtrl on release
            PressAndReleaseKey((RichKey){ RCtrl, 0 } );
            break;
        case LeftAltMode: // send Left Alt on release
            PressAndReleaseKey((RichKey){ LAlt, 0 } );
            break;
        case RightAltMode: // send Right Alt on release
            PressAndReleaseKey((RichKey){ RAlt, 0 } );
            break;
        case GamingBacktickMode: // send Backtick on release
            PressAndReleaseKey((RichKey){ 0, _Backtick } );
            break;
        case GamingTabMode: // send Tab on release
            PressAndReleaseKey((RichKey){ 0, _Tab } );
            break;
        case GamingCtrlMode: // send Escape on release
            PressAndReleaseKey((RichKey){ 0, _Escape } );
            break;
        case GamingAltMode: // send LAlt on release
            PressAndReleaseKey((RichKey){ LAlt, 0 } );
            break;
        case GamingSpaceMode: // send Space on release
            PressAndReleaseKey((RichKey){ 0, _Space } );
            break;
        case BlackDesertCapsLockMode: // send Ctrl on release
            PressAndReleaseKey((RichKey){ LCtrl, 0 } );
            break;
        case BlackDesertSpaceMode: // send Space on release
            PressAndReleaseKey((RichKey){ 0, _Space } );
            break;
    }
    SetMode(EntryPointMode, Clean);
}

// ****************************************************************************
// Helper Functions
// ****************************************************************************

void LoadOSMode() {
    OSMode osMode = Windows;
    EEPROM.get( OSModeSlot, osMode );
    CurrentOSMode = osMode;
}

ControlCode ChangeOSMode(OSMode osMode) {
    CurrentModeState = Used;
    CurrentOSMode = osMode;
    EEPROM.put( OSModeSlot, osMode );
#if defined(ARDUINO_ARCH_RP2040)
    // Flush the change from the RAM mirror to flash.
    EEPROM.commit();
#endif
    Log("new OSMode: " + GetOSModeString(osMode));
    return Stop;
}

void SetMode(Mode mode, ModeState modeState) {
    Log("set Mode: " + GetModeString(mode));
    CurrentMode = mode;
    CurrentModeState = modeState;
}

ControlCode EnterMode(Mode mode, ModeState modeState) {
    SetMode(mode, modeState);
    return Restart;
}

ControlCode ChangeConfiguration(KeyboardLayout layout, Mode entryPointMode) {
    CurrentModeState = Used;
    CurrentLayout = layout;
    EntryPointMode = entryPointMode;
    Log("new entry point Mode: " + GetModeString(entryPointMode));
    return Stop;
}

ControlCode _sendKeyCombo(uint8_t mods, uint8_t keycode, uint8_t outbuf[8], bool realmods) {
    CurrentModeState = Used;
    MergeKeyIntoBuffer((RichKey){ mods, keycode }, outbuf, realmods);
    return Continue;
}

ControlCode SendKey(uint8_t keycode, uint8_t outbuf[8]) {
    return _sendKeyCombo(0, keycode, outbuf, false);
}

ControlCode SendModifiers(uint8_t mods, uint8_t outbuf[8]) {
    return _sendKeyCombo(mods, 0, outbuf, true);
}

ControlCode UnsetModifiers(uint8_t mods, uint8_t outbuf[8]) {
    outbuf[0] &= ~mods;
    return Continue;
}

ControlCode SendKeyCombo(uint8_t mods, uint8_t keycode, uint8_t outbuf[8]) {
    return _sendKeyCombo(mods, keycode, outbuf, false);
}

ControlCode SendOnlyKey(uint8_t keycode, uint8_t outbuf[8]) {
    return SendOnlyKeyCombo(0, keycode, outbuf);
}

ControlCode SendOnlyKeyCombo(uint8_t mods, uint8_t keycode, uint8_t outbuf[8]) {
    CurrentModeState = Used;
    OverwriteBufferWithKey(outbuf, (RichKey){ mods, keycode }, false);
    return Continue;
}

ControlCode SendRichKey(RichKey key, uint8_t outbuf[8]) {
    CurrentModeState = Used;
    MergeKeyIntoBuffer(key, outbuf, true);
    return Continue;
}

ControlCode InvalidKey() {
    CurrentModeState = Used;
    return Stop;
}

// map key presses according to the current mode
ControlCode MapKey(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    return KeyMaps[CurrentMode](inbuf, i, outbuf);
}


// ****************************************************************************
// Logging
// ****************************************************************************


String GetOSModeString(OSMode osMode) {
    switch (osMode){
        case Windows:    return "Win";
        case OSX:        return "OSX";
    }
}

String GetModeString(Mode mode) {
    switch (mode){
        case NormalNoKeysMode:        return "NormalNoKeys";
        case ModalNoKeysMode:         return "ModalNoKeys";
        case EscapeMode:              return "Escape";
        case RightCtrlMode:           return "RightCtrl";
        case NormalTypingMode:        return "NormalTyping";
        case ModalTypingMode:         return "ModalTyping";
        case LeftAltMode:             return "LeftAlt";
        case LeftModMode:             return "LeftMod";
        case RightAltMode:            return "RightAlt";
        case RightModMode:            return "RightMod";
        case AltTabMode:              return "AltTab";
        case WindowSnapMode:          return "WindowSnap";
        case NumPadMode:              return "NumPad";
        case GamingNoKeysMode:        return "GamingNoKeys";
        case GamingBacktickMode:      return "GamingBacktick";
        case GamingTabMode:           return "GamingTab";
        case GamingCapsLockMode:      return "GamingCapsLock";
        case GamingShiftMode:         return "GamingShift";
        case GamingCtrlMode:          return "GamingCtrl";
        case GamingAltMode:           return "GamingAlt";
        case GamingSpaceMode:         return "GamingSpace";
        case BlackDesertNoKeysMode:   return "BlackDesertNoKeys";
        case BlackDesertCapsLockMode: return "BlackDesertCapsLock";
        case BlackDesertSpaceMode:    return "BlackDesertSpace";
        case BlackDesertAltMode:      return "BlackDesertAlt";
        default:                      return "<unknown>";
    }
}

String GetModeStateString(ModeState modeState) {
    return (modeState == Used) ? "*" : "";
}

String GetLayoutString(KeyboardLayout layout) {
    switch (layout){
        case qwerty:    return "QY";
        case dvorak:    return "DV";
        // case dvorakProgrammer:   return "DVP";
    }
}

// ****************************************************************************
// Shared Function Implementations
// ****************************************************************************

void InitializeState() {
#if defined(ARDUINO_ARCH_RP2040)
    // On the RP2040 core, EEPROM is emulated in a flash sector and must be
    // initialised before use (and committed after writes, see ChangeOSMode).
    // The AVR core needs neither call.
    EEPROM.begin(256);
#endif
    LoadOSMode();
}

void TransformBuffer(uint8_t inbuf[8], uint8_t outbuf[8]) {
    if (NumKeysOrModsPressed(inbuf) == 0) {
        HandleLastKeyReleased();
    } else {
        int i = 0;
        while (i < 8) {
            if (i==1 || !inbuf[i]) {
                i++;
                continue;
            }
            switch (MapKey(inbuf, i, outbuf)) {
                case Continue: i++; break;
                case Stop: i=8; break;
                case Restart: i=0; break;
            }
        }
    }
}

String GetStateString() {
    String spaces = "                              ";
    String stateStr = "[" + GetOSModeString(CurrentOSMode) + "." + GetLayoutString(CurrentLayout) + "." + GetModeString(CurrentMode) + GetModeStateString(CurrentModeState) + "]";
    String neededSpaces = spaces.substring(0, 26 - stateStr.length());

    return stateStr + neededSpaces;
}
