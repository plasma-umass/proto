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
#ifndef _MEMWRAPPER_H_
#define _MEMWRAPPER_H_
#include <sys/types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>

#include "internalheap.h" 

extern "C" {
  inline void * MMAP_SHARED(int size) {
    void * temp;
    temp = WRAP(mmap)(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(temp == MAP_FAILED) {
      PRFATAL("Couldn't do mmap");
    }
//    fprintf(stderr, "map the shared from temp %p size %d\n", temp, size);
    return temp;
  }

  inline void * MMAP_PRIVATE(int size) {
    void * temp;
    temp = WRAP(mmap)(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(temp == MAP_FAILED) {
      PRFATAL("Couldn't do mmap");
    }
    return temp;
  }

  inline void MUNMAP(void * ptr, int length) {
    WRAP(munmap)(ptr, length); 
  }

  #define MALLOC_SHARED(size) (InternalHeap::getInstance().malloc(size))
  #define FREE_SHARED(ptr) (InternalHeap::getInstance().free(ptr))
  #define MALLOC_PRIVATE(size) (PrivateHeap::malloc(size)) 
  #define FREE_PRIVATE(ptr) (PrivateHeap::free(ptr)) 
//  #define ALIGN_TO_PAGE(size) (size%4096 == 0 ? size:(size+4096)/4096*4096)
  #define ALIGN_TO_CACHELINE(size) (size%64 == 0 ? size:(size+64)/64*64)
};

#endif /* _MALLOC_H_ */
