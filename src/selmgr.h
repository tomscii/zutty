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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace zutty {

   class SelectionManager
   {
   public:
      SelectionManager (Display*, Window, Atom);

      using PasteCallbackFn = std::function <void (const std::string&)>;
      void getSelection (Time, PasteCallbackFn&&);
      bool setSelection (Time, const std::string&);

      void onPropertyNotify (XPropertyEvent& event);
      void onSelectionClear (XSelectionClearEvent& event);
      void onSelectionNotify (XSelectionEvent& event);
      void onSelectionRequest (XSelectionRequestEvent& event);

   private:
      Display* dpy;
      Window win;

      const Atom incr;
      const Atom targets;
      const Atom prop;
      const size_t chunkSize;

      const Atom selection;
      const Atom target;

      bool selOwned = false;
      std::string content;
      std::vector <unsigned char> incoming;
      PasteCallbackFn pasteCallback;

      enum class State: uint8_t
      {
         Idle,
         WaitingForIncrAck,
         WaitingForSelNotify,
         ReadingIncr
      };
      State state = State::Idle;

      size_t cliPos;
      Window cliWin;
      Atom cliProp;
   };

} // namespace zutty
