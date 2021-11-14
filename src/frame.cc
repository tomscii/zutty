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
#include "log.h"

namespace zutty
{
   Frame::Frame () {}

   Frame::Frame (uint16_t winPx_, uint16_t winPy_,
                 uint16_t nCols_, uint16_t nRows_,
                 uint16_t& marginTop_, uint16_t& marginBottom_,
                 uint16_t saveLines_)
      : winPx (winPx_)
      , winPy (winPy_)
      , nCols (nCols_)
      , nRows (nRows_)
      , saveLines (saveLines_)
      , scrollHead (0)
      , marginTop (0)
      , marginBottom (nRows + saveLines)
      , historyRows (0)
      , viewOffset (0)
      , margins (false)
      , cells (CharVdev::make_cells (nCols, nRows + saveLines))
   {
      marginTop_ = marginTop;
      marginBottom_ = nRows;
      damage.totalCells = nCols * (nRows + saveLines);
      highMemUsageReport ();
   }

   void
   Frame::dropScrollbackHistory ()
   {
      viewOffset = 0;
      historyRows = 0;
      expose ();
   }

   void
   Frame::setMargins (uint16_t marginTop_, uint16_t marginBottom_)
   {
      unwrapCellStorage ();
      scrollHead = marginTop = marginTop_;
      marginBottom = marginBottom_;
      margins = true;
      expose ();
   }

   void
   Frame::resetMargins (uint16_t& marginTop_, uint16_t& marginBottom_)
   {
      unwrapCellStorage ();
      scrollHead = marginTop = marginTop_ = 0;
      marginBottom = nRows + saveLines;
      marginBottom_ = nRows;
      margins = false;
      expose ();
   }

   void
   Frame::resize (uint16_t winPx_, uint16_t winPy_,
                  uint16_t nCols_, uint16_t nRows_,
                  uint16_t& marginTop_, uint16_t& marginBottom_)
   {
      if (winPx == winPx_ && winPy == winPy_)
         return;

      winPx = winPx_;
      winPy = winPy_;

      if (nCols == nCols_ && nRows == nRows_)
         return;

      auto newCells = CharVdev::make_cells (nCols_, nRows_ + saveLines);
      CharVdev::Cell* dst = newCells.get ();

      const int rowLen = std::min (nCols, nCols_);
      const int nCopyRows = std::min (nRows, nRows_);
      CharVdev::Cell* p = dst;
      for (int pY = 0; pY < nCopyRows; ++pY)
      {
         memcpy (p, getPhysRowPtr (pY), rowLen * cellSize);
         p += nCols_;
      }
      p = dst + (nRows_ + saveLines - historyRows) * nCols_;
      for (int pY = -historyRows; pY < 0; ++pY)
      {
         memcpy (p, getPhysRowPtr (pY), rowLen * cellSize);
         p += nCols_;
      }

      cells = std::move (newCells);
      nCols = nCols_;
      nRows = nRows_;
      scrollHead = 0;
      marginTop = marginTop_ = 0;
      marginBottom_ = nRows;
      marginBottom = nRows + saveLines;
      margins = false;
      viewOffset = 0;
      damage.totalCells = nCols * (nRows + saveLines);
      highMemUsageReport ();
   }

   void
   Frame::fullCopyCells (CharVdev::Cell * const dst)
   {
      CharVdev::Cell* p = dst;
      for (int pY = 0; pY < nRows; ++pY)
      {
         memcpy (p, getViewRowPtr (pY), nCols * cellSize);
         p += nCols;
      }
   }

   void
   Frame::deltaCopyCells (CharVdev::Cell * const dst)
   {
      CharVdev::Cell* p = dst;
      for (int pY = -viewOffset; pY < nRows - viewOffset; ++pY)
      {
         damageDeltaCopy (p, nCols * getPhysicalRow (pY), nCols);
         p += nCols;
      }
   }

   Rect
   Frame::getSnappedSelection () const
   {
      Rect ret = selection;

      if (ret.null ())
         return ret;

      if (selection.rectangular)
         return ret;

      switch (snapTo)
      {
      case SelectSnapTo::Char:
         break;
      case SelectSnapTo::Word:
      {
         const auto* cp = getViewRowPtr (ret.tl.y);
         while (ret.tl.x < nCols && cp [ret.tl.x].uc_pt == ' ')
            ++ret.tl.x;
         while (ret.tl.x > 0 && cp [ret.tl.x - 1].uc_pt != ' ')
            --ret.tl.x;

         cp = getViewRowPtr (ret.br.y);
         while (ret.br.x > 0 && cp [ret.br.x].uc_pt == ' ')
            --ret.br.x;
         while (ret.br.x < nCols && cp [ret.br.x].uc_pt != ' ')
            ++ret.br.x;
      }
         break;
      case SelectSnapTo::Line:
         ret.tl.x = 0;
         ret.br.x = nCols;
         break;
      default: break;
      }

      return ret;
   }

