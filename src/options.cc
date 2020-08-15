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

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

   Display* dpy = nullptr;

   XrmDatabase xrmOptionsDb = nullptr;

   // Used as storage for strings that will be freed on exit
   std::vector <std::string> strRefs;

   using zutty::options::optionsTable;
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

} // namespace

namespace zutty::options {

   void
   initialize (int* argc, char** argv)
   {
      XrmInitialize ();
      XrmParseCommand (&xrmOptionsDb,
                       xrmOptionsTable.data (), xrmOptionsTable.size (),
                       "zutty", argc, argv);
   }

   void
   setDisplay (Display* dpy_)
   {
      dpy = dpy_;
   }

   void
   printUsage ()
   {
      std::cout << "zutty [-option ...] [shell]\n\n"
                << "options:\n";
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

   const char*
   get (const char* name, const char* fallback)
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
            if (strcmp (e.option, name) == 0)
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
   convSelectionTarget (Atom& outSelectionTarget)
   {
      const char* opt = get ("selection");
      if (!opt)
         throw std::runtime_error ("-selection: missing value");

      switch (opt [0])
      {
      case 'p': outSelectionTarget = XA_PRIMARY; return;
      case 's': outSelectionTarget = XA_SECONDARY; return;
      case 'c': outSelectionTarget = XA_CLIPBOARD (dpy); return;
      default:
         throw std::runtime_error ("-selection: expected one of: "
                                   "primary, secondary, clipboard");
      }
   }

} // namespace zutty::options
