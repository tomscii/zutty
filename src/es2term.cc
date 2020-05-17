#include "font.h"
#include "gl.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <assert.h>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const std::string fontpath = "/usr/share/fonts/X11/misc/";
static const std::string fontext = ".pcf.gz";
static std::string fontname = "9x18";
static font::Font fnt;

static GLuint computeProgram, drawProgram;
static GLuint atlas_texture = 0;
static GLuint output_texture = 0;
static GLfloat view_rotx = 0.0, view_roty = 0.0;
static GLint attr_pos = 0, attr_vertexTexCoord = 1;
static GLint compute_uni_glyphPixels, compute_uni_atlasPixels,
   compute_uni_winPixels;
static GLint draw_uni_winPixels;
static int win_width = 300, win_height = 300;

static void
draw(void)
{
   glUseProgram(computeProgram);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, output_texture);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, atlas_texture);
   glUniform2i(compute_uni_glyphPixels, fnt.getPx(), fnt.getPy());
   glUniform2f(compute_uni_atlasPixels,
               (GLfloat) fnt.getPx() * fnt.getNx(),
               (GLfloat) fnt.getPy() * fnt.getNy());
   glUniform2f(compute_uni_winPixels,
               (GLfloat) win_width, (GLfloat) win_height);
   glCheckError();

   glDispatchCompute(win_width, win_height, 1);
   glCheckError();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
   glCheckError();

   const GLfloat verts[4][2] = {
      { -1,  1 },
      {  1,  1 },
      { -1, -1 },
      {  1, -1 }
   };
   const GLfloat texCoords[4][2] = {
      { 0, 0 },
      { 1, 0 },
      { 0, 1 },
      { 1, 1 }
   };

   glUseProgram(drawProgram);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glDisable(GL_CULL_FACE);
   glDisable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, output_texture);

   glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
   glVertexAttribPointer(attr_vertexTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
                         texCoords);
   glEnableVertexAttribArray(attr_pos);
   glEnableVertexAttribArray(attr_vertexTexCoord);
   glUniform2f(draw_uni_winPixels, (GLfloat) win_width, (GLfloat) win_height);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

   glDisableVertexAttribArray(attr_pos);
   glDisableVertexAttribArray(attr_vertexTexCoord);
}


/* new window size or exposure */
static void
resize(int width, int height)
{
   win_width = width;
   win_height = height;
   std::cout << "resize: " << win_width << " x " << win_height << std::endl;
   glViewport(0, 0, (GLint) win_width, (GLint) win_height);

   glUseProgram(computeProgram);

   if (output_texture)
   {
      glDeleteTextures(1, &output_texture);
   }
   glGenTextures(1, &output_texture);
   std::cout << "output_texture=" << output_texture << std::endl;
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, output_texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, win_width, win_height);
   glBindImageTexture(0, output_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                      GL_RGBA32F);
   glCheckError();

   draw();
}


