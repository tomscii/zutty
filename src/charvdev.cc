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

#include <algorithm>
#include <cassert>
#include <iostream>

namespace {

   static const char *computeShaderSource = R"(#version 310 es

layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) writeonly lowp uniform image2D imgOut;
layout (binding = 1) uniform lowp sampler2DArray atlas;
layout (binding = 2) uniform lowp sampler2D atlasMap;
uniform lowp ivec2 glyphPixels;
uniform lowp ivec2 sizeChars;

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

   ivec2 charCode =
      ivec2 (bitfieldExtract (cell.charData, 0, 8),  // Lowest byte
             bitfieldExtract (cell.charData, 8, 8)); // Next-lowest byte

   // fontIdx == 0 -> Normal; 1 -> Bold
   uint fontIdx = bitfieldExtract (cell.charData, 16, 1);
   uint underline = bitfieldExtract (cell.charData, 17, 1);
   uint inverse = bitfieldExtract (cell.charData, 18, 1);

   ivec2 atlasPos = ivec2 (vec2 (256) * texelFetch (atlasMap, charCode, 0).zw);

   vec3 fgColor = vec3 (float (bitfieldExtract (cell.fg, 0, 8)),
                        float (bitfieldExtract (cell.fg, 8, 8)),
                        float (bitfieldExtract (cell.fg, 16, 8))) / 255.0;

   vec3 bgColor = vec3 (float (bitfieldExtract (cell.bg, 0, 8)),
                        float (bitfieldExtract (cell.bg, 8, 8)),
                        float (bitfieldExtract (cell.bg, 16, 8))) / 255.0;

   if (inverse == uint (1)) {
      fgColor = vec3 (1.0) - fgColor;
      bgColor = vec3 (1.0) - bgColor;
   }

   for (int j = 0; j < glyphPixels.x; j++) {
      for (int k = 0; k < glyphPixels.y; k++) {
         ivec2 txCoords = atlasPos * glyphPixels + ivec2 (j, k);
         ivec3 txc = ivec3 (txCoords, fontIdx);
         float lumi = texelFetch (atlas, txc, 0).r;
         vec4 pixel = vec4 (fgColor * lumi + bgColor * (1.0 - lumi), 1.0);
         ivec2 pxCoords = charPos * glyphPixels + ivec2 (j, k);
         imageStore (imgOut, pxCoords, pixel);
      }
   }

   if (underline == uint (1)) {
      for (int j = 0; j < glyphPixels.x; j++) {
         vec4 pixel = vec4 (fgColor, 1.0);
         ivec2 pxCoords = charPos * glyphPixels + ivec2 (j, glyphPixels.y - 1);
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
         char log[1000];
         GLsizei len;
         glGetShaderInfoLog (shader, 1000, &len, log);
         std::cerr << "Error compiling " << name << " shader:\n"
                   << log << std::endl;
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
         char log[1000];
         GLsizei len;
         glGetProgramInfoLog (program, 1000, &len, log);
         std::cerr << "Error: linking " << name << " program:\n"
                   << log << std::endl;
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
}

namespace zutty {

   CharVdev::CharVdev (const Font& priFont_,
                       const Font& altFont_,
                       uint16_t borderPx_)
      : priFont (priFont_)
      , altFont (altFont_)
      , borderPx (borderPx_)
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
      glUniform2i (compU_glyphPixels, priFont.getPx (), priFont.getPy ());
      glUniform2i (compU_sizeChars, nCols, nRows);
      glCheckError ();

      // Setup atlas texture
      setupTexture (GL_TEXTURE1, GL_TEXTURE_2D_ARRAY, T_atlas);
      glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8,
                     priFont.getPx () * priFont.getNx (),
                     priFont.getPy () * priFont.getNy (),
                     2);
      glCheckError ();

      glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                      0,    // mipmap level, always zero
                      0, 0, // X and Y offsets into texture area
                      0,    // layer index offset
                      priFont.getPx () * priFont.getNx (),
                      priFont.getPy () * priFont.getNy (),
                      1,    // number of layers, i.e., fonts, loaded
                      GL_RED, GL_UNSIGNED_BYTE, priFont.getAtlasData ());
      glCheckError ();

      glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                      0,    // mipmap level, always zero
                      0, 0, // X and Y offsets into texture area
                      1,    // layer index offset
                      priFont.getPx () * priFont.getNx (),
                      priFont.getPy () * priFont.getNy (),
                      1,    // number of layers, i.e., fonts, loaded
                      GL_RED, GL_UNSIGNED_BYTE, altFont.getAtlasData ());
      glCheckError ();

      // Setup atlas mapping texture
      auto atlasMap = std::vector <uint8_t> ();
      atlasMap.resize (2 * 256 * 256, 0);
      auto it = priFont.getAtlasMap ().begin ();
      auto itEnd = priFont.getAtlasMap ().end ();
      for (; it != itEnd; ++it)
      {
         atlasMap [2 * it->first] = it->second.x;
         atlasMap [2 * it->first + 1] = it->second.y;
      }
      setupTexture (GL_TEXTURE2, GL_TEXTURE_2D, T_atlasMap);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 256, 256, 0,
                   GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, atlasMap.data ());
   }

   CharVdev::~CharVdev ()
   {
   }

   void
   CharVdev::resize (uint16_t pxWidth_, uint16_t pxHeight_)
   {
      assert (cells == nullptr); // no mapping in place

      if (pxWidth == pxWidth_ && pxHeight == pxHeight_)
         return;

      pxWidth = pxWidth_;
      pxHeight = pxHeight_;
      nCols = (pxWidth - (2 * borderPx)) / priFont.getPx ();
      nRows = (pxHeight - (2 * borderPx)) / priFont.getPy ();

      std::cout << "resize to " << pxWidth << " x " << pxHeight
                << " pixels, " << nCols << " x " << nRows << " chars"
                << std::endl;

      GLint viewWidth = nCols * priFont.getPx ();
      GLint viewHeight = nRows * priFont.getPy ();
      glViewport (borderPx, pxHeight - viewHeight - borderPx,
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
      glCheckError ();

      glDispatchCompute (nCols, nRows, 1);
      glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
      glCheckError ();

      glUseProgram (P_draw);
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
                                   GL_MAP_WRITE_BIT));

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

      std::cout << "compute program:"
                << "\n  uniform glyphPixels at " << compU_glyphPixels
                << "\n  uniform sizeChars at " << compU_sizeChars
                << std::endl;

      P_draw = glCreateProgram ();
      glAttachShader (P_draw, S_fragment);
      glAttachShader (P_draw, S_vertex);
      linkProgram (P_draw, "draw");
      glUseProgram (P_draw);

      A_pos = glGetAttribLocation (P_draw, "pos");
      A_vertexTexCoord = glGetAttribLocation (P_draw, "vertexTexCoord");
      drawU_viewPixels = glGetUniformLocation (P_draw, "viewPixels");

      std::cout << "draw program:"
                << "\n  attrib pos at " << A_pos
                << "\n  attrib vertexTexCoord at " << A_vertexTexCoord
                << "\n  uniform viewPixels at " << drawU_viewPixels
                << std::endl;
   }

} // namespace zutty
