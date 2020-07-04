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

#include "renderer.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <sys/time.h>

namespace {

   using namespace zutty;

   uint64_t draw_count = 0;

   void
   benchDraw (CharVdev::Mapping& m)
   {
      CharVdev::Cell* cells = m.cells;
      uint16_t nCols = m.nCols;
      uint16_t nRows = m.nRows;

      uint16_t cRow = nRows - 1;
      uint16_t cCol = nCols - 1;
      uint64_t cnt = ++draw_count;

      while (cnt)
      {
         uint16_t digit = (cnt % 10) + '0';
         cells [cRow * nCols + cCol].uc_pt = digit;
         cells [cRow * nCols + cCol].fg = {255, 255, 255};
         cells [cRow * nCols + cCol].bg = {0, 0, 0};
         cnt /= 10;
         --cCol;
      }
   }
}

namespace zutty {

   Renderer::Renderer (const Font& priFont,
                       const Font& altFont,
                       uint16_t borderPx,
                       const std::function <void ()>& initDisplay,
                       const std::function <void ()>& swapBuffers_,
                       bool benchmark)
      : swapBuffers {swapBuffers_}
      , thr {&Renderer::renderThread, this,
             priFont, altFont, borderPx, initDisplay, benchmark}
   {
   }

   Renderer::~Renderer ()
   {
      Vterm vt;
      vt.exit = true;
      update (vt);
      thr.join ();
   }

   void
   Renderer::update (const Vterm& vt)
   {
      std::unique_lock <std::mutex> lock (vtMutex);
      nextVt = vt;
      nextVt.seqNo = ++seqNo;
      lock.unlock ();
      vtCond.notify_one();
   }

   void
   Renderer::renderThread (const Font& priFont,
                           const Font& altFont,
                           uint16_t borderPx,
                           const std::function <void ()>& initDisplay,
                           bool benchmark)
   {
      initDisplay ();

      charVdev = std::make_unique <CharVdev> (priFont, altFont, borderPx);

      Vterm lastVt;

      int n_redraws = 0;
      struct timeval tv;
      struct timeval tv_next;
      if (benchmark)
      {
         gettimeofday (&tv, nullptr);
         tv_next = tv;
         tv_next.tv_sec += 10;
      }

      while (1)
      {
         std::unique_lock <std::mutex> lock (vtMutex);
         vtCond.wait (lock,
                      [&] ()
                      {
                         return lastVt.seqNo != nextVt.seqNo;
                      });

         if (nextVt.exit)
            return;

         if ((lastVt.winPx != nextVt.winPx) ||
             (lastVt.winPy != nextVt.winPy))
            charVdev->resize (nextVt.winPx, nextVt.winPy);

         lastVt = nextVt;
         lock.unlock ();

         {
            CharVdev::Mapping m = charVdev->getMapping ();
            assert (m.nCols == lastVt.nCols);
            assert (m.nRows == lastVt.nRows);

            lastVt.copyCells (m.cells);

            if (benchmark)
               benchDraw (m);
         }

         charVdev->draw ();
         swapBuffers ();

         if (benchmark)
         {
            ++n_redraws;
            gettimeofday (&tv, nullptr);
            if (tv.tv_sec >= tv_next.tv_sec)
            {
               tv = tv_next;
               tv_next.tv_sec += 10;
               std::cout << n_redraws << " redraws in 10 seconds" << std::endl;
               n_redraws = 0;
            }
         }
      }
   }

} // namespace zutty
