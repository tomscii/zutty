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

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>
#include <string>
#include <vector>

namespace font {

class Font {
public:
   Font(const std::string& filename);
   ~Font();

   uint16_t getPx() const { return px; };
   uint16_t getPy() const { return py; };
   uint16_t getNx() const { return nx; };
   uint16_t getNy() const { return ny; };
   const uint8_t* getAtlas() const { return atlas_buf.data(); };

private:
   std::string filename;
   uint16_t px; // glyph width in pixels
   uint16_t py; // glyph height in pixels
   uint16_t nx; // number of glyphs in atlas texture per row
   uint16_t ny; // number of rows in atlas texture
   std::vector <uint8_t> atlas_buf; // loaded atlas data

   void load();
   void load_face(FT_Face face, FT_ULong c);
   void get_max_bbox(FT_Face face, unsigned& width, unsigned& height);
};

} // namespace font
