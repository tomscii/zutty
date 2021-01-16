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

#include "frame.h"

namespace zutty {

   Frame::Frame () {}

   Frame::Frame (uint16_t winPx_, uint16_t winPy_,
                 uint16_t nCols_, uint16_t nRows_)
      : winPx (winPx_)
      , winPy (winPy_)
      , nCols (nCols_)
      , nRows (nRows_)
      , scrollHead (0)
      , marginTop (0)
      , marginBottom (nRows)
      , cells (CharVdev::make_cells (nCols, nRows))
   {}

   void
   Frame::resize (uint16_t winPx_, uint16_t winPy_,
                  uint16_t nCols_, uint16_t nRows_)
   {
      if (winPx == winPx_ && winPy == winPy_)
         return;

      winPx = winPx_;
      winPy = winPy_;

      if (nCols == nCols_ && nRows == nRows_)
         return;

      linearizeCellStorage ();
      const CharVdev::Cell* src = cells.get ();
      auto newCells = CharVdev::make_cells (nCols_, nRows_);
      CharVdev::Cell* dst = newCells.get ();

      const int nRowsToCopy = std::min (nRows, nRows_);
      const int rowLen = std::min (nCols, nCols_);
      for (int k = 0; k < nRowsToCopy; ++k)
         memcpy (dst + k * nCols_, src + k * nCols,
                 rowLen * sizeof (CharVdev::Cell));

      cells = std::move (newCells);
      nCols = nCols_;
      nRows = nRows_;
      scrollHead = 0;
      marginTop = 0;
      marginBottom = nRows;
   }

   void
   Frame::copyCells (CharVdev::Cell * const dst)
   {
      constexpr const size_t cellSize = sizeof (CharVdev::Cell);

      CharVdev::Cell* p = dst;
      CharVdev::Cell* s = cells.get ();
      uint32_t n = marginTop * nCols;
      memcpy (p, s, n * cellSize);

      p += n;
      n = (marginBottom - scrollHead) * nCols;
      memcpy (p, s + scrollHead * nCols, n * cellSize);

      p += n;
      n = (scrollHead - marginTop) * nCols;
      memcpy (p, s + marginTop * nCols, n * cellSize);

      p += n;
      n = (nRows - marginBottom) * nCols;
      memcpy (p, s + marginBottom * nCols, n * cellSize);
   }

   inline void
   Frame::damageDeltaCopy (CharVdev::Cell* dst, uint32_t start, uint32_t end)
   {
      if (damage.end <= start || end <= damage.start)
         return; // no intersection

      if (start < damage.start)
      {
         dst += (damage.start - start);
         start = damage.start;
      }

      if (end > damage.end)
      {
         end = damage.end;
      }

      CharVdev::Cell* const src = cells.get ();

      for (size_t i = 0, j = start; j < end; ++i, ++j)
      {
         if (dst [i] != src [j])
         {
            dst [i] = src [j];
            dst [i].dirty = 1;
         }
      }
   }

   void
   Frame::deltaCopyCells (CharVdev::Cell * const dst)
   {
      CharVdev::Cell* p = dst;
      uint32_t n = marginTop * nCols;
      damageDeltaCopy (p, 0, n);

      p += n;
      n = (marginBottom - scrollHead) * nCols;
      damageDeltaCopy (p, scrollHead * nCols, marginBottom * nCols);

      p += n;
      n = (scrollHead - marginTop) * nCols;
      damageDeltaCopy (p, marginTop * nCols, scrollHead * nCols);

      p += n;
      n = (nRows - marginBottom) * nCols;
      damageDeltaCopy (p, marginBottom * nCols, nRows * nCols);
   }

   void
   Frame::linearizeCellStorage ()
   {
      auto newCells = CharVdev::make_cells (nCols, nRows);
      copyCells (newCells.get ());
      cells = std::move (newCells);
      scrollHead = marginTop;
   }

} // namespace zutty
