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
#include "frame.h"

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace zutty
{
   class Renderer
   {
   public:
      Renderer (const std::function <void ()>& initDisplay,
                const std::function <void ()>& swapBuffers,
                Fontpack* fontpk);

      ~Renderer ();

      void update (const Frame& frame);

   private:
      std::unique_ptr <CharVdev> charVdev;
      const std::function <void ()> swapBuffers;
      Frame nextFrame;
      uint64_t seqNo = 0;
      bool done = false;

      std::condition_variable cond;
      std::mutex mx;
      std::thread thr;

      void renderThread (const std::function <void ()>& initDisplay,
                         Fontpack* fontpk);
   };

} // namespace zutty
