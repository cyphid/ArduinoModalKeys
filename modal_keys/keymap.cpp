#include "modal_keys.h"
#include "keys.h"
#include "keymap.h"
#include "helpers.h"
#include "layout_qwerty.h"
#include "layout_dvorak.h"
// #include "layout_dvorak_programmer.h"

#include <EEPROM.h>

// Map of where in EEPROM storage to store each config variable.
// Slots are spaced 4 bytes apart: EEPROM.put/get serialize the full enum,
// which is 4 bytes on ARM (RP2040), so adjacent addresses would overlap.
#define OSModeSlot 0
#define KeyboardLayoutSlot 4
#define EntryPointModeSlot 8

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
void LoadConfiguration();
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
    return (RichKey){ LCtrl, 0 }; // unreachable; satisfies -Werror=return-type
}

RichKey LCtrlMod() {
    switch(CurrentOSMode){
        case Windows: return (RichKey) { LCtrl, 0 };
        case OSX: return (RichKey) { LCtrl, 0 };
    }
    return (RichKey){ LCtrl, 0 }; // unreachable; satisfies -Werror=return-type
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
    return 0; // unreachable; satisfies -Werror=return-type
}

uint8_t WindowSnapModifierKeycode() {
    switch(CurrentOSMode){
        case Windows: return LCtrl | LGui;
        case OSX: return LCtrl | LGui | LShift;
    }
    return LCtrl | LGui; // unreachable; satisfies -Werror=return-type
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
        case KC_Escape:    return Continue;
        case KC_F1:        return ChangeConfiguration(qwerty, NormalNoKeysMode);
        case KC_F2:        return ChangeConfiguration(dvorak, ModalNoKeysMode);
        case KC_F3:        return ChangeConfiguration(qwerty, GamingNoKeysMode);
        case KC_F4:        return ChangeConfiguration(qwerty, BlackDesertNoKeysMode);
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
        case KC_CapsLock:         return Continue;
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
        case KC_1:        return ChangeConfiguration(qwerty, NormalNoKeysMode);
        case KC_2:        return ChangeConfiguration(dvorak, ModalNoKeysMode);
        case KC_3:        return ChangeConfiguration(qwerty, GamingNoKeysMode);
        case KC_4:        return ChangeConfiguration(qwerty, BlackDesertNoKeysMode);
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
        case KC_Escape:  return EnterMode(EscapeMode, Clean);
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
        case KC_Escape:   return EnterMode(EscapeMode, Clean);
        case KC_CapsLock: return EnterMode(CapsLockMode, Clean);
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
            case KC_CapsLock:         return SendRichKey(CapsLockMod(), outbuf);
        }
        // lookup key for current keyboard layout
        if (inkey >= KC_A && inkey <= KC_CapsLock){
            uint8_t shiftOn = outbuf[1] & (LShift | RShift);
            UnsetModifiers(LShift | RShift, outbuf);
            KeySpec keySpec = Keymap[CurrentLayout][inkey - KC_A];
            uint8_t mappedShift = shiftOn ? keySpec.shift2 : keySpec.shift1;
            uint8_t mappedKey = shiftOn ? keySpec.key2 : keySpec.key1;
            return SendKeyCombo(mappedShift, mappedKey, outbuf);
        }

        return SendKey(inkey, outbuf);
    }
    return Continue; // i == 1 (reserved byte): nothing to map
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
        case KC_X:             return EnterMode(NumPadMode, Used);
        case KC_C:             return EnterMode(WindowSnapMode, Used);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]) {
        // alt mode modifiers
        case KC_Q:             return SendModifiers(LShift, outbuf);
        case KC_W:             return SendModifiers(LAlt, outbuf);
        case KC_E:             return SendModifiers(LCtrl, outbuf);
        case KC_R:             return SendModifiers(LGui, outbuf);
        // normalTypingMode mode modifiers
        case KC_A:             return EnterMode(LeftModMode, Used);
        case KC_S:             return EnterMode(LeftModMode, Used);
        case KC_D:             return EnterMode(LeftModMode, Used);
        case KC_F:             return EnterMode(LeftModMode, Used);

        // Left Hand keys
        case KC_Backtick:      return EnterMode(AltTabMode, Used);
        case KC_Tab:           return EnterMode(AltTabMode, Used);
        case KC_1:             return SendKey(KC_F1, outbuf);
        case KC_2:             return SendKey(KC_F2, outbuf);
        case KC_3:             return SendKey(KC_F3, outbuf);
        case KC_4:             return SendKey(KC_F4, outbuf);
        case KC_5:             return SendKey(KC_F5, outbuf);
        case KC_6:             return SendKey(KC_F6, outbuf);

        // Right Hand keys
        case KC_Y:             return SendKey(KC_Escape, outbuf);
        case KC_U:             return SendKey(KC_Home, outbuf);
        case KC_I:             return SendKey(KC_PgUp, outbuf);
        case KC_O:             return SendKey(KC_PgDn, outbuf);
        case KC_P:             return SendKey(KC_End, outbuf);
        case KC_LeftBracket:   return SendKey(KC_Enter, outbuf);
        case KC_RightBracket:  return SendKey(KC_Menu, outbuf);
        case KC_Backslash:     return EnterMode(AltTabMode, Used);
        case KC_Backspace:     return SendKey(KC_CapsLock, outbuf);
        case KC_H:             return SendKey(KC_Backspace, outbuf);
        case KC_J:             return SendKey(KC_Left, outbuf);
        case KC_K:             return SendKey(KC_Up, outbuf);
        case KC_L:             return SendKey(KC_Down, outbuf);
        case KC_Semicolon:     return SendKey(KC_Right, outbuf);
        case KC_Apostrophe:    return SendKey(KC_Delete, outbuf);
        case KC_7:             return SendKey(KC_F7, outbuf);
        case KC_8:             return SendKey(KC_F8, outbuf);
        case KC_9:             return SendKey(KC_F9, outbuf);
        case KC_0:             return SendKey(KC_F10, outbuf);
        case KC_Dash:          return SendKey(KC_F11, outbuf);
        case KC_Equals:        return SendKey(KC_F12, outbuf);
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
        case KC_A:             return SendModifiers(LShift, outbuf);
        case KC_S:             return SendModifiers(LAlt, outbuf);
        case KC_D:             return SendModifiers(LCtrl, outbuf);
        case KC_F:             return SendModifiers(LGui, outbuf);
        // Right hand keys
        case KC_7:             return SendKey(KC_7, outbuf);
        case KC_8:             return SendKey(KC_8, outbuf);
        case KC_9:             return SendKey(KC_9, outbuf);
        case KC_0:             return SendKey(KC_0, outbuf);
        case KC_Dash:          return SendKey(KC_LeftBracket, outbuf);
        case KC_Equals:        return SendKey(KC_RightBracket, outbuf);
        case KC_LeftBracket:   return SendKey(KC_ForwardSlash, outbuf);
        case KC_RightBracket:  return SendKey(KC_Equals, outbuf);
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
        case KC_U:             return SendModifiers(RGui, outbuf);
        case KC_I:             return SendModifiers(RCtrl, outbuf);
        case KC_O:             return SendModifiers(LAlt, outbuf); // RAlt is treated as Alt Grave and doesn't work as Meta key sometimes on Linux
        case KC_P:             return SendModifiers(RShift, outbuf);
        // normalTypingMode mode modifiers
        case KC_J:             return EnterMode(RightModMode, Used);
        case KC_K:             return EnterMode(RightModMode, Used);
        case KC_L:             return EnterMode(RightModMode, Used);
        case KC_Semicolon:     return EnterMode(RightModMode, Used);
        // Left Hand keys
        case KC_Tab:           return EnterMode(AltTabMode, Used);
        case KC_1:             return SendKey(KC_F1, outbuf);
        case KC_2:             return SendKey(KC_F2, outbuf);
        case KC_3:             return SendKey(KC_F3, outbuf);
        case KC_4:             return SendKey(KC_F4, outbuf);
        case KC_5:             return SendKey(KC_F5, outbuf);
        case KC_6:             return SendKey(KC_F6, outbuf);
        // left numpad
        case KC_Q:             return SendKeyCombo(RShift, KC_Semicolon, outbuf);
        case KC_W:             return SendKey(KC_1, outbuf);
        case KC_E:             return SendKey(KC_2, outbuf);
        case KC_R:             return SendKey(KC_3, outbuf);
        case KC_T:             return SendKey(KC_NumpadTimes, outbuf);

        case KC_A:             return SendKey(KC_Backspace, outbuf);
        case KC_S:             return SendKey(KC_4, outbuf);
        case KC_D:             return SendKey(KC_5, outbuf);
        case KC_F:             return SendKey(KC_6, outbuf);
        case KC_G:             return SendKey(KC_NumpadMinus, outbuf);

        case KC_Z:             return SendKey(KC_7, outbuf);
        case KC_X:             return SendKey(KC_8, outbuf);
        case KC_C:             return SendKey(KC_9, outbuf);
        case KC_V:             return SendKey(KC_NumpadDivide, outbuf);
        case KC_B:             return SendKey(KC_NumpadPlus, outbuf);
        case KC_Space:         return SendKey(KC_0, outbuf);
        // right hand numpad helpers
        case KC_Enter:         return SendKey(KC_Enter, outbuf);
        case KC_Fullstop:      return SendKey(KC_Fullstop, outbuf);
        case KC_Comma:         return SendKey(KC_Comma, outbuf);

        // Right Hand keys
        case KC_Backslash:     return EnterMode(AltTabMode, Used);
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
        case KC_J:             return SendModifiers(RGui, outbuf);
        case KC_K:             return SendModifiers(RCtrl, outbuf);
        case KC_L:             return SendModifiers(LAlt, outbuf); // RAlt is treated as Alt Grave and doesn't work as Meta key sometimes on Linux
        case KC_Semicolon:     return SendModifiers(RShift, outbuf);
        // Left Hand keys
        case KC_Backtick:      return SendKey(KC_Backtick, outbuf);
        case KC_1:             return SendKey(KC_1, outbuf);
        case KC_2:             return SendKey(KC_2, outbuf);
        case KC_3:             return SendKey(KC_3, outbuf);
        case KC_4:             return SendKey(KC_4, outbuf);
        case KC_5:             return SendKey(KC_5, outbuf);
        case KC_6:             return SendKey(KC_6, outbuf);
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
        case KC_Backtick:      return SendKey(KC_Backtick, outbuf);
        // Tab
        case KC_Tab:           return SendKey(KC_Tab, outbuf);
        case KC_Backslash:     return SendKey(KC_Tab, outbuf);
        // Shift
        case KC_Q:             return SendModifiers(LShift, outbuf);
        case KC_P:             return SendModifiers(RShift, outbuf);
        // Escape
        case KC_Escape:        return SendKey(KC_Escape, outbuf);
        case KC_Y:             return SendKey(KC_Escape, outbuf);
        // arrow keys
        case KC_Left:          return SendKey(KC_Left, outbuf);
        case KC_Up:            return SendKey(KC_Up, outbuf);
        case KC_Down:          return SendKey(KC_Down, outbuf);
        case KC_Right:         return SendKey(KC_Right, outbuf);
        case KC_J:             return SendKey(KC_Left, outbuf);
        case KC_K:             return SendKey(KC_Up, outbuf);
        case KC_L:             return SendKey(KC_Down, outbuf);
        case KC_Semicolon:     return SendKey(KC_Right, outbuf);
    }
    // all other keys
    return InvalidKey();
}

