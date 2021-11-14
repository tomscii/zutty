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

#include <cstdint>
#include <functional>

namespace zutty
{
   // The glyph to display instead of valid codes with no available glyph
   constexpr const uint16_t Missing_Glyph_Marker = 0x0000;
   // The "question mark" to display in place of invalid/unsupported unicode
   constexpr const uint16_t Unicode_Replacement_Character = 0xfffd;

   struct Utf8Encoder
   {
      template <typename Fn>
      static void
      pushUnicode (uint32_t cp, Fn&& byteSink)
      {
         if (cp < 0x80)
         {
            byteSink (cp);
         }
         else if (cp < 0x0800)
         {
            byteSink ((cp >> 6) | 0xc0);
            byteSink ((cp & 0x3f) | 0x80);
         }
         else if (cp < 0x10000)
         {
            byteSink ((cp >> 12) | 0xe0);
            byteSink (((cp >> 6) & 0x3f) | 0x80);
            byteSink ((cp & 0x3f) | 0x80);
         }
         else
         {
            byteSink ((cp >> 18) | 0xf0);
            byteSink (((cp >> 12) & 0x3f) | 0x80);
            byteSink (((cp >> 6) & 0x3f) | 0x80);
            byteSink ((cp & 0x3f) | 0x80);
         }
      }
   };

   class Utf8Decoder
   {
   public:
      using CodepointSink = std::function <void ()>;

      Utf8Decoder (CodepointSink&& fn): cpSink (fn) {}

      void checkPrematureEOS ()
      {
         if (remaining > 0)
         {
            remaining = 0;
            valid = false;
            unicode = Unicode_Replacement_Character;
            cpSink ();
         }
      }

      uint32_t getUnicode () const
      {
         return unicode;
      }

      void setUnicode (uint32_t cp)
      {
         unicode = cp;
      }

      void onUnicode (uint32_t ch)
      {
         if (!ch)
            return;

         unicode = ch;
         valid = true;
         cpSink ();
      }

      void pushByte (unsigned char ch)
      {
         if ((ch >> 6) == 0x2) // 10xx'xxxx
         {
            if (remaining > 0)
            {
               if (1 < remaining && remaining < 3 &&
                   (!ch || (unicode == 0 && ch < 0xa0)))
                  valid = false; // reject overlong encodings
               unicode <<= 6;
               unicode += ch & 0x3f;
               --remaining;
            }
            else
               valid = false; // unexpected UTF-8 continuation byte

            if (remaining == 0)
            {
               if (!valid)
                  unicode = Unicode_Replacement_Character;
               cpSink ();
            }
         }
         else if ((ch >> 5) == 0x6) // 110x'xxxx
         {
            checkPrematureEOS ();
            unicode = ch & 0x1f;
            remaining = 1;
            valid = unicode > 1; // reject overlong encodings
         }
         else if ((ch >> 4) == 0xe) // 1110'xxxx
         {
            checkPrematureEOS ();
            unicode = ch & 0x0f;
            remaining = 2;
            valid = true;
         }
         else if ((ch >> 3) == 0x1e) // 1111'0xxx
         {
            checkPrematureEOS ();
            unicode = ch & 0x07;
            remaining = 3;
            valid = true;
         }
         else if ((ch >> 2) == 0x3e) // 1111'10xx
         {
            checkPrematureEOS ();
            unicode = ch & 0x03;
            remaining = 4;
            valid = false; // not supported
         }
         else if ((ch >> 1) == 0x7e) // 1111'110x
         {
            checkPrematureEOS ();
            unicode = ch & 0x01;
            remaining = 5;
            valid = false; // not supported
         }
         else if (ch == 0xfe || ch == 0xff)
         {
            // illegal UTF-8 characters
            unicode = Unicode_Replacement_Character;
            remaining = 0;
            valid = false;
            cpSink ();
         }
      }

   private:
      uint32_t unicode = 0;
      bool valid = false;
      uint8_t remaining = 0;
      CodepointSink cpSink;
   };

} // namespace zutty
