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

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "base64.h"
#include "font.h"
#include "options.h"
#include "pty.h"
#include "renderer.h"
#include "selmgr.h"
#include "vterm.h"

#include <cassert>
#include <iostream>
#include <langinfo.h>
#include <memory>
#include <poll.h>
#include <pwd.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

using zutty::CharVdev;
using zutty::Vterm;
using zutty::Renderer;

static const std::string fontpath = "/usr/share/fonts/X11/misc/";
static const std::string fontext = ".pcf.gz";

static std::unique_ptr <zutty::Font> priFont = nullptr;
static std::unique_ptr <zutty::Font> altFont = nullptr;
static std::unique_ptr <Renderer> renderer = nullptr;
static std::unique_ptr <Vterm> vt = nullptr;
static std::unique_ptr <SelectionManager> selMgr = nullptr;

/*
 * Create an RGB, double-buffered X window.
 * Return the window and context handles.
 */
static void
make_x_window (Display * x_dpy, EGLDisplay egl_dpy,
               const char * name, int width, int height,
               Window *winRet, EGLContext * ctxRet,
               EGLSurface * surfRet)
{
   static const EGLint attribs[] = {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_NONE
   };
   static const EGLint ctx_attribs[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
   };

   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   XVisualInfo *visInfo, visTemplate;
   int num_visuals;
   EGLContext ctx;
   EGLConfig config;
   EGLint num_configs;
   EGLint vid;

   scrnum = DefaultScreen (x_dpy);
   root = RootWindow (x_dpy, scrnum);

   if (!eglChooseConfig (egl_dpy, attribs, &config, 1, &num_configs)) {
      std::cerr << "Error: couldn't get an EGL visual config" << std::endl;
      exit(1);
   }

   assert (config);
   assert (num_configs > 0);

   if (!eglGetConfigAttrib (egl_dpy, config, EGL_NATIVE_VISUAL_ID, &vid)) {
      std::cerr << "Error: eglGetConfigAttrib() failed" << std::endl;
      exit (1);
   }

   /* The X window visual must match the EGL config */
   visTemplate.visualid = vid;
   visInfo = XGetVisualInfo (x_dpy, VisualIDMask, &visTemplate, &num_visuals);
   if (!visInfo) {
      std::cerr << "Error: couldn't get X visual" << std::endl;
      exit (1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap (x_dpy, root, visInfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | FocusChangeMask |
      PropertyChangeMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask |
      PointerMotionMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow (x_dpy, root, 0, 0, width, height,
                        0, visInfo->depth, InputOutput,
                        visInfo->visual, mask, &attr);

   {
      /* set NET_WM_PID to the the process ID to link the window to the pid */
      Atom _NET_WM_PID = XInternAtom (x_dpy, "_NET_WM_PID", false);
      pid_t pid = getpid ();
      XChangeProperty (x_dpy, win, _NET_WM_PID, XA_CARDINAL,
                       32, PropModeReplace, (unsigned char *)&pid, 1);
   }

   {
      /* set WM_CLIENT_MACHINE to the hostname */
      char name [256];
      if (gethostname (name, sizeof (name)) < 0)
      {
         std::cerr << "Error: couldn't get hostname" << std::endl;
         exit (1);
      }
      name [sizeof (name) - 1] = '\0';
      char *hostname [] = { name };
      XTextProperty text_prop;
      XStringListToTextProperty (hostname, 1, &text_prop);
      XSetWMClientMachine (x_dpy, win, &text_prop);
      XFree (text_prop.value);
   }

   {
      XSizeHints sizehints;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize;
      XSetNormalHints (x_dpy, win, &sizehints);
      XSetStandardProperties (x_dpy, win, name, name,
                              None, nullptr, 0, &sizehints);
   }

   eglBindAPI (EGL_OPENGL_ES_API);

   ctx = eglCreateContext (egl_dpy, config, EGL_NO_CONTEXT, ctx_attribs);
   if (!ctx) {
      std::cerr << "Error: eglCreateContext failed" << std::endl;
      exit (1);
   }

   /* test eglQueryContext() */
   {
      EGLint val;
      eglQueryContext (egl_dpy, ctx, EGL_CONTEXT_CLIENT_VERSION, &val);
      assert (val == 2);
   }

   *surfRet = eglCreateWindowSurface (egl_dpy, config,
                                      (EGLNativeWindowType)win, nullptr);
   if (!*surfRet) {
      std::cerr << "Error: eglCreateWindowSurface failed" << std::endl;
      exit (1);
   }

   /* sanity checks */
   {
      EGLint val;
      eglQuerySurface (egl_dpy, *surfRet, EGL_WIDTH, &val);
      assert (val == width);
      eglQuerySurface (egl_dpy, *surfRet, EGL_HEIGHT, &val);
      assert (val == height);
      assert (eglGetConfigAttrib (egl_dpy, config, EGL_SURFACE_TYPE, &val));
      assert (val & EGL_WINDOW_BIT);
   }

   XFree (visInfo);

   *winRet = win;
   *ctxRet = ctx;
}

static void
resolveShell (char* progPath)
{
   char resolvedPath [PATH_MAX];
   if (progPath [0] == '/')
      return; // absolute path; we are done
   else if (progPath [0] == '.' &&
            realpath (progPath, resolvedPath) != nullptr)
   {
      strncpy (progPath, resolvedPath, PATH_MAX);
      return;
   }

   // go through PATH and try to resolve our program
   char* PATH = strdup (getenv ("PATH"));
   if (PATH)
   {
      char testPath [PATH_MAX+1];
      char* p = strtok (PATH, ":");
      while (p)
      {
         snprintf (testPath, PATH_MAX+1, "%s/%s", p, progPath);
         if (realpath (testPath, resolvedPath) != nullptr)
         {
            strncpy (progPath, resolvedPath, PATH_MAX);
            free (PATH);
            return;
         }
         p = strtok (nullptr, ":");
      }
      free (PATH);
   }

   // look at $SHELL
   char* SHELL = getenv ("SHELL");
   struct stat statbuf;
   if (SHELL && stat (SHELL, &statbuf) == 0 && (statbuf.st_mode & S_IXOTH))
   {
      strncpy (progPath, SHELL, PATH_MAX-1);
      return;
   }

   // obtain user's shell from /etc/passwd
   auto pwent = getpwuid (getuid ());
   SHELL = pwent->pw_shell;
   if (SHELL && stat (SHELL, &statbuf) == 0 && (statbuf.st_mode & S_IXOTH))
   {
      strncpy (progPath, SHELL, PATH_MAX-1);
      return;
   }

   // last resort
   strcpy (progPath, "/bin/sh");
}

static void
validateShell (char* progPath)
{
   resolveShell (progPath);

   // validate against etries in /etc/shells
   char* permShell = getusershell ();
   while (permShell)
   {
      if (strcmp (progPath, permShell) == 0)
      {
         endusershell ();
         return;
      }
      permShell = getusershell ();
   }
   endusershell ();

   // progPath is *not* one of the permitted user shells
   unsetenv ("SHELL");
}

static int
startShell (const char* const argv[])
{
   int fdm;
   pid_t pid;
   struct winsize size;

   size.ws_col = opts.nCols;
   size.ws_row = opts.nRows;
   pid = pty_fork (&fdm, nullptr, 0, nullptr, &size);

   if (pid < 0) {
      throw std::runtime_error ("fork error");
   } else if (pid == 0) { // child:
      struct termios term;
      if (tcgetattr (STDIN_FILENO, &term) < 0)
         throw std::runtime_error ("tcgetattr");
      term.c_iflag |= IUTF8;
      if (tcsetattr (STDIN_FILENO, TCSANOW, &term) < 0)
         throw std::runtime_error ("tcsetattr");

      if (setenv ("TERM", "xterm-256color", 1) < 0)
         throw std::runtime_error ("can't setenv (TERM)");

      if (execvp (argv[0], (char * const *) argv) < 0)
         throw std::runtime_error (std::string ("can't execvp: ") + argv [0]);
   }

   return fdm;
}

static VtModifier
convertKeyState (KeySym ks, unsigned int state)
{
   VtModifier mod = VtModifier::none;
   if (state & ShiftMask)
      switch (ks)
      {
      // Discard shift state for certain keypad keys, since that is implicit in
      // the fact that we received these keysyms instead of XK_KP_Home, etc.
      case XK_KP_Decimal:
      case XK_KP_0: case XK_KP_1: case XK_KP_2: case XK_KP_3: case XK_KP_4:
      case XK_KP_5: case XK_KP_6: case XK_KP_7: case XK_KP_8: case XK_KP_9:
         break;
      default:
         mod = mod | VtModifier::shift;
      }
   if (state & ControlMask)
      mod = mod | VtModifier::control;
   if (state & Mod1Mask)
      mod = mod | VtModifier::alt;
   return mod;
}

static bool
onKeyPress (XEvent& event, XIC& xic, int pty_fd)
{
   using Key = VtKey;
   XKeyEvent& xkevt = event.xkey;

   KeySym ks;
   char buffer [16];
   const int avail = sizeof (buffer) - 1;
   int nbytes = 0;

   if (xic)
   {
      Status status;
      nbytes = XmbLookupString (xic, &xkevt, buffer, avail, &ks, &status);
      if (status == XBufferOverflow) {
         std::cerr << "KeyPress event: buffer size " << sizeof (buffer)
                   << " is too small for XmbLookupString, would have needed "
                   << " a buffer with " << nbytes + 1 << " bytes."
                   << std::endl;
         return true;
      }
   }
   else
   {
      nbytes = XLookupString (&xkevt, buffer, avail, &ks, nullptr);
   }
   buffer [nbytes] = '\0';

   // Special key combinations that are handled by Zutty itself:
   if ((ks == XK_Insert || ks == XK_KP_Insert) &&
       xkevt.state == ShiftMask)
   {
      selMgr->getSelection (xkevt.time,
                            [] (const std::string& s)
                            { vt->pasteSelection (s); });
      return false;
   }
   if ((ks == XK_space || ks == XK_KP_Space) &&
       (xkevt.state & (Button1Mask | Button3Mask)))
   {
      vt->selectRectangularModeToggle ();
      return false;
   }

   VtModifier mod = convertKeyState (ks, xkevt.state);
   switch (ks)
   {
#define KEYSEND(XKey, VtKey)                    \
      case XKey:                                \
         vt->writePty (VtKey, mod);             \
         return false

      KEYSEND (XK_Return,           Key::Return);
      KEYSEND (XK_BackSpace,        Key::Backspace);
      KEYSEND (XK_Tab,              Key::Tab);
      KEYSEND (XK_Insert,           Key::Insert);
      KEYSEND (XK_Delete,           Key::Delete);
      KEYSEND (XK_Home,             Key::Home);
      KEYSEND (XK_End,              Key::End);
      KEYSEND (XK_Up,               Key::Up);
      KEYSEND (XK_Down,             Key::Down);
      KEYSEND (XK_Left,             Key::Left);
      KEYSEND (XK_Right,            Key::Right);
      KEYSEND (XK_Page_Up,          Key::PageUp);
      KEYSEND (XK_Page_Down,        Key::PageDown);
      KEYSEND (XK_F1,               Key::F1);
      KEYSEND (XK_F2,               Key::F2);
      KEYSEND (XK_F3,               Key::F3);
      KEYSEND (XK_F4,               Key::F4);
      KEYSEND (XK_F5,               Key::F5);
      KEYSEND (XK_F6,               Key::F6);
      KEYSEND (XK_F7,               Key::F7);
      KEYSEND (XK_F8,               Key::F8);
      KEYSEND (XK_F9,               Key::F9);
      KEYSEND (XK_F10,              Key::F10);
      KEYSEND (XK_F11,              Key::F11);
      KEYSEND (XK_F12,              Key::F12);
      KEYSEND (XK_F13,              Key::F13);
      KEYSEND (XK_F14,              Key::F14);
      KEYSEND (XK_F15,              Key::F15);
      KEYSEND (XK_F16,              Key::F16);
      KEYSEND (XK_F17,              Key::F17);
      KEYSEND (XK_F18,              Key::F18);
      KEYSEND (XK_F19,              Key::F19);
      KEYSEND (XK_F20,              Key::F20);
      KEYSEND (XK_KP_0,             Key::KP_0);
      KEYSEND (XK_KP_1,             Key::KP_1);
      KEYSEND (XK_KP_2,             Key::KP_2);
      KEYSEND (XK_KP_3,             Key::KP_3);
      KEYSEND (XK_KP_4,             Key::KP_4);
      KEYSEND (XK_KP_5,             Key::KP_5);
      KEYSEND (XK_KP_6,             Key::KP_6);
      KEYSEND (XK_KP_7,             Key::KP_7);
      KEYSEND (XK_KP_8,             Key::KP_8);
      KEYSEND (XK_KP_9,             Key::KP_9);
      KEYSEND (XK_KP_F1,            Key::KP_F1);
      KEYSEND (XK_KP_F2,            Key::KP_F2);
      KEYSEND (XK_KP_F3,            Key::KP_F3);
      KEYSEND (XK_KP_F4,            Key::KP_F4);
      KEYSEND (XK_KP_Up,            Key::KP_Up);
      KEYSEND (XK_KP_Down,          Key::KP_Down);
      KEYSEND (XK_KP_Left,          Key::KP_Left);
      KEYSEND (XK_KP_Right,         Key::KP_Right);
      KEYSEND (XK_KP_Prior,         Key::KP_PageUp);
      KEYSEND (XK_KP_Next,          Key::KP_PageDown);
      KEYSEND (XK_KP_Add,           Key::KP_Plus);
      KEYSEND (XK_KP_Insert,        Key::KP_Insert);
      KEYSEND (XK_KP_Delete,        Key::KP_Delete);
      KEYSEND (XK_KP_Begin,         Key::KP_Begin);
      KEYSEND (XK_KP_Home,          Key::KP_Home);
      KEYSEND (XK_KP_End,           Key::KP_End);
      KEYSEND (XK_KP_Subtract,      Key::KP_Minus);
      KEYSEND (XK_KP_Multiply,      Key::KP_Star);
      KEYSEND (XK_KP_Divide,        Key::KP_Slash);
      KEYSEND (XK_KP_Separator,     Key::KP_Comma);
      KEYSEND (XK_KP_Decimal,       Key::KP_Dot);
      KEYSEND (XK_KP_Equal,         Key::KP_Equal);
      KEYSEND (XK_KP_Space,         Key::KP_Space);
      KEYSEND (XK_KP_Tab,           Key::KP_Tab);
      KEYSEND (XK_KP_Enter,         Key::KP_Enter);
#ifdef DEBUG
      KEYSEND (XK_Print,            Key::Print);
#endif

#undef KEYSEND

#define KEYIGN(XKey)                            \
      case XKey:                                \
         return false

      // Ignore clauses for modifiers to avoid sending NUL bytes
      KEYIGN (XK_Shift_L);
      KEYIGN (XK_Shift_R);
      KEYIGN (XK_Control_L);
      KEYIGN (XK_Control_R);
      KEYIGN (XK_Caps_Lock);
      KEYIGN (XK_Shift_Lock);
      KEYIGN (XK_Meta_L);
      KEYIGN (XK_Meta_R);
      KEYIGN (XK_Alt_L);
      KEYIGN (XK_Alt_R);
      KEYIGN (XK_Super_L);
      KEYIGN (XK_Super_R);
      KEYIGN (XK_Hyper_L);
      KEYIGN (XK_Hyper_R);

#undef KEYIGN

   default:
      if (! XFilterEvent (&event, xkevt.window))
      {
         if (nbytes > 1)
         {
            if (vt->writePty (buffer) < nbytes)
               return true;
         }
         else
         {
            if (vt->writePty (buffer [0], mod) < 1)
               return true;
         }
      }
      return false;
   }
}

// Data shared between mouse event handlers
struct MouseContext
{
   // cycle selection SnapTo behaviour based on double/triple clicks
   constexpr const static int Multi_Click_Threshold_Ms = 250;
   Time lastButtonReleasedAt = 0;
   unsigned int lastButtonReleased = 0;
   bool selectionOngoing = false;
};
static MouseContext mouseCtx;

static inline bool
isMouseProtocol (unsigned int state, const MouseTrackingState& mouseTrk)
{
   return !mouseCtx.selectionOngoing &&
      !(state & ShiftMask) &&
      mouseTrk.mode != MouseTrackingMode::Disabled;
}

static inline void
mouseProtoSend (MouseTrackingEnc enc, int eventType, unsigned int modstate,
                int button, int cx, int cy)
{
   int cb = 0;
   if (eventType == MotionNotify)
   {
      if (modstate & Button1Mask) cb = 32;
      else if (modstate & Button2Mask) cb = 33;
      else if (modstate & Button3Mask) cb = 34;
      else cb = 35;
   }
   else if (eventType == ButtonRelease && enc != MouseTrackingEnc::SGR)
   {
      cb = 3;
   }
   else // normal button encoding
   {
      if (button == 4)
         cb = 64; // Mouse wheel up
      else if (button == 5)
         cb = 65; // Mouse wheel down
      else
         cb = button - 1; // Mouse button 1..3
   }

   if (modstate & ShiftMask) cb += 4;
   if (modstate & Mod1Mask) cb += 8;
   if (modstate & ControlMask) cb += 16;

   std::ostringstream oss;
   switch (enc)
   {
   case MouseTrackingEnc::Default:
      oss << "\e[M" << (char)(32 + cb) << (char)(32 + cx) << (char)(32 + cy);
      vt->writePty (oss.str ().c_str ());
      break;
   case MouseTrackingEnc::UTF8:
      oss << "\e[M";
      Utf8Encoder::pushUnicode (32 + cb, [&] (char ch) { oss << ch; });
      Utf8Encoder::pushUnicode (32 + cx, [&] (char ch) { oss << ch; });
      Utf8Encoder::pushUnicode (32 + cy, [&] (char ch) { oss << ch; });
      vt->writePty (oss.str ().c_str ());
      break;
   case MouseTrackingEnc::SGR:
      oss << "\e[<" << cb << ";" << cx << ";" << cy
          << (eventType == ButtonRelease ? "m" : "M");
      vt->writePty (oss.str ().c_str ());
      break;
   case MouseTrackingEnc::URXVT:
      oss << "\e[" << cb + 32 << ";" << cx << ";" << cy << "M";
      vt->writePty (oss.str ().c_str ());
      break;
   }
}

static inline void
mouseProtoConvCoords (int px, int py, uint16_t& out_cx, uint16_t& out_cy)
{
   out_cx = std::max (0, (px - opts.border - 1) / priFont->getPx ()) + 1;
   out_cy = std::max (0, (py - opts.border - 1) / priFont->getPy ()) + 1;
}

static inline void
onButtonPressMouseProto (XButtonEvent& xbevt,
                         const MouseTrackingState& mouseTrk)
{
   uint16_t cx, cy;

   if (xbevt.button > 5)
      return;

   switch (mouseTrk.mode)
   {
   case MouseTrackingMode::VT200:
   case MouseTrackingMode::VT200_ButtonEvent:
   case MouseTrackingMode::VT200_AnyEvent:
      mouseProtoConvCoords (xbevt.x, xbevt.y, cx, cy);
      mouseProtoSend (mouseTrk.enc, ButtonPress,
                      xbevt.state, xbevt.button, cx, cy);
      break;
   case MouseTrackingMode::X10_Compat:
      mouseProtoConvCoords (xbevt.x, xbevt.y, cx, cy);
      mouseProtoSend (mouseTrk.enc, ButtonPress, 0, xbevt.button, cx, cy);
      break;
   case MouseTrackingMode::Disabled:
      break;
   }
}

static inline void
onButtonReleaseMouseProto (XButtonEvent& xbevt,
                           const MouseTrackingState& mouseTrk)
{
   uint16_t cx, cy;

   if (xbevt.button > 3)
      return;

   switch (mouseTrk.mode)
   {
   case MouseTrackingMode::VT200:
   case MouseTrackingMode::VT200_ButtonEvent:
   case MouseTrackingMode::VT200_AnyEvent:
      mouseProtoConvCoords (xbevt.x, xbevt.y, cx, cy);
      mouseProtoSend (mouseTrk.enc, ButtonRelease,
                      xbevt.state, xbevt.button, cx, cy);
      break;
   case MouseTrackingMode::X10_Compat:
   case MouseTrackingMode::Disabled:
      break;
   }
}

static inline void
onMotionNotifyMouseProto (XMotionEvent& xmoevt,
                          const MouseTrackingState& mouseTrk)
{
   uint16_t cx, cy;
   static uint16_t lastCx = 65535;
   static uint16_t lastCy = 65535;

   switch (mouseTrk.mode)
   {
   case MouseTrackingMode::VT200_ButtonEvent:
      if (! (xmoevt.state & (Button1Mask | Button2Mask | Button3Mask)))
         break;
      // fall through
   case MouseTrackingMode::VT200_AnyEvent:
      mouseProtoConvCoords (xmoevt.x, xmoevt.y, cx, cy);
      if (cx != lastCx || cy != lastCy)
      {
         mouseProtoSend (mouseTrk.enc, MotionNotify,
                         xmoevt.state, 0, cx, cy);
         lastCx = cx;
         lastCy = cy;
      }
      break;
   default:
      break;
   }
}

static void
onButtonPress (XButtonEvent& xbevt, bool& holdPtyIn)
{
   const auto& mouseTrk = vt->getMouseTrackingState ();
   if (isMouseProtocol (xbevt.state, mouseTrk))
   {
      onButtonPressMouseProto (xbevt, mouseTrk);
      return;
   }

   const auto& lastReleaseTime = mouseCtx.lastButtonReleasedAt;
   bool cycleSnapTo =
      xbevt.button == mouseCtx.lastButtonReleased &&
      xbevt.time - lastReleaseTime < mouseCtx.Multi_Click_Threshold_Ms;
   switch (xbevt.button)
   {
   case 1:
      vt->selectStart (xbevt.x, xbevt.y, cycleSnapTo);
      mouseCtx.selectionOngoing = true;
      holdPtyIn = true;
      break;
   case 3:
      vt->selectExtend (xbevt.x, xbevt.y, cycleSnapTo);
      mouseCtx.selectionOngoing = true;
      holdPtyIn = true;
      break;
   default:
      break;
   }
}

static void
onButtonRelease (XButtonEvent& xbevt, bool& holdPtyIn)
{
   const auto& mouseTrk = vt->getMouseTrackingState ();
   if (isMouseProtocol (xbevt.state, mouseTrk))
   {
      onButtonReleaseMouseProto (xbevt, mouseTrk);
      return;
   }

   mouseCtx.lastButtonReleased = xbevt.button;
   mouseCtx.lastButtonReleasedAt = xbevt.time;

   switch (xbevt.button)
   {
   case 1: case 3:
   {
      std::string utf8_sel;
      holdPtyIn = false;
      mouseCtx.selectionOngoing = false;
      if (vt->selectFinish (utf8_sel))
         selMgr->setSelection (xbevt.time, utf8_sel);
   }
   break;
   case 2:
      selMgr->getSelection (xbevt.time,
                            [] (const std::string& s)
                            { vt->pasteSelection (s); });
      break;
   case 4: vt->mouseWheelUp (); break;
   case 5: vt->mouseWheelDown (); break;
      break;
   }
}

static void
onMotionNotify (XMotionEvent& xmoevt)
{
   const auto& mouseTrk = vt->getMouseTrackingState ();
   if (isMouseProtocol (xmoevt.state, mouseTrk))
      onMotionNotifyMouseProto (xmoevt, mouseTrk);
   else if (xmoevt.state & (Button1Mask | Button3Mask))
      vt->selectUpdate (xmoevt.x, xmoevt.y);
}

static bool
x11Event (XEvent& event, XIC& xic, int pty_fd, bool& destroyed, bool& holdPtyIn)
{
   static bool exposed = false;
   bool redraw = false;
   destroyed = false;

   switch (event.type) {
   case Expose:
      exposed = true;
      redraw = true;
      break;
   case ConfigureNotify:
      vt->resize (event.xconfigure.width, event.xconfigure.height);
      redraw = true;
      break;
   case ReparentNotify:
      std::cout << "ReparentNotify" << std::endl;
      redraw = true;
      break;
   case MappingNotify:
      std::cout << "MappingNotify" << std::endl;
      break;
   case MapNotify:
      std::cout << "MapNotify" << std::endl;
      break;
   case UnmapNotify:
      std::cout << "UnmapNotify" << std::endl;
      break;
   case DestroyNotify:
      std::cout << "DestroyNotify" << std::endl;
      destroyed = true;
      return true;
   case KeyPress:
      if (onKeyPress (event, xic, pty_fd))
         return true;
      redraw = true;
      break;
   case KeyRelease:
      break;
   case ButtonPress:
      onButtonPress (event.xbutton, holdPtyIn);
      break;
   case ButtonRelease:
      onButtonRelease (event.xbutton, holdPtyIn);
      break;
   case MotionNotify:
      onMotionNotify (event.xmotion);
      break;
   case FocusIn:
      if (vt->getMouseTrackingState ().focusEventMode)
         vt->writePty ("\e[I");
      vt->setHasFocus (true);
      break;
   case FocusOut:
      if (vt->getMouseTrackingState ().focusEventMode)
         vt->writePty ("\e[O");
      vt->setHasFocus (false);
      break;
   case PropertyNotify:
      selMgr->onPropertyNotify (event.xproperty);
      break;
   case SelectionClear:
      vt->selectClear ();
      selMgr->onSelectionClear (event.xselectionclear);
      break;
   case SelectionNotify:
      selMgr->onSelectionNotify (event.xselection);
      break;
   case SelectionRequest:
      selMgr->onSelectionRequest (event.xselectionrequest);
      break;
   default:
      std::cout << "X event.type = " << event.type << std::endl;
      break;
   }

   if (exposed && redraw) {
      vt->redraw ();
   }

   return false;
}

static bool
eventLoop (Display* dpy, Window win, XIC& xic, int pty_fd)
{
   int x11_fd = XConnectionNumber (dpy);
   std::cout << "x11_fd = " << x11_fd << std::endl;
   std::cout << "pty_fd = " << pty_fd << std::endl;

   struct pollfd pollset[] = {
      {pty_fd, POLLIN, 0},
      {x11_fd, POLLIN, 0},
   };

   bool holdPtyIn = false;
   while (1) {
      pollset [0].fd = holdPtyIn ? -pty_fd : pty_fd;
      if (poll (pollset, 2, -1) < 0)
         return false;

      if (pollset[0].revents & POLLHUP)
         return false;

      if (pollset[0].revents & POLLIN)
         vt->readPty ();

      if (pollset[1].revents & POLLIN)
         while (XPending (dpy))
         {
            XEvent event;
            bool destroyed = false;

            XNextEvent (dpy, &event);
            if (x11Event (event, xic, pty_fd, destroyed, holdPtyIn))
               return destroyed;
         }
   }
}

static void
handleOsc (Display* dpy, Window win, int cmd, const std::string& arg)
{
   switch (cmd)
   {
   case 0: // Change Icon Name & Window Title
   case 2: // Change Window Title
      XStoreName (dpy, win, arg.c_str ());
      break;
   case 52: // Manipulate Selection Data
   {
      std::size_t p = arg.find_first_of(";");
      if (p == std::string::npos)
      {
         logT << "Malformed argument to OSC 52 (missing ';'): '"
              << arg << "'" << std::endl;
         break;
      }
      std::string pc = arg.substr (0, p); // Currently not used
      std::string pd = arg.substr (p + 1);
      logT << "OSC 52: pc='" << pc << "', pd='" << pd << "'" << std::endl;

      if (pd == "?")
      {
         selMgr->getSelection (
            CurrentTime,
            [] (const std::string& s)
            {
               std::ostringstream oss;
               oss << "\e]52;;" << zutty::base64::encode (s) << "\e\\";
               vt->writePty (oss.str ().c_str ());
            });
      }
      else
      {
         std::string sel = zutty::base64::decode (pd);
         selMgr->setSelection (CurrentTime, sel);
      }
   }
      break;
   default:
      std::cout << "unhandled OSC: '" << cmd << ";" << arg << "'" << std::endl;
      break;
   }
}

static void
printGLInfo (EGLDisplay egl_dpy)
{
   std::cout << "\nEGL_VERSION     = " << eglQueryString (egl_dpy, EGL_VERSION)
             << "\nEGL_VENDOR      = " << eglQueryString (egl_dpy, EGL_VENDOR)
             << "\nEGL_EXTENSIONS  = " << eglQueryString (egl_dpy, EGL_EXTENSIONS)
             << "\nEGL_CLIENT_APIS = " << eglQueryString (egl_dpy, EGL_CLIENT_APIS)
             << std::endl;

   std::cout << "\nGL_RENDERER     = " << glGetString (GL_RENDERER)
             << "\nGL_VERSION      = " << glGetString (GL_VERSION)
             << "\nGL_VENDOR       = " << glGetString (GL_VENDOR)
             << "\nGL_EXTENSIONS   = " << glGetString (GL_EXTENSIONS)
             << std::endl;

   int work_grp_cnt[3];
   glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
   glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
   glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
   std::cout << "\nCompute shader:\n"
             << "- max. global (total) work group counts:"
             << " x=" << work_grp_cnt[0]
             << " y=" << work_grp_cnt[1]
             << " z=" << work_grp_cnt[2]
             << std::endl;

   int work_grp_size[3];
   glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
   glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
   glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
   std::cout << "- max. local (per-shader) work group sizes:"
             << " x=" << work_grp_size[0]
             << " y=" << work_grp_size[1]
             << " z=" << work_grp_size[2]
             << std::endl;

   int work_grp_inv;
   glGetIntegerv (GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
   std::cout << "- max. local work group invocations: " << work_grp_inv << "\n"
             << std::endl;
}

int
main (int argc, char* argv[])
{
   Display* x_dpy = nullptr;
   Window win;
   EGLSurface egl_surf;
   EGLContext egl_ctx;
   EGLDisplay egl_dpy;
   EGLint egl_major, egl_minor;
   XIC xic = nullptr;
   XIM xim;
   XIMStyles* xim_styles;
   XIMStyle xim_style = 0;
   char* modifiers;
   char* imvalret;
   int i;

   {
      const char* loc;
      bool warn = false;
      loc = setlocale (LC_ALL, "");
      if (!loc)
      {
         std::cout << "Warning: could not set locale!" << std::endl;
         warn = true;
      }
      else if (strcmp (nl_langinfo (CODESET), "UTF-8") != 0)
      {
         std::cout << "Warning: non-UTF-8 locale: " << loc << std::endl;
         warn = true;
      }
      if (warn)
         std::cout << "Expect broken international characters "
                   << "(or fix your locale)!"
                   << std::endl;
   }

   if (! XInitThreads ())
   {
      std::cerr << "Error: couldn't initialize XLib for multithreaded use"
                << std::endl;
      return -1;
   }

   opts.initialize (&argc, argv);

   x_dpy = XOpenDisplay (opts.display);
   if (!x_dpy)
   {
      std::cerr << "Error: couldn't open display " << opts.display << std::endl;
      return -1;
   }
   opts.setDisplay (x_dpy);

   opts.parse ();

   char progPath [PATH_MAX];
   strncpy (progPath, opts.shell, PATH_MAX-1);
   char* defaultShArgv [] = { progPath, nullptr };
   char** shArgv = defaultShArgv;
   if (argc > 1 && strcmp (argv [1], "-e") == 0)
   {
      shArgv = argv + 2;
      opts.title = argv [2];
   }
   else if (argc == 2)
   {
      strncpy (progPath, argv [1], PATH_MAX-1);
      validateShell (progPath);
   }
   int pty_fd = startShell (shArgv);

   egl_dpy = eglGetDisplay ((EGLNativeDisplayType)x_dpy);
   if (!egl_dpy)
   {
      std::cerr << "Error: eglGetDisplay() failed" << std::endl;
      return -1;
   }

   if (!eglInitialize (egl_dpy, &egl_major, &egl_minor))
   {
      std::cerr << "Error: eglInitialize() failed" << std::endl;
      return -1;
   }

   modifiers = XSetLocaleModifiers ("@im=none");
   if (modifiers == nullptr)
   {
      std::cerr << "Error: XSetLocaleModifiers() failed" << std::endl;
      return -1;
   }

   xim = XOpenIM (x_dpy, nullptr, nullptr, nullptr);
   if (xim == nullptr)
   {
      std::cerr << "Warning: XOpenIM failed" << std::endl;
   }

   if (xim)
   {
      imvalret = XGetIMValues (xim, XNQueryInputStyle, &xim_styles, nullptr);
      if (imvalret != nullptr || xim_styles == nullptr)
      {
         std::cerr << "No styles supported by input method" << std::endl;
      }

      if (xim_styles) {
         xim_style = 0;
         for (i = 0;  i < xim_styles->count_styles;  i++)
         {
            if (xim_styles->supported_styles [i] ==
                (XIMPreeditNothing | XIMStatusNothing))
            {
               xim_style = xim_styles->supported_styles [i];
               break;
            }
         }

         if (xim_style == 0)
         {
            std::cerr << "Insufficient input method support" << std::endl;
         }
         XFree (xim_styles);
      }
   }

   priFont = std::make_unique <zutty::Font> (fontpath + opts.fontname + fontext);
   altFont = std::make_unique <zutty::Font> (fontpath + opts.fontname + "B" + fontext,
                                             * priFont.get ());
   int win_width = 2 * opts.border + opts.nCols * priFont->getPx ();
   int win_height = 2 * opts.border + opts.nRows * priFont->getPy ();

   make_x_window (x_dpy, egl_dpy, opts.title, win_width, win_height,
                  &win, &egl_ctx, &egl_surf);

   XMapWindow (x_dpy, win);

   if (xim && xim_style) {
      xic = XCreateIC (xim, XNInputStyle, xim_style,
                       XNClientWindow, win, XNFocusWindow, win,
                       nullptr);

      if (xic == nullptr) {
         std::cerr << "XCreateIC failed, compose key won't work" << std::endl;
      }
   }

   if (!eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
   {
      std::cerr << "Error: eglMakeCurrent() failed" << std::endl;
      return -1;
   }

   selMgr = std::make_unique <SelectionManager> (x_dpy, win);

   renderer = std::make_unique <Renderer> (
      * priFont.get (),
      * altFont.get (),
      [egl_dpy, egl_surf, egl_ctx] ()
      {
         if (!eglMakeCurrent (egl_dpy, egl_surf, egl_surf, egl_ctx))
            throw std::runtime_error ("Error: eglMakeCurrent() failed");
         if (opts.glinfo)
            printGLInfo (egl_dpy);
      },
      [egl_dpy, egl_surf] ()
      {
         eglSwapBuffers (egl_dpy, egl_surf);
      });

   vt = std::make_unique <Vterm> (priFont->getPx (), priFont->getPy (),
                                  win_width, win_height, pty_fd);
   vt->setRefreshHandler ([] (const Frame& f) { renderer->update (f); });
   vt->setOscHandler ([&] (int cmd, const std::string& arg)
                      { handleOsc (x_dpy, win, cmd, arg); });

   // We might not get a ConfigureNotify event when the window first appears:
   vt->resize (win_width, win_height);

   bool destroyed = eventLoop (x_dpy, win, xic, pty_fd);

   renderer = nullptr; // ~Renderer () shuts down renderer thread

   eglDestroyContext (egl_dpy, egl_ctx);
   eglDestroySurface (egl_dpy, egl_surf);
   eglTerminate (egl_dpy);

   if (! destroyed)
      XDestroyWindow (x_dpy, win);
   XCloseDisplay (x_dpy);

   return 0;
}