ControlCode WindowSnap_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // exit condition: first key pressed is no longer KC_C
    if (inbuf[2] != KC_C) return EnterMode(LeftAltMode, Used);

    // map modifier
    if (i == 0) switch (inbuf[i]) {
        case LAlt:         return Continue;
    }
    // map first key
    if (i == 2) switch (inbuf[i]) { // must be KC_C because of exit guard
        default:           return SendModifiers(WindowSnapModifierKeycode(), outbuf);
    }
    // all other keys
    return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
}

ControlCode NumPad_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // exit condition: first key pressed is no longer KC_X
    if (inbuf[2] != KC_X)             return EnterMode(LeftAltMode, Used);

    // map modifier
    if (i == 0) switch (inbuf[i]) {
        case LAlt:                  return Continue;
        default:                    return mapNormalKeyToCurrentLayout(inbuf, i, outbuf);
    }
    // map first key
    if (i == 2) switch (inbuf[i]) { // must be KC_X because of exit guard
        default:                    return Continue;
    }
    // map subsequent keys
    if (i > 2)  switch (inbuf[i]) {
        case KC_7:                    return SendKey(KC_Numpad7, outbuf);
        case KC_8:                    return SendKey(KC_Numpad8, outbuf);
        case KC_9:                    return SendKey(KC_Numpad9, outbuf);
        case KC_0:                    return SendKey(KC_NumpadTimes, outbuf);
        case KC_Dash:                 return SendKey(KC_VolumeDown, outbuf);
        case KC_Equals:               return SendKey(KC_VolumeUp, outbuf);
        case KC_U:                    return SendKey(KC_Numpad4, outbuf);
        case KC_I:                    return SendKey(KC_Numpad5, outbuf);
        case KC_O:                    return SendKey(KC_Numpad6, outbuf);
        case KC_P:                    return SendKey(KC_NumpadMinus, outbuf);
        case KC_LeftBracket:          return SendKey(KC_NumpadEnter, outbuf);
        case KC_RightBracket:         return SendKey(KC_NumLock, outbuf);
        case KC_Backslash:            return SendKey(KC_NumLock, outbuf);
        case KC_H:                    return SendKey(KC_Backspace, outbuf);
        case KC_J:                    return SendKey(KC_Numpad1, outbuf);
        case KC_K:                    return SendKey(KC_Numpad2, outbuf);
        case KC_L:                    return SendKey(KC_Numpad3, outbuf);
        case KC_Semicolon:            return SendKey(KC_NumpadPlus, outbuf);
        case KC_Apostrophe:           return SendKeyCombo(LShift, KC_Dash, outbuf); // Underscore
        case KC_N:                    return SendKeyCombo(LShift, KC_Semicolon, outbuf); // Colon
        case KC_M:                    return SendKey(KC_Numpad0, outbuf);
        case KC_Comma:                return SendKey(KC_Comma, outbuf);
        case KC_Fullstop:             return SendKey(KC_NumpadDot, outbuf);
        case KC_ForwardSlash:         return SendKey(KC_NumpadDivide, outbuf);
        case KC_Space:                return SendKey(KC_Space, outbuf);
        case KC_Enter:                return SendKey(KC_Enter, outbuf);
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
        case LGui:        return SendKey(KC_Backspace, outbuf);
        case LAlt:        return EnterMode(GamingAltMode, Clean);
        case RCtrl:       return EnterMode(RightCtrlMode, Clean);
        default:          return Stop;
    }
    // map first key
    if (i == 2) switch (inbuf[i]) {
        case KC_Escape:     return EnterMode(EscapeMode, Clean);
        // LH function keys ==> RH function keys
        case KC_F1:             return SendKey(KC_F7, outbuf);
        case KC_F2:             return SendKey(KC_F8, outbuf);
        case KC_F3:             return SendKey(KC_F9, outbuf);
        case KC_F4:             return SendKey(KC_F10, outbuf);
        case KC_F5:             return SendKey(KC_F11, outbuf);
        case KC_F6:             return SendKey(KC_F12, outbuf);
        // LH numbers ==> LH function keys
        case KC_1:          return SendKey(KC_F1, outbuf);
        case KC_2:          return SendKey(KC_F2, outbuf);
        case KC_3:          return SendKey(KC_F3, outbuf);
        case KC_4:          return SendKey(KC_F4, outbuf);
        case KC_5:          return SendKey(KC_F5, outbuf);
        case KC_6:          return SendKey(KC_F6, outbuf);
        // custom modifiers
        case KC_Backtick:   return EnterMode(GamingBacktickMode, Clean);
        case KC_Tab:        return EnterMode(GamingTabMode, Clean);
        case KC_CapsLock:   return EnterMode(GamingCapsLockMode, Clean);
        case KC_Space:      return EnterMode(GamingSpaceMode, Clean);
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
        case KC_Backtick:      return Continue;
        case KC_Space:         return SendModifiers(LShift, outbuf);
        // backtick + row0 number ==> ctrl + LH function key
        case KC_1:             return SendKeyCombo(LCtrl, KC_F1, outbuf);
        case KC_2:             return SendKeyCombo(LCtrl, KC_F2, outbuf);
        case KC_3:             return SendKeyCombo(LCtrl, KC_F3, outbuf);
        case KC_4:             return SendKeyCombo(LCtrl, KC_F4, outbuf);
        case KC_5:             return SendKeyCombo(LCtrl, KC_F5, outbuf);
        case KC_6:             return SendKeyCombo(LCtrl, KC_F6, outbuf);
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
        case KC_Tab:           return Continue;
        case KC_Space:         return SendModifiers(LShift, outbuf);
        // Tab + row1 letter ==> Alt + LH number
        case KC_Q:             return SendKeyCombo(LAlt, KC_1, outbuf);
        case KC_W:             return SendKeyCombo(LAlt, KC_2, outbuf);
        case KC_E:             return SendKeyCombo(LAlt, KC_3, outbuf);
        case KC_R:             return SendKeyCombo(LAlt, KC_4, outbuf);
        case KC_T:             return SendKeyCombo(LAlt, KC_5, outbuf);
        // Tab + row2 letter ==> Alt + RH number
        case KC_A:             return SendKeyCombo(LAlt, KC_6, outbuf);
        case KC_S:             return SendKeyCombo(LAlt, KC_7, outbuf);
        case KC_D:             return SendKeyCombo(LAlt, KC_8, outbuf);
        case KC_F:             return SendKeyCombo(LAlt, KC_9, outbuf);
        case KC_G:             return SendKeyCombo(LAlt, KC_0, outbuf);
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
        case KC_CapsLock:      return SendModifiers(LCtrl, outbuf);
        case KC_Space:         return SendModifiers(LShift, outbuf);
        // CapsLock + row1 letter ==> Ctrl + LH number
        case KC_Q:             return SendKeyCombo(LCtrl, KC_1, outbuf);
        case KC_W:             return SendKeyCombo(LCtrl, KC_2, outbuf);
        case KC_E:             return SendKeyCombo(LCtrl, KC_3, outbuf);
        case KC_R:             return SendKeyCombo(LCtrl, KC_4, outbuf);
        case KC_T:             return SendKeyCombo(LCtrl, KC_5, outbuf);
        // Capslock + row2 letter => Ctrl + RH number
        case KC_A:             return SendKeyCombo(LCtrl, KC_6, outbuf);
        case KC_S:             return SendKeyCombo(LCtrl, KC_7, outbuf);
        case KC_D:             return SendKeyCombo(LCtrl, KC_8, outbuf);
        case KC_F:             return SendKeyCombo(LCtrl, KC_9, outbuf);
        case KC_G:             return SendKeyCombo(LCtrl, KC_0, outbuf);
    }
    // all other keys
    return InvalidKey();
}

