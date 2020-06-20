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

#include <iostream>

namespace zutty {

   Vterm::Vterm (uint16_t glyphPx_, uint16_t glyphPy_,
                 uint16_t winPx_, uint16_t winPy_,
                 int ptyFd_)
      : glyphPx (glyphPx_)
      , glyphPy (glyphPy_)
      , winPx (winPx_)
      , winPy (winPy_)
      , nCols (winPx / glyphPx)
      , nRows (winPy / glyphPy)
      , ptyFd (ptyFd_)
      , cells (std::shared_ptr <CharVdev::Cell> (
                  new CharVdev::Cell [nRows * nCols]))
      , cp (cells.get ())
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
      cp = cells.get ();
      curX = 0;
      curY = 0;

      struct winsize size;
      size.ws_col = nCols;
      size.ws_row = nRows;
      if (ioctl (ptyFd, TIOCSWINSZ, &size) < 0)
         throw std::runtime_error ("TIOCSWINSZ failed");
   }

   void
   Vterm::readPty ()
   {
      unsigned char buf [1024];
      ssize_t n = read (ptyFd, buf, sizeof (buf));
      std::cout << "n = " << n << std::endl;
      for (int k = 0; k < n; ++k)
      {
         if (buf[k] < ' ')
            std::cout << "<" << (unsigned int)buf[k] << ">";
         else
            std::cout << buf[k];

         switch (buf [k])
         {
         case '\r':
            cp -= curX;
            curX = 0;
            break;
         case '\n':
            cp += nCols;
            curY += 1;
            if (curY == nRows)
            {
               curY = 0;
               cp = cells.get () + curX;
            }
            break;
         default:
            cp->uc_pt = buf [k];
            cp->fg = fg;
            cp->bg = bg;
            ++cp;
            ++curX;
            if (curX == nCols)
            {
               curX = 0;
               ++curY;
               if (curY == nRows)
               {
                  curY = 0;
                  cp = cells.get ();
               }
            }
         }
      }
      std::cout << std::endl;
   }

} // namespace zutty
