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
#include "selmgr.h"

#include <X11/Xmu/Atoms.h>

#include <iostream>

namespace zutty {

   SelectionManager::SelectionManager (Display* dpy_, Window win_)
      : dpy (dpy_)
      , win (win_)
      , incr (XInternAtom (dpy, "INCR", False))
      , targets (XInternAtom (dpy, "TARGETS", False))
      , prop (XInternAtom(dpy, "_ZUTTY_SELECTION", False))
      , chunkSize (XExtendedMaxRequestSize (dpy)
                   ? XExtendedMaxRequestSize (dpy) >> 2
                   : XMaxRequestSize (dpy) >> 2)
      , selection (opts.selection)
      , target (XA_UTF8_STRING (dpy))
   {
      std::cout << "SelectionManager: chunkSize=" << chunkSize << std::endl;
   }

   void
   SelectionManager::getSelection (Time time, PasteCallbackFn&& cb)
   {
      if (selOwned)
      {
         cb (content);
         return;
      }

      incoming.clear ();
      pasteCallback = cb;
      XConvertSelection (dpy, selection, target, prop, win, time);
      state = State::WaitingForSelNotify;
   }

   bool
   SelectionManager::setSelection (Time time, const std::string& content_)
   {
      XSetSelectionOwner(dpy, selection, win, time);
      if (XGetSelectionOwner (dpy, selection) == win)
      {
         content = content_;
         selOwned = true;
      }
      else
      {
         selOwned = false;
      }
      return selOwned;
   }

   void
   SelectionManager::onPropertyNotify (XPropertyEvent& event)
   {
    #if 0
      std::cout << "SelectionManager::onPropertyNotify"
                << " on '" << XGetAtomName (dpy, event.atom) << "' "
                << (event.state == PropertyNewValue ? "(NewValue)" : "(Delete)")
                << std::endl;
    #endif
      if (state == State::WaitingForIncrAck &&
          event.atom == cliProp &&
          event.state == PropertyDelete)
      {
         // Send next chunk of ongoing INCR transfer
         size_t len = std::min (chunkSize, content.length () - cliPos);
         if (len > 0)
         {
            // transfer next chunk of data
            std::cout << "sending next INCR chunk..." << std::endl;
            XChangeProperty (dpy, cliWin, cliProp, target, 8, PropModeReplace,
                             (const unsigned char *) content.data () + cliPos,
                             len);
         }
         else
         {
            // empty property signals end of transfer
            std::cout << "signaling end of INCR transfer..." << std::endl;
            XChangeProperty (dpy, cliWin, cliProp, target, 8, PropModeReplace,
                             nullptr, 0);
            state = State::Idle;
         }
         XFlush (dpy);
         cliPos += len;
         return;
      }

      if (state == State::ReadingIncr &&
          event.atom == prop &&
          event.state == PropertyNewValue)
      {
         // Receive next chunk of ongoing INCR transfer
         Atom type = None;
         int propFormat;
         unsigned long propSize, propItems;
         unsigned char* buffer;
         XGetWindowProperty (dpy, win, prop, 0, 0, False, AnyPropertyType,
                             &type, &propFormat, &propItems, &propSize,
                             &buffer);
         XFree (buffer);

         if (propSize == 0) {
            // no more data, exit from loop
            XDeleteProperty (dpy, win, prop);
            state = State::Idle;

            std::cout << "Received INCR end of transfer" << std::endl;
            pasteCallback (std::string ((char *) incoming.data (),
                                        incoming.size ()));
            incoming.clear ();
            return;
         }

         // the property contains text of known size
         XGetWindowProperty (dpy, win, prop, 0, propSize, False,
                             AnyPropertyType, &type, &propFormat, &propItems,
                             &propSize, &buffer);

         std::cout << "Received INCR data size=" << propSize
                   << " format=" << propFormat
                   << " items=" << propItems
                   << " bytes=" << (propFormat >> 3) * propItems
                   << std::endl;
         size_t len = (propFormat >> 3) * propItems;
         size_t pos = incoming.size ();
         incoming.resize (pos + len);
         memcpy (incoming.data () + pos, buffer, len);

         XFree (buffer);

         // delete property to get the next chunk
         XDeleteProperty (dpy, win, prop);
         XFlush (dpy);
         return;
      }
   }

