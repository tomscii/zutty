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

/* The source code in this file is inspired by code samples in the book
 *   Advanced Programming in the UNIX Environment, 3rd Edition
 *   by W. Richard Stevens & Stephen A. Rago
 *   Addison-Wesley, 2013
 *
 * The original example code of the book is available from
 *   http://www.apuebook.com/code3e.html
 */

#include "log.h"
#include "pty.h"

#define _POSIX_C_SOURCE 200809L

#if defined(SOLARIS) /* Solaris 10 */
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 700
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>

#if defined(BSD) || defined(MACOS) || !defined(TIOCGWINSZ)
#include <sys/ioctl.h>
#endif

#if defined(SOLARIS)
#include <stropts.h>
#endif

namespace zutty
{
   int
   ptym_open (char* pts_name, int pts_namesz)
   {
      char* ptr;
      int fdm;

      if ((fdm = posix_openpt (O_RDWR)) < 0)
         SYS_ERROR ("can't open master pty: posix_openpt()");
      if (grantpt (fdm) < 0) // grant access to slave
         SYS_ERROR ("can't open master pty: grantpt()");
      if (unlockpt (fdm) < 0) // clear slave's lock flag
         SYS_ERROR ("can't open master pty: unlockpt()");
      if ((ptr = ptsname (fdm)) == nullptr) // get slave's name
         SYS_ERROR ("can't open master pty: ptsname()");

      // Return slave name, null-terminated to handle strlen(ptr) > pts_namesz.
      strncpy (pts_name, ptr, pts_namesz);
      pts_name [pts_namesz - 1] = '\0';
      return fdm;
   }

   int
   ptys_open (char* pts_name)
   {
      int fds = open (pts_name, O_RDWR);
      if (fds < 0)
         SYS_ERROR ("can't open slave pty: open()");

   #if defined(SOLARIS)
      // Check if stream is already set up by autopush facility.
      int setup;
      if ((setup = ioctl (fds, I_FIND, "ldterm")) < 0)
         SYS_ERROR ("can't open slave pty: ioctl(I_FIND, ldterm)");

      if (setup == 0)
      {
         if (ioctl (fds, I_PUSH, "ptem") < 0)
            SYS_ERROR ("can't open slave pty: ioctl(I_PUSH, ptem)");
         if (ioctl (fds, I_PUSH, "ldterm") < 0)
            SYS_ERROR ("can't open slave pty: ioctl(I_PUSH, ldterm)");
         if (ioctl (fds, I_PUSH, "ttcompat") < 0)
            SYS_ERROR ("can't open slave pty: ioctl(I_PUSH, ttcompat)");
      }
   #endif
      return fds;
   }

   pid_t
   pty_fork (int& o_ptyFd, int cols, int rows)
   {
      pid_t pid;
      char pts_name [20];
      int fdm = ptym_open (pts_name, sizeof (pts_name));

      pid = fork ();

      if (pid < 0)
      {
         return pid;
      }
      else if (pid == 0) // child process
      {
         if (setsid () < 0)
            SYS_ERROR ("setsid");

         // System V acquires controlling terminal on open ().
         int fds = ptys_open (pts_name);

         close (fdm); // all done with master in child

      #if defined(BSD)
         // TIOCSCTTY is the BSD way to acquire a controlling terminal.
         if (ioctl (fds, TIOCSCTTY, nullptr) < 0)
            SYS_ERROR ("TIOCSCTTY");
      #endif

         // Set the pty slave's window size.
         pty_resize (fds, cols, rows);

         // Slave becomes stdin/stdout/stderr of child.
         redirectFds (fds);

      #if defined(LINUX) || defined(MACOS)
         // Setup terminal attributes
         struct termios term;
         if (tcgetattr (STDIN_FILENO, &term) < 0)
            SYS_ERROR ("tcgetattr");
         term.c_iflag |= IUTF8;
         if (tcsetattr (STDIN_FILENO, TCSANOW, &term) < 0)
            SYS_ERROR ("tcsetattr");
      #endif
      }
      else // parent process
      {
         o_ptyFd = fdm;
      }
      return pid;
   }

   void pty_resize (int ptyFd, int cols, int rows)
   {
      struct winsize wsize {};
      wsize.ws_col = cols;
      wsize.ws_row = rows;
      if (ioctl (ptyFd, TIOCSWINSZ, &wsize) < 0)
         SYS_ERROR("TIOCSWINSZ on pty");
   }

} // namespace zutty
