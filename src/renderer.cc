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
      std::unique_lock <std::mutex> lk (mx);
      done = true;
      nextFrame.seqNo = ++seqNo;
      lk.unlock ();
      cond.notify_one ();
      thr.join ();
   }

   void
   Renderer::update (const Frame& frame)
   {
      std::unique_lock <std::mutex> lk (mx);
      nextFrame = frame;
      nextFrame.seqNo = ++seqNo;
      lk.unlock ();
      cond.notify_one ();
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

      Frame lastFrame;

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
         std::unique_lock <std::mutex> lk (mx);
         cond.wait (lk,
                    [&] ()
                    {
                       return lastFrame.seqNo != nextFrame.seqNo;
                    });

         if (done)
            return;

         if ((lastFrame.winPx != nextFrame.winPx) ||
             (lastFrame.winPy != nextFrame.winPy))
            charVdev->resize (nextFrame.winPx, nextFrame.winPy);

         lastFrame = nextFrame;
         lk.unlock ();

         {
            CharVdev::Mapping m = charVdev->getMapping ();
            assert (m.nCols == lastFrame.nCols);
            assert (m.nRows == lastFrame.nRows);

            lastFrame.copyCells (m.cells);

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
