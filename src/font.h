#pragma once

#include "gl.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>
#include <string>
#include <vector>

namespace font {

class Font {
public:
   Font();
   ~Font();

   /* This must be called within the GL context before any further use.
    * atlas_texture will refer to a newly setup texture containing the atlas.
    */
   void init(const std::string& filename, GLuint& atlas_texture);

   uint16_t getPx() const { return px; };
   uint16_t getPy() const { return py; };
   uint16_t getNx() const { return nx; };
   uint16_t getNy() const { return ny; };

private:
   std::string filename;
   uint16_t atlas_x; // atlas texture width in pixels
   uint16_t atlas_y; // atlas texture height in pixels
   uint16_t px;      // glyph width in pixels
   uint16_t py;      // glyph height in pixels
   uint16_t nx;      // number of glyphs in atlas texture per row
   uint16_t ny;      // number of rows in atlas texture

   void load(GLuint& atlas_texture);
   void load_face(FT_Face face, uint8_t* atlas_buf, FT_ULong c);
   void get_max_bbox(FT_Face face, unsigned& width, unsigned& height);
};

} // namespace font
