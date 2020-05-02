#pragma once

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h> // for GL_RED_EXT
#include <EGL/egl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <vector>

namespace font {

struct vec4 {
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

class Text {
public:
    Text();
    ~Text();

    /* This must be called within the GL context before any further use. */
    void init(const std::string& filename);

    void draw();

private:
    std::string filename;
    GLuint atlas_texture;
    unsigned atlas_x; // atlas width in pixels
    unsigned atlas_y; // atlas height in pixels
    unsigned px;      // glyph width in pixels
    unsigned py;      // glyph height in pixels
    unsigned nx;      // number of glyphs in atlas texture per row
    unsigned ny;      // number of rows in atlas texture

    void load_font();
    void load_face(FT_Face face, GLubyte* atlas_buf, FT_ULong c);
    void get_max_bbox(FT_Face face, unsigned& width, unsigned& height);
};

} // namespace font
