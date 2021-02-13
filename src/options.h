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

#include "base.h"

#include <X11/Xresource.h>

#include <cstdint>
#include <string>
#include <vector>

namespace zutty {

   struct OptionDesc
   {
      const char* option;
      XrmOptionKind parseType;
      const char* implValue;
      const char* hardDefault;
      const char* helpDescr;
   };

#if defined(FREEBSD)
   static constexpr const char* fontpath = "/usr/local/share/fonts";
#elif defined(OPENBSD)
   static constexpr const char* fontpath = "/usr/X11R6/lib/X11/fonts";
#else
   static constexpr const char* fontpath = "/usr/share/fonts";
#endif

   static const std::vector <OptionDesc> optionsTable = {
      {"altScroll", XrmoptionNoArg,    "true",  "false",     "Alternate scroll mode"},
      {"autoCopy",  XrmoptionNoArg,    "true",  "false",     "Sync primary to clipboard"},
      {"bg",        XrmoptionSepArg,   nullptr, "000000",    "Background color"},
      {"border",    XrmoptionSepArg,   nullptr, "2",         "Border width in pixels"},
      {"display",   XrmoptionSepArg,   nullptr, nullptr,     "Display to connect to"},
      {"fg",        XrmoptionSepArg,   nullptr, "ffffff",    "Foreground color"},
      {"font",      XrmoptionSepArg,   nullptr, "9x18",      "Font to use"},
      {"fontsize",  XrmoptionSepArg,   nullptr, "16",        "Font size"},
      {"fontpath",  XrmoptionSepArg,   nullptr, fontpath,    "Font search path"},
      {"geometry",  XrmoptionSepArg,   nullptr, "80x24",     "Terminal size in chars"},
      {"glinfo",    XrmoptionNoArg,    "true",  "false",     "Print OpenGL information"},
      {"help",      XrmoptionNoArg,    "true",  "false",     "Print usage information"},
      {"rv",        XrmoptionNoArg,    "true",  "false",     "Reverse video"},
      {"shell",     XrmoptionSepArg,   nullptr, nullptr,     "Shell program to run"},
      {"title",     XrmoptionSepArg,   nullptr, "Zutty",     "Window title"},
      {"quiet",     XrmoptionNoArg,    "true",  "false",     "Silence logging output"},
      {"verbose",   XrmoptionNoArg,    "true",  "false",     "Output info messages"},
      {"e",         XrmoptionSkipLine, nullptr, nullptr,     "Command line to run"},
   };

   struct Options
   {
      // N.B.: no static initializers - will decode hardDefault fields above!
      uint16_t border;
      const char* display;
      const char* fontname;
      const char* fontpath;
      uint8_t fontsize;
      uint16_t nCols;
      uint16_t nRows;
      bool glinfo;
      const char* shell;
      const char* title;
      Color fg;
      Color bg;
      bool rv;
      bool altScrollMode;
      bool autoCopyMode;
      bool quiet;
      bool verbose;

      void initialize (int* argc, char** argv);
      void setDisplay (Display* dpy);
      void parse ();

      void printVersion () const;
      void printUsage () const;
   };

} // namespace zutty

extern zutty::Options opts;
