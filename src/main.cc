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
#include <X11/Xmu/Error.h>

#include "base.h"
#include "base64.h"
#include "fontpack.h"
#include "options.h"
#include "pty.h"
#include "renderer.h"
#include "selmgr.h"
#include "vterm.h"
#include "wm_icons.h"

#include <cassert>
#include <langinfo.h>
#include <memory>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

using zutty::Fontpack;
using zutty::MouseTrackingState;
using zutty::MouseTrackingMode;
using zutty::MouseTrackingEnc;
using zutty::Vterm;
using zutty::VtKey;
using zutty::VtModifier;
using zutty::Renderer;
using zutty::SelectionManager;

static std::unique_ptr <Fontpack> fontpk = nullptr;
static std::unique_ptr <Renderer> renderer = nullptr;
static std::unique_ptr <Vterm> vt = nullptr;
static std::unique_ptr <SelectionManager> selMgr = nullptr;

static Display* xDisplay = nullptr;
static Window xWindow;
static Atom wmDeleteMessage;
static XSizeHints sizeHints;
static Colormap colormap;

static void
convertColor (const zutty::Color& color, XColor& xcolor)
{
   std::ostringstream oss;
   oss << color;
   if (XParseColor (xDisplay, colormap, oss.str ().c_str (), &xcolor) == 0)
   {
      logE << "XParseColor (): could not resolve " << oss.str () << std::endl;
   }
}

static void
setXCursor ()
{
   const static int cursorW = 9;
   const static int cursorH = 16;
   const static int cursorHotX = 4;
   const static int cursorHotY = 7;
   const static unsigned char cursorBits [] =
   {
      0x00, 0x00, 0xee, 0x00, 0x38, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00,
      0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00,
      0x10, 0x00, 0x38, 0x00, 0xee, 0x00, 0x00, 0x00
   };
   const static unsigned char cursorMaskBits [] =
   {
      0xef, 0x01, 0xff, 0x01, 0xff, 0x01, 0x7c, 0x00, 0x38, 0x00, 0x38, 0x00,
      0x38, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38, 0x00,
      0x7c, 0x00, 0xff, 0x01, 0xff, 0x01, 0xef, 0x01
   };

   XColor xc_fg;
   convertColor (opts.fg, xc_fg);
   XColor xc_bg;
   convertColor (opts.bg, xc_bg);
   Window root = DefaultRootWindow (xDisplay);
   Pixmap pxcur = XCreateBitmapFromData (
      xDisplay, root, (char *)cursorBits, cursorW, cursorH);
   Pixmap pxmask = XCreateBitmapFromData(
      xDisplay, root, (char *)cursorMaskBits, cursorW, cursorH);
   Cursor cursor = XCreatePixmapCursor(
      xDisplay, pxcur, pxmask, &xc_fg, &xc_bg, cursorHotX, cursorHotY);
   XFreePixmap(xDisplay, pxcur);
   XFreePixmap(xDisplay, pxmask);
   XRecolorCursor (xDisplay, cursor, &xc_fg, &xc_bg);
   XDefineCursor (xDisplay, xWindow, cursor);
}

static void
setUtf8prop (const char* prop_name, const std::string & value)
{
   Atom utf8 = XInternAtom (xDisplay, "UTF8_STRING", false);
   Atom prop = XInternAtom (xDisplay, prop_name, false);
   XChangeProperty (xDisplay, xWindow, prop, utf8, 8, PropModeReplace,
                    (const unsigned char*) value.data (), value.size ());
}

static void
setXWindowName (const std::string & name)
{
   XStoreName (xDisplay, xWindow, name.c_str ());
   setUtf8prop ("_NET_WM_NAME", name);
}

static void
setXWindowIconName (const std::string & name)
{
   XSetIconName (xDisplay, xWindow, name.c_str ());
   setUtf8prop ("_NET_WM_ICON_NAME", name);
}

