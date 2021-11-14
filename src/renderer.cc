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

namespace zutty
{
   Renderer::Renderer (const std::function <void ()>& initDisplay,
                       const std::function <void ()>& swapBuffers_,
                       Fontpack* fontpk)
      : swapBuffers {swapBuffers_}
      , thr (&Renderer::renderThread, this, initDisplay, fontpk)
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
   Renderer::renderThread (const std::function <void ()>& initDisplay,
                           Fontpack* fontpk)
   {
      initDisplay ();

      charVdev = std::make_unique <CharVdev> (fontpk);

      Frame lastFrame;
      bool delta = false;

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

         if (lastFrame.seqNo + 1 != nextFrame.seqNo)
            delta = false;

         lastFrame = nextFrame;
         lk.unlock ();

         if (charVdev->resize (lastFrame.winPx, lastFrame.winPy))
            delta = false;

         {
            CharVdev::Mapping m = charVdev->getMapping ();
            assert (m.nCols == lastFrame.nCols);
            assert (m.nRows == lastFrame.nRows);

            if (delta)
               lastFrame.deltaCopyCells (m.cells);
            else
               lastFrame.fullCopyCells (m.cells);
         }

         charVdev->setDeltaFrame (delta);
         charVdev->setCursor (lastFrame.getCursor ());
         charVdev->setSelection (lastFrame.getSnappedSelection ());

         if (lastFrame.seqNo == nextFrame.seqNo)
         {
            charVdev->draw ();
            swapBuffers ();
            delta = true;
         }
         else
         {
            // skip drawing outdated frame; force full redraw next time
            delta = false;
         }
      }
   }

} // namespace zutty
