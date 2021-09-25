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

#include "options.h"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace zutty
{
// log streams that are runtime switchable via command line options
// [ -quiet = none; default = zlog only; -verbose = both ]
#define zlog                                    \
   if (opts.quiet) {;}                          \
   else std::cout

#define vlog                                    \
   if (opts.quiet || !opts.verbose) {;}         \
   else std::cout

// N.B.: Offset into __FILE__ to skip over path prefix of the source files
// as seen by the compiler. Since the build is run from build/ and all the
// sources are under src/, we need to skip the '../src/' prefix (7 chars).
// THIS IS LIKELY COMPILER DEPENDENT, IT WORKS ON LINUX & GCC BUT YMMV.
#define plog(Ostream,Prefix)                            \
   Ostream << Prefix << " [" << (& __FILE__ [7]) << ":" \
           << std::setw (3) << __LINE__ << "] "

#define logE      plog(zlog, "E") << "Error: "
#define logW      plog(zlog, "W") << "Warning: "
#define logU      plog(zlog, "W") << "(Unimplemented) "
#define logI      plog(vlog, "I")

// trace logs are only present (and consume CPU) if compiled in:
#ifdef DEBUG
   #define logT      plog(vlog, "T")
#else
   #define logT      false && std::cout
#endif // DEBUG

   inline void
   printArgs ()
   {
      zlog << std::endl;
   }

   template <typename T, typename... Args>
   inline void
   printArgs (T arg, Args... args)
   {
      zlog << arg;
      printArgs (args...);
   }

   void redirectFds (int fd);
   void restoreFds ();

#define logSysE(...)    logE; zutty::printArgs(__VA_ARGS__)
#define logSysW(...)    logW; zutty::printArgs(__VA_ARGS__)

#define SYS_ERROR(...)                                                  \
   do {                                                                 \
      const auto ec = errno;                                            \
      zutty::restoreFds ();                                             \
      logSysE(__VA_ARGS__, ": ", strerror(ec), " (errno=", ec, ")");    \
      exit(1);                                                          \
   } while (0);

#define SYS_WARN(...)                                                   \
   do {                                                                 \
      const auto ec = errno;                                            \
      logSysW(__VA_ARGS__, ": ", strerror(ec), " (errno=", ec, ")");    \
   } while (0);


   inline std::string
   dumpBuffer (const unsigned char* start, const unsigned char* end)
   {
      if (opts.quiet)
         return "";

      std::ostringstream os;
      int count = 0;
      os << "'";
      for (auto it = start; it != end; ++it)
      {
         switch (*it)
         {
         case '\a': os << "\\a"; break;
         case '\b': os << "\\b"; break;
         case '\e': os << "\\e"; break;
         case '\f': os << "\\f"; break;
         case '\n': os << "\\n"; break;
         case '\r': os << "\\r"; break;
         case '\t': os << "\\t"; break;
         case '\v': os << "\\v"; break;
         case '\x7f': os << "\\x7f"; break; // DEL
         default:
            if (*it < ' ' || *it >= 0x80)
               os << "\\x" << std::hex << std::setw(2) << std::setfill('0')
                  << (unsigned int)*it;
            else
               os << *it;
            break;
         }
         ++count;
      }
      if (count)
      {
         os << "' (" << count << " bytes)" << std::endl;
         return os.str ();
      }
      else
         return "";
   }

} // namespace zutty
