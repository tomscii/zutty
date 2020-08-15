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

#include <X11/Xresource.h>

#include <cstdint>
#include <string>
#include <vector>

namespace zutty::options {

struct OptionDesc
{
   const char* option;
   XrmOptionKind parseType;
   const char* implValue;
   const char* hardDefault;
   const char* helpDescr;
};

static const std::vector <OptionDesc> optionsTable = {
   {"border",   XrmoptionSepArg,   nullptr, "2",         "Border width in pixels"},
   {"display",  XrmoptionSepArg,   nullptr, nullptr,     "Display to connect to"},
   {"font",     XrmoptionSepArg,   nullptr, "9x18",      "Font to use"},
   {"geometry", XrmoptionSepArg,   nullptr, "80x24",     "Terminal size in chars"},
   {"glinfo",   XrmoptionNoArg,    "true",  "false",     "Print OpenGL information"},
   {"help",     XrmoptionNoArg,    "true",  "false",     "Print usage information"},
   {"shell",    XrmoptionSepArg,   nullptr, "/bin/bash", "Shell program to run"},
   {"title",    XrmoptionSepArg,   nullptr, "Zutty",     "Window title"},
   {"selection",XrmoptionSepArg,   nullptr, "primary",   "Selection target"},
   {"e",        XrmoptionSkipLine, nullptr, nullptr,     "Command line to run"},
};

void initialize (int* argc, char** argv);
void setDisplay (Display* dpy);

void printUsage ();

const char* get (const char* name, const char* fallback = nullptr);
bool getBool (const char* name);

// Convert/validate functions for bespoke options:
void convBorder (uint16_t& outBorder);
void convGeometry (uint16_t& outCols, uint16_t& outRows);
void convSelectionTarget (Atom& outTarget);

} // namespace zutty::options
