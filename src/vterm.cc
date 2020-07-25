/* This file is part of Zutty.
 * Copyright (C) 2020 Tom Szilagyi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * See the file LICENSE for the full license.
 */

#include "pty.h"
#include "vterm.h"

#include <cstring>

namespace {

   using namespace zutty;
   using Key = VtKey;
   using InputSpec = Vterm::InputSpec;

   #define ESC "\x1b"
   #define CSI ESC "["
   #define SS3 ESC "O"
   // magic byte to act as a placeholder for the Modifier Code:
   #define MC "\xff"

   const InputSpec is_Ansi [] =
   {
      {Key::Backspace,   "\x7f"},
      {Key::Tab,         "\t"},
      {Key::Return,      "\r"},
      {Key::Insert,      CSI "2~"},    {Key::KP_Insert,   CSI "2~"},
      {Key::Delete,      CSI "3~"},    {Key::KP_Delete,   CSI "3~"},
      {Key::PageUp,      CSI "5~"},    {Key::KP_PageUp,   CSI "5~"},
      {Key::PageDown,    CSI "6~"},    {Key::KP_PageDown, CSI "6~"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_Mod_Ansi [] =
   {
      {Key::Insert,      CSI "2;" MC "~"},  {Key::KP_Insert,   CSI "2;" MC "~"},
      {Key::Delete,      CSI "3;" MC "~"},  {Key::KP_Delete,   CSI "3;" MC "~"},
      {Key::PageUp,      CSI "5;" MC "~"},  {Key::KP_PageUp,   CSI "5;" MC "~"},
      {Key::PageDown,    CSI "6;" MC "~"},  {Key::KP_PageDown, CSI "6;" MC "~"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_Ansi_FunctionKeys [] =
   {
      {Key::F1,          SS3 "P"},     {Key::KP_F1,       SS3 "P"},
      {Key::F2,          SS3 "Q"},     {Key::KP_F2,       SS3 "Q"},
      {Key::F3,          SS3 "R"},     {Key::KP_F3,       SS3 "R"},
      {Key::F4,          SS3 "S"},     {Key::KP_F4,       SS3 "S"},
      {Key::F5,          CSI "15~"},
      {Key::F6,          CSI "17~"},
      {Key::F7,          CSI "18~"},
      {Key::F8,          CSI "19~"},
      {Key::F9,          CSI "20~"},
      {Key::F10,         CSI "21~"},
      {Key::F11,         CSI "23~"},
      {Key::F12,         CSI "24~"},
      {Key::F13,         CSI "25~"},
      {Key::F14,         CSI "26~"},
      {Key::F15,         CSI "28~"},
      {Key::F16,         CSI "29~"},
      {Key::F17,         CSI "31~"},
      {Key::F18,         CSI "32~"},
      {Key::F19,         CSI "33~"},
      {Key::F20,         CSI "34~"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_Mod_Ansi_FunctionKeys [] =
   {
      {Key::F1,          CSI "1;" MC "P"},   {Key::KP_F1,    CSI "1;" MC "P"},
      {Key::F2,          CSI "1;" MC "Q"},   {Key::KP_F2,    CSI "1;" MC "Q"},
      {Key::F3,          CSI "1;" MC "R"},   {Key::KP_F3,    CSI "1;" MC "R"},
      {Key::F4,          CSI "1;" MC "S"},   {Key::KP_F4,    CSI "1;" MC "S"},
      {Key::F5,          CSI "15;" MC "~"},
      {Key::F6,          CSI "17;" MC "~"},
      {Key::F7,          CSI "18;" MC "~"},
      {Key::F8,          CSI "19;" MC "~"},
      {Key::F9,          CSI "20;" MC "~"},
      {Key::F10,         CSI "21;" MC "~"},
      {Key::F11,         CSI "23;" MC "~"},
      {Key::F12,         CSI "24;" MC "~"},
      {Key::F13,         CSI "25;" MC "~"},
      {Key::F14,         CSI "26;" MC "~"},
      {Key::F15,         CSI "28;" MC "~"},
      {Key::F16,         CSI "29;" MC "~"},
      {Key::F17,         CSI "31;" MC "~"},
      {Key::F18,         CSI "32;" MC "~"},
      {Key::F19,         CSI "33;" MC "~"},
      {Key::F20,         CSI "34;" MC "~"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_Ansi_KeypadKeys [] =
   {
      {Key::KP_Space,    " "},
      {Key::KP_Tab,      "\t"},
      {Key::KP_Enter,    "\r"},
      {Key::KP_Star,     "*"},
      {Key::KP_Plus,     "+"},
      {Key::KP_Comma,    ","},
      {Key::KP_Minus,    "-"},
      {Key::KP_Dot,      "."},
      {Key::KP_Slash,    "/"},
      {Key::KP_0,        "0"},
      {Key::KP_1,        "1"},
      {Key::KP_2,        "2"},
      {Key::KP_3,        "3"},
      {Key::KP_4,        "4"},
      {Key::KP_5,        "5"},
      {Key::KP_6,        "6"},
      {Key::KP_7,        "7"},
      {Key::KP_8,        "8"},
      {Key::KP_9,        "9"},
      {Key::KP_Equal,    "="},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_Appl_KeypadKeys [] =
   {
      {Key::KP_Space,    SS3 " "},
      {Key::KP_Tab,      SS3 "I"},
      {Key::KP_Enter,    SS3 "M"},
      {Key::KP_Star,     SS3 "j"},
      {Key::KP_Plus,     SS3 "k"},
      {Key::KP_Comma,    SS3 "l"},
      {Key::KP_Minus,    SS3 "m"},
      {Key::KP_Dot,      SS3 "n"},
      {Key::KP_Slash,    SS3 "o"},
      {Key::KP_0,        SS3 "p"},
      {Key::KP_1,        SS3 "q"},
      {Key::KP_2,        SS3 "r"},
      {Key::KP_3,        SS3 "s"},
      {Key::KP_4,        SS3 "t"},
      {Key::KP_5,        SS3 "u"},
      {Key::KP_6,        SS3 "v"},
      {Key::KP_7,        SS3 "w"},
      {Key::KP_8,        SS3 "x"},
      {Key::KP_9,        SS3 "y"},
      {Key::KP_Equal,    SS3 "X"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_Mod_Appl_KeypadKeys [] =
   {
      {Key::KP_Space,    SS3 MC " "},
      {Key::KP_Tab,      SS3 MC "I"},
      {Key::KP_Enter,    SS3 MC "M"},
      {Key::KP_Star,     SS3 MC "j"},
      {Key::KP_Plus,     SS3 MC "k"},
      {Key::KP_Comma,    SS3 MC "l"},
      {Key::KP_Minus,    SS3 MC "m"},
      {Key::KP_Dot,      SS3 MC "n"},
      {Key::KP_Slash,    SS3 MC "o"},
      {Key::KP_0,        SS3 MC "p"},
      {Key::KP_1,        SS3 MC "q"},
      {Key::KP_2,        SS3 MC "r"},
      {Key::KP_3,        SS3 MC "s"},
      {Key::KP_4,        SS3 MC "t"},
      {Key::KP_5,        SS3 MC "u"},
      {Key::KP_6,        SS3 MC "v"},
      {Key::KP_7,        SS3 MC "w"},
      {Key::KP_8,        SS3 MC "x"},
      {Key::KP_9,        SS3 MC "y"},
      {Key::KP_Equal,    SS3 MC "X"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_VT52_KeypadKeys [] =
   {
      {Key::KP_Space,    ESC "? "},
      {Key::KP_Tab,      ESC "?I"},
      {Key::KP_Enter,    ESC "?M"},
      {Key::KP_Star,     ESC "?j"},
      {Key::KP_Plus,     ESC "?k"},
      {Key::KP_Comma,    ESC "?l"},
      {Key::KP_Minus,    ESC "?m"},
      {Key::KP_Dot,      ESC "?n"},
      {Key::KP_Slash,    ESC "?o"},
      {Key::KP_0,        ESC "?p"},
      {Key::KP_1,        ESC "?q"},
      {Key::KP_2,        ESC "?r"},
      {Key::KP_3,        ESC "?s"},
      {Key::KP_4,        ESC "?t"},
      {Key::KP_5,        ESC "?u"},
      {Key::KP_6,        ESC "?v"},
      {Key::KP_7,        ESC "?w"},
      {Key::KP_8,        ESC "?x"},
      {Key::KP_9,        ESC "?y"},
      {Key::KP_Equal,    ESC "?X"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_VT52_FunctionKeys [] =
   {
      {Key::F1,          ESC "P"},      {Key::KP_F1,       ESC "P"},
      {Key::F2,          ESC "Q"},      {Key::KP_F2,       ESC "Q"},
      {Key::F3,          ESC "R"},      {Key::KP_F3,       ESC "R"},
      {Key::F4,          ESC "S"},      {Key::KP_F4,       ESC "S"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_Ansi_CursorKeys [] =
   {
      {Key::Up,          CSI "A"},      {Key::KP_Up,       CSI "A"},
      {Key::Down,        CSI "B"},      {Key::KP_Down,     CSI "B"},
      {Key::Right,       CSI "C"},      {Key::KP_Right,    CSI "C"},
      {Key::Left,        CSI "D"},      {Key::KP_Left,     CSI "D"},
      /* TODO is there a Begin key? */  {Key::KP_Begin,    CSI "E"},
      {Key::Home,        CSI "H"},      {Key::KP_Home,     CSI "H"},
      {Key::End,         CSI "F"},      {Key::KP_End,      CSI "F"},
   };

   const InputSpec is_Appl_CursorKeys [] =
   {
      {Key::Up,          SS3 "A"},      {Key::KP_Up,       SS3 "A"},
      {Key::Down,        SS3 "B"},      {Key::KP_Down,     SS3 "B"},
      {Key::Right,       SS3 "C"},      {Key::KP_Right,    SS3 "C"},
      {Key::Left,        SS3 "D"},      {Key::KP_Left,     SS3 "D"},
      /* TODO is there a Begin key? */  {Key::KP_Begin,    SS3 "E"},
      {Key::Home,        SS3 "H"},      {Key::KP_Home,     SS3 "H"},
      {Key::End,         SS3 "F"},      {Key::KP_End,      SS3 "F"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_Mod_CursorKeys [] =
   {
      {Key::Up,          CSI "1;" MC "A"},   {Key::KP_Up,     CSI "1;" MC "A"},
      {Key::Down,        CSI "1;" MC "B"},   {Key::KP_Down,   CSI "1;" MC "B"},
      {Key::Right,       CSI "1;" MC "C"},   {Key::KP_Right,  CSI "1;" MC "C"},
      {Key::Left,        CSI "1;" MC "D"},   {Key::KP_Left,   CSI "1;" MC "D"},
      /* TODO is there a Begin key? */       {Key::KP_Begin,  CSI "1;" MC "E"},
      {Key::Home,        CSI "1;" MC "H"},   {Key::KP_Home,   CSI "1;" MC "H"},
      {Key::End,         CSI "1;" MC "F"},   {Key::KP_End,    CSI "1;" MC "F"},
   };

   const InputSpec is_VT52_CursorKeys [] =
   {
      {Key::Up,          ESC "A"},      {Key::KP_Up,       ESC "A"},
      {Key::Down,        ESC "B"},      {Key::KP_Down,     ESC "B"},
      {Key::Right,       ESC "C"},      {Key::KP_Right,    ESC "C"},
      {Key::Left,        ESC "D"},      {Key::KP_Left,     ESC "D"},
      /* TODO is there a Begin key? */  {Key::KP_Begin,    ESC "E"},
      {Key::Home,        ESC "H"},      {Key::KP_Home,     ESC "H"},
      {Key::End,         ESC "F"},      {Key::KP_End,      ESC "F"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_ReturnKey_ANL [] =
   {
      {Key::Return,      "\r\n"},
      {Key::KP_Enter,    "\r\n"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_BackspaceKey_BkSp [] =
   {
      {Key::Backspace,   "\b"},
      {Key::NONE,        nullptr},
   };

   #undef ESC
   #undef CSI
   #undef SS3

   uint8_t
   getModifierCode (VtModifier modifiers)
   {
      switch (modifiers)
      {
      case VtModifier::none:              return 0;
      case VtModifier::shift:             return 2;
      case VtModifier::alt:               return 3;
      case VtModifier::shift_alt:         return 4;
      case VtModifier::control:           return 5;
      case VtModifier::shift_control:     return 6;
      case VtModifier::control_alt:       return 7;
      case VtModifier::shift_control_alt: return 8;
      }
      return 0;
   }


   void
   makePalette256 (CharVdev::Color p[])
   {
      // Standard colors
      p[  0] = {  0,   0,   0};
      p[  1] = {205,   0,   0};
      p[  2] = {  0, 205,   0};
      p[  3] = {205, 205,   0};
      p[  4] = {  0,   0, 238};
      p[  5] = {205,   0, 205};
      p[  6] = {  0, 205, 205};
      p[  7] = {229, 229, 229};

      // High-intensity colors
      p[  8] = {127, 127, 127};
      p[  9] = {255,   0,   0};
      p[ 10] = {  0, 255,   0};
      p[ 11] = {255, 255,   0};
      p[ 12] = { 92,  92, 255};
      p[ 13] = {255,   0, 255};
      p[ 14] = {  0, 255, 255};
      p[ 15] = {255, 255, 255};

      // 216 colors
      for (uint8_t r = 0; r < 6; ++r)
         for (uint8_t g = 0; g < 6; ++g)
            for (uint8_t b = 0; b < 6; ++b)
            {
               uint8_t ri = r ? 55 + 40 * r : 0;
               uint8_t gi = g ? 55 + 40 * g : 0;
               uint8_t bi = b ? 55 + 40 * b : 0;
               p[16 + 36 * r + 6 * g + b] = {ri, gi, bi};
            }

      // Grayscale colors
      for (uint8_t s = 0; s < 24; ++s)
      {
         uint8_t i = 8 + 10 * s;
         p[232 + s] = {i, i, i};
      }
   }

   /* These tables perform translation of built-in "hard" character sets
    * to 16-bit Unicode points. All sets are defined as 96 characters, even
    * those originally designated by DEC as 94-character sets.
    *
    * These tables are referenced by Vterm::charCodes (see below).
    */

   // Ref: https://en.wikipedia.org/wiki/DEC_Special_Graphics
   const uint16_t uc_DecSpec [] =
   {
      0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
      0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
      0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
      0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,

      0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
      0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
      0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
      0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,

      0x25c6, 0x2592, 0x2409, 0x240c, 0x240d, 0x240a, 0x00b0, 0x00b1,
      0x2424, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0x23ba,
      0x23bb, 0x2500, 0x23bc, 0x23bd, 0x251c, 0x2524, 0x2534, 0x252c,
      0x2502, 0x2264, 0x2265, 0x03c0, 0x2260, 0x00a3, 0x00b7, 0x0020,
   };

   // Ref: https://en.wikipedia.org/wiki/Multinational_Character_Set
   const uint16_t uc_DecSuppl [] =
   {
      0x0020, 0x00a1, 0x00a2, 0x00a3, 0x0024, 0x00a5, 0x0026, 0x00a7,
      0x00a4, 0x00a9, 0x00aa, 0x00ab, 0x002c, 0x002d, 0x002e, 0x002f,
      0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x0034, 0x00b5, 0x00b6, 0x00b7,
      0x0038, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x003e, 0x00bf,

      0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
      0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
      0x0050, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x0152,
      0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x0178, 0x005e, 0x00df,

      0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
      0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
      0x0070, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x0153,
      0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00ff, 0x007e, 0x007f,
   };

   // Ref: https://en.wikipedia.org/wiki/DEC_Technical_Character_Set
   const uint16_t uc_DecTechn [] =
   {
      0x0020, 0x23b7, 0x250c, 0x2500, 0x2320, 0x2321, 0x2502, 0x23a1,
      0x23a3, 0x23a4, 0x23a6, 0x239b, 0x239d, 0x239e, 0x23a0, 0x23a8,
      0x23ac, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
      0x0020, 0x0020, 0x0020, 0x0020, 0x2264, 0x2260, 0x2265, 0x222b,

      0x2234, 0x221d, 0x221e, 0x00f7, 0x0394, 0x2207, 0x03a6, 0x0393,
      0x223c, 0x2243, 0x0398, 0x00d7, 0x039b, 0x21d4, 0x21d2, 0x2261,
      0x03a0, 0x03a8, 0x0020, 0x03a3, 0x0020, 0x0020, 0x221a, 0x03a9,
      0x039e, 0x03a5, 0x2282, 0x2283, 0x2229, 0x222a, 0x2227, 0x2228,

      0x00ac, 0x03b1, 0x03b2, 0x03c7, 0x03b4, 0x03b5, 0x03c6, 0x03b3,
      0x03b7, 0x03b9, 0x03b8, 0x03ba, 0x03bb, 0x0020, 0x03bd, 0x2202,
      0x03c0, 0x03c8, 0x03c1, 0x03c3, 0x03c4, 0x0020, 0x0192, 0x03c9,
      0x03be, 0x03c5, 0x03b6, 0x2190, 0x2191, 0x2192, 0x2193, 0x007f,
   };

   // Ref: https://en.wikipedia.org/wiki/ISO/IEC_8859-1
   const uint16_t uc_IsoLatin1 [] =
   {
      0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
      0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
      0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
      0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,

      0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
      0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
      0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
      0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,

      0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
      0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
      0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
      0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff,
   };

   // Same as ASCII, but with Pound sign (0x00a3 in place of 0x0023)
   const uint16_t uc_IsoUK [] =
   {
      0x0020, 0x0021, 0x0022, 0x00a3, 0x0024, 0x0025, 0x0026, 0x0027,
      0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
      0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
      0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,

      0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
      0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
      0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
      0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,

      0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
      0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
      0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
      0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
   };
}

namespace zutty {

   Vterm::Vterm (uint16_t glyphPx_, uint16_t glyphPy_,
                 uint16_t winPx_, uint16_t winPy_,
                 uint16_t borderPx_, int ptyFd_)
      : winPx (winPx_)
      , winPy (winPy_)
      , nCols ((winPx - 2 * borderPx_) / glyphPx_)
      , nRows ((winPy - 2 * borderPx_) / glyphPy_)
      , borderPx (borderPx_)
      , glyphPx (glyphPx_)
      , glyphPy (glyphPy_)
      , ptyFd (ptyFd_)
      , refreshVideo ([] (const Frame&) {})
      , setTitle ([] (const std::string&) {})
      , frame_pri (winPx, winPy, nCols, nRows)
      , cf (&frame_pri)
   {
      makePalette256 (palette256);
      resetTerminal ();
   }

   void
   Vterm::setRefreshHandler (
      const std::function <void (const Frame&)>& refreshVideo_)
   {
      refreshVideo = refreshVideo_;
   }

   void
   Vterm::setTitleHandler (
      const std::function <void (const std::string&)>& setTitle_)
   {
      setTitle = setTitle_;
   }

   void
   Vterm::resize (uint16_t winPx_, uint16_t winPy_)
   {
      winPx = winPx_;
      winPy = winPy_;

      uint16_t nCols_ = (winPx - 2 * borderPx) / glyphPx;
      uint16_t nRows_ = (winPy - 2 * borderPx) / glyphPy;

      if (nCols == nCols_ && nRows == nRows_)
      {
         cf->winPx = winPx;
         cf->winPy = winPy;
         return;
      }

      hideCursor ();

      if (altScreenBufferMode)
      {
         frame_alt = Frame (winPx, winPy, nCols_, nRows_);
      }
      else
      {
         frame_pri.resize (winPx, winPy, nCols_, nRows_);
         frame_alt.freeCells ();
      }

      nCols = nCols_;
      nRows = nRows_;
      normalizeCursorPos ();
      showCursor ();

      struct winsize size;
      size.ws_col = nCols;
      size.ws_row = nRows;
      if (ioctl (ptyFd, TIOCSWINSZ, &size) < 0)
         throw std::runtime_error ("TIOCSWINSZ failed");
   }

   int
   Vterm::writePty (VtKey key, VtModifier modifiers_)
   {
      modifiers = modifiers_;
      const auto& spec = getInputSpec (key);
      if (modifiers == VtModifier::none)
      {
         return writePty (spec.input);
      }
      else
      {
         // substitute the MC token with the actual modifier code
         static char buf [32];
         int k = 0;
         for (const char* p = spec.input; *p != '\0'; ++p)
            if (*p == *MC)
               buf [k++] = '0' + getModifierCode (modifiers);
            else
               buf [k++] = *p;
         buf [k] = '\0';
         return writePty (buf);
      }
   }

   using Key = VtKey;
   using Mod = VtModifier;

   Vterm::InputSpecTable *
   Vterm::getInputSpecTable ()
   {
      static InputSpecTable ist [] =
      {
         { [this] () { return (autoNewlineMode == true); },
           is_ReturnKey_ANL
         },

         { [this] () { return (bkspSendsDel == false); },
           is_BackspaceKey_BkSp
         },

         { [this] () { return (compatLevel == CompatibilityLevel::VT52); },
           is_VT52_CursorKeys
         },
         { [this] () { return (compatLevel == CompatibilityLevel::VT52); },
           is_VT52_FunctionKeys
         },
         { [this] () { return (compatLevel == CompatibilityLevel::VT52 &&
                               keypadMode == KeypadMode::Application); },
           is_VT52_KeypadKeys
         },

         { [this] () { return (modifiers != Mod::none); },
           is_Mod_CursorKeys
         },
         { [this] () { return (cursorKeyMode == CursorKeyMode::Application); },
           is_Appl_CursorKeys
         },
         { [this] () { return (modifiers != Mod::none &&
                               keypadMode == KeypadMode::Application); },
           is_Mod_Appl_KeypadKeys
         },
         { [this] () { return (keypadMode == KeypadMode::Application); },
           is_Appl_KeypadKeys
         },

         // entries to use with modifier keys being held
         { [this] () { return (modifiers != Mod::none); },
           is_Mod_Ansi
         },
         { [this] () { return (modifiers != Mod::none); },
           is_Mod_Ansi_FunctionKeys
         },

         // default entries
         { [] () { return true; }, is_Ansi },
         { [] () { return true; }, is_Ansi_CursorKeys },
         { [] () { return true; }, is_Ansi_FunctionKeys },
         { [] () { return true; }, is_Ansi_KeypadKeys },

         // end marker to delimit iteration
         { [] () { return true; }, nullptr }
      };
      return ist;
   }

   void
   Vterm::resetInputSpecTable ()
   {
      for (InputSpecTable* e = getInputSpecTable (); e->specs != nullptr; ++e)
         e->visited = false;
   }

   const Vterm::InputSpec *
   Vterm::selectInputSpecs ()
   {
      InputSpecTable* ist = getInputSpecTable ();
      for (auto e = ist; e->specs != nullptr; ++e)
      {
         if (!e->visited)
         {
            e->visited = true;
            if (e->predicate ())
               return e->specs;
         }
      }
      return nullptr;
   }

   const Vterm::InputSpec &
   Vterm::getInputSpec (Key key)
   {
      static InputSpec nullSpec = {Key::NONE, ""};

      resetInputSpecTable ();
      const InputSpec* specs;
      while ((specs = selectInputSpecs ()) != nullptr)
      {
         for (int k = 0; specs [k].key != Key::NONE; ++k)
            if (specs [k].key == key)
               return specs [k];
      }

      return nullSpec;
   }

#define COLLECT_NUMERIC_PARAMS                                        \
   case '0': case '1': case '2': case '3': case '4':                  \
   case '5': case '6': case '7': case '8': case '9':                  \
      inputOps [nInputOps - 1] *= 10;                                 \
      inputOps [nInputOps - 1] += ch - '0';                           \
      break;                                                          \
   case ';':                                                          \
      if (nInputOps < maxEscOps)                                      \
         inputOps [nInputOps ++] = 0;                                 \
      else                                                            \
      {                                                               \
         logE << "inputOps full, increase maxEscOps (currently: "     \
              << maxEscOps << ")!" << std::endl;                      \
         setState (InputState::Normal);                               \
      }                                                               \
      break

   void
   Vterm::processInput (unsigned char* input, int inputSize)
   {
      lastEscBegin = 0;
      lastNormalBegin = 0;
      lastStopPos = 0;
      hideCursor ();
      for (readPos = 0; readPos < inputSize; ++readPos)
      {
         const unsigned char& ch = input [readPos];
         switch (inputState)
         {
         case InputState::Normal:
            switch (ch)
            {
            case '\x00': // ignore NUL
               break;
            case '\e':
               setState (InputState::Escape);
               inputOps [0] = 0;
               nInputOps = 1;
               lastEscBegin = readPos;
               break;
            case '\r': traceNormalInput (); inp_CR (); break;
            case '\f': // fall through, treat as LineFeed ('\n')
            case '\v': // fall through, treat as LineFeed ('\n')
            case '\n': traceNormalInput (); esc_IND (); break;
            case '\t': traceNormalInput (); inp_HT (); break;
            case '\b': traceNormalInput (); csi_CUB (); break;
            case '\a':
               traceNormalInput ();
               logI << "* Bell *" << std::endl;
               break;
            case '\x0e': traceNormalInput (); charsetState.gl = 1; break;
            case '\x0f': traceNormalInput (); charsetState.gl = 0; break;
            case '\x05': // ENQ - Enquiry
               traceNormalInput ();
               writePty ("This is Zutty.\r\n");
               break;
            default: inputGraphicChar (ch);
            }
            break;
         case InputState::Escape:
            switch (ch)
            {
            case '\x18': case '\x1a': // CAN and SUB interrupts ESC sequence
               setState (InputState::Normal); break;
            case '\e': // ESC restarts ESC sequence
               inputOps [0] = 0;
               nInputOps = 1;
               lastEscBegin = readPos;
               break;
            case '#': setState (InputState::Esc_Hash); break;
            case '%': setState (InputState::Esc_Pct); break;
            case '[': setState (InputState::CSI); break;
            case ']': argBuf.clear (); setState (InputState::OSC); break;
            case '(': case ')': case '*': case '+':
            case '-': case '.': case '/':
            case ',': case '$': // from ISO/IEC 2022 (absorbed, treat as no-op)
               scsDst = ch;
               scsMod = '\0';
               setState (InputState::SelectCharset);
               break;
            case 'D': esc_IND (); break;
            case 'M': esc_RI (); break;
            case 'E': esc_NEL (); break;
            case 'H': esc_HTS (); break;
            case 'N': charsetState.ss = 2; setState (InputState::Normal); break;
            case 'O': charsetState.ss = 3; setState (InputState::Normal); break;
            case 'P': argBuf.clear (); setState (InputState::DCS); break;
            case 'c': esc_RIS (); break;
            case '6': esc_BI (); break;
            case '7': esc_DECSC (); break;
            case '8': esc_DECRC (); break;
            case '9': esc_FI (); break;
            case '=':
               keypadMode = KeypadMode::Application;
               setState (InputState::Normal);
               break;
            case '>':
               keypadMode = KeypadMode::Normal;
               setState (InputState::Normal);
               break;
            case '<':
               compatLevel = CompatibilityLevel::VT400;
               setState (InputState::Normal);
               break;
            case '~': charsetState.gr = 1; setState (InputState::Normal); break;
            case 'n': charsetState.gl = 2; setState (InputState::Normal); break;
            case '}': charsetState.gr = 2; setState (InputState::Normal); break;
            case 'o': charsetState.gl = 3; setState (InputState::Normal); break;
            case '|': charsetState.gr = 3; setState (InputState::Normal); break;
            case '\\': setState (InputState::Normal); break; // ignore lone ST
            default: unhandledInput (ch); break;
            }
            break;
         case InputState::Esc_Hash:
            switch (ch)
            {
            case '3':
               logU << "DECDHL: Double-height, top half" << std::endl;
               setState (InputState::Normal);
               break;
            case '4':
               logU << "DECDHL: Double-height, bottom half" << std::endl;
               setState (InputState::Normal);
               break;
            case '5':
               logU << "DECSWL: Single-width line" << std::endl;
               setState (InputState::Normal);
               break;
            case '6':
               logU << "DECDWL: Double-width line" << std::endl;
               setState (InputState::Normal);
               break;
            case '8': esch_DECALN (); break;
            default: unhandledInput (ch); break;
            }
            break;
         case InputState::Esc_Pct:
            switch (ch)
            {
            case '@':
               logT << "Select charset: default (ISO-8859-1)" << std::endl;
               charsetState = CharsetState {};
               charsetState.g [charsetState.gr] = Charset::IsoLatin1;
               setState (InputState::Normal);
               break;
            case 'G':
               logT << "Select charset: UTF-8" << std::endl;
               charsetState = CharsetState {};
               setState (InputState::Normal);
               break;
            default: unhandledInput (ch); break;
            }
            break;
         case InputState::SelectCharset:
            if (ch < 0x30) // intermediate
               scsMod = ch;
            else
               esc_DCS (ch);
            break;
         case InputState::CSI:
            switch (ch)
            {
            COLLECT_NUMERIC_PARAMS;
            case '\e': setState (InputState::Normal); break;
            case 'A': csi_CUU (); break;
            case 'B': csi_CUD (); break;
            case 'C': csi_CUF (); break;
            case 'D': csi_CUB (); break;
            case 'E': csi_CNL (); break;
            case 'F': csi_CPL (); break;
            case 'G': csi_CHA (); break;
            case 'H': case 'f': csi_CUP (); break;
            case 'I': csi_CHT (); break;
            case 'J': csi_ED (); break;
            case 'K': csi_EL (); break;
            case 'L': csi_IL (); break;
            case 'M': csi_DL (); break;
            case 'P': csi_DCH (); break;
            case 'S': csi_SU (); break;
            case 'T': csi_SD (); break;
            case 'X': csi_ECH (); break;
            case 'Z': csi_CBT (); break;
            case '@': csi_ICH (); break;
            case '`': csi_HPA (); break;
            case 'a': csi_HPR (); break;
            case 'b': csi_REP (); break;
            case 'c': csi_priDA (); break;
            case 'd': csi_VPA (); break;
            case 'e': csi_VPR (); break;
            case 'g': csi_TBC (); break;
            case 'h': csi_SM (); break;
            case 'l': csi_RM (); break;
            case 'm': csi_SGR (); break;
            case 'n': csi_DSR (); break;
            case 'r': csi_STBM (); break;
            case 's': csi_SCOSC (); break;
            case 'u': csi_SCORC (); break;
            case '\"': setState (InputState::CSI_Quote); break;
            case '!': setState (InputState::CSI_Bang); break;
            case '?': setState (InputState::CSI_priv); break;
            case ' ': setState (InputState::CSI_SPC); break;
            case '>': setState (InputState::CSI_GT); break;
            case '\a': break; // ignore
            case '\b': // undo last character in CSI sequence:
               if (readPos && input [readPos - 1] == ';')
                  --nInputOps;
               else
                  inputOps [nInputOps - 1] /= 10;
               break;
            case '\t': inp_HT (); setState (InputState::CSI); break;
            case '\r': inp_CR (); setState (InputState::CSI); break;
            case '\f': // fall through
            case '\v': esc_IND (); setState (InputState::CSI); break;
            default: unhandledInput (ch); break;
            }
            break;
         case InputState::CSI_Bang:
            switch (ch)
            {
            case 'p': csi_DECSTR (); break;
            default: unhandledInput (ch); break;
            }
            break;
         case InputState::CSI_Quote:
            switch (ch)
            {
            case 'p': csiq_DECSCL (); break;
            default: unhandledInput (ch); break;
            }
            break;
         case InputState::CSI_SPC:
            switch (ch)
            {
            case '@': csi_ecma48_SL (); break;
            case 'A': csi_ecma48_SR (); break;
            default: unhandledInput (ch); break;
            }
            break;
         case InputState::CSI_GT:
            switch (ch)
            {
            COLLECT_NUMERIC_PARAMS;
            case 'c': csi_secDA (); break;
            default: unhandledInput (ch); break;
            }
            break;
         case InputState::CSI_priv:
            switch (ch)
            {
            COLLECT_NUMERIC_PARAMS;
            case '\e': setState (InputState::Normal); break;
            case 'h': csi_privSM (); break;
            case 'l': csi_privRM (); break;
            default: unhandledInput (ch); break;
            }
            break;
         case InputState::DCS:
            switch (ch)
            {
            case '\e': setState (InputState::DCS_Esc); break;
            default: argBuf.push_back (ch); break;
            }
            break;
         case InputState::DCS_Esc:
            switch (ch)
            {
            case '\\': handle_DCS (); break;
            default:
               argBuf.push_back ('\e');
               argBuf.push_back (ch);
               setState (InputState::DCS);
               break;
            }
            break;
         case InputState::OSC:
            switch (ch)
            {
            case '\a': handle_OSC (); break;
            case '\e': setState (InputState::OSC_Esc); break;
            default: argBuf.push_back (ch); break;
            }
            break;
         case InputState::OSC_Esc:
            switch (ch)
            {
            case '\\': handle_OSC (); break;
            default:
               argBuf.push_back ('\e');
               argBuf.push_back (ch);
               setState (InputState::OSC);
               break;
            }
            break;
         }
      }
      traceNormalInput ();
      showCursor ();
      redraw ();
   }

   const uint16_t* Vterm::charCodes [] =
   {
      // Sync this with enumerators of Charset!
      nullptr, // Dummy slot for UTF-8 (handled differently)
      uc_DecSpec,
      uc_DecSuppl,
      uc_DecSuppl, // Slot for 'User-preferred supplemental'
      uc_DecTechn,
      uc_IsoLatin1,
      uc_IsoUK
   };

} // namespace zutty
