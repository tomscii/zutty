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

#include "font.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace zutty {

   Font::Font (const std::string& filename_)
      : filename (filename_)
   {
      load ();
   }

   /* Load an alternate font based on an already loaded primary font,
    * conforming to the same atlas geometry (incl. position mapping)
    * and starting with a copy of the atlas texture data.
    * It is an error if the alternate font has different geometry.
    * Any code point not having a glyph in the alternate font will
    * have the glyph of the primary font in its atlas.
    * Any code point not having a glyph in the primary font will be
    * discarded.
    */
   Font::Font (const std::string& filename_, const Font& priFont)
      : filename (filename_)
      , px (priFont.getPx ())
      , py (priFont.getPy ())
      , nx (priFont.getNx ())
      , ny (priFont.getNy ())
      , atlasBuf (priFont.getAtlas ())
      , atlasMap (priFont.getAtlasMap ())
   {
      load (true); // load as overlay
   }

   void Font::clearAtlasData ()
   {
      atlasBuf.clear ();
      atlasBuf.shrink_to_fit ();
   }

   // private methods

   /* Load font from glyph bitmaps rasterized by FreeType.
    * Store the bitmaps into an atlas bitmap stored in atlasBuf.
    */
   void Font::load (bool overlay)
   {
      FT_Library ft;
      FT_Face face;

      if (FT_Init_FreeType (&ft))
         throw std::runtime_error ("Could not initialize FreeType library");
      std::cout << "Loading " << filename << " as overlay" << std::endl;
      if (FT_New_Face (ft, filename.c_str (), 0, &face))
         throw std::runtime_error (std::string ("Failed to load font ") + filename);

      std::cout << "Font family: " << face->family_name << std::endl
                << "Font style: " << face->style_name << std::endl
                << "Number of faces: " << face->num_faces << std::endl
                << "Number of glyphs: " << face->num_glyphs << std::endl;

      if (face->num_fixed_sizes < 1)
         throw std::runtime_error (filename + ": no fixed sizes found; "
                                   "es2term requires a fixed font!");

      std::cout << "Available sizes:";
      for (int i = 0; i < face->num_fixed_sizes; ++i)
         std::cout << " " << face->available_sizes[i].width
                   << "x" << face->available_sizes[i].height;
      std::cout << std::endl;

      // Just use the first available size for now (FIXME)
      const auto& facesize = face->available_sizes[0];

      if (overlay)
      {
         if (px != facesize.width)
            throw std::runtime_error (
               filename + ": size mismatch, expected px=" + std::to_string (px)
               + ", got: " + std::to_string (facesize.width));
         if (py != facesize.height)
            throw std::runtime_error (
               filename + ": size mismatch, expected py=" + std::to_string (py)
               + ", got: " + std::to_string (facesize.height));
      }
      else
      {
         px = facesize.width;
         py = facesize.height;
      }
      std::cout << "Loading size " << px << "x" << py << std::endl;

      if (FT_Set_Pixel_Sizes (face, px, py))
         throw std::runtime_error ("Could not set pixel sizes");

      /* Given that we have face->num_glyphs glyphs to load, with each
       * individual glyph having a size of px * py, compute nx and ny so
       * that the resulting atlas texture geometry is closest to a square.
       * We use one extra glyph space to guarantee a blank glyph at (0,0).
       */
      /* TODO we must guarantee that the atlas will be addressable by
       * one byte in each dimension, i.e., 256x256 max, even if this
       * means that its pixel size won't be as close to a square as
       * otherwise possible.
       */
      if (!overlay)
      {
         unsigned n_glyphs = face->num_glyphs + 1;
         unsigned long total_pixels = n_glyphs * px * py;
         double side = sqrt (total_pixels);
         nx = side / px;
         ny = side / py;
         while (nx * ny < n_glyphs)
         {
            if (px * nx < py * ny)
               ++nx;
            else
               ++ny;
         }
         std::cout << "Atlas texture geometry: " << nx << "x" << ny
                   << " glyphs of " << px << "x" << py << " each, "
                   << "yielding pixel size " << nx*px << "x" << ny*py << "."
                   << std::endl;
         std::cout << "Atlas holds space for " << nx*ny << " glyphs, "
                   << n_glyphs << " will be used, empty: "
                   << nx*ny - n_glyphs << " ("
                   << 100.0 * (nx*ny - n_glyphs) / (nx*ny)
                   << "%)" << std::endl;

         size_t atlas_bytes = nx * px * ny * py;
         std::cout << "Allocating " << atlas_bytes << " bytes for atlas buffer"
                   << std::endl;
         atlasBuf.resize (atlas_bytes, 0);
      }

      FT_UInt gindex;
      FT_ULong charcode = FT_Get_First_Char (face, &gindex);
      while (gindex != 0)
      {
         if (overlay)
         {
            const auto& it = atlasMap.find (charcode);
            if (it != atlasMap.end ())
            {
               loadFace (face, charcode, it->second);
            }
         }
         else
         {
            loadFace (face, charcode);
         }
         charcode = FT_Get_Next_Char (face, charcode, &gindex);
      }

      FT_Done_Face (face);
      FT_Done_FreeType (ft);
   }

   void Font::loadFace (const FT_Face& face, FT_ULong c)
   {
      const uint8_t atlas_row = atlas_seq / nx;
      const uint8_t atlas_col = atlas_seq - nx * atlas_row;
      const AtlasPos apos = {atlas_col, atlas_row};

      loadFace (face, c, apos);
      atlasMap [c] = apos;
      ++atlas_seq;
   }

   void Font::loadFace (const FT_Face& face, FT_ULong c, const AtlasPos& apos)
   {
      if (c > std::numeric_limits<uint16_t>::max ())
      {
         std::cout << "loadFace: skipping code point " << c
                   << " as it exceeds 16 bits storage limit."
                   << std::endl;
      }

      if (FT_Load_Char (face, c, FT_LOAD_RENDER))
      {
         throw std::runtime_error (
            std::string ("FreeType: Failed to load glyph for char ") +
            std::to_string (c));
      }

      if (FT_Render_Glyph (face->glyph, FT_RENDER_MODE_NORMAL))
         throw std::runtime_error (
            std::string ("FreeType: Failed to render glyph for char ") +
            std::to_string (c));

      const int atlas_row_offset = nx * px * py;
      const int atlas_glyph_offset = apos.y * atlas_row_offset + apos.x * px;

      const unsigned int bh = face->glyph->bitmap.rows;
      const unsigned int bw = face->glyph->bitmap.width;

      /* Load bitmap into atlas buffer area. Each row in the bitmap
       * occupies bitmap.pitch bytes (with padding); this is the
       * increment in the input bitmap array per row.
       *
       * Interpretation of bytes within the bitmap rows is subject to
       * bitmap.pixel_mode, essentially either 8 bits (256-scale gray)
       * per pixel, or 1 bit (mono) per pixel. Leftmost pixel is MSB.
       *
       */
      const auto& bmp = face->glyph->bitmap;
      const uint8_t* bmp_src_row;
      uint8_t* atl_dst_row;
      switch (bmp.pixel_mode)
      {
      case FT_PIXEL_MODE_MONO:
         for (unsigned int j = 0; j < bh; ++j) {
            bmp_src_row = bmp.buffer + j * bmp.pitch;
            atl_dst_row = atlasBuf.data () + atlas_glyph_offset + j * nx * px;
            uint8_t byte = 0;
            for (unsigned int k = 0; k < bw; ++k) {
               if (k % 8 == 0) {
                  byte = *bmp_src_row++;
               }
               *atl_dst_row++ = (byte & 0x80) ? 0xFF : 0;
               byte <<= 1;
            }
         }
         break;
      case FT_PIXEL_MODE_GRAY:
         for (unsigned int j = 0; j < bh; ++j) {
            bmp_src_row = bmp.buffer + j * bmp.pitch;
            atl_dst_row = atlasBuf.data () + atlas_glyph_offset + j * nx * px;
            for (unsigned int k = 0; k < bw; ++k) {
               *atl_dst_row++ = *bmp_src_row++;
            }
         }
         break;
      default:
         throw std::runtime_error (
            std::string ("Unhandled pixel_type=") +
            std::to_string (bmp.pixel_mode));
      }
   }

} // namespace zutty
