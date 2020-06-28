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
}

namespace zutty {

   Vterm::Vterm (uint16_t glyphPx_, uint16_t glyphPy_,
                 uint16_t winPx_, uint16_t winPy_,
                 int ptyFd_)
      : winPx (winPx_)
      , winPy (winPy_)
      , nCols (winPx / glyphPx_)
      , nRows (winPy / glyphPy_)
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

      uint16_t nCols_ = winPx / glyphPx;
      uint16_t nRows_ = winPy / glyphPy;

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
} // namespace zutty
