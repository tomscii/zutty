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

namespace {

   using namespace zutty;

   Display* dpy = nullptr;

   XrmDatabase xrmOptionsDb = nullptr;

   // Used as storage for strings that will be freed on exit
   std::vector <std::string> strRefs;

   std::vector <XrmOptionDescRec> xrmOptionsTable =
      [] {
         // prevent realloc that would invalidate ptrs
         strRefs.reserve (3 * optionsTable.size ());
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
         }
         return rv;
      } ();

   const char*
   get (const char* name, const char* fallback = nullptr)
   {
      XrmValue xrmValue;
      char* xrmType;
      char buf [80] = "zutty.";
      strncat (buf, name, sizeof (buf) - 1);

      if (XrmGetResource (xrmOptionsDb, buf, "Zutty", &xrmType, &xrmValue))
         return xrmValue.addr;
      const char* xDefault = dpy ? XGetDefault (dpy, "Zutty", name) : nullptr;
      if (xDefault)
         return xDefault;
      else
         for (const auto& e: optionsTable)
            if (strcmp (e.option, name) == 0 && e.hardDefault)
               return e.hardDefault;
      return fallback;
   }

   bool
   getBool (const char* name)
   {
      const char* opt = get (name);
      if (!opt)
         return false;
      return strcmp (opt, "true") == 0;
   }

   void
   convBorder (uint16_t& outBorder)
   {
      const char* opt = get ("border");
      if (!opt)
         throw std::runtime_error ("-border: missing value");

      std::stringstream iss (opt);
      int bw;
      iss >> bw;
      if (iss.fail () || bw < 0 || bw > 32767)
         throw std::runtime_error ("-border: expected unsigned short");
      outBorder = bw;
   }

   void
   convGeometry (uint16_t& outCols, uint16_t& outRows)
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

   void
   convSelection (Atom& outSelection)
   {
      const char* opt = get ("selection");
      if (!opt)
         throw std::runtime_error ("-selection: missing value");

      switch (opt [0])
      {
      case 'p': outSelection = XA_PRIMARY; return;
      case 's': outSelection = XA_SECONDARY; return;
      case 'c': outSelection = XA_CLIPBOARD (dpy); return;
      default:
         throw std::runtime_error ("-selection: expected one of: "
                                   "primary, secondary, clipboard");
      }
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
                                "'; expected six-digit hex RGB color");
   }

   void
   convColor (const char* name, zutty::Color& outColor)
   {
      const char* opt = get (name);
      if (!opt)
         throw std::runtime_error (std::string ("-") + name +
                                   ": missing value");

      if (strlen (opt) != 6)
         throw std::runtime_error (std::string ("-") + name +
                                   ": expected six-digit hex RGB color");

      outColor.red =
         (convHexDigit (name, opt [0]) << 4) + convHexDigit (name, opt [1]);
      outColor.green =
         (convHexDigit (name, opt [2]) << 4) + convHexDigit (name, opt [3]);
      outColor.blue =
         (convHexDigit (name, opt [4]) << 4) + convHexDigit (name, opt [5]);
   }

} // namespace

zutty::Options opts;

namespace zutty {

   void
   Options::initialize (int* argc, char** argv)
   {
      XrmInitialize ();
      XrmParseCommand (&xrmOptionsDb,
                       xrmOptionsTable.data (), xrmOptionsTable.size (),
                       "zutty", argc, argv);
      display = get ("display", getenv ("DISPLAY"));
      setenv ("DISPLAY", display, 1);
   }

   void
   Options::setDisplay (Display* dpy_)
   {
      dpy = dpy_;
   }

   void
   Options::parse ()
   {
      if (getBool ("help"))
      {
         printUsage ();
         exit (0);
      }

      try
      {
         convBorder (border);
         fontname = get ("font");
         convGeometry (nCols, nRows);
         glinfo = getBool ("glinfo");
         shell = get ("shell");
         title = get ("title");
         convSelection (selection);
         convColor ("fg", fg);
         convColor ("bg", bg);
         rv = getBool ("rv");
         if (rv)
            std::swap (fg, bg);
         altScrollMode = getBool ("altScroll");
         quiet = getBool ("quiet");
         verbose = getBool ("verbose");
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
                              ? XGetDefault (dpy, "Zutty", e.option)
                              : nullptr;
         if (xDefault)
            std::cout << " (configured: " << xDefault << ")";
         else if (e.hardDefault && e.parseType != XrmoptionNoArg)
            std::cout << " (default: " << e.hardDefault << ")";
         std::cout << "\n";
      }
      std::cout << std::endl;
   }

} // namespace zutty
