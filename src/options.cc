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

#include "options.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>

#include <stdlib.h>

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
   using namespace zutty;

   Display* dpy = nullptr;

   XrmDatabase xrmOptionsDb = nullptr;

   // Used as storage for strings that will be freed on exit
   std::vector <std::string> strRefs;

   std::vector <XrmOptionDescRec> xrmOptionsTable =
      [] {
         // prevent realloc that would invalidate ptrs
         strRefs.reserve (4 * optionsTable.size ());
         std::vector <XrmOptionDescRec> rv;
         for (const auto& e: optionsTable)
         {
            XrmOptionDescRec rec;
            strRefs.push_back (std::string ("-") + e.option);
            rec.option = (char *)strRefs.back ().c_str ();
            strRefs.push_back (std::string (".") + e.option);
            rec.specifier = (char *)strRefs.back ().c_str ();
            rec.argKind = e.parseType;
            rec.value = nullptr;
            if (e.implValue)
            {
               strRefs.push_back (std::string (e.implValue));
               rec.value = (XPointer)strRefs.back ().c_str ();
            }
            rv.push_back (rec);

            // For boolean options, generate sibling option to negate value
            // (activated via +option). Reuse already populated rec.
            if (e.parseType == XrmoptionNoArg)
            {
               if (strcmp (e.implValue, "true") == 0)
               {
                  strRefs.push_back (std::string ("+") + e.option);
                  rec.option = (char *)strRefs.back ().c_str ();
                  rec.value = (XPointer)"false";
                  rv.push_back (rec);
               }
               else if (strcmp (e.implValue, "false") == 0)
               {
                  strRefs.push_back (std::string ("+") + e.option);
                  rec.option = (char *)strRefs.back ().c_str ();
                  rec.value = (XPointer)"true";
                  rv.push_back (rec);
               }
            }
         }
         return rv;
      } ();

   const char*
   get (const char* name, const char* fallback = nullptr,
        OptionSource* src = nullptr)
   {
      XrmValue xrmValue;
      char* xrmType;
      char buf [80] = "zutty.";
      strncat (buf, name, sizeof (buf) - 1);

      auto withSource = [=] (const OptionSource s, const char* rv)
      {
         if (src)
            *src = s;
         return rv;
      };

      if (XrmGetResource (xrmOptionsDb, buf, opts.name, &xrmType, &xrmValue))
         return withSource (OptionSource::CmdLine, xrmValue.addr);
      const char* xDefault = dpy ? XGetDefault (dpy, opts.name, name) : nullptr;
      if (xDefault)
         return withSource (OptionSource::ResourceCfg, xDefault);
      else
      {
         for (const auto& e: optionsTable)
            if (strcmp (e.option, name) == 0 && e.hardDefault)
               return withSource (OptionSource::HardDefault, e.hardDefault);
         for (const auto& r: resourceTable)
            if (strcmp (r.resource, name) == 0 && r.hardDefault)
               return withSource (OptionSource::HardDefault, r.hardDefault);
      }
      return withSource (OptionSource::NONE, fallback);
   }

   void
   getBorder (uint16_t& outBorder)
   {
      const char* opt = get ("border");
      if (!opt)
         throw std::runtime_error ("-border: missing value");

      std::stringstream iss (opt);
      int bw;
      iss >> bw;
      if (iss.fail () || bw < 0 || bw > 3000)
         throw std::runtime_error ("-border: expected unsigned, max. 3000");
      outBorder = bw;
   }

   void
   getSaveLines (uint16_t& outSaveLines)
   {
      const char* opt = get ("saveLines");
      if (!opt)
         throw std::runtime_error ("-saveLines: missing value");

      std::stringstream iss (opt);
      int sl;
      iss >> sl;
      if (iss.fail () || sl < 0 || sl > 50000)
         throw std::runtime_error ("-saveLines: expected unsigned, max. 50000");
      outSaveLines = sl;
   }

   void
   getFontsize (uint8_t& outFontsize)
   {
      const char* opt = get ("fontsize");
      if (!opt)
         throw std::runtime_error ("-fontsize: missing value");

      std::stringstream iss (opt);
      int fs;
      iss >> fs;
      if (iss.fail () || fs < 1 || fs > 255)
         throw std::runtime_error ("-fontsize: expected integer within 1..255");
      outFontsize = fs;
   }

   void
   getGeometry (uint16_t& outCols, uint16_t& outRows)
   {
      const char* opt = get ("geometry");
      if (!opt)
         throw std::runtime_error ("-geometry: missing value");

      std::stringstream iss (opt);
      int cols, rows;
      char fill;
      iss >> cols >> fill >> rows;
      if (iss.fail () || fill != 'x' || cols < 1 || rows < 1)
         throw std::runtime_error ("-geometry: expected format <COLS>x<ROWS>");
      outCols = cols;
      outRows = rows;
   }

   uint8_t
   convHexDigit (const char* name, const char ch)
   {
      if (ch >= '0' && ch <= '9')
         return ch - '0';

      if (ch >= 'a' && ch <= 'f')
         return ch - 'a' + 10;

      if (ch >= 'A' && ch <= 'F')
         return ch - 'A' + 10;

      throw std::runtime_error (std::string ("-") + name +
                                ": illegal hex digit '" + ch +
                                "'; expected hex RGB color");
   }

   void
   convColor (const char* name, const char* opt, zutty::Color& outColor)
   {
      const char* val = (opt [0] == '#') ? opt + 1 : opt;
      switch (strlen (val))
      {
      case 3:
         // N.B.: 17 == (1 << 4) + 1
         outColor.red = 17 * convHexDigit (name, val [0]);
         outColor.green =  17 * convHexDigit (name, val [1]);
         outColor.blue = 17 * convHexDigit (name, val [2]);
         break;
      case 6:
         outColor.red =
            (convHexDigit (name, val [0]) << 4) + convHexDigit (name, val [1]);
         outColor.green =
            (convHexDigit (name, val [2]) << 4) + convHexDigit (name, val [3]);
         outColor.blue =
            (convHexDigit (name, val [4]) << 4) + convHexDigit (name, val [5]);
         break;
      default:
         throw std::runtime_error (std::string ("-") + name +
                                   ": expected hex RGB color");
      }
   }

} // namespace

