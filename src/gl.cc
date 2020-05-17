#include "gl.h"

namespace gl {

void CheckError_(const char* file, int line)
{
   GLenum error;
   while ((error = glGetError()) != GL_NO_ERROR) {
      std::string estr;
      switch (error) {
      case GL_INVALID_ENUM:      estr = "INVALID_ENUM"; break;
      case GL_INVALID_VALUE:     estr = "INVALID_VALUE"; break;
      case GL_INVALID_OPERATION: estr = "INVALID_OPERATION"; break;
      case GL_OUT_OF_MEMORY:     estr = "OUT_OF_MEMORY"; break;
      default: estr = "glGetError() = " + std::to_string(error); break;
      }
      throw std::runtime_error(estr + " at " + file + ":" + std::to_string(line));
   }
}

} // namespace
