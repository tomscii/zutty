#pragma once

#define GL_GLEXT_PROTOTYPES 1
#include <GLES3/gl31.h>
#include <EGL/egl.h>

#include <stdexcept>

#define glCheckError() gl::CheckError_(__FILE__, __LINE__)

namespace gl {

void CheckError_(const char* file, int line);

} // namespace
