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

#include "log.h"
#include "options.h"
#include "selmgr.h"

#include <X11/Xmu/Atoms.h>

namespace zutty
{
   SelectionManager::SelectionManager (Display* dpy_, Window win_)
      : dpy (dpy_)
      , win (win_)
      , primary (XA_PRIMARY)
      , clipboard (XA_CLIPBOARD (dpy))
      , incr (XInternAtom (dpy, "INCR", False))
      , prop (XInternAtom(dpy, "_ZUTTY_SELECTION", False))
      , target (XA_UTF8_STRING (dpy))
      , targets (XInternAtom (dpy, "TARGETS", False))
      , chunkSize (XExtendedMaxRequestSize (dpy)
                   ? XExtendedMaxRequestSize (dpy) >> 2
                   : XMaxRequestSize (dpy) >> 2)
   {
      // N.B.: create map entries:
      ctx [primary].content = "";
      ctx [clipboard].content = "";

      logT << "SelectionManager: chunkSize=" << chunkSize << std::endl;
   }

   void
   SelectionManager::getSelection (Atom selection,
                                   Time time, PasteCallbackFn&& cb)
   {
      Context& cx = ctx [selection];

      if (cx.owned)
      {
         cb (true, cx.content);
         return;
      }

      cx.incoming.clear ();
      cx.pasteCallback = cb;
      XConvertSelection (dpy, selection, target, prop, win, time);
      cx.state = State::WaitingForSelNotify;
   }

   bool
   SelectionManager::setSelection (Atom selection,
                                   Time time, const std::string& content_)
   {
      Context& cx = ctx [selection];

      XSetSelectionOwner(dpy, selection, win, time);
      if (XGetSelectionOwner (dpy, selection) == win)
      {
         cx.content = content_;
         cx.owned = true;
      }
      else
      {
         cx.owned = false;
      }
      return cx.owned;
   }

   bool
   SelectionManager::copySelection (Atom dest, Atom source)
   {
      Context& cx = ctx [source];

      if (! cx.owned)
         return false;

      return setSelection (dest, CurrentTime, cx.content);
   }

   void
   SelectionManager::handleInboundIncr (Context& cx)
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
         cx.state = State::Idle;

