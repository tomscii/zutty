#include "font.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <iostream>

static GLfloat view_rotx = 0.0, view_roty = 0.0;
static GLint attr_pos = 0, attr_vertexTexCoord = 1;

static const std::string fontpath = "/usr/share/fonts/X11/misc/";
static const std::string fontext = ".pcf.gz";
static std::string fontname = "9x18";

static font::Text txt;

static void
draw(void)
{
   static const GLfloat verts[4][2] = {
      { -1,  1 },
      {  1,  1 },
      { -1, -1 },
      {  1, -1 }
   };
   static const GLfloat texCoords[4][2] = {
      { 0, 0 },
      { 1, 0 },
      { 0, 1 },
      { 1, 1 }
   };

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glDisable(GL_CULL_FACE);
   glDisable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   txt.draw ();

   glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
   glVertexAttribPointer(attr_vertexTexCoord, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
   glEnableVertexAttribArray(attr_pos);
   glEnableVertexAttribArray(attr_vertexTexCoord);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

   glDisableVertexAttribArray(attr_pos);
   glDisableVertexAttribArray(attr_vertexTexCoord);
}


/* new window size or exposure */
static void
reshape(int width, int height)
{
   std::cout << "reshape: width=" << width << " height=" << height << std::endl;
   glViewport(0, 0, (GLint) width, (GLint) height);
}


static void
create_shaders(void)
{
   static const char *fragShaderText =
       "#version 300 es\n"
       "precision mediump float;\n"

       "in vec2 texCoord;\n"
       "uniform sampler2D atlas;\n"
       "uniform vec4 color;\n"
       "layout(location = 0) out vec4 outColor;\n"

       "void main() {\n"
       "    vec4 texcol = vec4(1.0, 1.0, 1.0, texture(atlas, texCoord).r);\n"
       "    outColor    = vec4(vec3(color.rgb), texcol.a);\n"
       "}\n";
   static const char *vertShaderText =
       "#version 300 es\n"
       "layout(location = 0) in vec2 pos;\n"
       "layout(location = 1) in vec2 vertexTexCoord;\n"
       "out vec2 texCoord;\n"

       "void main() {\n"
       "  texCoord = vertexTexCoord;\n"
       "  gl_Position = vec4(pos, 0.0, 1.0);\n"
       "}\n";
   GLuint fragShader, vertShader, program;
   GLint stat;

   fragShader = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(fragShader, 1, (const char **) &fragShaderText, nullptr);
   glCompileShader(fragShader);
   glGetShaderiv(fragShader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      std::cerr << "Error: fragment shader did not compile!" << std::endl;
      exit(1);
   }

   vertShader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertShader, 1, (const char **) &vertShaderText, nullptr);
   glCompileShader(vertShader);
   glGetShaderiv(vertShader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      std::cerr << "Error: vertex shader did not compile!" << std::endl;
      exit(1);
   }

   program = glCreateProgram();
   glAttachShader(program, fragShader);
   glAttachShader(program, vertShader);
   glLinkProgram(program);

   glGetProgramiv(program, GL_LINK_STATUS, &stat);
   if (!stat) {
      char log[1000];
      GLsizei len;
      glGetProgramInfoLog(program, 1000, &len, log);
      std::cerr << "Error: linking:\n%s" << log << std::endl;
      exit(1);
   }

   glUseProgram(program);

   if (1) {
      /* test setting attrib locations */
      glBindAttribLocation(program, attr_pos, "pos");
      glBindAttribLocation(program, attr_vertexTexCoord, "vertexTexCoord");
      glLinkProgram(program);  /* needed to put attribs into effect */
   }
   else
   {
      /* test automatic attrib locations */
      attr_pos = glGetAttribLocation(program, "pos");
      attr_vertexTexCoord = glGetAttribLocation(program, "vertexTexCoord");
   }

   std::cout << "Attrib pos at " << attr_pos << std::endl
             << "Attrib vertexTexCoord at " << attr_vertexTexCoord << std::endl;
}


