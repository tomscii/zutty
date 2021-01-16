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
#include "log.h"
#include "options.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>

namespace zutty {

   Font::Font (const std::string& filename_)
      : filename (filename_)
      , overlay (false)
   {
      load ();
   }

   Font::Font (const std::string& filename_, const Font& priFont)
      : filename (filename_)
      , overlay (true)
      , px (priFont.getPx ())
      , py (priFont.getPy ())
      , baseline (priFont.getBaseline ())
      , nx (priFont.getNx ())
      , ny (priFont.getNy ())
      , atlasBuf (priFont.getAtlas ())
      , atlasMap (priFont.getAtlasMap ())
   {
      load ();
   }

   // private methods

   void Font::load ()
   {
      FT_Library ft;
      FT_Face face;

      if (FT_Init_FreeType (&ft))
         throw std::runtime_error ("Could not initialize FreeType library");
      logI << "Loading " << filename << " as "
           << (overlay ? "overlay" : "primary") << std::endl;
      if (FT_New_Face (ft, filename.c_str (), 0, &face))
         throw std::runtime_error (std::string ("Failed to load font ") +
                                   filename);

      logT << "Family: " << face->family_name
           << "; Style: " << face->style_name
           << "; Faces: " << face->num_faces
           << "; Glyphs: " << face->num_glyphs
           << std::endl;

      if (face->num_fixed_sizes > 0)
         loadFixed (face);
      else
         loadScaled (face);

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
         while ((unsigned) nx * ny < n_glyphs)
         {
            if (px * nx < py * ny)
               ++nx;
            else
               ++ny;
         }
         logT << "Atlas texture geometry: " << nx << "x" << ny
              << " glyphs of " << px << "x" << py << " each, "
              << "yielding pixel size " << nx*px << "x" << ny*py << "."
              << std::endl;
         logT << "Atlas holds space for " << nx*ny << " glyphs, "
              << n_glyphs << " will be used, empty: "
              << nx*ny - n_glyphs << " ("
              << 100.0 * (nx*ny - n_glyphs) / (nx*ny)
              << "%)" << std::endl;

         size_t atlas_bytes = nx * px * ny * py;
         logT << "Allocating " << atlas_bytes << " bytes for atlas buffer"
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

      if (loadSkipCount)
      {
         logI << "Skipped loading " << loadSkipCount << " code point(s) "
              << "outside the Basic Multilingual Plane"
              << std::endl;
      }

      FT_Done_Face (face);
      FT_Done_FreeType (ft);

      setupSupportedCodes ();
   }

   void Font::loadFixed (const FT_Face& face)
   {
      int bestIdx = -1;
      int bestHeightDiff = std::numeric_limits<int>::max ();
      {
         std::ostringstream oss;
         oss << "Available sizes:";
         for (int i = 0; i < face->num_fixed_sizes; ++i)
         {
            oss << " " << face->available_sizes[i].width
                << "x" << face->available_sizes[i].height;

            int diff = abs (opts.fontsize - face->available_sizes[i].height);
            if (diff < bestHeightDiff)
            {
               bestIdx = i;
               bestHeightDiff = diff;
            }
         }
         logT << oss.str () << std::endl;
      }

      logT << "Configured size: " << (int)opts.fontsize
           << "; Best matching fixed size: "
           << face->available_sizes[bestIdx].width
           << "x" << face->available_sizes[bestIdx].height
           << std::endl;

      if (bestHeightDiff > 1 && face->units_per_EM > 0)
      {
         logT << "Size mismatch too large, fallback to rendering outlines."
              << std::endl;
         loadScaled (face);
         return;
      }

      const auto& facesize = face->available_sizes [bestIdx];

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
         baseline = 0;
      }
      logI << "Glyph size " << px << "x" << py << std::endl;

      if (FT_Set_Pixel_Sizes (face, px, py))
         throw std::runtime_error ("Could not set pixel sizes");

      if (!overlay && face->height)
      {
         // If we are loading a fixed bitmap strike of an otherwise scaled
         // font, we need the baseline metric.
         baseline = py * (double)face->ascender / face->height;
      }
   }

   void Font::loadScaled (const FT_Face& face)
   {
      logI << "Pixel size " << (int)opts.fontsize << std::endl;
      if (FT_Set_Pixel_Sizes (face, opts.fontsize, opts.fontsize))
         throw std::runtime_error ("Could not set pixel sizes");

      if (!overlay)
      {
         px = opts.fontsize *
              (double)face->max_advance_width / face->units_per_EM;
         py = px * (double)face->height / face->max_advance_width + 1;
         baseline = py * (double)face->ascender / face->height;
      }
      logI << "Glyph size " << px << "x" << py << std::endl;
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
        #ifdef DEBUG
         logT << "Skip loading code point 0x" << std::hex << c << std::dec
              << " outside the Basic Multilingual Plane" << std::endl;
        #endif
         ++loadSkipCount;
         return;
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


      // destination pixel offset
      const unsigned int dx = face->glyph->bitmap_left > 0
                            ? face->glyph->bitmap_left : 0;
      const unsigned int dy = baseline && baseline > face->glyph->bitmap_top
                            ? baseline - face->glyph->bitmap_top : 0;

      // raw/rasterized bitmap dimensions
      const unsigned int bh = std::min (face->glyph->bitmap.rows, py - dy);
      const unsigned int bw = std::min (face->glyph->bitmap.width, px - dx);

      const int atlas_row_offset = nx * px * py;
      const int atlas_glyph_offset = apos.y * atlas_row_offset + apos.x * px;
      const int atlas_write_offset = atlas_glyph_offset + nx * px * dy + dx;

      if (overlay) // clear glyph area, as we are overwriting an existing glyph
      {
         for (unsigned int j = 0; j < bh; ++j) {
            uint8_t* atl_dst_row =
               atlasBuf.data () + atlas_glyph_offset + j * nx * px;
            for (unsigned int k = 0; k < bw; ++k) {
               *atl_dst_row++ = 0;
            }
         }
      }

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
            atl_dst_row = atlasBuf.data () + atlas_write_offset + j * nx * px;
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
            atl_dst_row = atlasBuf.data () + atlas_write_offset + j * nx * px;
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

   void
   Font::setupSupportedCodes ()
   {
      supportedCodes.reserve (atlasMap.size ());
      auto it = atlasMap.begin ();
      const auto itEnd = atlasMap.end ();
      for ( ; it != itEnd; ++it)
      {
         supportedCodes.push_back (it->first);
      }
      std::sort (supportedCodes.begin (), supportedCodes.end ());
   }

} // namespace zutty
