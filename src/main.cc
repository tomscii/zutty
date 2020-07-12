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

#include "font.h"
#include "pty.h"
#include "renderer.h"
#include "vterm.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <poll.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using zutty::CharVdev;
using zutty::Vterm;
using zutty::Renderer;

static bool benchMode = false;

static const std::string fontpath = "/usr/share/fonts/X11/misc/";
static const std::string fontext = ".pcf.gz";
static std::string fontname = "9x18";

static int win_width, win_height;
static uint16_t geomCols = 80;
static uint16_t geomRows = 25;
static uint16_t borderPx = 2;
static const char* title = "zutty";

static std::unique_ptr <zutty::Font> priFont = nullptr;
static std::unique_ptr <zutty::Font> altFont = nullptr;
static std::unique_ptr <Renderer> renderer = nullptr;
static std::unique_ptr <Vterm> vt = nullptr;

//#define DEMO
#ifdef DEMO
static void
demo_draw (Vterm& vt)
{
   static uint32_t draw_count = 0;

   CharVdev::Cell * cells = vt.cells.get ();
   uint16_t nCols = vt.nCols;
   uint16_t nRows = vt.nRows;

   uint32_t nGlyphs = priFont->getSupportedCodes ().size ();
   if (nGlyphs > nRows * nCols)
      nGlyphs = nRows * nCols;
   for (uint32_t k = 0; k < nGlyphs; ++k)
   {
      cells [k].bold = (draw_count >> 3) & 1;
      cells [k].underline = (draw_count >> 4) & 1;
      cells [k].inverse = ((draw_count >> 5) & 3) == 3;
   }

#if 0
   for (int i = 0; i < 10; ++i)
   {
      uint16_t c1 = rand () % nCols;
      uint16_t r1 = rand () % nRows;
      uint16_t c2 = rand () % nCols;
      uint16_t r2 = rand () % nRows;

      std::swap (cells [r1 * nCols + c1], cells [r2 * nCols + c2]);
   }
#endif

   ++draw_count;
}

static void
demo_resize (Vterm& vt)
{
   CharVdev::Cell * cells = vt.cells.get ();
   uint16_t nCols = vt.nCols;
   uint16_t nRows = vt.nRows;

   const CharVdev::Cell * cellsEnd = & cells [nRows * nCols];

   CharVdev::Color fg = {255, 255, 255};
   CharVdev::Color bg = {0, 0, 0};
   uint16_t prev_uc = 0;
   const auto & allCodes = priFont->getSupportedCodes ();
   auto it = allCodes.begin ();
   const auto itEnd = allCodes.end ();
   for ( ; it != itEnd && cells < cellsEnd; ++it, ++cells)
   {
      if (prev_uc + 1 != *it)
      {
         bg.red = rand () % 128;
         bg.blue = rand () % 128;
         bg.green = rand () % 128;
      }
      prev_uc = *it;

      (* cells).uc_pt = *it;
      (* cells).bold = 1;
      (* cells).fg = fg;
      (* cells).bg = bg;
   }
   for ( ; cells < cellsEnd; ++cells)
   {
      (* cells).uc_pt = ' ';
      (* cells).bold = 0;
      (* cells).inverse = 0;
      (* cells).underline = 0;
      (* cells).fg = {0, 0, 0};
      (* cells).bg = {72, 48, 96};
   }
}
#endif

static void
draw ()
{
#ifdef DEMO
   demo_draw (* vt.get ());
#endif

   vt->redraw ();
}

/* new window size or exposure */
static void
resize (int width, int height)
{
   vt->resize (width, height);

#ifdef DEMO
   demo_resize (* vt.get ());
#endif
}

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
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask |
      FocusChangeMask;
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


