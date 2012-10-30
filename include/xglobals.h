// -*- C++ -*-

/*
  Copyright (c) 2012-2013, University of Massachusetts Amherst.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   xglobals.h
 * @brief  Globals handling.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _XGLOBALS_H_
#define _XGLOBALS_H_

#include "xdefines.h"
#include "memwrapper.h"
#include "pageowner.h"
#include "xprotect.h"

#if defined(__APPLE__)
// We are going to use the Mach-O substitutes for _end, etc.,
// despite the strong admonition not to. Beware.
#include <mach-o/getsect.h>
#endif

extern "C" {
#if !defined(__APPLE__)
  extern char __data_start;
  extern int _end;
#endif

// Macros to align to the nearest page down and up, respectively.

#define PAGE_ALIGN_DOWN(x) (((size_t) (x)) & ~xdefines::PAGE_SIZE_MASK)
#define PAGE_ALIGN_UP(x) ((((size_t) (x)) + xdefines::PAGE_SIZE_MASK) & ~xdefines::PAGE_SIZE_MASK)

// Macros that define the start and end addresses of program-wide globals.
#if defined(__APPLE__)
#define GLOBALS_START  PAGE_ALIGN_DOWN(((size_t) get_etext() - 1))
#define GLOBALS_END    PAGE_ALIGN_UP(((size_t) get_end() - 1))
#else
#define GLOBALS_START  PAGE_ALIGN_DOWN((size_t) &__data_start)
#define GLOBALS_END    PAGE_ALIGN_UP(((size_t) &_end - 1))
#endif

#define GLOBALS_SIZE   (GLOBALS_END - GLOBALS_START)
}

/// @class xglobals
/// @brief Maps the globals region onto a persistent store.
class xglobals : public xprotect {
public:
  xglobals (void)
    : xprotect((void *) GLOBALS_START, (size_t) GLOBALS_SIZE)
  {
  }
};

#endif
