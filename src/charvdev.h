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
#include "geometry.h"
#include "gl.h"

#include <cstdint>
#include <memory>
#include <string>

namespace zutty {

   // The glyph to display instead of valid codes with no available glyph
   constexpr const uint16_t Missing_Glyph_Marker = 0x0000;
   // The "question mark" to display in place of invalid/unsupported unicode
   constexpr const uint16_t Unicode_Replacement_Character = 0xfffd;

   class CharVdev
   {
   public:
      explicit CharVdev (const Font& priFont,
                         const Font& altFont,
                         uint16_t borderPx);

      ~CharVdev ();

      void resize (uint16_t pxWidth_, uint16_t pxHeight_);
      void draw ();

      struct Color
      {
         uint8_t red;
         uint8_t green;
         uint8_t blue;
      };

      struct Cell
      {
         uint16_t uc_pt = ' ';
         uint8_t bold: 1;
         uint8_t underline: 1;
         uint8_t inverse: 1;
         uint16_t _fill0: 13;
         Color fg = {255, 255, 255};
         uint8_t _fill1;
         Color bg = {0, 0, 0};
         uint8_t _fill2;

         // For the lack of bitfield initializers:
         Cell (): bold (0), underline (0), inverse (0) {}

         using Ptr = std::shared_ptr <Cell>;
      };
      static_assert (sizeof (Cell) == 12);

      static Cell::Ptr make_cells (uint16_t nCols, uint16_t nRows)
      {
         return std::shared_ptr <Cell> (new Cell [nRows * nCols],
                                        std::default_delete <Cell []> ());
      }

      struct Mapping
      {
         explicit Mapping (uint16_t nCols_, uint16_t nRows_, Cell *& cells_);
         ~Mapping ();

         uint16_t nCols;
         uint16_t nRows;
         Cell *& cells;
      };

      Mapping getMapping ();

      struct Cursor
      {
         Color color = {255, 255, 255};
         uint16_t posX = 0;
         uint16_t posY = 0;

         enum class Style: uint8_t
         {
            hidden = 0,
            filled_block = 1,
            hollow_block = 2
         };
         Style style = Style::hidden;
      };

      void setCursor (const Cursor& cursor);
      void setSelection (const Rect& selection);

   private:
      uint16_t nCols;
      uint16_t nRows;
      uint16_t pxWidth;
      uint16_t pxHeight;

      // GL ids of programs, buffers, textures, attributes and uniforms:
      GLuint P_compute, P_draw;
      GLuint B_text = 0;
      GLuint T_atlas = 0;
      GLuint T_atlasMap = 0;
      GLuint T_output = 0;
      GLint A_pos, A_vertexTexCoord;
      GLint compU_glyphPixels, compU_sizeChars, compU_cursorColor;
      GLint compU_cursorPos, compU_cursorStyle;
      GLint compU_selectRect, compU_selectRectMode;
      GLint drawU_viewPixels;

      const Font& priFont;
      const Font& altFont;
      uint16_t borderPx;

      Cell * cells = nullptr; // valid pointer if mapped, else nullptr

      void createShaders ();
   };

} // namespace zutty
