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
#include <string>
#include <vector>

// Inspired by: https://stackoverflow.com/a/34571089

namespace
{
   static constexpr const char* syms =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

   static std::vector <int> rtab =
      [] {
            std::vector <int> rt (256, -1);
            for (int i = 0; i < 64; i++)
               rt [syms [i]] = i;
            return rt;
         } ();
}

namespace zutty
{
namespace base64
{

   static std::string
   encode (const std::string& in)
   {
      std::string out;
      out.reserve (in.size () * 4 / 3 + 3);

      int val = 0;
      int valb = -6;
      for (unsigned char c: in) {
         val = (val << 8) + c;
         valb += 8;
         while (valb >= 0) {
            out.push_back (syms [(val >> valb) & 0x3F]);
            valb -= 6;
         }
      }
      if (valb > -6)
         out.push_back (syms [((val << 8) >> (valb + 8)) & 0x3F]);
      while (out.size () % 4)
         out.push_back ('=');
      return out;
   }

   static std::string
   decode (const std::string& in)
   {
      std::string out;
      out.reserve (in.size () * 3 / 4);

      int val = 0;
      int valb = -8;
      for (unsigned char c: in) {
         if (rtab [c] == -1)
            break;
         val = (val << 6) + rtab [c];
         valb += 6;
         if (valb >= 0) {
            out.push_back ((val >> valb) & 0xFF);
            valb -= 8;
         }
      }
      return out;
   }

} // namespace base64
} // namespace zutty