         logT << "Received INCR end of transfer" << std::endl;
         cx.pasteCallback (true, std::string ((char *) cx.incoming.data (),
                                              cx.incoming.size ()));
         cx.incoming.clear ();
         return;
      }

      // the property contains text of known size
      XGetWindowProperty (dpy, win, prop, 0, propSize, False,
                          AnyPropertyType, &type, &propFormat, &propItems,
                          &propSize, &buffer);

      logT << "Received INCR data size=" << propSize
           << " format=" << propFormat
           << " items=" << propItems
           << " bytes=" << (propFormat >> 3) * propItems
           << std::endl;
      size_t len = (propFormat >> 3) * propItems;
      size_t pos = cx.incoming.size ();
      cx.incoming.resize (pos + len);
      memcpy (cx.incoming.data () + pos, buffer, len);

      XFree (buffer);

      // delete property to get the next chunk
      XDeleteProperty (dpy, win, prop);
      XFlush (dpy);
   }

   void
   SelectionManager::handleOutboundIncr (Context& cx)
   {
      // Send next chunk of ongoing INCR transfer
      size_t len = std::min (chunkSize, cx.content.length () - cx.cliPos);
      if (len > 0)
      {
         logT << "Sending next INCR chunk..." << std::endl;
         XChangeProperty (dpy, cx.cliWin, cx.cliProp, target, 8, PropModeReplace,
                          (const unsigned char *) cx.content.data () + cx.cliPos,
                          len);
      }
      else
      {
         logT << "Signaling end of INCR transfer..." << std::endl;
         XChangeProperty (dpy, cx.cliWin, cx.cliProp, target, 8, PropModeReplace,
                          nullptr, 0);
         cx.state = State::Idle;
      }
      XFlush (dpy);
      cx.cliPos += len;
   }

   void
   SelectionManager::onPropertyNotify (XPropertyEvent& event)
   {
      logT << "onPropertyNotify"
           << " on '" << XGetAtomName (dpy, event.atom) << "' "
           << (event.state == PropertyNewValue ? "(NewValue)" : "(Delete)")
           << std::endl;

      Context& cxp = ctx [primary];
      Context& cxc = ctx [clipboard];

      if (event.state == PropertyDelete)
      {
         if (cxp.state == State::WaitingForIncrAck &&
             cxp.cliProp == event.atom)
         {
            handleOutboundIncr (cxp);
         }
         else if (cxc.state == State::WaitingForIncrAck &&
                  cxc.cliProp == event.atom)
         {
            handleOutboundIncr (cxc);
         }
      }
      else if (event.state == PropertyNewValue && event.atom == prop)
      {
         if (cxp.state == State::ReadingIncr)
            handleInboundIncr (cxp);
         else if (cxc.state == State::ReadingIncr)
            handleInboundIncr (cxc);
      }
   }

   void
   SelectionManager::onSelectionClear (XSelectionClearEvent& event)
   {
      logT << "onSelectionClear for " << XGetAtomName (dpy, event.selection)
           << std::endl;

      if (ctx.find (event.selection) == ctx.end ())
      {
         logE << "No selection state for "
              << XGetAtomName (dpy, event.selection) << std::endl;
         return;
      }

      ctx [event.selection].owned = false;
      ctx [event.selection].content = "";
   }

   void
   SelectionManager::onSelectionNotify (XSelectionEvent& event)
   {
      logT << "onSelectionNotify for " << XGetAtomName (dpy, event.selection)
           << std::endl;

      if (ctx.find (event.selection) == ctx.end ())
      {
         logE << "No selection state for "
              << XGetAtomName (dpy, event.selection) << std::endl;
         return;
      }

      Context& cx = ctx [event.selection];

      if (cx.state != State::WaitingForSelNotify)
      {
         logW << "Ignoring XSelectionEvent in state=" << (int)cx.state
              << std::endl;
         return;
      }

      if (event.property == None) {
         logW << "Conversion to requested target '"
              << XGetAtomName (dpy, target) << "' failed." << std::endl;
         cx.pasteCallback (false, "");
         cx.state = State::Idle;
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
         logT << "Starting INCR by deleting property" << std::endl;
         XDeleteProperty (dpy, win, prop);
         XFlush (dpy);
         cx.state = State::ReadingIncr;
         return;
      }

      // not using INCR mechanism, just read the property
      XGetWindowProperty (dpy, win, prop, 0, propSize, False,
                          AnyPropertyType, &type, &propFormat, &propItems,
                          &propSize, &buffer);
      XDeleteProperty (dpy, win, prop);

      logT << "Received data size=" << propSize
           << " format=" << propFormat
           << " items=" << propItems
           << " bytes=" << (propFormat >> 3) * propItems
           << std::endl;
      size_t len = (propFormat >> 3) * propItems;
      size_t pos = cx.incoming.size ();
      cx.incoming.resize (pos + len);
      memcpy (cx.incoming.data () + pos, buffer, len);
      cx.pasteCallback (true, std::string ((char *) cx.incoming.data (),
                                           cx.incoming.size ()));
      cx.incoming.clear ();

      XFree (buffer);
      cx.state = State::Idle;
   }

   void
   SelectionManager::onSelectionRequest (XSelectionRequestEvent& event)
   {
      logT << "onSelectionRequest for " << XGetAtomName (dpy, event.selection)
           << std::endl;

      if (ctx.find (event.selection) == ctx.end ())
      {
         logE << "No selection state for "
              << XGetAtomName (dpy, event.selection) << std::endl;
         return;
      }

      Context& cx = ctx [event.selection];

      if (cx.state != State::Idle)
      {
         logW << "Ignoring XSelectionRequestEvent in state=" << (int)cx.state
              << std::endl;
         return;
      }

      if (! cx.owned)
      {
         logW << "Ignoring selection request for "
              << XGetAtomName (dpy, event.selection)
              << " that we do not own."
              << std::endl;
         return;
      }

      cx.cliWin = event.requestor;
      cx.cliProp = event.property;
      cx.cliPos = 0;

      if (event.target == targets) // response to TARGETS request
      {
         Atom types [2] = { targets, target };
         XChangeProperty (dpy, cx.cliWin, cx.cliProp, XA_ATOM, 32,
                          PropModeReplace, (const unsigned char *) types, 2);
      }
      else if (chunkSize < cx.content.size ()) // INCR response
      {
         logT << "Sending INCR response" << std::endl;
         XChangeProperty (dpy, cx.cliWin, cx.cliProp, incr, 32,
                          PropModeReplace, nullptr, 0);
         XSelectInput (dpy, cx.cliWin, PropertyChangeMask);
         cx.state = State::WaitingForIncrAck;
      }
      else // normal response (send all data)
      {
         logT << "Sending normal response" << std::endl;
         XChangeProperty (dpy, cx.cliWin, cx.cliProp, target, 8,
                          PropModeReplace,
                          (const unsigned char *) cx.content.data (),
                          cx.content.size ());
      }

      // send SelectionNotify event in response
      {
         XEvent res;
         res.xselection.type = SelectionNotify;
         res.xselection.property = cx.cliProp;
         res.xselection.display = event.display;
         res.xselection.requestor = cx.cliWin;
         res.xselection.selection = event.selection;
         res.xselection.target = event.target;
         res.xselection.time = event.time;
         XSendEvent (dpy, event.requestor, 0, 0, &res);
         XFlush (dpy);
      }
   }

} // namespace zutty
