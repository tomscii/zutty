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

#include <string.h>
#include <unistd.h>

namespace zutty
{
   int origFds [3] = {0, 0, 0};
   int targetFds [3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};

   void saveFds ()
   {
      for (int i = 0; i < 3; ++i)
      {
         origFds [i] = dup (targetFds [i]);
         if (origFds [i] < 0)
            SYS_ERROR ("dup ", i);
      }
   }

   void
   restoreFds ()
   {
      for (int i = 0; i < 3; ++i)
      {
         if (origFds [i])
         {
            dup2 (origFds [i], targetFds [i]);
            close (origFds [i]);
         }
      }
   }

   void redirectFds (int fd)
   {
      saveFds ();

      for (int i = 0; i < 3; ++i)
         if (dup2 (fd, targetFds [i]) != targetFds [i])
            SYS_ERROR ("dup2 to ", i);

      if (fd != targetFds [0] && fd != targetFds [1] && fd != targetFds [2])
         close (fd);
   }

} // namespace zutty
