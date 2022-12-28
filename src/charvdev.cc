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

#include "charvdev.h"
#include "log.h"
#include "options.h"

#include <algorithm>
#include <cassert>
#include <iostream>

namespace
{
   static const char *computeShaderSource = R"(#version 310 es

layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) writeonly lowp uniform image2D imgOut;
layout (binding = 1) uniform lowp sampler2DArray atlas;
layout (binding = 2) uniform lowp sampler2D atlasMap;
layout (binding = 3) uniform lowp sampler2DArray atlas_dw;
layout (binding = 4) uniform lowp sampler2D atlasMap_dw;
uniform lowp ivec2 glyphPixels;
uniform lowp ivec2 sizeChars;
uniform lowp ivec3 cursorColor;
uniform lowp ivec4 cursorPos; // .xy: current; .zw: previous
uniform lowp int cursorStyle;
uniform lowp ivec4 selectRect;
uniform lowp int selectRectMode;
uniform highp ivec2 selectDamage;
uniform lowp int deltaFrame;
uniform lowp int showWraps;
uniform lowp int hasDoubleWidth;

struct Cell
{
   highp uint charData;
   highp uint fg;
   highp uint bg;
};

layout (std430, binding = 0) buffer CharVideoMem
{
   Cell cells[];
} vmem;

