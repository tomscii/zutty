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

#pragma once

#include "charvdev.h"

namespace zutty {

   class Frame
   {
   public:
      explicit Frame ();

      explicit Frame (uint16_t winPx_, uint16_t winPy_,
                      uint16_t nCols_, uint16_t nRows_,
                      uint16_t& marginTop_, uint16_t& marginBottom_);

      void resize (uint16_t winPx_, uint16_t winPy_,
                   uint16_t nCols_, uint16_t nRows_,
                   uint16_t& marginTop_, uint16_t& marginBottom_);

      void setMargins (uint16_t marginTop_, uint16_t marginBottom_);
      void resetMargins (uint16_t& marginTop_, uint16_t& marginBottom_);

      void fillCells (uint16_t ch, const CharVdev::Cell& attrs);
      void copyCells (CharVdev::Cell * const dest);
      void deltaCopyCells (CharVdev::Cell * const dest);

      operator bool () const { return cells != nullptr; }
      void freeCells () { cells = nullptr; }

      const CharVdev::Cell & getCell (uint16_t pY, uint16_t pX) const;
      CharVdev::Cell & getCell (uint16_t pY, uint16_t pX);

      void eraseInRow (uint16_t pY, uint16_t startX, uint16_t count,
                       const CharVdev::Cell& attrs);
      void moveInRow (uint16_t pY, uint16_t dstX, uint16_t srcX,
                      uint16_t count);
      void copyRow (uint16_t dstY, uint16_t srcY, uint16_t startX,
                    uint16_t count);

      void scrollUp (uint16_t count);
      void scrollDown (uint16_t count);

      void expose () { damage.add (0, nRows * nCols); };
      void resetDamage () { damage.reset (); };

      uint64_t seqNo = 0; // update counter (used by Renderer)

      uint16_t winPx = 0;
      uint16_t winPy = 0;
      uint16_t nCols = 0;
      uint16_t nRows = 0;

      CharVdev::Cursor cursor;
      Rect selection;

   private:
      uint16_t scrollHead;   // scrolling area row offset of logical top row
      uint16_t marginTop;    // current margin top (number of rows above)
      uint16_t marginBottom; // current margin bottom (number of rows above + 1)

      CharVdev::Cell::Ptr cells = nullptr;

      struct Damage
      {
         uint32_t start = 0;
         uint32_t end = 0;

         void reset ()
         {
            start = 0;
            end = 0;
         }

         void add (uint32_t start_, uint32_t end_)
         {
            if (start == end) // null state
            {
               start = start_;
               end = end_;
            }
            else
            {
               start = std::min (start, start_);
               end = std::max (end, end_);
            }
         }
      };
      Damage damage;

      uint32_t getIdx (uint16_t pY, uint16_t pX) const;
      const CharVdev::Cell & operator [] (uint32_t idx) const;
      CharVdev::Cell & operator [] (uint32_t idx);

      void eraseRange (uint32_t start, uint32_t end,
                       const CharVdev::Cell& attrs);
      void copyCells (uint32_t dstIx, uint32_t srcIx, uint32_t count);
      void moveCells (uint32_t dstIx, uint32_t srcIx, uint32_t count);

      void damageDeltaCopy (CharVdev::Cell* dst, uint32_t start, uint32_t end);
      void unwrapCellStorage ();
   };

} // namespace zutty

#include "frame.icc"
