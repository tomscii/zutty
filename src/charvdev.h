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

#include "base.h"
#include "fontpack.h"
#include "gl.h"
#include "options.h"
#include "utf8.h"

#include <cstdint>
#include <memory>
#include <string>

namespace zutty
{
   class CharVdev
   {
   public:
      explicit CharVdev (Fontpack* fontpk);

      ~CharVdev ();

      bool resize (uint16_t pxWidth_, uint16_t pxHeight_);
      void draw ();

      struct Cell
      {
         uint16_t uc_pt = ' ';
         uint8_t dwidth: 1;
         uint8_t dwidth_cont: 1;
         uint8_t bold: 1;
         uint8_t italic: 1;
         uint8_t underline: 1;
         uint8_t inverse: 1;
         uint8_t wrap: 1;
         uint8_t dirty: 1;
         uint16_t _fill0: 8;
         Color fg;
         uint8_t _fill1;
         Color bg;
         uint8_t _fill2;

         Cell ():
            dwidth (0), dwidth_cont (0),
            bold (0), italic (0), underline (0), inverse (0), wrap (0),
            dirty (0), fg (opts.fg), bg (opts.bg)
         {}

         using Ptr = std::shared_ptr <Cell>;

         bool operator == (const Cell& rhs) const
         {
            return memcmp (this, &rhs, sizeof (Cell)) == 0;
         }

         bool operator != (const Cell& rhs) const
         {
            return ! operator == (rhs);
         }
      };
      static_assert (sizeof (Cell) == 12, "Cell size mismatch");

      static Cell::Ptr make_cells (uint16_t nCols, uint16_t nRows)
      {
         return std::shared_ptr <Cell> (new Cell [nRows * nCols],
                                        std::default_delete <Cell []> ());
      }

      struct Mapping
      {
         Mapping (uint16_t nCols_, uint16_t nRows_, Cell *& cells_);
         ~Mapping ();

         uint16_t nCols;
         uint16_t nRows;
         Cell *& cells;
      };

      Mapping getMapping ();

      struct Cursor
      {
         Color color = opts.cr;
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
      void setDeltaFrame (bool delta);

   private:
      uint16_t px;
      uint16_t py;
      uint16_t nCols;
      uint16_t nRows;
      uint16_t pxWidth;
      uint16_t pxHeight;
      bool hasDoubleWidth = false;

      // GL ids of programs, buffers, textures, attributes and uniforms:
      GLuint P_compute, P_draw;
      GLuint B_text = 0;
      GLuint T_atlas = 0;
      GLuint T_atlasMap = 0;
      GLuint T_atlas_dw = 0;
      GLuint T_atlasMap_dw = 0;
      GLuint T_output = 0;
      GLint A_pos, A_vertexTexCoord;
      GLint compU_glyphPixels, compU_sizeChars, compU_cursorColor;
      GLint compU_cursorPos, compU_cursorStyle;
      GLint compU_selectRect, compU_selectRectMode, compU_selectDamage;
      GLint compU_deltaFrame, compU_showWraps, compU_hasDoubleWidth;
      GLint drawU_viewPixels;

      Cell * cells = nullptr; // valid pointer if mapped, else nullptr

      void createShaders ();
   };

} // namespace zutty