ControlCode GamingShift_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        uint8_t key = 0;
        if (inbuf[i] & LGui) {
            key = KC_Backspace;
        }

        uint8_t mods = inbuf[i] & ~LGui;

        return SendKeyCombo(mods, key, outbuf);
    }
    // map first key
    if (i == 2) switch (inbuf[i]) {
        case KC_CapsLock:   return EnterMode(GamingCapsLockMode, Clean);
        // Shift + row1 letter ==> Shift + LH number
        case KC_Q:             return SendKey(KC_1, outbuf);
        case KC_W:             return SendKey(KC_2, outbuf);
        case KC_E:             return SendKey(KC_3, outbuf);
        case KC_R:             return SendKey(KC_4, outbuf);
        case KC_T:             return SendKey(KC_5, outbuf);
        // Shift + row2 letter ==> Shift + RH number
        case KC_A:             return SendKey(KC_6, outbuf);
        case KC_S:             return SendKey(KC_7, outbuf);
        case KC_D:             return SendKey(KC_8, outbuf);
        case KC_F:             return SendKey(KC_9, outbuf);
        case KC_G:             return SendKey(KC_0, outbuf);
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
            key = KC_Backspace;
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
    return Continue; // i == 1 (reserved byte): nothing to map
}

ControlCode GamingAlt_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) {
        uint8_t key = 0;
        if (inbuf[i] & LGui) {
            key = KC_Backspace;
        }

        uint8_t mods = inbuf[i] & ~LGui & ~LAlt;

        return SendKeyCombo(mods, key, outbuf);
    }
    // map any key
    if (i >= 2) switch (inbuf[i]){
        case KC_Backtick:      return EnterMode(AltTabMode, Used);
        case KC_Tab:           return EnterMode(AltTabMode, Used);
        case KC_CapsLock:      return SendModifiers(LCtrl, outbuf);
        // Space + R1,R2 letter keys ==> navigation keys
        case KC_Q:             return SendKey(KC_Home, outbuf);
        case KC_W:             return SendKey(KC_PgUp, outbuf);
        case KC_E:             return SendKey(KC_Up, outbuf);
        case KC_R:             return SendKey(KC_PgDn, outbuf);
        case KC_T:             return SendKey(KC_End, outbuf);

        case KC_A:             return SendKey(KC_Backspace, outbuf);
        case KC_S:             return SendKey(KC_Left, outbuf);
        case KC_D:             return SendKey(KC_Down, outbuf);
        case KC_F:             return SendKey(KC_Right, outbuf);
        case KC_G:             return SendKey(KC_Space, outbuf);
        // Space + R3 letter keys ==> misc extras
        case KC_Z:             return SendKey(KC_Insert, outbuf);
        case KC_X:             return SendKey(KC_Backslash, outbuf);
        case KC_C:             return SendKey(KC_Delete, outbuf);
        case KC_V:             return SendKey(KC_LeftBracket, outbuf);
        case KC_B:             return SendKey(KC_RightBracket, outbuf);
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
        case KC_Space:         return Continue;
        case KC_Backtick:      return EnterMode(GamingBacktickMode, Used);
        case KC_Tab:           return EnterMode(GamingTabMode, Used);
        case KC_CapsLock:      return EnterMode(GamingCapsLockMode, Used);
         // Space + row0 number ==> ctrl + LH function key
        case KC_1:             return SendKeyCombo(LCtrl, KC_F1, outbuf);
        case KC_2:             return SendKeyCombo(LCtrl, KC_F2, outbuf);
        case KC_3:             return SendKeyCombo(LCtrl, KC_F3, outbuf);
        case KC_4:             return SendKeyCombo(LCtrl, KC_F4, outbuf);
        case KC_5:             return SendKeyCombo(LCtrl, KC_F5, outbuf);
        case KC_6:             return SendKeyCombo(LCtrl, KC_F6, outbuf);
        // Space + R1 letter keys ==> LH number
        case KC_Q:             return SendKey(KC_1, outbuf);
        case KC_W:             return SendKey(KC_2, outbuf);
        case KC_E:             return SendKey(KC_3, outbuf);
        case KC_R:             return SendKey(KC_4, outbuf);
        case KC_T:             return SendKey(KC_5, outbuf);
        // Space + R2 letter keys ==> RH number
        case KC_A:             return SendKey(KC_6, outbuf);
        case KC_S:             return SendKey(KC_7, outbuf);
        case KC_D:             return SendKey(KC_8, outbuf);
        case KC_F:             return SendKey(KC_9, outbuf);
        case KC_G:             return SendKey(KC_0, outbuf);
        // Space + R3 letter keys ==> misc extras
        case KC_Z:             return SendOnlyKey(KC_NumpadMinus, outbuf);
        case KC_X:             return SendOnlyKey(KC_NumpadPlus, outbuf);
        case KC_C:             return SendOnlyKey(KC_Pause, outbuf);
        case KC_V:             return SendOnlyKey(KC_Pause, outbuf);
        case KC_B:             return SendOnlyKey(KC_Pause, outbuf);
    }
    // all other keys
    return InvalidKey();
}