zutty::Options opts;

namespace zutty
{
   void
   Options::initialize (int* argc, char** argv)
   {
      XrmInitialize ();
      XrmParseCommand (&xrmOptionsDb,
                       xrmOptionsTable.data (), xrmOptionsTable.size (),
                       "zutty", argc, argv);

      display = get ("display", getenv ("DISPLAY"));
      if (display)
         setenv ("DISPLAY", display, 1);

      name = get ("name", getenv ("RESOURCE_NAME"));
      if (name && (strchr (name, '.') || strchr (name, '*')))
         throw std::runtime_error ("-name: supplied value contains "
                                   "illegal characters");
      if (!name)
         name = "Zutty";
   }

   void
   Options::setDisplay (Display* dpy_)
   {
      dpy = dpy_;
   }

   bool
   Options::getBool (const char* name, bool defaultValue)
   {
      const char* opt = get (name);
      if (!opt)
         return defaultValue;
      return strcmp (opt, "true") == 0;
   }

   void
   Options::getColor (const char* name, zutty::Color& outColor)
   {
      const char* opt = get (name);
      if (!opt)
         throw std::runtime_error (std::string ("-") + name +
                                   ": missing value");
      convColor (name, opt, outColor);
   }

   int
   Options::getInteger (const char* name, int min, int max)
   {
      const char* opt = get (name);
      if (!opt)
         return min;

      std::stringstream iss (opt);
      int ret;
      iss >> ret;
      if (iss.fail ())
         return min;

      return std::min (std::max (min, ret), max);
   }

   void
   Options::handlePrintOpts ()
   {
      if (getBool ("help"))
      {
         printUsage ();
         exit (0);
      }

      if (getBool ("listres"))
      {
         printResources ();
         exit (0);
      }
   }

   void
   Options::parse ()
   {
      handlePrintOpts ();
      try
      {
         getBorder (border);
         getSaveLines (saveLines);
         dwfontname = get ("dwfont");
         fontname = get ("font");
         fontpath = get ("fontpath");
         getFontsize (fontsize);
         getGeometry (nCols, nRows);
         glinfo = getBool ("glinfo");
         shell = get ("shell", getenv ("SHELL"));
         if (!shell)
            shell = "bash";
         title = get ("title", nullptr, &titleSource);
         getColor ("fg", fg);
         getColor ("bg", bg);
         rv = getBool ("rv");
         if (rv)
            std::swap (fg, bg);
         if (get ("cr"))
            getColor ("cr", cr);
         else
            cr = fg;
         altScrollMode = getBool ("altScroll");
         altSendsEscape = getBool ("altSendsEscape");
         autoCopyMode = getBool ("autoCopy");
         boldColors = getBool ("boldColors");
         login = getBool ("login");
         showWraps = getBool ("showWraps");
         quiet = getBool ("quiet");
         verbose = getBool ("verbose");
         modifyOtherKeys = getInteger ("modifyOtherKeys", 0, 2);
      }
      catch (const std::exception& e)
      {
         std::cout << "Error: " << e.what () << "!\n"
                   << "Try -help for usage options." << std::endl;
         exit (-1);
      }
   }

   void
   Options::printVersion () const
   {
      std::cout << "Zutty " ZUTTY_VERSION "\n"
                << "Copyright (C) 2020 Tom Szilagyi\n\n"
                << "This program comes with ABSOLUTELY NO WARRANTY.\n"
                << "Zutty is free software, and you are welcome to redistribute it\n"
                << "under the terms and conditions of the GNU GPL v3 (or later).\n"
                << std::endl;
   }

   void
   Options::printUsage () const
   {
      printVersion ();
      std::cout << "Usage:\n"
                << "  zutty [-option ...] [shell]\n\n"
                << "Options:\n";
      size_t maxw = 0;
      for (const auto& e: optionsTable)
         maxw = std::max (maxw, strlen (e.option));
      for (const auto& e: optionsTable)
      {
         std::cout << "  -" << std::left << std::setw (maxw + 3) << e.option;
         std::cout << e.helpDescr;
         const char* xDefault = dpy
                              ? XGetDefault (dpy, opts.name, e.option)
                              : nullptr;
         if (xDefault)
            std::cout << " (configured: " << xDefault << ")";
         else if (e.hardDefault && e.parseType != XrmoptionNoArg)
            std::cout << " (default: " << e.hardDefault << ")";
         std::cout << "\n";
      }
      std::cout << std::endl;
   }

   void
   Options::printResources () const
   {
      printVersion ();
      std::cout << "Resources:\n";
      size_t maxw = 0;
      for (const auto& r: resourceTable)
         maxw = std::max (maxw, strlen (r.resource));
      for (const auto& r: resourceTable)
      {
         std::cout << "  " << std::left << std::setw (maxw + 3) << r.resource;
         std::cout << r.helpDescr;
         const char* xDefault = dpy
                              ? XGetDefault (dpy, opts.name, r.resource)
                              : nullptr;
         if (xDefault)
            std::cout << " (configured: " << xDefault << ")";
         else if (r.hardDefault)
            std::cout << " (default: " << r.hardDefault << ")";
         std::cout << "\n";
      }
      std::cout << std::endl;
   }

} // namespace zutty
