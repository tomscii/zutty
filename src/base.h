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
#include <iomanip>
#include <iostream>

namespace zutty
{
   struct Color
   {
      uint8_t red;
      uint8_t green;
      uint8_t blue;

      bool operator == (const Color& rhs) const
      {
         return red == rhs.red && green == rhs.green && blue == rhs.blue;
      }
   };

   inline std::ostream&
   operator << (std::ostream& os, const Color& c)
   {
      os << "rgb:" << std::hex << std::setfill('0')
         // N.B.: Output components as 32-bit for compatibility
         << std::setw(2) << (int)c.red << std::setw(2) << (int)c.red << "/"
         << std::setw(2) << (int)c.green << std::setw(2) << (int)c.green << "/"
         << std::setw(2) << (int)c.blue << std::setw(2) << (int)c.blue;
      return os;
   }

   struct Point
   {
      int x = -1;
      int y = -1;

      Point () = default;
      Point (int x_, int y_): x (x_), y (y_) {}

      bool operator < (const Point& rhs) const
      {
         return y < rhs.y || (y == rhs.y && x < rhs.x);
      }

      bool operator == (const Point& rhs) const
      {
         return x == rhs.x && y == rhs.y;
      }

      bool operator <= (const Point& rhs) const
      {
         return operator < (rhs) || operator == (rhs);
      }
   };

   inline std::ostream&
   operator << (std::ostream& os, const Point& p)
   {
      os << "(" << p.x << "," << p.y << ")";

      return os;
   }

   struct Rect
   {
      Point tl; // top left corner
      Point br; // bottom right corner
      bool rectangular = false;

      Rect () = default;
      Rect (Point tl_, Point br_):
         tl (tl_), br (br_) {}
      Rect (int x, int y):
         tl (x, y), br (x + 1, y) {}
      Rect (int x1, int y1, int x2, int y2):
         tl (x1, y1), br (x2, y2) {}

      bool null () const
      {
         return tl == Point () && br == Point ();
      }

      bool empty () const
      {
         return tl == br;
      }

      Point mid () const
      {
         return Point ((tl.x + br.x) / 2, (tl.y + br.y) / 2);
      }

      void clear ()
      {
         tl = Point ();
         br = Point ();
      }

      void toggleRectangular ()
      {
         rectangular = !rectangular;
      }
   };

   inline std::ostream&
   operator << (std::ostream& os, const Rect& r)
   {
      os << "Rect{tl=" << r.tl << " " << "br=" << r.br;
      if (r.rectangular)
         os << " rectangular}";
      else
         os << " regular}";

      return os;
   }

} // namespace zutty
