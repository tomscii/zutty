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

namespace zutty
{
   struct OptionDesc
   {
      const char* option;
      XrmOptionKind parseType;
      const char* implValue;
      const char* hardDefault;
      const char* helpDescr;
   };

   struct ResourceDesc
   {
      const char* resource;
      const char* hardDefault;
      const char* helpDescr;
   };

#if defined(FREEBSD)
   static constexpr const char* fontpath = "/usr/local/share/fonts";
#elif defined(NETBSD)
   static constexpr const char* fontpath = "/usr/X11R7/lib/X11/fonts";
#elif defined(OPENBSD)
   static constexpr const char* fontpath = "/usr/X11R6/lib/X11/fonts";
#else
   static constexpr const char* fontpath = "/usr/share/fonts";
#endif

#define NoArg  XrmoptionNoArg
#define SepArg XrmoptionSepArg
#define SkipLn XrmoptionSkipLine
   static const std::vector <OptionDesc> optionsTable = {
      // option       parseType implValue hardDefault helpDescr
      {"altScroll",   NoArg,    "true",    "false",   "Alternate scroll mode"},
      {"autoCopy",    NoArg,    "true",    "false",   "Sync primary to clipboard"},
      {"bg",          SepArg,   nullptr,   "#000",    "Background color"},
      {"boldColors",  NoArg,    "true",    "true",    "Enable bright for bold"},
      {"border",      SepArg,   nullptr,   "2",       "Border width in pixels"},
      {"cr",          SepArg,   nullptr,   nullptr,   "Cursor color"},
      {"display",     SepArg,   nullptr,   nullptr,   "Display to connect to"},
      {"dwfont",      SepArg,   nullptr,   "18x18ja", "Double-width font to use"},
      {"fg",          SepArg,   nullptr,   "#fff",    "Foreground color"},
      {"font",        SepArg,   nullptr,   "9x18",    "Font to use"},
      {"fontsize",    SepArg,   nullptr,   "16",      "Font size"},
      {"fontpath",    SepArg,   nullptr,   fontpath,  "Font search path"},
      {"geometry",    SepArg,   nullptr,   "80x24",   "Terminal size in chars"},
      {"glinfo",      NoArg,    "true",    "false",   "Print OpenGL information"},
      {"help",        NoArg,    "true",    "false",   "Print usage listing and quit"},
      {"listres",     NoArg,    "true",    "false",   "Print resource listing and quit"},
      {"login",       NoArg,    "true",    "false",   "Start shell as a login shell"},
      {"name",        SepArg,   nullptr,   nullptr,   "Instance name for Xrdb and WM_CLASS"},
      {"rv",          NoArg,    "true",    "false",   "Reverse video"},
      {"saveLines",   SepArg,   nullptr,   "500",     "Lines of scrollback history"},
      {"shell",       SepArg,   nullptr,   nullptr,   "Shell program to run"},
      {"showWraps",   NoArg,    "true",    "false",   "Show wrap marks at right margin"},
      {"title",       SepArg,   nullptr,   "Zutty",   "Window title"},
      {"quiet",       NoArg,    "true",    "false",   "Silence logging output"},
      {"verbose",     NoArg,    "true",    "false",   "Output info messages"},
      {"e",           SkipLn,   nullptr,   nullptr,   "Command line to run"},
   };
#undef NoArg
#undef SepArg
#undef SkipL

   static const std::vector <ResourceDesc> resourceTable = {
      // resource           hardDefault    helpDescr
      {"altSendsEscape",    "true",        "Encode Alt key as ESC prefix"},
      {"modifyOtherKeys",   "1",           "Key modifier encoding level; 0..2"},
      {"color0",            "#000000",     "Palette color 0"},
      {"color1",            "#cd0000",     "Palette color 1"},
      {"color2",            "#00cd00",     "Palette color 2"},
      {"color3",            "#cdcd00",     "Palette color 3"},
      {"color4",            "#0000ee",     "Palette color 4"},
      {"color5",            "#cd00cd",     "Palette color 5"},
      {"color6",            "#00cdcd",     "Palette color 6"},
      {"color7",            "#e5e5e5",     "Palette color 7"},
      {"color8",            "#7f7f7f",     "Palette color 8"},
      {"color9",            "#ff0000",     "Palette color 9"},
      {"color10",           "#00ff00",     "Palette color 10"},
      {"color11",           "#ffff00",     "Palette color 11"},
      {"color12",           "#5c5cff",     "Palette color 12"},
      {"color13",           "#ff00ff",     "Palette color 13"},
      {"color14",           "#00ffff",     "Palette color 14"},
      {"color15",           "#ffffff",     "Palette color 15"},
   };

   enum class OptionSource
   {  // in order of increasing precedence:
      NONE, HardDefault, ResourceCfg, CmdLine
   };

   struct Options
   {
      // N.B.: no static initializers - will decode hardDefault fields above!
      uint8_t fontsize;
      uint8_t modifyOtherKeys;
      uint16_t border;
      uint16_t nCols;
      uint16_t nRows;
      uint16_t saveLines;
      const char* display;
      const char* dwfontname;
      const char* fontname;
      const char* fontpath;
      const char* name;
      const char* shell;
      const char* title;
      OptionSource titleSource = OptionSource::NONE;
      Color bg;
      Color cr;
      Color fg;
      bool altScrollMode;
      bool altSendsEscape;
      bool autoCopyMode;
      bool boldColors;
      bool glinfo;
      bool login;
      bool showWraps;
      bool quiet;
      bool rv;
      bool verbose;

      void initialize (int* argc, char** argv);
      void setDisplay (Display* dpy);
      void handlePrintOpts ();
      void parse ();

      void printVersion () const;
      void printUsage () const;
      void printResources () const;

      // getters for resources not automatically parsed by parse ()
      bool getBool (const char* name, bool defaultValue = false);
      void getColor (const char* name, zutty::Color& outColor);
      int getInteger (const char* name, int min, int max);
   };

} // namespace zutty

extern zutty::Options opts;
