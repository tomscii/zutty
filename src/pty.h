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

#pragma once

#include <sys/types.h>
#include <unistd.h>

namespace zutty
{
   pid_t pty_fork (int& o_ptyFd, int cols, int rows);

   void pty_resize (int ptyFd, int cols, int rows);

} // namespace zutty
