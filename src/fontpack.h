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

#include "font.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace zutty
{
   class Fontpack
   {
   public:
      /* Initialize a Fontpack by locating and loading fonts under fontpath.
       * Four styles are looked for: Regular, Bold, Italic and Bold Italic;
       * all but the first are optional. If not even a regular variant of
       * the requested font can be loaded, an exception is thrown.
       * Additionally, a double-width font with the given name is optionally
       * located and initialized.
       */
      Fontpack (const std::string& fontpath,
                const std::string& fontname,
                const std::string& dwfontname);

      ~Fontpack () = default;

      uint16_t getPx () const { return px; };
      uint16_t getPy () const { return py; };

      const Font& getRegular () const {
         return * fontRegular.get ();
      };

      bool hasBold () const { return fontBold.get () != nullptr; }

      const Font& getBold () const {
         if (! hasBold ())
            throw std::runtime_error ("No Bold font variant present!");
         return * fontBold.get ();
      };

      bool hasItalic () const { return fontItalic.get () != nullptr; }

      const Font& getItalic () const {
         if (! hasItalic ())
            throw std::runtime_error ("No Italic font variant present!");
         return * fontItalic.get ();
      };

      bool hasBoldItalic () const { return fontBoldItalic.get () != nullptr; }

      const Font& getBoldItalic () const {
         if (! hasBoldItalic ())
            throw std::runtime_error ("No BoldItalic font variant present!");
         return * fontBoldItalic.get ();
      };

      bool hasDoubleWidth () const { return fontDoubleWidth.get () != nullptr; }

      const Font& getDoubleWidth () const {
         if (! hasDoubleWidth ())
            throw std::runtime_error ("No DoubleWidth font present!");
         return * fontDoubleWidth.get ();
      };

      void releaseFonts ()
      {
         fontRegular = nullptr;
         fontBold = nullptr;
         fontItalic = nullptr;
         fontBoldItalic = nullptr;
         fontDoubleWidth = nullptr;
      }

   private:
      uint16_t px = 0; // glyph width in pixels
      uint16_t py = 0; // glyph height in pixels
      std::unique_ptr <Font> fontRegular = nullptr;
      std::unique_ptr <Font> fontBold = nullptr;
      std::unique_ptr <Font> fontItalic = nullptr;
      std::unique_ptr <Font> fontBoldItalic = nullptr;
      std::unique_ptr <Font> fontDoubleWidth = nullptr;
   };

} // namespace zutty