void main ()
{
   ivec2 charPos = ivec2 (gl_GlobalInvocationID.xy);
   int idx = sizeChars.x * charPos.y + charPos.x;
   Cell cell = vmem.cells[idx];

   if (deltaFrame == 1)
   {
      uint dirty = bitfieldExtract (cell.charData, 23, 1);
      if (dirty == 0u &&
          charPos != cursorPos.xy && charPos != cursorPos.zw &&
          (idx < selectDamage.x || idx >= selectDamage.y))
         return;
   }
   vmem.cells[idx].charData = bitfieldInsert (cell.charData, 0u, 23, 1);

   ivec2 charCode =
      ivec2 (bitfieldExtract (cell.charData, 0, 8),  // Lowest byte
             bitfieldExtract (cell.charData, 8, 8)); // Next-lowest byte

   uint dwidth = bitfieldExtract (cell.charData, 16, 1);
   uint dwidth_cont = bitfieldExtract (cell.charData, 17, 1);
   if (dwidth_cont == 1u) // double-width cell continuation - drawn by left half
      return;

   if (dwidth == 1u && charPos.x < sizeChars.x - 1)
   {
      // check validity (dwidth_cont marker in the cell to the right)
      if (bitfieldExtract (vmem.cells[idx + 1].charData, 17, 1) != 1u)
         dwidth = 0u;
   }

   uint fontIdx = 0u; // 0 -> Normal; 1 -> Bold; 2 -> Italic; 3 -> BoldItalic
   if (dwidth == 0u)
      fontIdx = bitfieldExtract (cell.charData, 18, 2);
   uint underline = bitfieldExtract (cell.charData, 20, 1);
   uint inverse = bitfieldExtract (cell.charData, 21, 1);
   uint wrap = bitfieldExtract (cell.charData, 22, 1);

   ivec2 atlasPos;
   if (dwidth == 0u)
      atlasPos = ivec2 (vec2 (256) * texelFetch (atlasMap, charCode, 0).zw);
   else
      atlasPos = ivec2 (vec2 (256) * texelFetch (atlasMap_dw, charCode, 0).zw);

   vec3 fgColor = vec3 (float (bitfieldExtract (cell.fg, 0, 8)),
                        float (bitfieldExtract (cell.fg, 8, 8)),
                        float (bitfieldExtract (cell.fg, 16, 8))) / 255.0;

   vec3 bgColor = vec3 (float (bitfieldExtract (cell.bg, 0, 8)),
                        float (bitfieldExtract (cell.bg, 8, 8)),
                        float (bitfieldExtract (cell.bg, 16, 8))) / 255.0;

   vec3 crColor = vec3 (cursorColor) / 255.0;

   if (selectRectMode == 1)
   {
      if (charPos.y >= selectRect.y && charPos.y <= selectRect.w &&
          charPos.x >= selectRect.x && charPos.x < selectRect.z)
         inverse ^= 1u;
   }
   else if ((charPos.y > selectRect.y && charPos.y < selectRect.w) ||
       (charPos.y == selectRect.y && charPos.x >= selectRect.x &&
        (charPos.y < selectRect.w || charPos.x < selectRect.z)) ||
       (charPos.y == selectRect.w && charPos.x < selectRect.z &&
        (charPos.y > selectRect.y || charPos.x > selectRect.x)))
      inverse ^= 1u;

   if (inverse == 1u)
   {
      vec3 tmp = fgColor;
      fgColor = bgColor;
      bgColor = tmp;
   }
   if (crColor == bgColor)
   {
      crColor = vec3 (1.0) - crColor;
   }
   if (charPos == cursorPos.xy && cursorStyle == 1)
   {
      fgColor = bgColor;
      bgColor = crColor;
   }

   ivec2 srcGlyphPixels = glyphPixels;
   if (dwidth == 1u)
      srcGlyphPixels = ivec2 (2, 1) * glyphPixels;

   if (dwidth == 0u)
   {  // render regular cell
      for (int j = 0; j < glyphPixels.x; j++)
      {
         for (int k = 0; k < glyphPixels.y; k++)
         {
            ivec2 txCoords = atlasPos * srcGlyphPixels + ivec2 (j, k);
            ivec3 txc = ivec3 (txCoords, fontIdx);
            float lumi = texelFetch (atlas, txc, 0).r;
            vec4 pixel = vec4 (fgColor * lumi + bgColor * (1.0 - lumi), 1.0);
            ivec2 pxCoords = charPos * glyphPixels + ivec2 (j, k);
            imageStore (imgOut, pxCoords, pixel);
         }
      }
   }
   else if (hasDoubleWidth == 1)
   {  // render double-width cell
      for (int j = 0; j < srcGlyphPixels.x; j++)
      {
         for (int k = 0; k < srcGlyphPixels.y; k++)
         {
            ivec2 txCoords = atlasPos * srcGlyphPixels + ivec2 (j, k);
            ivec3 txc = ivec3 (txCoords, fontIdx);
            float lumi = texelFetch (atlas_dw, txc, 0).r;
            vec4 pixel = vec4 (fgColor * lumi + bgColor * (1.0 - lumi), 1.0);
            ivec2 pxCoords = charPos * glyphPixels + ivec2 (j, k);
            imageStore (imgOut, pxCoords, pixel);
         }
      }
   }
   else
   {  // no double-width font -- draw an empty box
      for (int j = 0; j < srcGlyphPixels.x; j++)
      {
         for (int k = 0; k < srcGlyphPixels.y; k++)
         {
            float lumi = 0.0;
            if ((0 < j && j < srcGlyphPixels.x - 1) &&
                (0 < k && k < srcGlyphPixels.y - 1) &&
                (j == 1 || j == srcGlyphPixels.x - 2 ||
                 k == 1 || k == srcGlyphPixels.y - 2))
               lumi = 0.7;
            vec4 pixel = vec4 (fgColor * lumi + bgColor * (1.0 - lumi), 1.0);
            ivec2 pxCoords = charPos * glyphPixels + ivec2 (j, k);
            imageStore (imgOut, pxCoords, pixel);
         }
      }
   }

   if (underline == 1u)
   {
      for (int j = 0; j < srcGlyphPixels.x; j++)
      {
         vec4 pixel = vec4 (fgColor, 1.0);
         ivec2 pxCoords = charPos * glyphPixels +
                          ivec2 (j, srcGlyphPixels.y - 1);
         imageStore (imgOut, pxCoords, pixel);
      }
   }

   if (showWraps == 1 && wrap == 1u)
   {
      vec4 pixel = vec4 (fgColor, 1.0);
      for (int k = 0; k < srcGlyphPixels.y; k += 2)
      {
         ivec2 pxCoords = charPos * glyphPixels +
                          ivec2 (srcGlyphPixels.x - 1, k);
         imageStore (imgOut, pxCoords, pixel);
      }
   }

   if (charPos == cursorPos.xy && cursorStyle == 2)
   {
      vec4 pixel = vec4 (crColor, 1.0);
      for (int j = 0; j < srcGlyphPixels.x; j++)
      {
         ivec2 pxCoords = charPos * glyphPixels + ivec2 (j, 0);
         imageStore (imgOut, pxCoords, pixel);
         pxCoords += ivec2 (0, srcGlyphPixels.y - 1);
         imageStore (imgOut, pxCoords, pixel);
      }
      for (int k = 1; k < srcGlyphPixels.y - 1; k++)
      {
         ivec2 pxCoords = charPos * glyphPixels + ivec2 (0, k);
         imageStore (imgOut, pxCoords, pixel);
         pxCoords += ivec2 (srcGlyphPixels.x - 1, 0);
         imageStore (imgOut, pxCoords, pixel);
      }
   }
}
)";

   static const char *vertexShaderSource = R"(#version 310 es

