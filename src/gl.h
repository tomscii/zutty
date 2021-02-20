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

#define GL_GLEXT_PROTOTYPES 1
#include <GLES3/gl31.h>
#include <EGL/egl.h>

#include <stdexcept>

#ifdef DEBUG
#define glCheckError() gl::CheckError_(__FILE__, __LINE__)
#else
#define glCheckError()
#endif

namespace gl
{
   void CheckError_ (const char* file, int line);

} // namespace
