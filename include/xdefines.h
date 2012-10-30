// -*- C++ -*-

/*
  Copyright (c) 2007-8 Emery Berger, University of Massachusetts Amherst.

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
#ifndef _XDEFINES_H_
#define _XDEFINES_H_
#include <sys/types.h>
#include <syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>

#include "realfuncs.h"
#include "log.h"

extern "C" {
  typedef void * (threadFunction)(void *);
  #define RIP_ADDRESS 16
  #define CPU_CORES 8
  //#define CPU_CORES 8
};

class xdefines {
public:
  enum { MAX_THREADS = 4096 };
  enum { NUM_HEAPS = CPU_CORES }; // was 16
//  enum { PHEAP_SIZE = 1048576UL * 1200 }; // FIX ME 512 };
  enum { PHEAP_SIZE = 1048576UL * 1600 }; // FIX ME 512 };
  enum { PHEAP_CHUNK = PHEAP_SIZE/(CPU_CORES *2) };
  enum { FILE_BUFFER_SIZE = 40960UL };
//  enum { MAX_GLOBALS_SIZE = 1048576UL * 20 };
  enum { INTERNALHEAP_SIZE = 1048576UL * 100 }; // FIXME 10M 
  enum { PRIVATE_STACK_SIZE = 131072UL}; // FIXME 32page 
  enum { STACK_SIZE = 131072UL * 8}; // FIXME 32page*4 
  enum { PageSize = 4096UL };
  enum { PAGE_SIZE_MASK = (PageSize-1) };

#if defined(__i386__)
  enum { PHEAP_START_ADDR = 0x80000000 };
#elif defined(__x86_64__)
#error "No supported architecture!!"
#else
#error "No supported architecture!!"
#endif
  
// Define some starting address of heap and filemapping.
};

#endif
