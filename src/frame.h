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
#include "utf8.h"

namespace zutty
{
   class Frame
   {
   public:
      Frame ();

      Frame (uint16_t winPx_, uint16_t winPy_,
             uint16_t nCols_, uint16_t nRows_,
             uint16_t& marginTop_, uint16_t& marginBottom_,
             uint16_t saveLines_ = 0);

      void resize (uint16_t winPx_, uint16_t winPy_,
                   uint16_t nCols_, uint16_t nRows_,
                   uint16_t& marginTop_, uint16_t& marginBottom_);

      void dropScrollbackHistory ();
      void setMargins (uint16_t marginTop_, uint16_t marginBottom_);
      void resetMargins (uint16_t& marginTop_, uint16_t& marginBottom_);

      void fillCells (uint16_t ch, const CharVdev::Cell& attrs);
      void fullCopyCells (CharVdev::Cell * const dest);
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

      void pageUp (uint16_t count);
      void pageDown (uint16_t count);
      void pageToBottom ();
      uint16_t getHistoryRows () const { return historyRows; };

      void expose () { damage.expose (); };
      void resetDamage () { damage.reset (); };

      const CharVdev::Cursor& getCursor () const { return cursor; };
      void setCursorPos (uint16_t pY, uint16_t pX);
      void setCursorStyle (CharVdev::Cursor::Style cs);

      // selection state
      enum class SelectSnapTo: uint8_t
      {
         Char = 0, Word, Line, COUNT
      };
      void setSelectSnapTo (SelectSnapTo snapTo_) { snapTo = snapTo_; };
      void cycleSelectSnapTo () { snapTo = cycleSelectSnapTo (snapTo); };
      Rect& getSelection () { return selection; };
      const Rect& getSelection () const { return selection; };
      Rect getSnappedSelection () const;
      bool getSelectedUtf8 (std::string& utf8_selection) const;

      constexpr const static size_t cellSize = sizeof (CharVdev::Cell);

      uint64_t seqNo = 0; // update counter (used by Renderer)

      uint16_t winPx = 0;
      uint16_t winPy = 0;
      uint16_t nCols = 0;
      uint16_t nRows = 0;
      uint16_t saveLines = 0;

   private:
      uint16_t scrollHead;   // row offset of scrolling area's logical top row
      uint16_t marginTop;    // current margin top (number of rows above)
      uint16_t marginBottom; // current margin bottom (number of rows above + 1)
      uint16_t historyRows;  // number of history (off-screen) rows with data
      uint16_t viewOffset;   // how many rows above top row does the view start?
      bool margins = false;  // are there (non-default) top/bottom margins set?

      CharVdev::Cell::Ptr cells = nullptr;
      CharVdev::Cursor cursor;
      Rect selection;
      SelectSnapTo snapTo = SelectSnapTo::Char;

      struct Damage
      {
         uint32_t start = 0;
         uint32_t end = 0;
         uint32_t totalCells = 0;

         void reset ();
         void expose ();
         void add (uint32_t start_, uint32_t end_);
      };
      Damage damage;

      int getPhysicalRow (int pY) const;
      const CharVdev::Cell * getPhysRowPtr (int pY) const;
      const CharVdev::Cell * getViewRowPtr (int pY) const;
      uint32_t getIdx (uint16_t pY, uint16_t pX) const;
      const CharVdev::Cell & operator [] (uint32_t idx) const;
      CharVdev::Cell & operator [] (uint32_t idx);

      void eraseRange (uint32_t start, uint32_t end,
                       const CharVdev::Cell& attrs);
      void copyCells (uint32_t dstIx, uint32_t srcIx, uint32_t count);
      void moveCells (uint32_t dstIx, uint32_t srcIx, uint32_t count);

      void damageDeltaCopy (CharVdev::Cell* dst, uint32_t start, uint32_t count);
      void copyAllCells (CharVdev::Cell * const dest);
      void unwrapCellStorage ();

      static SelectSnapTo cycleSelectSnapTo (SelectSnapTo& snapTo)
      {
         return static_cast <SelectSnapTo> (
            (static_cast <uint8_t> (snapTo) + 1) %
            static_cast <uint8_t> (SelectSnapTo::COUNT));
      }

      void vscrollSelection (int vertOffset);
      void invalidateSelection (const Rect&& damage);

      void highMemUsageReport ();
   };

} // namespace zutty

#include "frame.icc"
