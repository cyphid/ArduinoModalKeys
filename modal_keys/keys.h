// https://www.win.tue.nl/~aeb/linux/kbd/scancodes-14.html

#if !defined(__KEYS_H_)
#define __KEYS_H_

#include <Arduino.h>

// Custom Flags
#define _CustomModifier   (1 << 0)

// Modifier Flags
#define LCtrl   (1 << 0)
#define LShift  (1 << 1)
#define LAlt    (1 << 2)
#define LGui    (1 << 3)
#define RCtrl   (1 << 4)
#define RShift  (1 << 5)
#define RAlt    (1 << 6)
#define RGui    (1 << 7)

// Key Scan Codes
//
// NOTE: KC_ prefixed `constexpr` constants, NOT `#define` macros named _A/_Up.
// Identifiers of the form _<uppercase> are reserved by the implementation:
// libstdc++ uses _Up/_Tp as template params and newlib <ctype.h> defines
// _U _L _N _S _P _C _X _B. As macros these leaked into system headers and broke
// the RP2040 build (the AVR core pulls in neither the STL nor newlib ctype).
constexpr uint8_t KC_A = 4;
constexpr uint8_t KC_B = 5;
constexpr uint8_t KC_C = 6;
constexpr uint8_t KC_D = 7;
constexpr uint8_t KC_E = 8;
constexpr uint8_t KC_F = 9;
constexpr uint8_t KC_G = 10;
constexpr uint8_t KC_H = 11;
constexpr uint8_t KC_I = 12;
constexpr uint8_t KC_J = 13;
constexpr uint8_t KC_K = 14;
constexpr uint8_t KC_L = 15;
constexpr uint8_t KC_M = 16;
constexpr uint8_t KC_N = 17;
constexpr uint8_t KC_O = 18;
constexpr uint8_t KC_P = 19;
constexpr uint8_t KC_Q = 20;
constexpr uint8_t KC_R = 21;
constexpr uint8_t KC_S = 22;
constexpr uint8_t KC_T = 23;
constexpr uint8_t KC_U = 24;
constexpr uint8_t KC_V = 25;
constexpr uint8_t KC_W = 26;
constexpr uint8_t KC_X = 27;
constexpr uint8_t KC_Y = 28;
constexpr uint8_t KC_Z = 29;
constexpr uint8_t KC_1 = 30;
constexpr uint8_t KC_2 = 31;
constexpr uint8_t KC_3 = 32;
constexpr uint8_t KC_4 = 33;
constexpr uint8_t KC_5 = 34;
constexpr uint8_t KC_6 = 35;
constexpr uint8_t KC_7 = 36;
constexpr uint8_t KC_8 = 37;
constexpr uint8_t KC_9 = 38;
constexpr uint8_t KC_0 = 39;
constexpr uint8_t KC_Enter = 40;
constexpr uint8_t KC_Escape = 41;
constexpr uint8_t KC_Backspace = 42;
constexpr uint8_t KC_Tab = 43;
constexpr uint8_t KC_Space = 44;
constexpr uint8_t KC_Dash = 45;
constexpr uint8_t KC_Equals = 46;
constexpr uint8_t KC_LeftBracket = 47;
constexpr uint8_t KC_RightBracket = 48;
constexpr uint8_t KC_Backslash = 49;
constexpr uint8_t KC_International2 = 50;
constexpr uint8_t KC_Semicolon = 51;
constexpr uint8_t KC_Apostrophe = 52;
constexpr uint8_t KC_Backtick = 53;
constexpr uint8_t KC_Comma = 54;
constexpr uint8_t KC_Fullstop = 55;
constexpr uint8_t KC_ForwardSlash = 56;
constexpr uint8_t KC_CapsLock = 57;
constexpr uint8_t KC_F1 = 58;
constexpr uint8_t KC_F2 = 59;
constexpr uint8_t KC_F3 = 60;
constexpr uint8_t KC_F4 = 61;
constexpr uint8_t KC_F5 = 62;
constexpr uint8_t KC_F6 = 63;
constexpr uint8_t KC_F7 = 64;
constexpr uint8_t KC_F8 = 65;
constexpr uint8_t KC_F9 = 66;
constexpr uint8_t KC_F10 = 67;
constexpr uint8_t KC_F11 = 68;
constexpr uint8_t KC_F12 = 69;
constexpr uint8_t KC_PrintScreen = 70;
constexpr uint8_t KC_ScrollLock = 71;
constexpr uint8_t KC_Pause = 72;
constexpr uint8_t KC_Insert = 73;
constexpr uint8_t KC_Home = 74;
constexpr uint8_t KC_PgUp = 75;
constexpr uint8_t KC_Delete = 76;
constexpr uint8_t KC_End = 77;
constexpr uint8_t KC_PgDn = 78;
constexpr uint8_t KC_Right = 79;
constexpr uint8_t KC_Left = 80;
constexpr uint8_t KC_Down = 81;
constexpr uint8_t KC_Up = 82;
constexpr uint8_t KC_NumLock = 83;
constexpr uint8_t KC_NumpadDivide = 84;
constexpr uint8_t KC_NumpadTimes = 85;
constexpr uint8_t KC_NumpadMinus = 86;
constexpr uint8_t KC_NumpadPlus = 87;
constexpr uint8_t KC_NumpadEnter = 88;
constexpr uint8_t KC_Numpad1 = 89;
constexpr uint8_t KC_Numpad2 = 90;
constexpr uint8_t KC_Numpad3 = 91;
constexpr uint8_t KC_Numpad4 = 92;
constexpr uint8_t KC_Numpad5 = 93;
constexpr uint8_t KC_Numpad6 = 94;
constexpr uint8_t KC_Numpad7 = 95;
constexpr uint8_t KC_Numpad8 = 96;
constexpr uint8_t KC_Numpad9 = 97;
constexpr uint8_t KC_Numpad0 = 98;
constexpr uint8_t KC_NumpadDot = 99;
constexpr uint8_t KC_International1 = 100;
constexpr uint8_t KC_Menu = 101;

constexpr uint8_t KC_F13 = 104;
constexpr uint8_t KC_F14 = 105;
constexpr uint8_t KC_F15 = 106;
constexpr uint8_t KC_F16 = 107;
constexpr uint8_t KC_F17 = 108;
constexpr uint8_t KC_F18 = 109;
constexpr uint8_t KC_F19 = 110;
constexpr uint8_t KC_F20 = 111;
constexpr uint8_t KC_F21 = 112;
constexpr uint8_t KC_F22 = 113;
constexpr uint8_t KC_F23 = 114;
constexpr uint8_t KC_F24 = 115;

constexpr uint8_t KC_Help = 117;

constexpr uint8_t KC_Undo = 122;
constexpr uint8_t KC_Cut = 123;
constexpr uint8_t KC_Copy = 124;
constexpr uint8_t KC_Paste = 125;

constexpr uint8_t KC_Mute = 127;
constexpr uint8_t KC_VolumeUp = 128;
constexpr uint8_t KC_VolumeDown = 129;


struct RichKey {
    uint8_t mods;
    uint8_t key;
    uint8_t flags;
};

struct KeySpec {
    uint8_t shift1; // shift modifiers when shift is not pressed
    uint8_t key1; // the key to map to when shift is not pressed
    uint8_t shift2; // shift modifiers when shift is pressed
    uint8_t key2; // the key to map to when shift is pressed
};

bool operator==(const RichKey& lhs, const RichKey& rhs);

#endif // __KEYS_H_
