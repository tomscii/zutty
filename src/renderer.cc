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

namespace zutty {

   Renderer::Renderer (const Font& priFont,
                       const Font& altFont,
                       const std::function <void ()>& initDisplay,
                       const std::function <void ()>& swapBuffers_)
      : swapBuffers {swapBuffers_}
      , thr {&Renderer::renderThread, this,
             priFont, altFont, initDisplay}
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
                           const std::function <void ()>& initDisplay)
   {
      initDisplay ();

      charVdev = std::make_unique <CharVdev> (priFont, altFont);

      Frame lastFrame;

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
         }

         charVdev->setCursor (lastFrame.cursor);
         charVdev->setSelection (lastFrame.selection);
         charVdev->draw ();
         swapBuffers ();
      }
   }

} // namespace zutty
