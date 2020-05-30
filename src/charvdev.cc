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

void main () {
   vec3 color = vec3 (0.85, 0.85, 0.85);

   ivec2 charPos = ivec2 (gl_GlobalInvocationID.xy);
   ivec4 charData = ivec4 (texelFetch (text, charPos, 0) * 256.0);
   ivec2 charCode = charData.xy; // .zw: attrs, colors
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

      text_data = std::unique_ptr <uint8_t []> (new uint8_t [4 * nCols * nRows]);
      memset (text_data.get (), 0, 4 * nCols * nRows);
      for (uint16_t j = 0; j < nRows; ++j)
      {
         for (uint16_t k = 0; k < nCols; ++k)
         {
            text_data[4 * (j * nCols + k)]     = k > 255 ? 0 : k;
            text_data[4 * (j * nCols + k) + 1] = k > 255 ? 0 : j;
            text_data[4 * (j * nCols + k) + 2] = 0; // color and
            text_data[4 * (j * nCols + k) + 3] = 0; // attributes
         }
      }

      std::cout << "resize to " << pxWidth << " x " << pxHeight
                << " pixels, " << nCols << " x " << nRows << " chars"
                << std::endl;

      GLint viewWidth = nCols * fnt.getPx ();
      GLint viewHeight = nRows * fnt.getPy ();
      glViewport (0, pxHeight - viewHeight, viewWidth, viewHeight);

      glUseProgram (P_draw);

      glUniform2f (drawU_viewPixels, (GLfloat)viewWidth, (GLfloat)viewHeight);

      glUseProgram (P_compute);

      setupTexture (GL_TEXTURE0, T_output);
      glTexStorage2D (GL_TEXTURE_2D, 1, GL_RGBA32F, viewWidth, viewHeight);
      glBindImageTexture (0, T_output, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                          GL_RGBA32F);
      glCheckError ();

      setupTexture (GL_TEXTURE2, T_text);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, nCols, nRows, 0, GL_RGBA,
                    GL_UNSIGNED_BYTE, text_data.get ());
      glCheckError ();
   }

   void
   CharVdev::draw ()
   {
      {
         uint16_t cRow = nRows - 1;
         uint16_t cCol = nCols - 1;
         uint32_t cnt = ++draw_count;
         while (cnt)
         {
            uint32_t digit = (cnt % 10) + 18;
            text_data[4 * (cRow * nCols + cCol)] = digit;
            text_data[4 * (cRow * nCols + cCol) + 1] = 0;
            cnt /= 10;
            --cCol;
         }
      }

      glUseProgram (P_compute);
      glActiveTexture (GL_TEXTURE0);
      glBindTexture (GL_TEXTURE_2D, T_output);
      glActiveTexture (GL_TEXTURE1);
      glBindTexture (GL_TEXTURE_2D, T_atlas);
      glActiveTexture (GL_TEXTURE2);
      glBindTexture (GL_TEXTURE_2D, T_text);
      glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, nCols, nRows, GL_RGBA,
                       GL_UNSIGNED_BYTE, text_data.get ());
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

      std::cout << "compute program:"
                << "\n  uniform glyphPixels at " << compU_glyphPixels
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