// ================== BlackDesert Mode ===================

ControlCode BlackDesertEntryPoint_keymap(uint8_t inbuf[8], uint8_t i, uint8_t outbuf[8]) {
    // map modifier
    if (i == 0) switch (inbuf[i]) {
        case LCtrl:       return SendKey(KC_Escape, outbuf);
        case LGui:        return SendKey(KC_Enter, outbuf);
        case LAlt:        return EnterMode(BlackDesertAltMode, Clean);
        case RCtrl:       return EnterMode(RightCtrlMode, Clean);
        default:          return EnterMode(NormalTypingMode, Used);
    }
    // map first key
    if (i == 2) switch (inbuf[i]) {
        case KC_Escape:     return EnterMode(EscapeMode, Clean);
        // LH function keys ==> RH function keys
        case KC_F1:             return SendKey(KC_F7, outbuf);
        case KC_F2:             return SendKey(KC_F8, outbuf);
        case KC_F3:             return SendKey(KC_F9, outbuf);
        case KC_F4:             return SendKey(KC_F10, outbuf);
        case KC_F5:             return SendKey(KC_F11, outbuf);
        case KC_F6:             return SendKey(KC_F12, outbuf);
        // LH numbers ==> LH function keys
        case KC_Backtick:   return SendKey(KC_Insert, outbuf);
        case KC_1:          return SendKey(KC_F1, outbuf);
        case KC_2:          return SendKey(KC_F2, outbuf);
        case KC_3:          return SendKey(KC_F3, outbuf);
        case KC_4:          return SendKey(KC_F4, outbuf);
        case KC_5:          return SendKey(KC_F5, outbuf);
        case KC_6:          return SendKey(KC_F6, outbuf);
        // custom modifiers
        case KC_CapsLock:   return EnterMode(BlackDesertCapsLockMode, Clean);
        case KC_Space:      return EnterMode(BlackDesertSpaceMode, Clean);
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
        case KC_CapsLock:      return Continue;
        case KC_Space:         return SendModifiers(LCtrl, outbuf);
        // CapsLock + R1 letter keys ==> LH number
        case KC_Q:             return SendKey(KC_P, outbuf);
        case KC_W:             return SendKey(KC_O, outbuf);
        case KC_E:             return SendKey(KC_I, outbuf);
        case KC_R:             return SendKey(KC_U, outbuf);
        case KC_T:             return SendKey(KC_Y, outbuf);
        // CapsLock + R2 letter keys ==> RH number
        case KC_A:             return SendKey(KC_Semicolon, outbuf);
        case KC_S:             return SendKey(KC_L, outbuf);
        case KC_D:             return SendKey(KC_K, outbuf);
        case KC_F:             return SendKey(KC_J, outbuf);
        case KC_G:             return SendKey(KC_H, outbuf);
        // CapsLock + R3 letter keys ==> misc extras
        case KC_Z:             return SendKey(KC_Fullstop, outbuf);
        case KC_X:             return SendKey(KC_Comma, outbuf);
        case KC_C:             return SendKey(KC_M, outbuf);
        case KC_V:             return SendKey(KC_N, outbuf);
        case KC_B:             return SendKey(KC_B, outbuf);
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
        case KC_Space:         return Continue;
         // Space + row0 number ==> ctrl + LH function key
        case KC_1:             return SendKey(KC_F7, outbuf);
        case KC_2:             return SendKey(KC_F8, outbuf);
        case KC_3:             return SendKey(KC_F9, outbuf);
        case KC_4:             return SendKey(KC_F10, outbuf);
        case KC_5:             return SendKey(KC_F11, outbuf);
        case KC_6:             return SendKey(KC_F12, outbuf);
        // Space + R1 letter keys ==> LH number
        case KC_Q:             return SendKey(KC_1, outbuf);
        case KC_W:             return SendKey(KC_2, outbuf);
        case KC_E:             return SendKey(KC_3, outbuf);
        case KC_R:             return SendKey(KC_4, outbuf);
        case KC_T:             return SendKey(KC_5, outbuf);
        // Space + R2 letter keys ==> RH number
        case KC_A:             return SendKey(KC_6, outbuf);
        case KC_S:             return SendKey(KC_7, outbuf);
        case KC_D:             return SendKey(KC_8, outbuf);
        case KC_F:             return SendKey(KC_9, outbuf);
        case KC_G:             return SendKey(KC_0, outbuf);
        // Space + R3 letter keys ==> misc extras
        case KC_Z:             return SendOnlyKey(KC_Left, outbuf);
        case KC_X:             return SendOnlyKey(KC_Up, outbuf);
        case KC_C:             return SendOnlyKey(KC_Down, outbuf);
        case KC_V:             return SendOnlyKey(KC_Right, outbuf);
        case KC_B:             return SendOnlyKey(KC_CapsLock, outbuf);
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
        case KC_Backtick:      return EnterMode(AltTabMode, Used);
        case KC_Tab:           return EnterMode(AltTabMode, Used);
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
            PressAndReleaseKey((RichKey){ 0, KC_Escape } );
            break;
        case CapsLockMode: // send Escape on release
            PressAndReleaseKey((RichKey){ 0, KC_Escape } );
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
            PressAndReleaseKey((RichKey){ 0, KC_Backtick } );
            break;
        case GamingTabMode: // send Tab on release
            PressAndReleaseKey((RichKey){ 0, KC_Tab } );
            break;
        case GamingCtrlMode: // send Escape on release
            PressAndReleaseKey((RichKey){ 0, KC_Escape } );
            break;
        case GamingAltMode: // send LAlt on release
            PressAndReleaseKey((RichKey){ LAlt, 0 } );
            break;
        case GamingSpaceMode: // send Space on release
            PressAndReleaseKey((RichKey){ 0, KC_Space } );
            break;
        case BlackDesertCapsLockMode: // send Ctrl on release
            PressAndReleaseKey((RichKey){ LCtrl, 0 } );
            break;
        case BlackDesertSpaceMode: // send Space on release
            PressAndReleaseKey((RichKey){ 0, KC_Space } );
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
    // A freshly flashed/erased EEPROM (or flash-emulated EEPROM on the RP2040)
    // reads back as 0xFF bytes, which is not a valid OSMode. Fall back to the
    // Windows default rather than running with an unknown OS mode.
    if (osMode != Windows && osMode != OSX) {
        osMode = Windows;
    }
    CurrentOSMode = osMode;
}

void LoadConfiguration() {
    KeyboardLayout layout = CurrentLayout;
    Mode entryPointMode = EntryPointMode;
    EEPROM.get( KeyboardLayoutSlot, layout );
    EEPROM.get( EntryPointModeSlot, entryPointMode );
    // Validate both values: freshly flashed/erased EEPROM reads as 0xFF bytes,
    // and only the entry-point modes selectable via ChangeConfiguration are
    // legal here. If either value is invalid, keep the compiled-in defaults.
    bool layoutValid = (layout == qwerty || layout == dvorak);
    bool modeValid = (entryPointMode == NormalNoKeysMode ||
                      entryPointMode == ModalNoKeysMode ||
                      entryPointMode == GamingNoKeysMode ||
                      entryPointMode == BlackDesertNoKeysMode);
    if (layoutValid && modeValid) {
        CurrentLayout = layout;
        EntryPointMode = entryPointMode;
        CurrentMode = entryPointMode;
    }
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
    EEPROM.put( KeyboardLayoutSlot, layout );
    EEPROM.put( EntryPointModeSlot, entryPointMode );
#if defined(ARDUINO_ARCH_RP2040)
    // Flush the change from the RAM mirror to flash.
    EEPROM.commit();
#endif
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
    return "<unknown>"; // unreachable; satisfies -Werror=return-type
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
    return "<unknown>"; // unreachable; satisfies -Werror=return-type
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
    LoadConfiguration();
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
