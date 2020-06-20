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
      , cells (std::shared_ptr <CharVdev::Cell> (
                  new CharVdev::Cell [nRows * nCols]))
      , cur (0)
      , top (0)
   {
      memset (cells.get (), 0, nRows * nCols * sizeof (CharVdev::Cell));
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
      top = 0;
      posX = 0;
      posY = 0;

      struct winsize size;
      size.ws_col = nCols;
      size.ws_row = nRows;
      if (ioctl (ptyFd, TIOCSWINSZ, &size) < 0)
         throw std::runtime_error ("TIOCSWINSZ failed");
   }

   void
   Vterm::copyCells (CharVdev::Cell * const dst)
   {
      uint32_t end = nRows * nCols;
      memcpy (dst, cells.get () + top, (end - top) * sizeof (CharVdev::Cell));
      memcpy (dst + (end - top), cells.get (), top * sizeof (CharVdev::Cell));
   }

} // namespace zutty
