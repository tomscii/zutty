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

#include <cstdint>
#include <memory>

namespace zutty {

   class Vterm
   {
   public:
      Vterm () = default;

      Vterm (uint16_t glyphPx, uint16_t glyphPy,
             uint16_t winPx, uint16_t winPy,
             int ptyFd);

      ~Vterm () = default;

      void resize (uint16_t winPx, uint16_t winPy);

      void readPty ();

      uint16_t glyphPx;
      uint16_t glyphPy;
      uint16_t winPx;
      uint16_t winPy;
      uint16_t nCols;
      uint16_t nRows;
      int ptyFd;

      std::shared_ptr <CharVdev::Cell> cells;
      CharVdev::Cell * cp;
      uint16_t curX = 0;
      uint16_t curY = 0;
      CharVdev::Color fg = {255, 255, 255};
      CharVdev::Color bg = {0, 0, 0};

      uint64_t seqNo = 0; // update counter
      bool exit = false;
   };

} // namespace zutty