static void
init(void)
{
#if 1 /* test code */
   typedef void (*proc)();
   proc p = eglGetProcAddress("glMapBufferOES");
   assert(p);
#endif

   glClearColor(0.4, 0.4, 0.4, 0.0);

   create_shaders();

   txt.init (fontpath + fontname + fontext);
}


/*
 * Create an RGB, double-buffered X window.
 * Return the window and context handles.
 */
static void
make_x_window(Display *x_dpy, EGLDisplay egl_dpy,
              const char *name,
              int x, int y, int width, int height,
              Window *winRet,
              EGLContext *ctxRet,
              EGLSurface *surfRet)
{
   static const EGLint attribs[] = {
      EGL_RED_SIZE, 1,
      EGL_GREEN_SIZE, 1,
      EGL_BLUE_SIZE, 1,
      EGL_DEPTH_SIZE, 1,
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

   scrnum = DefaultScreen( x_dpy );
   root = RootWindow( x_dpy, scrnum );

   if (!eglChooseConfig( egl_dpy, attribs, &config, 1, &num_configs)) {
      std::cerr << "Error: couldn't get an EGL visual config" << std::endl;
      exit(1);
   }

   assert(config);
   assert(num_configs > 0);

   if (!eglGetConfigAttrib(egl_dpy, config, EGL_NATIVE_VISUAL_ID, &vid)) {
      std::cerr << "Error: eglGetConfigAttrib() failed" << std::endl;
      exit(1);
   }

   /* The X window visual must match the EGL config */
   visTemplate.visualid = vid;
   visInfo = XGetVisualInfo(x_dpy, VisualIDMask, &visTemplate, &num_visuals);
   if (!visInfo) {
      std::cerr << "Error: couldn't get X visual" << std::endl;
      exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( x_dpy, root, visInfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( x_dpy, root, 0, 0, width, height,
		        0, visInfo->depth, InputOutput,
		        visInfo->visual, mask, &attr );

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(x_dpy, win, &sizehints);
      XSetStandardProperties(x_dpy, win, name, name,
                              None, (char **)nullptr, 0, &sizehints);
   }

   eglBindAPI(EGL_OPENGL_ES_API);

   ctx = eglCreateContext(egl_dpy, config, EGL_NO_CONTEXT, ctx_attribs );
   if (!ctx) {
      std::cerr << "Error: eglCreateContext failed" << std::endl;
      exit(1);
   }

   /* test eglQueryContext() */
   {
      EGLint val;
      eglQueryContext(egl_dpy, ctx, EGL_CONTEXT_CLIENT_VERSION, &val);
      assert(val == 2);
   }

   *surfRet = eglCreateWindowSurface(egl_dpy, config,
                                     (EGLNativeWindowType)win, nullptr);
   if (!*surfRet) {
      std::cerr << "Error: eglCreateWindowSurface failed" << std::endl;
      exit(1);
   }

   /* sanity checks */
   {
      EGLint val;
      eglQuerySurface(egl_dpy, *surfRet, EGL_WIDTH, &val);
      assert(val == width);
      eglQuerySurface(egl_dpy, *surfRet, EGL_HEIGHT, &val);
      assert(val == height);
      assert(eglGetConfigAttrib(egl_dpy, config, EGL_SURFACE_TYPE, &val));
      assert(val & EGL_WINDOW_BIT);
   }

   XFree(visInfo);

   *winRet = win;
   *ctxRet = ctx;
}


static void
event_loop(Display *dpy, Window win,
           EGLDisplay egl_dpy, EGLSurface egl_surf)
{
    while (1) {
        int redraw = 0;
        XEvent event;

        XNextEvent(dpy, &event);

        switch (event.type) {
        case Expose:
            redraw = 1;
            break;
        case ConfigureNotify:
            reshape(event.xconfigure.width, event.xconfigure.height);
            break;
        case KeyPress:
        {
            char buffer[10];
            int code;
            code = XLookupKeysym(&event.xkey, 0);
            if (code == XK_Left) {
                view_roty += 5.0;
            }
            else if (code == XK_Right) {
                view_roty -= 5.0;
            }
            else if (code == XK_Up) {
                view_rotx += 5.0;
            }
            else if (code == XK_Down) {
                view_rotx -= 5.0;
            }
            else {
                XLookupString(&event.xkey, buffer, sizeof(buffer),
                              nullptr, nullptr);
                if (buffer[0] == 27) {
                    /* escape */
                    return;
                }
            }
        }
        redraw = 1;
        break;
        default:
            ; /*no-op*/
        }

        if (redraw) {
            draw();
            eglSwapBuffers(egl_dpy, egl_surf);
        }
    }
}


static void
usage(void)
{
   std::cout << "Usage:\n"
             << "  -display <displayname>  set the display to run on\n"
             << "  -font <fontname>        font name to load (default: "
             << fontname << ")\n"
             << "  -info                   display OpenGL renderer info"
             << std::endl;
}


int
main(int argc, char *argv[])
{
   const int winWidth = 300, winHeight = 300;
   Display *x_dpy;
   Window win;
   EGLSurface egl_surf;
   EGLContext egl_ctx;
   EGLDisplay egl_dpy;
   char *dpyName = NULL;
   GLboolean printInfo = GL_FALSE;
   EGLint egl_major, egl_minor;
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0) {
         dpyName = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-font") == 0) {
         fontname = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-info") == 0) {
         printInfo = GL_TRUE;
      }
      else {
         usage();
         return -1;
      }
   }

   x_dpy = XOpenDisplay(dpyName);
   if (!x_dpy) {
      std::cerr << "Error: couldn't open display "
                << (dpyName ? dpyName : getenv("DISPLAY"))
                << std::endl;
      return -1;
   }

   egl_dpy = eglGetDisplay((EGLNativeDisplayType)x_dpy);
   if (!egl_dpy) {
      std::cerr << "Error: eglGetDisplay() failed" << std::endl;
      return -1;
   }

   if (!eglInitialize(egl_dpy, &egl_major, &egl_minor)) {
      std::cerr << "Error: eglInitialize() failed" << std::endl;
      return -1;
   }

   if (printInfo) {
      std::cout << "\nEGL_VERSION     = " << eglQueryString(egl_dpy, EGL_VERSION)
                << "\nEGL_VENDOR      = " << eglQueryString(egl_dpy, EGL_VENDOR)
                << "\nEGL_EXTENSIONS  = " << eglQueryString(egl_dpy, EGL_EXTENSIONS)
                << "\nEGL_CLIENT_APIS = " << eglQueryString(egl_dpy, EGL_CLIENT_APIS)
                << std::endl;
   }

   make_x_window(x_dpy, egl_dpy,
                 "es2term", 0, 0, winWidth, winHeight,
                 &win, &egl_ctx, &egl_surf);

   XMapWindow(x_dpy, win);
   if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx)) {
      std::cerr << "Error: eglMakeCurrent() failed" << std::endl;
      return -1;
   }

   if (printInfo) {
      std::cout << "\nGL_RENDERER     = " << glGetString(GL_RENDERER)
                << "\nGL_VERSION      = " << glGetString(GL_VERSION)
                << "\nGL_VENDOR       = " << glGetString(GL_VENDOR)
                << "\nGL_EXTENSIONS   = " << glGetString(GL_EXTENSIONS)
                << std::endl;
   }

   init();

   /* Set initial projection/viewing transformation.
    * We can't be sure we'll get a ConfigureNotify event when the window
    * first appears.
    */
   reshape(winWidth, winHeight);

   event_loop(x_dpy, win, egl_dpy, egl_surf);

   eglDestroyContext(egl_dpy, egl_ctx);
   eglDestroySurface(egl_dpy, egl_surf);
   eglTerminate(egl_dpy);


   XDestroyWindow(x_dpy, win);
   XCloseDisplay(x_dpy);

   return 0;
}
