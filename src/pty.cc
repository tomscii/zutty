#include "pty.h"
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

#define MAXLINE 4096 /* max line length */

/*
 * Print a message and return to caller.
 * Caller specifies "errnoflag".
 */
static void
err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
   char buf[MAXLINE];

   vsnprintf(buf, MAXLINE-1, fmt, ap);
   if (errnoflag)
      snprintf(buf+strlen(buf), MAXLINE-strlen(buf)-1, ": %s",
               strerror(error));
   strcat(buf, "\n");
   fflush(stdout); /* in case stdout and stderr are the same */
   fputs(buf, stderr);
   fflush(NULL); /* flushes all stdio output streams */
}

/*
 * Fatal error related to a system call.
 * Print a message and terminate.
 */
void
err_sys(const char *fmt, ...)
{
   va_list ap;

   va_start(ap, fmt);
   err_doit(1, errno, fmt, ap);
   va_end(ap);
   exit(1);
}


#include <errno.h>
#include <fcntl.h>
#if defined(SOLARIS)
#include <stropts.h>
#endif
#include <termios.h>

int
ptym_open(char *pts_name, int pts_namesz)
{
   char *ptr;
   int fdm, err;

   if ((fdm = posix_openpt(O_RDWR)) < 0)
      return(-1);
   if (grantpt(fdm) < 0) /* grant access to slave */
      goto errout;
   if (unlockpt(fdm) < 0) /* clear slave's lock flag */
      goto errout;
   if ((ptr = ptsname(fdm)) == NULL) /* get slave's name */
      goto errout;

   /*
    * Return name of slave.  Null terminate to handle
    * case where strlen(ptr) > pts_namesz.
    */
   strncpy(pts_name, ptr, pts_namesz);
   pts_name[pts_namesz - 1] = '\0';
   return(fdm); /* return fd of master */
errout:
   err = errno;
   close(fdm);
   errno = err;
   return(-1);
}

int
ptys_open(char *pts_name)
{
   int fds;
#if defined(SOLARIS)
   int err, setup;
#endif

   if ((fds = open(pts_name, O_RDWR)) < 0)
      return(-1);

#if defined(SOLARIS)
   /*
    * Check if stream is already set up by autopush facility.
    */
   if ((setup = ioctl(fds, I_FIND, "ldterm")) < 0)
      goto errout;

   if (setup == 0) {
      if (ioctl(fds, I_PUSH, "ptem") < 0)
         goto errout;
      if (ioctl(fds, I_PUSH, "ldterm") < 0)
         goto errout;
      if (ioctl(fds, I_PUSH, "ttcompat") < 0) {
      errout:
         err = errno;
         close(fds);
         errno = err;
         return(-1);
      }
   }
#endif
   return(fds);
}

pid_t
pty_fork(int *ptrfdm, char *slave_name, int slave_namesz,
         const struct termios *slave_termios,
         const struct winsize *slave_winsize)
{
   int fdm, fds;
   pid_t pid;
   char pts_name[20];

   if ((fdm = ptym_open(pts_name, sizeof(pts_name))) < 0)
      err_sys("can't open master pty: %s, error %d", pts_name, fdm);

   if (slave_name != NULL) {
      /*
       * Return name of slave.  Null terminate to handle case
       * where strlen(pts_name) > slave_namesz.
       */
      strncpy(slave_name, pts_name, slave_namesz);
      slave_name[slave_namesz - 1] = '\0';
   }

   if ((pid = fork()) < 0) {
      return(-1);
   } else if (pid == 0) { /* child */
      if (setsid() < 0)
         err_sys("setsid error");

      /*
       * System V acquires controlling terminal on open().
       */
      if ((fds = ptys_open(pts_name)) < 0)
         err_sys("can't open slave pty");
      close(fdm); /* all done with master in child */

#if defined(BSD)
      /*
       * TIOCSCTTY is the BSD way to acquire a controlling terminal.
       */
      if (ioctl(fds, TIOCSCTTY, (char *)0) < 0)
         err_sys("TIOCSCTTY error");
#endif
      /*
       * Set slave's termios and window size.
       */
      if (slave_termios != NULL) {
         if (tcsetattr(fds, TCSANOW, slave_termios) < 0)
            err_sys("tcsetattr error on slave pty");
      }
      if (slave_winsize != NULL) {
         if (ioctl(fds, TIOCSWINSZ, slave_winsize) < 0)
            err_sys("TIOCSWINSZ error on slave pty");
      }

      /*
       * Slave becomes stdin/stdout/stderr of child.
       */
      if (dup2(fds, STDIN_FILENO) != STDIN_FILENO)
         err_sys("dup2 error to stdin");
      if (dup2(fds, STDOUT_FILENO) != STDOUT_FILENO)
         err_sys("dup2 error to stdout");
      if (dup2(fds, STDERR_FILENO) != STDERR_FILENO)
         err_sys("dup2 error to stderr");
      if (fds != STDIN_FILENO && fds != STDOUT_FILENO &&
          fds != STDERR_FILENO)
         close(fds);
      return(0); /* child returns 0 just like fork() */
   } else { /* parent */
      *ptrfdm = fdm; /* return fd of master */
      return(pid); /* parent returns pid of child */
   }
}
