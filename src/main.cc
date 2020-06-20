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

#include "font.h"
#include "pty.h"
#include "renderer.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

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

static uint32_t draw_count = 0;

static std::unique_ptr <zutty::Font> priFont = nullptr;
static std::unique_ptr <zutty::Font> altFont = nullptr;
static std::unique_ptr <Renderer> renderer = nullptr;
static std::unique_ptr <Vterm> vt = nullptr;

static void
demo_draw (Vterm& vt)
{
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

static void
draw ()
{
   //demo_draw (* vt.get ());

   renderer->update (* vt.get ());
}

/* new window size or exposure */
static void
resize (int width, int height)
{
   vt->resize (width, height);

   //demo_resize (* vt.get ());
}

/*
 * Create an RGB, double-buffered X window.
 * Return the window and context handles.
 */
static void
make_x_window (Display * x_dpy, EGLDisplay egl_dpy,
               const char * name,
               int x, int y, int width, int height,
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
   attr.event_mask = StructureNotifyMask | ExposureMask |
      KeyPressMask | KeyReleaseMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow (x_dpy, root, 0, 0, width, height,
                        0, visInfo->depth, InputOutput,
                        visInfo->visual, mask, &attr);

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
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
      if (execvp (argv[0], (char * const *) argv) < 0)
         throw std::runtime_error (std::string ("can't execvp: ") + argv[0]);
   }

   return fdm;
}

static bool
sendChar (int pty_fd, uint8_t ch)
{
   if (write (pty_fd, &ch, 1) != 1)
   {
      std::cout << "write error" << std::endl;
      return true;
   }
   return false;
}

static bool
x11Event (XEvent& event, int pty_fd, bool& destroyed)
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
      char buffer[10];
      int code;
      code = XLookupKeysym (&event.xkey, 0);
      //std::cout << "KP code = " << code << std::endl;
      switch (code)
      {
      case XK_Escape:
         std::cout << "XK_Escape" << std::endl;
         return true;
      case XK_Return:
         std::cout << "XK_Return" << std::endl;
         if (sendChar (pty_fd, '\n'))
             return true;
         break;

#define KEYCASE(Key)                                                 \
         case Key:                                                   \
            std::cout << "KeyPress  : " << #Key << std::endl;        \
            break

      // Cursor control & motion
      KEYCASE (XK_Home);
      KEYCASE (XK_End);
      KEYCASE (XK_Left);
      KEYCASE (XK_Right);
      KEYCASE (XK_Up);
      KEYCASE (XK_Down);
      KEYCASE (XK_Page_Up);
      KEYCASE (XK_Page_Down);
      // TTY function keys
      KEYCASE (XK_BackSpace);
      KEYCASE (XK_Delete);
      KEYCASE (XK_Linefeed);
      KEYCASE (XK_Tab);
      // Modifiers
      KEYCASE (XK_Shift_L);
      KEYCASE (XK_Shift_R);
      KEYCASE (XK_Control_L);
      KEYCASE (XK_Control_R);
      KEYCASE (XK_Caps_Lock);
      KEYCASE (XK_Shift_Lock);
      KEYCASE (XK_Meta_L);
      KEYCASE (XK_Meta_R);
      KEYCASE (XK_Alt_L);
      KEYCASE (XK_Alt_R);
      KEYCASE (XK_Super_L);
      KEYCASE (XK_Super_R);
      KEYCASE (XK_Hyper_L);
      KEYCASE (XK_Hyper_R);
      default:
         XLookupString (&event.xkey, buffer, sizeof (buffer),
                        nullptr, nullptr);
         //std::cout << "buffer[0] = '" << buffer[0] << "'" << std::endl;
         if (sendChar (pty_fd, buffer[0]))
             return true;
         break;
      }
   }
   redraw = true;
   break;
   case KeyRelease:
   {
      int code;
      code = XLookupKeysym (&event.xkey, 0);
      //std::cout << "KR code = " << code << std::endl;
      switch (code)
      {
#undef KEYCASE
#define KEYCASE(Key)                                                 \
         case Key:                                                   \
            std::cout << "KeyRelease: " << #Key << std::endl;        \
            break
      // Modifiers
      KEYCASE (XK_Shift_L);
      KEYCASE (XK_Shift_R);
      KEYCASE (XK_Control_L);
      KEYCASE (XK_Control_R);
      KEYCASE (XK_Caps_Lock);
      KEYCASE (XK_Shift_Lock);
      KEYCASE (XK_Meta_L);
      KEYCASE (XK_Meta_R);
      KEYCASE (XK_Alt_L);
      KEYCASE (XK_Alt_R);
      KEYCASE (XK_Super_L);
      KEYCASE (XK_Super_R);
      KEYCASE (XK_Hyper_L);
      KEYCASE (XK_Hyper_R);
      default:
         break;
      }
   }
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
      {
         vt->readPty ();
         renderer->update (* vt.get ());
      }

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
   win_width = geomCols * priFont->getPx ();
   win_height = geomRows * priFont->getPy ();

   make_x_window (x_dpy, egl_dpy, "zutty", 0, 0, win_width, win_height,
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
                                  win_width, win_height, pty_fd);

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
