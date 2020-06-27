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

   struct InputSpec
   {
      Key key;
      const char * ansi;
      const char * appl;
   };

   const InputSpec inputSpecs [] =
   {
      {Key::Insert,      "\x1b[2~",   nullptr},
      {Key::Delete,      "\x1b[3~",   nullptr},
      {Key::Home,        "\x1b[H",    "\x1bOH"},
      {Key::End,         "\x1b[F",    "\x1bOF"},
      {Key::Up,          "\x1b[A",    "\x1bOA"},
      {Key::Down,        "\x1b[B",    "\x1bOB"},
      {Key::Right,       "\x1b[C",    "\x1bOC"},
      {Key::Left,        "\x1b[D",    "\x1bOD"},
      {Key::PageUp,      "\x1b[5~",   nullptr},
      {Key::PageDown,    "\x1b[6~",   nullptr},
      {Key::F1,          "\x1bOP",    nullptr},
      {Key::F2,          "\x1bOQ",    nullptr},
      {Key::F3,          "\x1bOR",    nullptr},
      {Key::F4,          "\x1bOS",    nullptr},
      {Key::F5,          "\x1b[15~",  nullptr},
      {Key::F6,          "\x1b[17~",  nullptr},
      {Key::F7,          "\x1b[18~",  nullptr},
      {Key::F8,          "\x1b[19~",  nullptr},
      {Key::F9,          "\x1b[20~",  nullptr},
      {Key::F10,         "\x1b[21~",  nullptr},
      {Key::F11,         "\x1b[23~",  nullptr},
      {Key::F12,         "\x1b[24~",  nullptr},
   };

   const InputSpec &
   getInputSpec (Key key)
   {
      static InputSpec nullSpec = {Key::NONE, "", ""};
      size_t nSpecs = sizeof (inputSpecs) / sizeof (InputSpec);

      for (size_t k = 0; k < nSpecs; ++k)
         if (inputSpecs [k].key == key)
            return inputSpecs [k];

      return nullSpec;
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
      if (winPx == winPx_ && winPy == winPy_)
         return;

      winPx = winPx_;
      winPy = winPy_;
      nCols = winPx / glyphPx;
      nRows = winPy / glyphPy;
      cells = std::shared_ptr <CharVdev::Cell> (
         new CharVdev::Cell [nRows * nCols]);
      memset (cells.get (), 0, nRows * nCols * sizeof (CharVdev::Cell));
      cur = 0;
      scrollHead = 0;
      marginTop = 0;
      marginBottom = nRows;
      posX = 0;
      posY = 0;
      tabStops.clear ();
      originMode = OriginMode::Absolute;

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
      InputSpec spec = getInputSpec (key);
      const char * str;

      if (cursorKeyMode == CursorKeyMode::Application)
         str = spec.appl ? spec.appl : spec.ansi;
      else
         str = spec.ansi;

      return writePty (str);
   }

} // namespace zutty
