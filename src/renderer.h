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
#include "vterm.h"

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace zutty {

   class Renderer {
   public:
      explicit Renderer (const Font& priFont,
                         const Font& altFont,
                         const std::function <void ()>& initDisplay,
                         const std::function <void ()>& swapBuffers,
                         bool benchmark);

      ~Renderer ();

      void update (const Vterm& vt);

   private:
      std::unique_ptr <CharVdev> charVdev;
      const std::function <void ()> swapBuffers;
      Vterm nextVt;
      uint64_t seqNo;

      std::condition_variable vtCond;
      std::mutex vtMutex;
      std::thread thr;

      void renderThread (const Font& priFont,
                         const Font& altFont,
                         const std::function <void ()>& initDisplay,
                         bool benchmark);
   };

} // namespace zutty
