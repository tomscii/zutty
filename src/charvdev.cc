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

#include <iostream>

namespace {

   static const char *computeShaderSource = R"(#version 310 es

layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) writeonly lowp uniform image2D imgOut;
layout (binding = 1) uniform lowp sampler2D atlas;
layout (binding = 2) uniform lowp sampler2D text;
uniform lowp ivec2 glyphPixels;
uniform lowp ivec2 sizeChars;

struct Cell
{
   highp int charData;
   highp int fg;
   highp int bg;
};

layout (std430, binding = 0) buffer CharVideoMem
{
   Cell cells[];
} vmem;

void main () {

   vec3 color = vec3 (0.85, 0.85, 0.85);

   ivec2 charPos = ivec2 (gl_GlobalInvocationID.xy);
   int idx = sizeChars.x * charPos.y + charPos.x;
   highp int charData = vmem.cells[idx].charData;
   ivec2 charCode =
        ivec2 (bitfieldExtract (charData, 0, 8),  // Lowest byte
               bitfieldExtract (charData, 8, 8)); // Next-lowest byte
   ivec2 atlasPos = charCode; // TODO lookup in atlas mapping

   for (int j = 0; j < glyphPixels.x; j++) {
      for (int k = 0; k < glyphPixels.y; k++) {
         ivec2 txCoords = atlasPos * glyphPixels + ivec2 (j, k);
         vec4 pixel = vec4 (color * texelFetch (atlas, txCoords, 0).r, 1.0);
         ivec2 pxCoords = charPos * glyphPixels + ivec2 (j, k);
         imageStore (imgOut, pxCoords, pixel);
      }
   }
}
)";

   static const char *vertexShaderSource = R"(#version 310 es

layout (location = 0) in vec2 pos;
layout (location = 1) in lowp vec2 vertexTexCoord;

out vec2 texCoord;

void main () {
   texCoord = vertexTexCoord;
   gl_Position = vec4 (pos, 0.0, 1.0);
}
)";

   static const char *fragmentShaderSource = R"(#version 310 es

in highp vec2 texCoord;

layout (rgba32f, binding = 0) readonly lowp uniform image2D imgOut;

uniform highp vec2 viewPixels;

layout (location = 0) out lowp vec4 outColor;

void main () {
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
   setupTexture (GLuint target, GLuint& texture)
   {
      if (texture)
      {
         glDeleteTextures (1, &texture);
      }
      glGenTextures (1, &texture);
      glActiveTexture (target);
      glBindTexture (GL_TEXTURE_2D, texture);
      glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   }
}

namespace zutty {

   CharVdev::CharVdev (const std::string& priFontPath,
                       const std::string& altFontPath)
      : fnt (priFontPath)
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
      glUniform2i (compU_glyphPixels, fnt.getPx (), fnt.getPy ());
      glUniform2i (compU_sizeChars, nCols, nRows);
      glCheckError ();

      setupTexture (GL_TEXTURE1, T_atlas);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RED,
                    fnt.getPx () * fnt.getNx (), fnt.getPy () * fnt.getNy (),
                    0, GL_RED, GL_UNSIGNED_BYTE, fnt.getAtlas ());
      glCheckError ();
   }

   CharVdev::~CharVdev ()
   {
   }

   void
   CharVdev::resize (uint16_t pxWidth_, uint16_t pxHeight_)
   {
      if (pxWidth == pxWidth_ && pxHeight == pxHeight_)
         return;

      pxWidth = pxWidth_;
      pxHeight = pxHeight_;
      nCols = pxWidth / fnt.getPx ();
      nRows = pxHeight / fnt.getPy ();

      std::cout << "resize to " << pxWidth << " x " << pxHeight
                << " pixels, " << nCols << " x " << nRows << " chars"
                << std::endl;

      GLint viewWidth = nCols * fnt.getPx ();
      GLint viewHeight = nRows * fnt.getPy ();
      glViewport (0, pxHeight - viewHeight, viewWidth, viewHeight);

      glUseProgram (P_draw);

      glUniform2f (drawU_viewPixels, (GLfloat)viewWidth, (GLfloat)viewHeight);

      glUseProgram (P_compute);

      glUniform2i (compU_sizeChars, nCols, nRows);

      setupTexture (GL_TEXTURE0, T_output);
      glTexStorage2D (GL_TEXTURE_2D, 1, GL_RGBA32F, viewWidth, viewHeight);
      glBindImageTexture (0, T_output, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                          GL_RGBA32F);
      glCheckError ();

      if (B_text)
      {
         glDeleteBuffers (1, &B_text);
      }
      glGenBuffers (1, &B_text);
      glBindBuffer (GL_SHADER_STORAGE_BUFFER, B_text);
      glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, B_text);
      glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (Cell) * nRows * nCols,
                    nullptr, GL_DYNAMIC_DRAW);
      Cell* cells = (Cell*) glMapBufferRange (
         GL_SHADER_STORAGE_BUFFER, 0, sizeof (Cell) * nRows * nCols,
         GL_MAP_WRITE_BIT/* | GL_MAP_FLUSH_EXPLICIT_BIT*/);
      glCheckError ();
      for (uint16_t j = 0; j < nRows; ++j)
      {
         for (uint16_t k = 0; k < nCols; ++k)
         {
            cells [j * nCols + k].uc_pt = k > 255 ? 0 : 256 * j + k;
            cells [j * nCols + k].attrs = 0;
         }
      }
      glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
      glCheckError ();
   }

   void
   CharVdev::draw ()
   {
      {
         uint16_t cRow = nRows - 1;
         uint16_t cCol = nCols - 1;
         uint32_t cnt = ++draw_count;

         Cell* cells = (Cell*) glMapBufferRange (
            GL_SHADER_STORAGE_BUFFER, 0, sizeof (Cell) * nRows * nCols,
            GL_MAP_WRITE_BIT/* | GL_MAP_FLUSH_EXPLICIT_BIT*/);
         glCheckError ();

         while (cnt)
         {
            uint32_t digit = (cnt % 10) + 18;
            cells [cRow * nCols + cCol].uc_pt = digit;
            cnt /= 10;
            --cCol;
         }
         glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
         glCheckError ();
      }

      glUseProgram (P_compute);
      glActiveTexture (GL_TEXTURE0);
      glBindTexture (GL_TEXTURE_2D, T_output);
      glActiveTexture (GL_TEXTURE1);
      glBindTexture (GL_TEXTURE_2D, T_atlas);
      glBindBuffer (GL_SHADER_STORAGE_BUFFER, B_text);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, B_text);
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

   // private methods

   void
   CharVdev::createShaders ()
   {
      GLuint S_compute, S_fragment, S_vertex;

      S_compute = createShader (GL_COMPUTE_SHADER, computeShaderSource,
                                "compute");
      S_fragment = createShader (GL_FRAGMENT_SHADER, fragmentShaderSource,
                                 "fragment");
      S_vertex = createShader (GL_VERTEX_SHADER, vertexShaderSource,
                               "vertex");

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
