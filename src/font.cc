#include "font.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace font {

Font::Font()
{
}

Font::~Font()
{
}

void Font::init(const std::string& filename_, GLuint& atlas_texture)
{
   filename = filename_;
   load(atlas_texture);
}

/* private methods */

/* Load font from glyph bitmaps rasterized by FreeType.
 * As a result, the atlas texture will be filled.
 * Also, the glyphs array is filled with metrics for each loaded glyph.
 * Part of this data is loaded into the metrics texture for shader lookup.
 */
void Font::load(GLuint& atlas_texture)
{
    FT_Library ft;
    FT_Face face;

    if (FT_Init_FreeType(&ft))
       throw std::runtime_error("Could not initialize FreeType library");
    std::cout << "Loading " << filename << std::endl;
    if (FT_New_Face(ft, filename.c_str(), 0, &face))
       throw std::runtime_error(std::string("Failed to load font ") + filename);

    std::cout << "Font family: " << face->family_name << std::endl
              << "Font style: " << face->style_name << std::endl
              << "Number of faces: " << face->num_faces << std::endl
              << "Number of glyphs: " << face->num_glyphs << std::endl;

    if (face->num_fixed_sizes < 1)
        throw std::runtime_error(filename + ": no fixed sizes found; "
                                 "es2term requires a fixed font!");

    std::cout << "Available sizes:";
    for (int i = 0; i < face->num_fixed_sizes; ++i)
        std::cout << " " << face->available_sizes[i].width
                  << "x" << face->available_sizes[i].height;
    std::cout << std::endl;
    // Just use the first available size
    px = face->available_sizes[0].width;
    py = face->available_sizes[0].height;
    std::cout << "Loading size " << px << "x" << py << std::endl;

    if (FT_Set_Pixel_Sizes(face, px, py))
       throw std::runtime_error("Could not set pixel sizes");

    /* Given that we have face->num_glyphs glyphs to load, with each
     * individual glyph having a size of px * py, compute nx and ny so
     * that the resulting atlas texture geometry is closest to a square.
     */
    {
        unsigned n_glyphs = face->num_glyphs;
        unsigned long total_pixels = n_glyphs * px * py;
        double side = sqrt(total_pixels);
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
    }

    atlas_x = nx * px;
    atlas_y = ny * py;
    size_t atlas_bytes = atlas_x * atlas_y;
    std::cout << "Allocating " << atlas_bytes << " bytes for atlas buffer"
              << std::endl;
    auto atlas_buf = std::unique_ptr <uint8_t []> (new uint8_t [atlas_bytes]);

    FT_ULong charcode;
    FT_UInt gindex;
    charcode = FT_Get_First_Char(face, &gindex);
    while (gindex != 0)
    {
       //std::cout << "charcode=" << charcode
       //          << " -> index=" << gindex
       //          << std::endl;
       load_face(face, atlas_buf.get(), charcode);
       charcode = FT_Get_Next_Char(face, charcode, &gindex);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &atlas_texture);
    std::cout << "atlas_texture=" << atlas_texture << std::endl;
    // TODO we could use GL_TEXTURE_RECT for simplicity (texel coords instead of
    // normalized) -- or maybe it's easier to compute the normalized coords in
    // the shader?
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, atlas_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas_x, atlas_y, 0, GL_RED,
                 GL_UNSIGNED_BYTE, atlas_buf.get());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
}

void Font::load_face(FT_Face face, uint8_t* atlas_buf, FT_ULong c)
{
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
       throw std::runtime_error(
          std::string("FreeType: Failed to load glyph for char ") +
          std::to_string(c));
    }

    if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
       throw std::runtime_error(
          std::string("FreeType: Failed to render glyph for char ") +
          std::to_string(c));

    static unsigned seq = 0;
    int atlas_row = seq / nx;
    int atlas_col = seq - nx * atlas_row;

    int atlas_row_offset = nx * px * py;
    int atlas_glyph_offset = atlas_row * atlas_row_offset + atlas_col * px;

    unsigned int bh = face->glyph->bitmap.rows;
    unsigned int bw = face->glyph->bitmap.width;

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
    GLubyte* bmp_src_row;
    GLubyte* atl_dst_row;
    switch (bmp.pixel_mode)
    {
    case FT_PIXEL_MODE_MONO:
        for (unsigned int j = 0; j < bh; ++j) {
            bmp_src_row = bmp.buffer + j * bmp.pitch;
            atl_dst_row = atlas_buf + atlas_glyph_offset + j * nx * px;
            GLubyte byte = 0;
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
            atl_dst_row = atlas_buf + atlas_glyph_offset + j * nx * px;
            for (unsigned int k = 0; k < bw; ++k) {
                *atl_dst_row++ = *bmp_src_row++;
            }
        }
        break;
    default:
       throw std::runtime_error(
           std::string("Unhandled pixel_type=") +
           std::to_string(bmp.pixel_mode));
    }

#if 0
    glyphs[c - 32] = Glyph{static_cast<int>(face->glyph->bitmap.width),
                           static_cast<int>(face->glyph->bitmap.rows),
                           face->glyph->bitmap_left,
                           face->glyph->bitmap_top,
                           face->glyph->advance.x / 64.0f,
                           1.0f * atlas_col * px / atlas_x,
                           1.0f * atlas_row * py / atlas_y,
                           1.0f * (atlas_col * px + bw) / atlas_x,
                           1.0f * (atlas_row * py + bh) / atlas_y};
#endif
    ++seq;
}

} // namespace font