static void
makeXWindow (const char* name, int width, int height, int px, int py,
             EGLDisplay eglDpy, EGLContext& eglCtx, EGLSurface& eglSurface)
{
   static const EGLint eglAttrs [] = {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_NONE
   };
   static const EGLint ctxAttrs [] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
   };

   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   XVisualInfo *visInfo, visTemplate;
   int numVisuals;
   EGLConfig config;
   EGLint numConfigs;
   EGLint vid;

   root = RootWindow (xDisplay, DefaultScreen (xDisplay));

   if (!eglChooseConfig (eglDpy, eglAttrs, &config, 1, &numConfigs)) {
      logE << "Couldn't get an EGL visual config" << std::endl;
      exit(1);
   }

   assert (config);
   assert (numConfigs > 0);

   if (!eglGetConfigAttrib (eglDpy, config, EGL_NATIVE_VISUAL_ID, &vid)) {
      logE << "eglGetConfigAttrib() failed" << std::endl;
      exit (1);
   }

   // The X window visual must match the EGL config
   visTemplate.visualid = vid;
   visInfo = XGetVisualInfo (xDisplay, VisualIDMask, &visTemplate, &numVisuals);
   if (!visInfo) {
      logE << "Couldn't get X visual" << std::endl;
      exit (1);
   }

   // window attributes
   colormap = XCreateColormap (xDisplay, root, visInfo->visual, AllocNone);
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = colormap;
   attr.event_mask = StructureNotifyMask | ExposureMask | FocusChangeMask |
      PropertyChangeMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask |
      PointerMotionMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   xWindow = XCreateWindow (xDisplay, root, 0, 0, width, height,
                            0, visInfo->depth, InputOutput,
                            visInfo->visual, mask, &attr);
   logI << "Window ID: " << xWindow << " / 0x" << std::hex << xWindow
        << std::dec << std::endl;
   if (setenv ("WINDOWID", std::to_string (xWindow).c_str (), 1) < 0)
      SYS_ERROR ("setenv (WINDOWID)");

   {
      XClassHint classHint;
      classHint.res_name = (char *)opts.name;
      classHint.res_class = (char *)"Zutty";

      XWMHints wmHints;
      wmHints.input = true;
      wmHints.initial_state = NormalState;
      wmHints.flags = InputHint | StateHint;

      sizeHints.width = width;
      sizeHints.height = height;
      sizeHints.base_width = 2 * opts.border;
      sizeHints.base_height = 2 * opts.border;
      sizeHints.min_width = 2 * opts.border + px;
      sizeHints.min_height = 2 * opts.border + py;
      sizeHints.width_inc = px;
      sizeHints.height_inc = py;
      sizeHints.win_gravity = NorthWestGravity;
      sizeHints.flags = PSize | PMinSize | PBaseSize | PResizeInc | PWinGravity;

      XmbSetWMProperties (xDisplay, xWindow, name, name,
                          nullptr, 0, &sizeHints, &wmHints, &classHint);
   }

   {
      std::string title {name};
      setXWindowName (title);
      setXWindowIconName (title);
   }

   {
      // set NET_WM_PID to the process ID to link the window to the pid
      Atom _NET_WM_PID = XInternAtom (xDisplay, "_NET_WM_PID", false);
      pid_t pid = getpid ();
      XChangeProperty (xDisplay, xWindow, _NET_WM_PID, XA_CARDINAL,
                       32, PropModeReplace, (unsigned char *)&pid, 1);
   }

   {
      // set NET_WM_ICON to wm_icons [] (16x16 and 32x32 32-bit cardinal ARGB)
      Atom _NET_WM_ICON = XInternAtom (xDisplay, "_NET_WM_ICON", false);
      XChangeProperty (xDisplay, xWindow, _NET_WM_ICON, XA_CARDINAL,
                       32, PropModeReplace, (const unsigned char*) wm_icons,
                       sizeof (wm_icons) / sizeof (wm_icons [0]));
   }

   wmDeleteMessage = XInternAtom (xDisplay, "WM_DELETE_WINDOW", False);
   XSetWMProtocols (xDisplay, xWindow, &wmDeleteMessage, 1);

   eglBindAPI (EGL_OPENGL_ES_API);

   eglCtx = eglCreateContext (eglDpy, config, EGL_NO_CONTEXT, ctxAttrs);
   if (!eglCtx)
   {
      logE << "eglCreateContext failed" << std::endl;
      exit (1);
   }

   // test eglQueryContext()
   {
      EGLint val;
      eglQueryContext (eglDpy, eglCtx, EGL_CONTEXT_CLIENT_TYPE, &val);
      assert (val == EGL_OPENGL_ES_API);
   }

   eglSurface = eglCreateWindowSurface (eglDpy, config,
                                        (EGLNativeWindowType)xWindow, nullptr);
   if (! eglSurface) {
      logE << "eglCreateWindowSurface failed" << std::endl;
      exit (1);
   }

   // sanity checks
   {
      EGLint val;
      eglQuerySurface (eglDpy, eglSurface, EGL_WIDTH, &val);
      assert (val == width);
      eglQuerySurface (eglDpy, eglSurface, EGL_HEIGHT, &val);
      assert (val == height);
      assert (eglGetConfigAttrib (eglDpy, config, EGL_SURFACE_TYPE, &val));
      assert (val & EGL_WINDOW_BIT);
   }

   XFree (visInfo);

   setXCursor ();
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

   // validate against entries in /etc/shells
   char* permShell = getusershell ();
   while (permShell)
   {
      if (strcmp (progPath, permShell) == 0)
      {
         endusershell ();
         setenv ("SHELL", progPath, 1);
         return;
      }
      permShell = getusershell ();
   }
   endusershell ();

   // progPath is *not* one of the permitted user shells
   unsetenv ("SHELL");
}

