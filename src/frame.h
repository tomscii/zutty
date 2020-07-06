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

   class Frame {
   public:
      Frame ();

      Frame (uint16_t winPx_, uint16_t winPy_,
             uint16_t nCols_, uint16_t nRows_);

      void linearizeCellStorage ();
      void copyCells (CharVdev::Cell * const dest);

      operator bool () const { return cells != nullptr; }
      void freeCells () { cells = nullptr; }

      uint32_t getIdx (uint16_t pY, uint16_t pX);
      CharVdev::Cell & operator [] (uint32_t idx);

      void copyCells (uint32_t dstIx, uint32_t srcIx, uint32_t count);
      void moveCells (uint32_t dstIx, uint32_t srcIx, uint32_t count);

      uint64_t seqNo = 0; // update counter (used by Renderer)

      uint16_t winPx = 0;
      uint16_t winPy = 0;
      uint16_t nCols = 0;
      uint16_t nRows = 0;

      uint32_t scrollHead;   // scrolling area start of top row (abs. offset)
      uint16_t marginTop;    // current margin top (number of rows above)
      uint16_t marginBottom; // current margin bottom (number of rows above + 1)

   private:
      CharVdev::Cell::Ptr cells = nullptr;
   };

} // namespace zutty

#include "frame.icc"