   void
   SelectionManager::onSelectionClear (XSelectionClearEvent& event)
   {
      std::cout << "SelectionManager::onSelectionClear" << std::endl;
      selOwned = false;
   }

   void
   SelectionManager::onSelectionNotify (XSelectionEvent& event)
   {
      std::cout << "SelectionManager::onSelectionNotify" << std::endl;

      if (state != State::WaitingForSelNotify)
      {
         std::cout << "Ignoring XSelectionEvent in state=" << (int)state
                   << std::endl;
         return;
      }

      if (event.property == None) {
         std::cout << "Error: Conversion to requested target '"
                   << XGetAtomName (dpy, target) << "' failed." << std::endl;
         state = State::Idle;
         return;
      }

      // find the size and format of the data in property
      Atom type = None;
      int propFormat;
      unsigned long propSize, propItems;
      unsigned char* buffer;
      XGetWindowProperty (dpy, win, prop, 0, 0, False, AnyPropertyType,
                          &type, &propFormat, &propItems, &propSize, &buffer);
      XFree (buffer);

      if (type == incr) {
         // start INCR mechanism by deleting property
         std::cout << "starting INCR by deleting property" << std::endl;
         XDeleteProperty (dpy, win, prop);
         XFlush (dpy);
         state = State::ReadingIncr;
         return;
      }

      // not using INCR mechanism, just read the property
      XGetWindowProperty (dpy, win, prop, 0, propSize, False,
                          AnyPropertyType, &type, &propFormat, &propItems,
                          &propSize, &buffer);
      XDeleteProperty (dpy, win, prop);

      std::cout << "Received data size=" << propSize
                << " format=" << propFormat
                << " items=" << propItems
                << " bytes=" << (propFormat >> 3) * propItems
                << std::endl;
      size_t len = (propFormat >> 3) * propItems;
      size_t pos = incoming.size ();
      incoming.resize (pos + len);
      memcpy (incoming.data () + pos, buffer, len);
      pasteCallback (std::string ((char *) incoming.data (), incoming.size ()));
      incoming.clear ();

      XFree (buffer);
      state = State::Idle;
   }

   void
   SelectionManager::onSelectionRequest (XSelectionRequestEvent& event)
   {
      std::cout << "SelectionManager::onSelectionRequest" << std::endl;

      if (state != State::Idle)
      {
         std::cout << "Ignoring XSelectionRequestEvent in state=" << (int)state
                   << std::endl;
         return;
      }

      cliWin = event.requestor;
      cliProp = event.property;
      cliPos = 0;

      if (event.target == targets) // response to TARGETS request
      {
         Atom types [2] = { targets, target };
         XChangeProperty (dpy, cliWin, cliProp, XA_ATOM, 32, PropModeReplace,
                          (const unsigned char *) types, 2);
      }
      else if (chunkSize < content.size ()) // INCR response
      {
         std::cout << "sending INCR response" << std::endl;
         XChangeProperty (dpy, cliWin, cliProp, incr, 32, PropModeReplace,
                          nullptr, 0);
         XSelectInput (dpy, cliWin, PropertyChangeMask);
         state = State::WaitingForIncrAck;
      }
      else // normal response (send all data)
      {
         std::cout << "sending normal response" << std::endl;
         XChangeProperty (dpy, cliWin, cliProp, target, 8, PropModeReplace,
                          (const unsigned char *) content.data (),
                          content.size ());
      }

      // send SelectionNotify event in response
      {
         XEvent res;
         res.xselection.type = SelectionNotify;
         res.xselection.property = cliProp;
         res.xselection.display = event.display;
         res.xselection.requestor = cliWin;
         res.xselection.selection = event.selection;
         res.xselection.target = event.target;
         res.xselection.time = event.time;
         XSendEvent (dpy, event.requestor, 0, 0, &res);
         XFlush (dpy);
      }
   }

} // namespace zutty