static void
setArgv0 (char* argv0)
{
   const char* shbase = strrchr (opts.shell, '/');
   if (shbase)
      ++shbase;
   else
      shbase = opts.shell;

   if (opts.login)
   {
      argv0 [0] = '-';
      strncpy (argv0 + 1, shbase, PATH_MAX-2);
   }
   else
   {
      strncpy (argv0, shbase, PATH_MAX-1);
   }
}

static void
sighandler (int sig, siginfo_t* info, void* ucontext)
{
   if (sig == SIGCHLD)
   {
      waitpid (info->si_pid, nullptr, 0);
   }
}

static void
setupSignals ()
{
   // The SIGCHLD handler is required to detect that the child process
   // has quit; its handler ensures we won't create zombies.
   {
      struct sigaction sa {};
      sa.sa_sigaction = sighandler;
      sa.sa_flags = SA_SIGINFO | SA_RESTART | SA_NOCLDSTOP;
      if (sigaction (SIGCHLD, &sa, nullptr) < 0)
         SYS_ERROR ("can't install SIGCHLD handler: sigaction()");
   }

   // SIGINT and SIGQUIT might have inherited handlers if Zutty was launched
   // from an interactive Bash shell. Restore the default handlers to enable
   // normal functionality (e.g., terminate a program under Zutty with Ctrl-C);
   // see bash(1) section SIGNALS.
   {
      struct sigaction sa {};
      sa.sa_handler = SIG_DFL;
      sa.sa_flags = 0;
      if (sigaction (SIGINT, &sa, nullptr) < 0)
         SYS_ERROR ("can't reset SIGINT handler to SIG_DFL: sigaction()");
      if (sigaction (SIGQUIT, &sa, nullptr) < 0)
         SYS_ERROR ("can't reset SIGQUIT handler to SIG_DFL: sigaction()");
   }
}

