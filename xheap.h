// -*- C++ -*-

/*
  Author: Emery Berger, http://www.cs.umass.edu/~emery
 
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

/*
 * @file   xheap.h
 * @brief  A basic bump pointer heap, used as a source for other heap layers.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */


#ifndef _XHEAP_H_
#define _XHEAP_H_

#include "xprotect.h"
#include "xdefines.h"
#include "memwrapper.h"
#include "xplock.h"

class xheap : public xprotect
{
  typedef xprotect parent;

public:

  xheap (void)
  {
	  char * base;

	  // Allocate a share page to hold all heap metadata.
    base = (char *)MMAP_SHARED(xdefines::PageSize);
    fprintf(stderr, "xheap, mapshared ptr %p\n", base);

	  // Put all "heap metadata" in this page.
    _position   = (char **)base;
    _remaining  = (size_t *)(base + 1 * sizeof(void *));
    _magic      = (size_t *)(base + 2 * sizeof(void *));
	  _lock       = new (base + 3*sizeof(void *)) xplock;
	
	  // Initialize the following content according the values of xpersist class.
    _start      = (char *)parent::base();
    _end        = _start + parent::size();
    *_position  = (char *)_start;
    *_remaining = parent::size();
    *_magic     = 0xCAFEBABE;
    
    PRINF("PROTECTEDHEAP:_start %p end is %p remaining %p-%x, position %p-%x. OFFSET %x\n", _start, _end, _remaining, *_remaining, _position, (int)*_position, (int)*_position-(int)_start);
  }

  // Do page alightment since we don't want one page is shared by different threads
  inline void * malloc (size_t sz) {
    sanityCheck();
    
    // Roud up the size to page aligned.
	  sz = xdefines::PageSize * ((sz + xdefines::PageSize - 1) / xdefines::PageSize);

	  _lock->lock();

    //PRINF (stderr, "%d : xheap malloc size %x, remainning %p-%x and position %p-%x: OFFSET %x\n", getpid(), sz, _remaining, *_remaining, _position, *_position, (int)*_position-(int)_start);
    if (*_remaining == 0 && *_remaining < sz) { 
      PRFATAL ("CRAP: remaining[%x], sz[%x] thread[%d]\n", *_remaining, sz, (int)pthread_self());
    }
   
    void * p = *_position;

    // Increment the bump pointer and drop the amount of memory.
    *_remaining -= sz;
    *_position += sz;

	  _lock->unlock();

    // Set pages owneship in this block.
    parent::setPagesOwner(p, sz);
 
    //PRWRN("%d: xheapmalloc %p with size %x, remainning %x\n", getpid(), p, sz, *_remaining);
    return p;
  }

  void initialize(void) {
    parent::initialize();
  }

  // These should never be used.
  inline void free (void * ptr) { sanityCheck(); }
  inline size_t getSize (void * ptr) { sanityCheck(); return 0; } // FIXME

private:

  void sanityCheck (void) {
    if (*_magic != 0xCAFEBABE) {
      PRERR("%d : WTF!\n", getpid());
      ::abort();
    }
  }

  /// The start of the heap area.
  volatile char *  _start;

  /// The end of the heap area.
  volatile char *  _end;

  /// Pointer to the current bump pointer.
  char **  _position;

  /// Pointer to the amount of memory remaining.
  size_t*  _remaining;

  size_t*  _magic;

  // We will use a lock to protect the allocation request from different threads.
  xplock* _lock;

};

#endif