static void
create_shaders(void)
{
   static const char *computeShaderText =
      "#version 310 es\n"

      "layout(local_size_x = 1, local_size_y = 1) in;\n"
      "layout(rgba32f, binding = 0) writeonly highp uniform image2D imgOut;\n"
      "layout(binding = 1) uniform sampler2D atlas;\n"

      "uniform ivec2 glyphPixels;\n"
      "uniform vec2 atlasPixels;\n"
      "uniform vec2 winPixels;\n"

      "void main() { \n"
      "   vec3 color = vec3(0.85, 0.85, 0.85);\n"
      //"   ivec2 charPos = ivec2(26,0);\n" // character within atlas
      "   ivec2 charPos = ivec2(gl_GlobalInvocationID.xy) / glyphPixels;\n"
      "   ivec2 pxCoords = ivec2(gl_GlobalInvocationID.xy);\n"
      "   ivec2 txCoords = charPos * glyphPixels + pxCoords % glyphPixels;\n"
      "   vec2 texCoords = (vec2(txCoords) + vec2(0.5, 0.5)) / atlasPixels;\n"
      "   vec4 pixel = vec4(color * texture(atlas, texCoords).r, 1.0); \n"
      "   imageStore(imgOut, pxCoords, pixel);\n"
      "}\n";
   static const char *vertexShaderText =
      "#version 310 es\n"

      "layout(location = 0) in vec2 pos;\n"
      "layout(location = 1) in vec2 vertexTexCoord;\n"

      "out vec2 texCoord;\n"

      "void main() {\n"
      "  texCoord = vertexTexCoord;\n"
      "  gl_Position = vec4(pos, 0.0, 1.0);\n"
      "}\n";
   static const char *fragmentShaderText =
      "#version 310 es\n"

      "precision highp float;\n"

      "in vec2 texCoord;\n"

      "layout(rgba32f, binding = 0) readonly highp uniform image2D imgOut;\n"

      "uniform vec2 winPixels;\n"

      "layout(location = 0) out vec4 outColor;\n"

      "void main() {\n"
      "   outColor    = imageLoad(imgOut, ivec2(texCoord * winPixels));\n"
      "}\n";
   GLuint computeShader, fragmentShader, vertexShader;
   GLint stat;

   computeShader = glCreateShader(GL_COMPUTE_SHADER);
   glShaderSource(computeShader, 1, &computeShaderText, nullptr);
   glCompileShader(computeShader);
   glGetShaderiv(computeShader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      char log[1000];
      GLsizei len;
      glGetShaderInfoLog(computeShader, 1000, &len, log);
      std::cerr << "Error compiling compute shader:\n" << log << std::endl;
      exit(1);
   }

   fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(fragmentShader, 1, &fragmentShaderText, nullptr);
   glCompileShader(fragmentShader);
   glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      char log[1000];
      GLsizei len;
      glGetShaderInfoLog(fragmentShader, 1000, &len, log);
      std::cerr << "Error compiling fragment shader:\n" << log << std::endl;
      exit(1);
   }

   vertexShader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertexShader, 1, &vertexShaderText, nullptr);
   glCompileShader(vertexShader);
   glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      char log[1000];
      GLsizei len;
      glGetShaderInfoLog(vertexShader, 1000, &len, log);
      std::cerr << "Error compiling vertex shader:\n" << log << std::endl;
      exit(1);
   }

   computeProgram = glCreateProgram();
   glAttachShader(computeProgram, computeShader);
   glLinkProgram(computeProgram);

   glGetProgramiv(computeProgram, GL_LINK_STATUS, &stat);
   if (!stat) {
      char log[1000];
      GLsizei len;
      glGetProgramInfoLog(computeProgram, 1000, &len, log);
      std::cerr << "Error: linking:\n%s" << log << std::endl;
      exit(1);
   }

   drawProgram = glCreateProgram();
   glAttachShader(drawProgram, fragmentShader);
   glAttachShader(drawProgram, vertexShader);
   glLinkProgram(drawProgram);

   glGetProgramiv(drawProgram, GL_LINK_STATUS, &stat);
   if (!stat) {
      char log[1000];
      GLsizei len;
      glGetProgramInfoLog(drawProgram, 1000, &len, log);
      std::cerr << "Error: linking:\n%s" << log << std::endl;
      exit(1);
   }

   glUseProgram(computeProgram);

   compute_uni_glyphPixels = glGetUniformLocation(computeProgram, "glyphPixels");
   compute_uni_atlasPixels = glGetUniformLocation(computeProgram, "atlasPixels");
   compute_uni_winPixels = glGetUniformLocation(computeProgram, "winPixels");

   std::cout << "compute program:"
             << "\n  uniform glyphPixels at " << compute_uni_glyphPixels
             << "\n  uniform atlasPixels at " << compute_uni_atlasPixels
             << "\n  uniform winPixels at " << compute_uni_winPixels
             << std::endl;

   glUseProgram(drawProgram);

   attr_pos = glGetAttribLocation(drawProgram, "pos");
   attr_vertexTexCoord = glGetAttribLocation(drawProgram, "vertexTexCoord");
   draw_uni_winPixels = glGetUniformLocation(drawProgram, "winPixels");

   std::cout << "draw program:"
             << "\n  attrib pos at " << attr_pos
             << "\n  attrib vertextexcoord at " << attr_vertexTexCoord
             << "\n  uniform winPixels at " << draw_uni_winPixels
             << std::endl;
}


static void
init(void)
{
#if 1 /* test code */
   typedef void (*proc)();
   proc p = eglGetProcAddress("glMapBufferOES");
   assert(p);
#endif

   glClearColor(1.0, 1.0, 1.0, 0.0);

   create_shaders();

   fnt.init (fontpath + fontname + fontext, atlas_texture);
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

   win = XCreateWindow(x_dpy, root, 0, 0, width, height,
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
      XSetNormalHints(x_dpy, win, &sizehints);
      XSetStandardProperties(x_dpy, win, name, name,
                             None, nullptr, 0, &sizehints);
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
            resize(event.xconfigure.width, event.xconfigure.height);
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
                 "es2term", 0, 0, win_width, win_height,
                 &win, &egl_ctx, &egl_surf);

   XMapWindow(x_dpy, win);
   if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx))
   {
      std::cerr << "Error: eglMakeCurrent() failed" << std::endl;
      return -1;
   }

   if (printInfo)
   {
      std::cout << "\nGL_RENDERER     = " << glGetString(GL_RENDERER)
                << "\nGL_VERSION      = " << glGetString(GL_VERSION)
                << "\nGL_VENDOR       = " << glGetString(GL_VENDOR)
                << "\nGL_EXTENSIONS   = " << glGetString(GL_EXTENSIONS)
                << std::endl;
   }

   {
      int work_grp_cnt[3];
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
      std::cout << "max global (total) work group counts:"
                << " x=" << work_grp_cnt[0]
                << " y=" << work_grp_cnt[1]
                << " z=" << work_grp_cnt[2]
                << std::endl;

      int work_grp_size[3];
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
      glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
      std::cout << "max local (per-shader) work group sizes:"
                << " x=" << work_grp_size[0]
                << " y=" << work_grp_size[1]
                << " z=" << work_grp_size[2]
                << std::endl;

      int work_grp_inv;
      glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
      std::cout << "max local work group invocations: " << work_grp_inv
                << std::endl;
   }

   init();

   /* Force initialization.
    * We can't be sure we'll get a ConfigureNotify event when the window
    * first appears.
    */
   resize(win_width, win_height);

   event_loop(x_dpy, win, egl_dpy, egl_surf);

   eglDestroyContext(egl_dpy, egl_ctx);
   eglDestroySurface(egl_dpy, egl_surf);
   eglTerminate(egl_dpy);


   XDestroyWindow(x_dpy, win);
   XCloseDisplay(x_dpy);

   return 0;
}