static int
startShell (const char* execPath, const char* const argv[])
{
   int ptyFd;
   pid_t pid;

   pid = zutty::pty_fork (ptyFd, opts.nCols, opts.nRows);

   if (pid < 0)
   {
      SYS_ERROR ("fork");
   }
   else if (pid == 0) // child:
   {
      // N.B.: DISPLAY and WINDOWID are inherited from parent
      // environment (set elsewhere)
      if (setenv ("TERM", "xterm-256color", 1) < 0)
         SYS_ERROR ("setenv TERM");

      if (setenv ("COLORTERM", "truecolor", 1) < 0)
         SYS_ERROR ("setenv COLORTERM");

      if (execvp (execPath, (char * const *) argv) < 0)
         SYS_ERROR ("execvp of ", execPath);
   }
   else // parent:
   {
      logT << "Shell subprocess started, pid: " << pid << std::endl;
   }

   return ptyFd;
}

static VtModifier
convertKeyState (KeySym ks, unsigned int state)
{
   VtModifier mod = VtModifier::none;
   if (state & ShiftMask)
      mod = mod | VtModifier::shift;
   if (state & ControlMask)
      mod = mod | VtModifier::control;
   if (state & Mod1Mask)
      mod = mod | VtModifier::alt;
   return mod;
}

static void
pasteCb (bool success, const std::string& content)
{
   if (success)
      vt->pasteSelection (content);
}

