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

#include "font.h"
#include "gl.h"

#include <cstdint>
#include <memory>
#include <string>

namespace zutty {

   struct Color
   {
      uint8_t red;
      uint8_t blue;
      uint8_t green;
   };

   struct Cell
   {
      uint16_t uc_pt;
      uint8_t bold: 1;
      uint8_t underline: 1;
      uint8_t inverse: 1;
      uint16_t _fill0: 13;
      Color fg;
      uint8_t _fill1;
      Color bg;
      uint8_t _fill2;
   };

   static_assert (sizeof (Cell) == 12);

   class CharVdev
   {
   public:
      CharVdev (const std::string& priFontPath,
                const std::string& altFontPath = "");

      ~CharVdev ();

      void resize (uint16_t pxWidth_, uint16_t pxHeight_);
      void draw ();

   private:
      uint16_t nCols;
      uint16_t nRows;
      uint16_t pxWidth;
      uint16_t pxHeight;
      Font fnt;
      Font fnt2;

      // GL ids of programs, buffers, textures, attributes and uniforms:
      GLuint P_compute, P_draw;
      GLuint B_text = 0;
      GLuint T_atlas = 0;
      GLuint T_atlasMap = 0;
      GLuint T_output = 0;
      GLint A_pos, A_vertexTexCoord;
      GLint compU_glyphPixels, compU_sizeChars, drawU_viewPixels;

      uint32_t draw_count = 0;

      void createShaders ();
   };

} // namespace zutty
