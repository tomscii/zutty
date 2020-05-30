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

   struct Cell
   {
      uint16_t uc_pt;
      uint8_t fg_red;
      uint8_t fg_blue;
      uint8_t fg_green;
      uint8_t bg_red;
      uint8_t bg_blue;
      uint8_t bg_green;
      uint8_t attrs;
   } __attribute__ ((packed));

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

      // GL ids of programs, textures, attributes and uniforms:
      GLuint P_compute, P_draw;
      GLuint T_text = 0;
      GLuint T_atlas = 0;
      GLuint T_output = 0;
      GLint A_pos, A_vertexTexCoord;
      GLint compU_glyphPixels, drawU_viewPixels;

      std::unique_ptr <uint8_t []> text_data;  // row-major order

      uint32_t draw_count = 0;

      void createShaders ();
   };

} // namespace zutty