static bool
onKeyPress (XEvent& event, XIC& xic, int ptyFd)
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
         logE << "KeyPress event: buffer size " << sizeof (buffer)
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

   VtModifier mod = convertKeyState (ks, xkevt.state);

   // Special key combinations that are handled by Zutty itself:
   if (ks == XK_Page_Up && mod == VtModifier::shift)
   {
      vt->pageUp ();
      return false;
   }
   if (ks == XK_Page_Down && mod == VtModifier::shift)
   {
      vt->pageDown ();
      return false;
   }
   if (ks == XK_C && mod == VtModifier::shift_control)
   {
      selMgr->copySelection (selMgr->getClipboard (), selMgr->getPrimary ());
      return false;
   }
   if (ks == XK_V && mod == VtModifier::shift_control)
   {
      selMgr->getSelection (selMgr->getClipboard (), xkevt.time, pasteCb);
      return false;
   }
   if (ks == XK_Insert && mod == VtModifier::shift)
   {
      selMgr->getSelection (selMgr->getPrimary (), xkevt.time, pasteCb);
      return false;
   }
   if ((ks == XK_space || ks == XK_KP_Space) &&
       (xkevt.state & (Button1Mask | Button3Mask)))
   {
      vt->selectRectangularModeToggle ();
      return false;
   }

   if (! (xkevt.state & Mod2Mask)) // NumLock is off
   {
      // Certain X keysyms are mutated depending on the Shift state,
      // which is redundant as the Shift modifier state is observed as
      // well. Here we reverse these transforms.
      switch (ks)
      {
      case XK_KP_Decimal:    ks = XK_KP_Delete; break;
      case XK_KP_0:          ks = XK_KP_Insert; break;
      case XK_KP_1:          ks = XK_KP_End;    break;
      case XK_KP_2:          ks = XK_KP_Down;   break;
      case XK_KP_3:          ks = XK_KP_Next;   break;
      case XK_KP_4:          ks = XK_KP_Left;   break;
      case XK_KP_5:          ks = XK_KP_Begin;  break;
      case XK_KP_6:          ks = XK_KP_Right;  break;
      case XK_KP_7:          ks = XK_KP_Home;   break;
      case XK_KP_8:          ks = XK_KP_Up;     break;
      case XK_KP_9:          ks = XK_KP_Prior;  break;
      default:
         break;
      }
   }

   if (XFilterEvent (&event, xkevt.window))
      return false;
   if (ks == XK_Num_Lock)
      return false;

   switch (ks)
   {
#define KEYSEND(XKey, VtKey)                    \
      case XKey:                                \
         logT << "Key: " #XKey << std::endl;    \
         vt->writePty (VtKey, mod, true);       \
         return false

      KEYSEND (XK_0,                Key::K0);
      KEYSEND (XK_1,                Key::K1);
      KEYSEND (XK_2,                Key::K2);
      KEYSEND (XK_3,                Key::K3);
      KEYSEND (XK_4,                Key::K4);
      KEYSEND (XK_5,                Key::K5);
      KEYSEND (XK_6,                Key::K6);
      KEYSEND (XK_7,                Key::K7);
      KEYSEND (XK_8,                Key::K8);
      KEYSEND (XK_9,                Key::K9);
      KEYSEND (XK_space,            Key::Space);
      KEYSEND (XK_Return,           Key::Return);
      KEYSEND (XK_BackSpace,        Key::Backspace);
      KEYSEND (XK_Tab,              Key::Tab);
      KEYSEND (XK_ISO_Left_Tab,     Key::Tab);
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
      KEYSEND (XK_grave,            Key::Backtick);
      KEYSEND (XK_asciitilde,       Key::Tilde);
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
      KEYSEND (XK_KP_Page_Up,       Key::KP_PageUp);
      KEYSEND (XK_KP_Page_Down,     Key::KP_PageDown);
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

      // Ignore some funky keys that cause problems on non-US keyboards
      KEYIGN (XK_Menu);
      KEYIGN (XK_ISO_Lock);
      KEYIGN (XK_ISO_Level2_Latch);
      KEYIGN (XK_ISO_Level3_Shift);  // AltGr
      KEYIGN (XK_ISO_Level3_Latch);
      KEYIGN (XK_ISO_Level3_Lock);
      KEYIGN (XK_ISO_Level5_Shift);
      KEYIGN (XK_ISO_Level5_Latch);
      KEYIGN (XK_ISO_Level5_Lock);
      KEYIGN (XK_ISO_Group_Shift);
      KEYIGN (XK_ISO_Group_Latch);
      KEYIGN (XK_ISO_Group_Lock);
      KEYIGN (XK_ISO_Next_Group);
      KEYIGN (XK_ISO_Next_Group_Lock);
      KEYIGN (XK_ISO_Prev_Group);
      KEYIGN (XK_ISO_Prev_Group_Lock);
      KEYIGN (XK_ISO_First_Group);
      KEYIGN (XK_ISO_First_Group_Lock);
      KEYIGN (XK_ISO_Last_Group);
      KEYIGN (XK_ISO_Last_Group_Lock);

#undef KEYIGN

   default:
      if (nbytes > 1)
      {
         if (vt->writePty (buffer, true) < nbytes)
            return true;
      }
      else
      {
         if (vt->writePty (buffer [0], mod, true) < 1)
            return true;
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
      switch (button)
      {
      case 1: cb = 0; break;
      case 2: cb = 1; break;
      case 3: cb = 2; break;
      case 4: cb = 64; break;   // wheel up
      case 5: cb = 65; break;   // wheel down
      case 6: cb = 66; break;   // wheel left
      case 7: cb = 67; break;   // wheel right
      case 8: cb = 128; break;
      case 9: cb = 129; break;
      case 10: cb = 130; break;
      case 11: cb = 131; break;
      }
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
      using zutty::Utf8Encoder;
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
   out_cx = std::max (0, (px - opts.border - 1) / fontpk->getPx ()) + 1;
   out_cy = std::max (0, (py - opts.border - 1) / fontpk->getPy ()) + 1;
}

static inline void
onButtonPressMouseProto (XButtonEvent& xbevt,
                         const MouseTrackingState& mouseTrk)
{
   uint16_t cx, cy;

   if (xbevt.button > 11)
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
      {
         selMgr->setSelection (selMgr->getPrimary (), xbevt.time, utf8_sel);
         if (opts.autoCopyMode)
            selMgr->copySelection (selMgr->getClipboard (),
                                   selMgr->getPrimary ());
      }
   }
   break;
   case 2:
      selMgr->getSelection (selMgr->getPrimary (), xbevt.time, pasteCb);
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
x11Event (XEvent& event, XIC& xic, int ptyFd, bool& destroyed, bool& holdPtyIn)
{
   static bool exposed = false;
   bool redraw = false;
   destroyed = false;

   switch (event.type) {
   case Expose:
      exposed = true;
      redraw = true;
      break;
   case ClientMessage:
      if ((unsigned long) event.xclient.data.l [0] == wmDeleteMessage)
      {
         logT << "WM Delete message" << std::endl;
         destroyed = true;
         return true;
      }
      else
      {
         logT << "ClientMessage" << std::endl;
      }
      break;
   case ConfigureNotify:
      vt->resize (event.xconfigure.width, event.xconfigure.height);
      if (sizeHints.width != event.xconfigure.width ||
          sizeHints.height != event.xconfigure.height)
      {
         sizeHints.width = event.xconfigure.width;
         sizeHints.height = event.xconfigure.height;
         XSetWMNormalHints (xDisplay, xWindow, &sizeHints);
      }
      redraw = true;
      break;
   case ReparentNotify:
      logT << "ReparentNotify" << std::endl;
      redraw = true;
      break;
   case MappingNotify:
      logT << "MappingNotify" << std::endl;
      break;
   case MapNotify:
      logT << "MapNotify" << std::endl;
      break;
   case UnmapNotify:
      logT << "UnmapNotify" << std::endl;
      break;
   case DestroyNotify:
      logT << "DestroyNotify" << std::endl;
      destroyed = true;
      return true;
   case KeyPress:
      return onKeyPress (event, xic, ptyFd);
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
      if (event.xselectionclear.selection == selMgr->getPrimary ())
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
      logT << "X event.type = " << event.type << std::endl;
      break;
   }

   if (exposed && redraw) {
      vt->redraw ();
   }

   return false;
}

static bool
eventLoop (XIC& xic, int ptyFd)
{
   int x11Fd = XConnectionNumber (xDisplay);
   logT << "x11Fd = " << x11Fd << std::endl;
   logT << "ptyFd = " << ptyFd << std::endl;

   struct pollfd pollset [] = {
      {ptyFd, POLLIN, 0},
      {x11Fd, POLLIN, 0},
   };

   bool holdPtyIn = false;
   while (1)
   {
      pollset [0].fd = holdPtyIn ? -ptyFd : ptyFd;
      if (poll (pollset, 2, -1) < 0)
      {
         if (errno == EINTR)
            continue;
         else
            return false;
      }

      if (pollset [0].revents & (POLLIN | POLLHUP))
         if (vt->readPty ())
            return false;

      if (pollset [1].revents & POLLIN)
         while (XPending (xDisplay))
         {
            XEvent event;
            bool destroyed = false;

            XNextEvent (xDisplay, &event);
            if (x11Event (event, xic, ptyFd, destroyed, holdPtyIn))
               return destroyed;
         }
   }
}

static void
handleOsc (int cmd, const std::string& arg)
{
   switch (cmd)
   {
   case 0: // Change Icon Name & Window Title
      setXWindowName (arg);
      setXWindowIconName (arg);
      break;
   case 1: // Change Icon Name
      setXWindowIconName (arg);
      break;
   case 2: // Change Window Title
      setXWindowName (arg);
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
      std::string pc = arg.substr (0, p);
      std::string pd = arg.substr (p + 1);
      logT << "OSC 52: pc='" << pc << "', pd='" << pd << "'" << std::endl;

      std::vector <Atom> targets {};
      if (pc == "" || pc.find ('s') != std::string::npos)
      {
         targets.push_back (selMgr->getPrimary ());
         targets.push_back (selMgr->getClipboard ());
      }
      else
      {
         for (auto c: pc)
            switch (c)
            {
            case 'p': targets.push_back (selMgr->getPrimary ()); break;
            case 'c': targets.push_back (selMgr->getClipboard ()); break;
            }
      }

      if (pd == "?")
      {
         // Iterate through targets via callback in continuation-passing style
         auto it = targets.begin ();
         auto iend = targets.end ();
         SelectionManager::PasteCallbackFn getSelectionCb =
            [&] (bool success, const std::string& content)
            {
               if (success)
               {
                  std::ostringstream oss;
                  oss << "\e]52;;" << zutty::base64::encode (content) << "\e\\";
                  vt->writePty (oss.str ().c_str ());
               }
               else if (++it != iend)
               {
                  selMgr->getSelection (*it, CurrentTime,
                                        std::move (getSelectionCb));
               }
            };
         if (it != iend)
            selMgr->getSelection (*it, CurrentTime, std::move (getSelectionCb));
      }
      else
      {
         for (const auto& target: targets)
         {
            std::string content = zutty::base64::decode (pd);
            selMgr->setSelection (target, CurrentTime, content);
         }
      }
   }
      break;
   default:
      logU << "unhandled OSC: '" << cmd << ";" << arg << "'" << std::endl;
      break;
   }
}