   bool
   Frame::getSelectedUtf8 (std::string& utf8_selection) const
   {
      const Rect sel = getSnappedSelection ();

      if (sel.empty ())
         return false;

      using utf16str = std::vector <uint16_t>;
      std::vector <utf16str> lines;
      bool wrap = false;

      // save lines from the selected range of the frame cell buffer
      auto addLine =
         [&] (int y, uint16_t x1, uint16_t x2)
         {
            utf16str line;
            bool wrapBack = wrap;
            wrap = false;
            const auto* cp = getViewRowPtr (y);
            for (uint16_t x = x1; x < x2; ++x)
            {
               const auto& cell = cp [x];
               if (!cell.dwidth_cont)
                  line.push_back (cell.uc_pt);
               if (cell.wrap)
               {
                  wrap = true;
                  break;
               }
            }

            while (!wrap && line.size () && line.back () == ' ')
               line.pop_back (); // discard trailing whitespace

            if (wrapBack && lines.size ())
               lines.back ().insert (lines.back ().end (),
                                     line.begin (), line.end ());
            else
               lines.push_back (line);
         };

      if (sel.tl.y == sel.br.y)
      {
         addLine (sel.tl.y, sel.tl.x, sel.br.x);
      }
      else if (sel.rectangular)
      {
         for (int y = sel.tl.y; y <= sel.br.y; ++y)
            addLine (y, sel.tl.x, sel.br.x);
      }
      else
      {
         addLine (sel.tl.y, sel.tl.x, nCols);
         for (int y = sel.tl.y + 1; y < sel.br.y; ++y)
            addLine (y, 0, nCols);
         addLine (sel.br.y, 0, sel.br.x);
      }

      // convert to UTF-8
      std::vector <char> utf8_out;
      auto sinkFn = [&] (char ch) { utf8_out.push_back (ch); };
      for (const auto& u16s: lines)
      {
         for (uint16_t cp: u16s)
            Utf8Encoder::pushUnicode (cp, sinkFn);
         utf8_out.push_back ('\n');
      }
      while (utf8_out.size () && utf8_out.back () == '\n')
         utf8_out.pop_back (); // discard trailing empty lines

      utf8_selection = std::string (utf8_out.data (), utf8_out.size ());

   #if DEBUG
      if (utf8_selection.size () <= 80)
         logT << "Selected " << utf8_selection.size () << " bytes:\n'"
              << utf8_selection << "'" << std::endl;
      else
         logT << "Selected " << utf8_selection.size () << " bytes:\n'"
              << utf8_selection.substr (0, 40) << "' ...\n'"
              << utf8_selection.substr (utf8_selection.size () - 40) << "'"
              << std::endl;
   #endif // DEBUG
      return true;
   }

   // private functions

   inline void
   Frame::damageDeltaCopy (CharVdev::Cell* dst, uint32_t start, uint32_t count)
   {
      uint32_t end = start + count;

      if (damage.end <= start || end <= damage.start)
         return; // no intersection

      if (start < damage.start)
      {
         dst += (damage.start - start);
         start = damage.start;
      }

      if (damage.end < end)
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
   Frame::copyAllCells (CharVdev::Cell * const dst)
   {
      CharVdev::Cell* p = dst;
      for (int pY = 0; pY < nRows; ++pY)
      {
         memcpy (p, getPhysRowPtr (pY), nCols * cellSize);
         p += nCols;
      }
      p = dst + (nRows + saveLines - historyRows) * nCols;
      for (int pY = -historyRows; pY < 0; ++pY)
      {
         memcpy (p, getPhysRowPtr (pY), nCols * cellSize);
         p += nCols;
      }
   }

   void
   Frame::unwrapCellStorage ()
   {
      if (scrollHead == marginTop)
         return;

      auto newCells = CharVdev::make_cells (nCols, nRows + saveLines);
      copyAllCells (newCells.get ());
      cells = std::move (newCells);
      scrollHead = marginTop;
   }

   void
   Frame::highMemUsageReport ()
   {
      auto allocKB = damage.totalCells * cellSize / 1024;
      if (allocKB > 8192)
      {
         logI << "Allocated " << allocKB << " KiB for cell storage; consider "
              << "decreasing saveLines (current value: " << saveLines
              << ") to reduce memory usage!"
              << std::endl;
      }
   }

} // namespace zutty