int
startShell (uint16_t nCols, uint16_t nRows, const char* const argv[])
{
   int fdm;
   pid_t pid;
   struct winsize size;

   size.ws_col = nCols;
   size.ws_row = nRows;
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
x11Event (XEvent& event, int pty_fd, bool& destroyed)
{
   using Key = VtKey;

   static bool exposed = false;
   bool redraw = false;
   destroyed = false;

   switch (event.type) {
   case Expose:
      exposed = true;
      redraw = true;
      break;
   case ConfigureNotify:
      resize (event.xconfigure.width, event.xconfigure.height);
      redraw = true;
      break;
   case ReparentNotify:
      std::cout << "ReparentNotify" << std::endl;
      redraw = true;
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
   {
      char buffer[8];
      KeySym ks;
      XLookupString (&event.xkey, buffer, sizeof (buffer), &ks, nullptr);
      VtModifier mod = convertKeyState (ks, event.xkey.state);
      switch (ks)
      {
#define KEYSEND(XKey, VtKey)                                            \
         case XKey:                                                     \
         vt->writePty (VtKey, mod);                                     \
            break

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

#undef KEYSEND

#define KEYIGN(XKey)                                              \
      case XKey:                                                  \
         break

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
         if (vt->writePty (buffer[0], mod) < 1)
            return true;
         break;
      }
   }
   redraw = true;
   break;
   case KeyRelease:
      break;
   case FocusIn:
      vt->setHasFocus (true);
      break;
   case FocusOut:
      vt->setHasFocus (false);
      break;
   default:
      std::cout << "X event.type = " << event.type << std::endl;
      break;
   }

   if (exposed && redraw) {
      draw ();
   }

   return false;
}

static bool
eventLoop (Display *dpy, Window win, int pty_fd)
{
   int x11_fd = XConnectionNumber (dpy);
   std::cout << "x11_fd = " << x11_fd << std::endl;
   std::cout << "pty_fd = " << pty_fd << std::endl;

   struct pollfd pollset[] = {
      {pty_fd, POLLIN, 0},
      {x11_fd, POLLIN, 0},
   };

   while (1) {
      if (poll (pollset, 2, -1) < 0)
         return false;

      if (pollset[0].revents & POLLHUP)
         return false;

      if (pollset[0].revents & POLLIN)
         vt->readPty ();

      XEvent event;
      bool destroyed = false;
      if (pollset[1].revents & POLLIN)
         if (XCheckWindowEvent (dpy, win, 0xffffffff, &event))
            if (x11Event (event, pty_fd, destroyed))
               return destroyed;
   }
}

static void
usage ()
{
   std::cout << "Usage:\n"
             << "  -display <dpy_name>      set the display to run on\n"
             << "  -font <fontname>         font name to load (default: "
             << fontname << ")\n"
             << "  -geometry <COLS>x<ROWS>  set display geometry (default: "
             << geomCols << "x" << geomRows << ")\n"
             << "  -border <Pixels>         border width (default: "
             << borderPx << ")\n"
             << "  -title <TITLE>           set window title (default: "
             << title << ")\n"
             << "  -info                    display OpenGL renderer info\n"
             << "  -bench                   redraw continuously; report FPS"
             << std::endl;
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
main (int argc, char *argv[])
{
   Display * x_dpy;
   Window win;
   EGLSurface egl_surf;
   EGLContext egl_ctx;
   EGLDisplay egl_dpy;
   char * dpyName = nullptr;
   bool printInfo = false;
   EGLint egl_major, egl_minor;
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp (argv[i], "-display") == 0) {
         dpyName = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-font") == 0) {
         fontname = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-geometry") == 0) {
         std::stringstream iss (argv[i+1]);
         int cols, rows;
         char fill;
         iss >> cols >> fill >> rows;
         if (iss.fail () || fill != 'x' || cols < 1 || rows < 1)
         {
            std::cerr << "Error: -geometry: expected format <COLS>x<ROWS>"
                      << std::endl;
            return -1;
         }
         geomCols = cols;
         geomRows = rows;
         ++i;
      }
      else if (strcmp(argv[i], "-border") == 0) {
         std::stringstream iss (argv[i+1]);
         int bw;
         iss >> bw;
         if (iss.fail () || bw < 0 || bw > 32767)
         {
            std::cerr << "Error: -border: expected unsigned short"
                      << std::endl;
            return -1;
         }
         borderPx = bw;
         ++i;
      }
      else if (strcmp(argv[i], "-title") == 0) {
         title = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-info") == 0) {
         printInfo = true;
      }
      else if (strcmp(argv[i], "-bench") == 0) {
         benchMode = true;
      }
      else {
         usage ();
         return -1;
      }
   }

   if (! XInitThreads ())
   {
      std::cerr << "Error: couldn't initialize XLib for multithreaded use"
                << std::endl;
      return -1;
   }

   x_dpy = XOpenDisplay (dpyName);
   if (!x_dpy)
   {
      std::cerr << "Error: couldn't open display "
                << (dpyName ? dpyName : getenv ("DISPLAY"))
                << std::endl;
      return -1;
   }

   egl_dpy = eglGetDisplay ((EGLNativeDisplayType)x_dpy);
   if (!egl_dpy)
   {
      std::cerr << "Error: eglGetDisplay() failed" << std::endl;
      return -1;
   }

   if (!eglInitialize (egl_dpy, &egl_major, &egl_minor)) {
      std::cerr << "Error: eglInitialize() failed" << std::endl;
      return -1;
   }

   priFont = std::make_unique <zutty::Font> (fontpath + fontname + fontext);
   altFont = std::make_unique <zutty::Font> (fontpath + fontname + "B" + fontext,
                                             * priFont.get ());
   win_width = 2 * borderPx + geomCols * priFont->getPx ();
   win_height = 2 * borderPx + geomRows * priFont->getPy ();

   make_x_window (x_dpy, egl_dpy, title, win_width, win_height,
                  &win, &egl_ctx, &egl_surf);

   XMapWindow (x_dpy, win);

   if (!eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
   {
      std::cerr << "Error: eglMakeCurrent() failed" << std::endl;
      return -1;
   }

   renderer = std::make_unique <Renderer> (
      * priFont.get (),
      * altFont.get (),
      borderPx,
      [egl_dpy, egl_surf, egl_ctx, printInfo] ()
      {
         if (!eglMakeCurrent (egl_dpy, egl_surf, egl_surf, egl_ctx))
            throw std::runtime_error ("Error: eglMakeCurrent() failed");
         if (printInfo)
            printGLInfo (egl_dpy);
      },
      [egl_dpy, egl_surf] ()
      {
         eglSwapBuffers (egl_dpy, egl_surf);
      },
      benchMode);

   const char * const sh_argv[] = { "bash", nullptr };
   int pty_fd = startShell (geomCols, geomRows, sh_argv);

   vt = std::make_unique <Vterm> (priFont->getPx (), priFont->getPy (),
                                  win_width, win_height, borderPx, pty_fd);
   vt->setRefreshHandler ([] (const Frame& f) { renderer->update (f); });
   vt->setTitleHandler ([&] (const std::string& title)
                        { XStoreName (x_dpy, win, title.c_str ()); });

   /* Force initialization.
    * We might not get a ConfigureNotify event when the window first appears.
    */
   resize (win_width, win_height);

   bool destroyed = eventLoop (x_dpy, win, pty_fd);

   renderer = nullptr; // ~Renderer () shuts down renderer thread

   eglDestroyContext (egl_dpy, egl_ctx);
   eglDestroySurface (egl_dpy, egl_surf);
   eglTerminate (egl_dpy);

   if (! destroyed)
      XDestroyWindow (x_dpy, win);
   XCloseDisplay (x_dpy);

   return 0;
}