static void
printGLInfo (EGLDisplay eglDpy)
{
   std::cout << "\nEGL_VERSION     = " << eglQueryString (eglDpy, EGL_VERSION)
             << "\nEGL_VENDOR      = " << eglQueryString (eglDpy, EGL_VENDOR)
             << "\nEGL_EXTENSIONS  = " << eglQueryString (eglDpy, EGL_EXTENSIONS)
             << "\nEGL_CLIENT_APIS = " << eglQueryString (eglDpy, EGL_CLIENT_APIS)
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

static int
handleXError (Display* dpy, XErrorEvent* ev)
{
   logE << "Exiting on received X error event:" << std::endl;
   XmuPrintDefaultErrorMessage(dpy, ev, stdout);
   fflush (stdout);

   renderer = nullptr; // ~Renderer () shuts down renderer thread
   exit (1);
   return 0;
}

static int
handleXIOError (Display* dpy)
{
   int err = errno;
   logE << "Fatal IO error " << err << " (" << strerror (err)
        << ") on X server " << DisplayString (dpy) << std::endl;

   renderer = nullptr; // ~Renderer () shuts down renderer thread
   exit (1);
   return 0;
}

int
main (int argc, char* argv[])
{
   EGLSurface eglSurface;
   EGLContext eglCtx;
   EGLDisplay eglDpy;
   EGLint eglMajor, eglMinor;
   XIC xic = nullptr;
   XIM xim;
   XIMStyles* ximStyles;
   XIMStyle ximStyle = 0;
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
      std::cout << "Error: couldn't initialize XLib for multithreaded use"
                << std::endl;
      return -1;
   }

   XSetErrorHandler(handleXError);
   XSetIOErrorHandler(handleXIOError);

   opts.initialize (&argc, argv);
   if (!opts.display)
   {
      opts.handlePrintOpts ();
      std::cout << "Error: DISPLAY not set!" << std::endl;
      return -1;
   }

   xDisplay = XOpenDisplay (opts.display);
   if (!xDisplay)
   {
      opts.handlePrintOpts ();
      std::cout << "Error: couldn't open display '" << opts.display << "'"
                << std::endl;
      return -1;
   }
   opts.setDisplay (xDisplay);

   opts.parse ();

   if (opts.verbose)
      opts.printVersion ();

   if (setenv ("ZUTTY_VERSION", ZUTTY_VERSION, 1) < 0)
      SYS_ERROR ("setenv (ZUTTY_VERSION)");

   char argv0 [PATH_MAX];
   char progPath [PATH_MAX];
   char* defaultShArgv [] = { argv0, nullptr };
   char** shArgv = defaultShArgv;
   if (argc > 2 && strcmp (argv [1], "-e") == 0)
   {
      shArgv = argv + 2;
      if (opts.titleSource != zutty::OptionSource::CmdLine)
         opts.title = argv [2];
      strncpy (progPath, argv [2], PATH_MAX-1);
   }
   else if (argc == 2)
   {
      setArgv0 (argv0);
      strncpy (progPath, argv [1], PATH_MAX-1);
      validateShell (progPath);
   }
   else
   {
      setArgv0 (argv0);
      strncpy (progPath, opts.shell, PATH_MAX-1);
      validateShell (progPath);
   }

   eglDpy = eglGetDisplay ((EGLNativeDisplayType)xDisplay);
   if (!eglDpy)
   {
      logE << "eglGetDisplay() failed" << std::endl;
      return -1;
   }

   if (!eglInitialize (eglDpy, &eglMajor, &eglMinor))
   {
      logE << "eglInitialize() failed" << std::endl;
      return -1;
   }

   xim = XOpenIM (xDisplay, nullptr, nullptr, nullptr);
   if (xim == nullptr)
   {
      logW << "XOpenIM failed" << std::endl;
   }

   if (xim)
   {
      imvalret = XGetIMValues (xim, XNQueryInputStyle, &ximStyles, nullptr);
      if (imvalret != nullptr || ximStyles == nullptr)
      {
         logW << "No styles supported by input method" << std::endl;
      }

      if (ximStyles) {
         ximStyle = 0;
         for (i = 0;  i < ximStyles->count_styles;  i++)
         {
            if (ximStyles->supported_styles [i] ==
                (XIMPreeditNothing | XIMStatusNothing))
            {
               ximStyle = ximStyles->supported_styles [i];
               break;
            }
         }

         if (ximStyle == 0)
         {
            logW << "Insufficient input method support" << std::endl;
         }
         XFree (ximStyles);
      }
   }

   fontpk = std::make_unique <Fontpack> (opts.fontpath, opts.fontname,
                                         opts.dwfontname);

   int winWidth = 2 * opts.border + opts.nCols * fontpk->getPx ();
   int winHeight = 2 * opts.border + opts.nRows * fontpk->getPy ();

   makeXWindow (opts.title,
                winWidth, winHeight, fontpk->getPx (), fontpk->getPy (),
                eglDpy, eglCtx, eglSurface);

   XMapWindow (xDisplay, xWindow);

   if (xim && ximStyle)
   {
      xic = XCreateIC (xim, XNInputStyle, ximStyle,
                       XNClientWindow, xWindow, XNFocusWindow, xWindow,
                       nullptr);

      if (xic == nullptr)
      {
         logW << "XCreateIC failed, compose key won't work" << std::endl;
      }
   }

   if (!eglMakeCurrent(eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
   {
      logE << "eglMakeCurrent() failed" << std::endl;
      return -1;
   }

   selMgr = std::make_unique <SelectionManager> (xDisplay, xWindow);

   renderer = std::make_unique <Renderer> (
      [eglDpy, eglSurface, eglCtx] ()
      {
         if (!eglMakeCurrent (eglDpy, eglSurface, eglSurface, eglCtx))
            throw std::runtime_error ("Error: eglMakeCurrent() failed");
         if (opts.glinfo)
            printGLInfo (eglDpy);
      },
      [eglDpy, eglSurface] ()
      {
         eglSwapBuffers (eglDpy, eglSurface);
      },
      fontpk.get ());

   setupSignals ();
   int ptyFd = startShell (progPath, shArgv);
   vt = std::make_unique <Vterm> (fontpk->getPx (), fontpk->getPy (),
                                  winWidth, winHeight, ptyFd);
   vt->setRefreshHandler ([] (const zutty::Frame& f) { renderer->update (f); });
   vt->setOscHandler ([] (int cmd, const std::string& arg)
                      { handleOsc (cmd, arg); });
   vt->setBellHandler ([] () { XBell (xDisplay, 0); });

   // We might not get a ConfigureNotify event when the window first appears:
   vt->resize (winWidth, winHeight);

   bool destroyed = eventLoop (xic, ptyFd);

   renderer = nullptr; // ~Renderer () shuts down renderer thread

   eglDestroyContext (eglDpy, eglCtx);
   eglDestroySurface (eglDpy, eglSurface);
   eglTerminate (eglDpy);

   if (! destroyed)
      XDestroyWindow (xDisplay, xWindow);
   XCloseDisplay (xDisplay);

   return 0;
}
