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
   using Key = Vterm::VtKey;
   using InputSpec = Vterm::InputSpec;

   const InputSpec is_Ansi [] =
   {
      {Key::Return,      "\r"},
      {Key::Backspace,   "\x7f"},
      {Key::Insert,      "\x1b[2~"},
      {Key::Delete,      "\x1b[3~"},
      {Key::Home,        "\x1b[H"},
      {Key::End,         "\x1b[F"},
      {Key::PageUp,      "\x1b[5~"},
      {Key::PageDown,    "\x1b[6~"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_Application [] =
   {
      {Key::Home,        "\x1bOH"},
      {Key::End,         "\x1bOF"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_FunctionKeys [] =
   {
      {Key::F1,          "\x1bOP"},
      {Key::KP_F1,       "\x1bOP"},
      {Key::F2,          "\x1bOQ"},
      {Key::KP_F2,       "\x1bOQ"},
      {Key::F3,          "\x1bOR"},
      {Key::KP_F3,       "\x1bOR"},
      {Key::F4,          "\x1bOS"},
      {Key::KP_F4,       "\x1bOS"},
      {Key::F5,          "\x1b[15~"},
      {Key::F6,          "\x1b[17~"},
      {Key::F7,          "\x1b[18~"},
      {Key::F8,          "\x1b[19~"},
      {Key::F9,          "\x1b[20~"},
      {Key::F10,         "\x1b[21~"},
      {Key::F11,         "\x1b[23~"},
      {Key::F12,         "\x1b[24~"},
      {Key::F13,         "\x1b[25~"},
      {Key::F14,         "\x1b[26~"},
      {Key::F15,         "\x1b[28~"},
      {Key::F16,         "\x1b[29~"},
      {Key::F17,         "\x1b[31~"},
      {Key::F18,         "\x1b[32~"},
      {Key::F19,         "\x1b[33~"},
      {Key::F20,         "\x1b[34~"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_FunctionKeys_VT52 [] =
   {
      {Key::F1,          "\x1bP"},
      {Key::KP_F1,       "\x1bP"},
      {Key::F2,          "\x1bQ"},
      {Key::KP_F2,       "\x1bQ"},
      {Key::F3,          "\x1bR"},
      {Key::KP_F3,       "\x1bR"},
      {Key::F4,          "\x1bS"},
      {Key::KP_F4,       "\x1bS"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_ArrowKeys_AnsiCursor [] =
   {
      {Key::Up,          "\x1b[A"},
      {Key::Down,        "\x1b[B"},
      {Key::Right,       "\x1b[C"},
      {Key::Left,        "\x1b[D"},
   };

   const InputSpec is_ArrowKeys_AnsiApp [] =
   {
      {Key::Up,          "\x1bOA"},
      {Key::Down,        "\x1bOB"},
      {Key::Right,       "\x1bOC"},
      {Key::Left,        "\x1bOD"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_ArrowKeys_VT52 [] =
   {
      {Key::Up,          "\x1b""A"},
      {Key::Down,        "\x1b""B"},
      {Key::Right,       "\x1b""C"},
      {Key::Left,        "\x1b""D"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_NumpadKeys_Numeric [] =
   {
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
      {Key::KP_Minus,    "-"},
      {Key::KP_Comma,    ","},
      {Key::KP_Dot,      "."},
      {Key::KP_Enter,    "\r"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_NumpadKeys_Ansi [] =
   {
      {Key::KP_F1,       "\x1bOP"},
      {Key::KP_F2,       "\x1bOQ"},
      {Key::KP_F3,       "\x1bOR"},
      {Key::KP_F4,       "\x1bOS"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_NumpadKeys_AnsiApp [] =
   {
      {Key::KP_0,        "\x1bOp"},
      {Key::KP_1,        "\x1bOq"},
      {Key::KP_2,        "\x1bOr"},
      {Key::KP_3,        "\x1bOs"},
      {Key::KP_4,        "\x1bOt"},
      {Key::KP_5,        "\x1bOu"},
      {Key::KP_6,        "\x1bOv"},
      {Key::KP_7,        "\x1bOw"},
      {Key::KP_8,        "\x1bOx"},
      {Key::KP_9,        "\x1bOy"},
      {Key::KP_Minus,    "\x1bOm"},
      {Key::KP_Comma,    "\x1bOl"},
      {Key::KP_Dot,      "\x1bOn"},
      {Key::KP_Enter,    "\x1bOM"},
      {Key::NONE,        nullptr},
   };

   const InputSpec is_NumpadKeys_VT52App [] =
   {
      {Key::KP_0,        "\x1b?p"},
      {Key::KP_1,        "\x1b?q"},
      {Key::KP_2,        "\x1b?r"},
      {Key::KP_3,        "\x1b?s"},
      {Key::KP_4,        "\x1b?t"},
      {Key::KP_5,        "\x1b?u"},
      {Key::KP_6,        "\x1b?v"},
      {Key::KP_7,        "\x1b?w"},
      {Key::KP_8,        "\x1b?x"},
      {Key::KP_9,        "\x1b?y"},
      {Key::KP_Minus,    "\x1b?m"},
      {Key::KP_Comma,    "\x1b?l"},
      {Key::KP_Dot,      "\x1b?n"},
      {Key::KP_Enter,    "\x1b?M"},
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
      , refreshVideo ([] (const Vterm&) {})
      , cells (std::shared_ptr <CharVdev::Cell> (
                  new CharVdev::Cell [nRows * nCols]))
      , cur (0)
      , scrollHead (0)
      , marginTop (0)
      , marginBottom (nRows)
   {
      memset (cells.get (), 0, nRows * nCols * sizeof (CharVdev::Cell));

      makePalette256 (palette256);
   }

   void
   Vterm::setRefreshHandler (
      const std::function <void (const Vterm&)>& refreshVideo_)
   {
      refreshVideo = refreshVideo_;
   }

   void
   Vterm::resize (uint16_t winPx_, uint16_t winPy_)
   {
      winPx = winPx_;
      winPy = winPy_;

      uint16_t nCols_ = (winPx - 2 * borderPx) / glyphPx;
      uint16_t nRows_ = (winPy - 2 * borderPx) / glyphPy;

      if (nCols == nCols_ && nRows == nRows_)
         return;

      nCols = nCols_;
      nRows = nRows_;

      cells = std::shared_ptr <CharVdev::Cell> (
         new CharVdev::Cell [nRows * nCols]);
      memset (cells.get (), 0, nRows * nCols * sizeof (CharVdev::Cell));

      resetTerminal ();

      struct winsize size;
      size.ws_col = nCols;
      size.ws_row = nRows;
      if (ioctl (ptyFd, TIOCSWINSZ, &size) < 0)
         throw std::runtime_error ("TIOCSWINSZ failed");
   }

   void
   Vterm::copyCells (CharVdev::Cell * const dst)
   {
      constexpr const size_t cellSize = sizeof (CharVdev::Cell);

      CharVdev::Cell* p = dst;
      CharVdev::Cell* s = cells.get ();
      uint32_t n = marginTop * nCols;
      memcpy (p, s, n * cellSize);

      p += n;
      n = marginBottom * nCols - scrollHead;
      memcpy (p, s + scrollHead, n * cellSize);

      p += n;
      n = scrollHead - marginTop * nCols;
      memcpy (p, s + marginTop * nCols, n * cellSize);

      p += n;
      n = (nRows - marginBottom) * nCols;
      memcpy (p, s + marginBottom * nCols, n * cellSize);
   }

   int
   Vterm::writePty (VtKey key)
   {
      const auto& spec = getInputSpec (key);
      return writePty (spec.input);
   }

   using Key = Vterm::VtKey;

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

         { [this] () { return (numpadMode == NumpadMode::Numeric); },
           is_NumpadKeys_Numeric
         },

         { [this] () { return (compatLevel != CompatibilityLevel::VT52 &&
                               numpadMode == NumpadMode::Application); },
           is_NumpadKeys_AnsiApp
         },

         { [this] () { return (compatLevel == CompatibilityLevel::VT52 &&
                               numpadMode == NumpadMode::Application); },
           is_NumpadKeys_VT52App
         },

         { [this] () { return (compatLevel == CompatibilityLevel::VT52); },
           is_ArrowKeys_VT52
         },
         { [this] () { return (compatLevel == CompatibilityLevel::VT52); },
           is_FunctionKeys_VT52
         },

         { [this] () { return (cursorKeyMode == CursorKeyMode::Application); },
           is_ArrowKeys_AnsiApp
         },

         // default entries
         { [] () { return true; }, is_Ansi },
         { [] () { return true; }, is_NumpadKeys_Ansi },
         { [] () { return true; }, is_ArrowKeys_AnsiCursor },
         { [] () { return true; }, is_FunctionKeys },

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
      readPos = 0;
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
               numpadMode = NumpadMode::Application;
               setState (InputState::Normal);
               break;
            case '>':
               numpadMode = NumpadMode::Numeric;
               setState (InputState::Normal);
               break;
            case '~': charsetState.gr = 1; setState (InputState::Normal); break;
            case 'n': charsetState.gl = 2; setState (InputState::Normal); break;
            case '}': charsetState.gr = 2; setState (InputState::Normal); break;
            case 'o': charsetState.gl = 3; setState (InputState::Normal); break;
            case '|': charsetState.gr = 3; setState (InputState::Normal); break;
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
            case 'p': esc_RIS (); break;
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
      refreshVideo (* this);
   }

   const uint16_t* Vterm::charCodes [] =
   {
      // Sync this with enumerators of Charset!
      // N.B.: skip UTF8 as that is handled differently.
      uc_DecSpec,
      uc_DecSuppl,
      uc_DecSuppl, // Slot for 'User-preferred supplemental'
      uc_DecTechn,
      uc_IsoLatin1,
      uc_IsoUK
   };

} // namespace zutty