layout (location = 0) in vec2 pos;
layout (location = 1) in lowp vec2 vertexTexCoord;

out vec2 texCoord;

void main ()
{
   texCoord = vertexTexCoord;
   gl_Position = vec4 (pos, 0.0, 1.0);
}
)";

   static const char *fragmentShaderSource = R"(#version 310 es

in highp vec2 texCoord;

layout (rgba32f, binding = 0) readonly lowp uniform image2D imgOut;

uniform highp vec2 viewPixels;

layout (location = 0) out lowp vec4 outColor;

void main ()
{
   outColor = imageLoad (imgOut, ivec2 (texCoord * viewPixels));
}
)";


   GLuint
   createShader (GLuint type, const char* src, const char* name)
   {
      GLint stat;
      GLuint shader = glCreateShader (type);
      glShaderSource (shader, 1, &src, nullptr);
      glCompileShader (shader);
      glGetShaderiv (shader, GL_COMPILE_STATUS, &stat);
      if (!stat) {
         char errlog[1000];
         GLsizei len;
         glGetShaderInfoLog (shader, 1000, &len, errlog);
         logE << "Compiling " << name << " shader:\n"
              << errlog << std::endl;
         exit (1);
      }
      return shader;
   }

   void
   linkProgram (GLuint program, const char* name)
   {
      GLint stat;
      glLinkProgram (program);
      glGetProgramiv (program, GL_LINK_STATUS, &stat);
      if (!stat) {
         char errlog[1000];
         GLsizei len;
         glGetProgramInfoLog (program, 1000, &len, errlog);
         logE << "Linking " << name << " program:\n"
              << errlog << std::endl;
         exit (1);
      }
   }

   void
   setupTexture (GLuint target, GLenum type, GLuint& texture)
   {
      if (texture)
      {
         glDeleteTextures (1, &texture);
      }
      glGenTextures (1, &texture);
      glActiveTexture (target);
      glBindTexture (type, texture);
      glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
      glTexParameteri (type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri (type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri (type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri (type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   }

   void
   setupAtlasTexture (const zutty::Font& fnt, int idx)
   {
      glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                      0,    // mipmap level, always zero
                      0, 0, // X and Y offsets into texture area
                      idx,  // layer index offset
                      fnt.getPx () * fnt.getNx (),
                      fnt.getPy () * fnt.getNy (),
                      1,    // number of layers, i.e., fonts, loaded
                      GL_RED, GL_UNSIGNED_BYTE, fnt.getAtlasData ());
      glCheckError ();
   }

   void
   setupAtlasMappingTexture (const zutty::Font& fnt,
                             GLuint target, GLuint& texture)
   {
      auto atlasMap = std::vector <uint8_t> ();
      atlasMap.resize (2 * 256 * 256, 0);

      // Pre-fill the mapping texture with references to "missing glyph"
      // and "replacement character" glyphs, if available in the font atlas.
      const auto itEnd = fnt.getAtlasMap ().end ();

      zutty::Font::AtlasPos apRC {};
      {
         auto rcIt =
            fnt.getAtlasMap ().find (zutty::Unicode_Replacement_Character);
         if (rcIt != itEnd)
            apRC = rcIt->second;
      }

      zutty::Font::AtlasPos apMG {};
      {
         auto mgIt = fnt.getAtlasMap ().find (zutty::Missing_Glyph_Marker);
         if (mgIt != itEnd)
            apMG = mgIt->second;
      }

      for (int k = 0; k < 256 * 256; ++k)
      {
         const auto& apos = ((k >= 0xd800 && k < 0xe000) || k >= 0xfffe)
                          ? apRC
                          : apMG;
         atlasMap [2 * k] = apos.x;
         atlasMap [2 * k + 1] = apos.y;
      }

      // Fill the mapping texture with supported characters
      for (auto it = fnt.getAtlasMap ().begin (); it != itEnd; ++it)
      {
         atlasMap [2 * it->first] = it->second.x;
         atlasMap [2 * it->first + 1] = it->second.y;
      }
      setupTexture (target, GL_TEXTURE_2D, texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 256, 256, 0,
                   GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, atlasMap.data ());
   }

   template <typename T> void
   setupStorageBuffer (GLuint index, GLuint& buffer, uint32_t n_items)
   {
      if (buffer)
      {
         glDeleteBuffers (1, &buffer);
      }
      glGenBuffers (1, &buffer);
      glBindBuffer (GL_SHADER_STORAGE_BUFFER, buffer);
      glBindBufferBase (GL_SHADER_STORAGE_BUFFER, index, buffer);
      std::size_t size = sizeof (T) * n_items;
      glBufferData (GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
   }

} // namespace

namespace zutty
{
   CharVdev::CharVdev (Fontpack* fontpk)
      : px (fontpk->getPx ())
      , py (fontpk->getPy ())
   {
      createShaders ();

      /*
       * Setup draw program
       */
      glUseProgram (P_draw);

      glDisable (GL_CULL_FACE);
      glDisable (GL_DEPTH_TEST);
      glEnable (GL_BLEND);
      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glCheckError ();

      static const GLfloat verts[4][2] = {
         { -1,  1 },
         {  1,  1 },
         { -1, -1 },
         {  1, -1 }
      };
      static const GLfloat texCoords[4][2] = {
         { 0, 0 },
         { 1, 0 },
         { 0, 1 },
         { 1, 1 }
      };

      glVertexAttribPointer (A_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
      glVertexAttribPointer (A_vertexTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
                             texCoords);
      glCheckError ();

      /*
       * Setup compute program
       */
      glUseProgram (P_compute);
      glUniform2i (compU_glyphPixels, px, py);
      glUniform2i (compU_sizeChars, nCols, nRows);
      glUniform1i (compU_showWraps, opts.showWraps ? 1 : 0);

      // Setup atlas texture
      setupTexture (GL_TEXTURE1, GL_TEXTURE_2D_ARRAY, T_atlas);
      const Font& reg = fontpk->getRegular ();

      // log font size
      logI << "font size: "
             << reg.getPx () << "x"
             << reg.getPy () << std::endl;
      glTexStorage3D (GL_TEXTURE_2D_ARRAY, 1, GL_R8,
                      reg.getPx () * reg.getNx (),
                      reg.getPy () * reg.getNy (),
                      4); // number of layers
      glCheckError ();

      setupAtlasTexture (reg, 0);

      if (fontpk->hasBold ())
         setupAtlasTexture (fontpk->getBold (), 1);
      else
         setupAtlasTexture (fontpk->getRegular (), 1);

      if (fontpk->hasItalic ())
         setupAtlasTexture (fontpk->getItalic (), 2);
      else
         setupAtlasTexture (fontpk->getRegular (), 2);

      if (fontpk->hasBoldItalic ())
         setupAtlasTexture (fontpk->getBoldItalic (), 3);
      else if (fontpk->hasItalic ())
         setupAtlasTexture (fontpk->getItalic (), 3);
      else if (fontpk->hasBold ())
         setupAtlasTexture (fontpk->getBold (), 3);
      else
         setupAtlasTexture (fontpk->getRegular (), 3);

      setupAtlasMappingTexture (reg, GL_TEXTURE2, T_atlasMap);

      // Setup atlas texture for double-width characters
      if (fontpk->hasDoubleWidth ())
      {
         hasDoubleWidth = true;

         setupTexture (GL_TEXTURE3, GL_TEXTURE_2D_ARRAY, T_atlas_dw);
         const Font& dw = fontpk->getDoubleWidth ();
         glTexStorage3D (GL_TEXTURE_2D_ARRAY, 1, GL_R8,
                         dw.getPx () * dw.getNx (),
                         dw.getPy () * dw.getNy (),
                         1); // number of layers
         glCheckError ();

         setupAtlasTexture (dw, 0);
         setupAtlasMappingTexture (dw, GL_TEXTURE3, T_atlasMap_dw);
      }
      glUniform1i (compU_hasDoubleWidth, hasDoubleWidth ? 1 : 0);

      // Now that it's all loaded into GL, no need to keep font data in-memory
      fontpk->releaseFonts ();
   }

   CharVdev::~CharVdev ()
   {
   }

   bool
   CharVdev::resize (uint16_t pxWidth_, uint16_t pxHeight_)
   {
      assert (cells == nullptr); // no mapping in place

      if (pxWidth == pxWidth_ && pxHeight == pxHeight_)
         return false;

      pxWidth = pxWidth_;
      pxHeight = pxHeight_;
      nCols = std::max (1, (pxWidth - 2 * opts.border) / px);
      nRows = std::max (1, (pxHeight - 2 * opts.border) / py);

      logI << "Resize to " << pxWidth << " x " << pxHeight
           << " pixels, " << nCols << " x " << nRows << " chars"
           << std::endl;

      GLint viewWidth = nCols * px;
      GLint viewHeight = nRows * py;
      glViewport (opts.border, pxHeight - viewHeight - opts.border,
                  viewWidth, viewHeight);

      glUseProgram (P_draw);

      glUniform2f (drawU_viewPixels, (GLfloat)viewWidth, (GLfloat)viewHeight);

      glUseProgram (P_compute);

      glUniform2i (compU_sizeChars, nCols, nRows);

      setupTexture (GL_TEXTURE0, GL_TEXTURE_2D, T_output);
      glTexStorage2D (GL_TEXTURE_2D, 1, GL_RGBA32F, viewWidth, viewHeight);
      glBindImageTexture (0, T_output, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                          GL_RGBA32F);
      glCheckError ();

      setupStorageBuffer <Cell> (0, B_text, nRows * nCols);

      return true;
   }

   void
   CharVdev::setCursor (const Cursor& cursor)
   {
      static uint16_t prevPosX = 0;
      static uint16_t prevPosY = 0;

      glUseProgram (P_compute);
      glUniform3i (compU_cursorColor,
                   cursor.color.red, cursor.color.green, cursor.color.blue);
      glUniform4i (compU_cursorPos, cursor.posX, cursor.posY, prevPosX, prevPosY);
      prevPosX = cursor.posX;
      prevPosY = cursor.posY;
      glUniform1i (compU_cursorStyle, static_cast <uint8_t> (cursor.style));
   }

   void
   CharVdev::setSelection (const Rect& sel)
   {
      static Rect prev;
      Rect damage (std::min (sel.tl, prev.tl), std::max (sel.br, prev.br));
      uint32_t damageStart = nCols * damage.tl.y + damage.tl.x;
      uint32_t damageEnd = nCols * damage.br.y + damage.br.x + 1;
      prev = sel;

      glUseProgram (P_compute);
      glUniform4i (compU_selectRect, sel.tl.x, sel.tl.y, sel.br.x, sel.br.y);
      glUniform1i (compU_selectRectMode, static_cast <int> (sel.rectangular));
      glUniform2i (compU_selectDamage, damageStart, damageEnd);
   }

   void
   CharVdev::setDeltaFrame (bool delta)
   {
      glUseProgram (P_compute);
      glUniform1i (compU_deltaFrame, delta ? 1 : 0);
   }

   void
   CharVdev::draw ()
   {
      assert (cells == nullptr); // no mapping in place

      glUseProgram (P_compute);
      glActiveTexture (GL_TEXTURE0);
      glBindTexture (GL_TEXTURE_2D, T_output);
      glActiveTexture (GL_TEXTURE1);
      glBindTexture (GL_TEXTURE_2D_ARRAY, T_atlas);
      glActiveTexture (GL_TEXTURE2);
      glBindTexture (GL_TEXTURE_2D, T_atlasMap);
      if (hasDoubleWidth)
      {
         glActiveTexture (GL_TEXTURE3);
         glBindTexture (GL_TEXTURE_2D_ARRAY, T_atlas_dw);
         glActiveTexture (GL_TEXTURE4);
         glBindTexture (GL_TEXTURE_2D, T_atlasMap_dw);
      }
      glCheckError ();

      glDispatchCompute (nCols, nRows, 1);
      glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
      glCheckError ();

      glUseProgram (P_draw);
      glClearColor (opts.bg.red / 255.0, opts.bg.green / 255.0,
                    opts.bg.blue / 255.0, 1.0);
      glClear (GL_COLOR_BUFFER_BIT);

      glActiveTexture (GL_TEXTURE0);
      glBindTexture (GL_TEXTURE_2D, T_output);

      glEnableVertexAttribArray (A_pos);
      glEnableVertexAttribArray (A_vertexTexCoord);
      glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
   }

   CharVdev::Mapping::Mapping (uint16_t nCols_, uint16_t nRows_, Cell *& cells_)
      : nCols (nCols_)
      , nRows (nRows_)
      , cells (cells_)
   {
   };

   CharVdev::Mapping::~Mapping ()
   {
      assert (cells != nullptr); // mapping in place

      glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
      cells = nullptr;
   };

   CharVdev::Mapping CharVdev::getMapping ()
   {
      assert (cells == nullptr); // no mapping in place

      cells = reinterpret_cast <Cell *> (
                 glMapBufferRange (GL_SHADER_STORAGE_BUFFER,
                                   0, sizeof (Cell) * nRows * nCols,
                                   GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));

      return CharVdev::Mapping (nCols, nRows, cells);
   };

   // private methods

   void
   CharVdev::createShaders ()
   {
      GLuint S_compute, S_fragment, S_vertex;

      S_compute =
         createShader (GL_COMPUTE_SHADER, computeShaderSource, "compute");
      S_fragment =
         createShader (GL_FRAGMENT_SHADER, fragmentShaderSource, "fragment");
      S_vertex =
         createShader (GL_VERTEX_SHADER, vertexShaderSource, "vertex");

      P_compute = glCreateProgram ();
      glAttachShader (P_compute, S_compute);
      linkProgram (P_compute, "compute");
      glUseProgram (P_compute);

      compU_glyphPixels = glGetUniformLocation (P_compute, "glyphPixels");
      compU_sizeChars = glGetUniformLocation (P_compute, "sizeChars");
      compU_cursorColor = glGetUniformLocation (P_compute, "cursorColor");
      compU_cursorPos = glGetUniformLocation (P_compute, "cursorPos");
      compU_cursorStyle = glGetUniformLocation (P_compute, "cursorStyle");
      compU_selectRect = glGetUniformLocation (P_compute, "selectRect");
      compU_selectRectMode = glGetUniformLocation (P_compute, "selectRectMode");
      compU_selectDamage = glGetUniformLocation (P_compute, "selectDamage");
      compU_deltaFrame = glGetUniformLocation (P_compute, "deltaFrame");
      compU_showWraps = glGetUniformLocation (P_compute, "showWraps");
      compU_hasDoubleWidth = glGetUniformLocation (P_compute, "hasDoubleWidth");

      logT << "compute program:"
           << " uniform glyphPixels=" << compU_glyphPixels
           << " sizeChars=" << compU_sizeChars
           << " cursorColor=" << compU_cursorColor
           << " cursorPos=" << compU_cursorPos
           << " cursorStyle=" << compU_cursorStyle
           << " selectRect=" << compU_selectRect
           << " selectRectMode=" << compU_selectRectMode
           << " selectDamage=" << compU_selectDamage
           << " deltaFrame=" << compU_deltaFrame
           << " showWraps=" << compU_showWraps
           << " hasDoubleWidth=" << compU_hasDoubleWidth
           << std::endl;

      P_draw = glCreateProgram ();
      glAttachShader (P_draw, S_fragment);
      glAttachShader (P_draw, S_vertex);
      linkProgram (P_draw, "draw");
      glUseProgram (P_draw);

      A_pos = glGetAttribLocation (P_draw, "pos");
      A_vertexTexCoord = glGetAttribLocation (P_draw, "vertexTexCoord");
      drawU_viewPixels = glGetUniformLocation (P_draw, "viewPixels");

      logT << "draw program:"
           << " attrib pos=" << A_pos
           << " vertexTexCoord=" << A_vertexTexCoord
           << " uniform viewPixels=" << drawU_viewPixels
           << std::endl;
   }

} // namespace zutty
