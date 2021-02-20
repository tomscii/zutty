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

#include "gl.h"

#include <string>

namespace gl
{
   void CheckError_ (const char* file, int line)
   {
      GLenum error;
      while ((error = glGetError ()) != GL_NO_ERROR) {
         std::string estr;
         switch (error) {
         case GL_INVALID_ENUM:      estr = "INVALID_ENUM"; break;
         case GL_INVALID_VALUE:     estr = "INVALID_VALUE"; break;
         case GL_INVALID_OPERATION: estr = "INVALID_OPERATION"; break;
         case GL_OUT_OF_MEMORY:     estr = "OUT_OF_MEMORY"; break;
         default: estr = "glGetError() = " + std::to_string (error); break;
         }
         throw std::runtime_error (estr + " at " + file + ":" +
                                   std::to_string (line));
      }
   }

} // namespace gl
