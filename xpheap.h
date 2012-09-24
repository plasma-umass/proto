// -*- C++ -*-

/*
  Copyright (c) 2011, University of Massachusetts Amherst.

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
 * @file   xpheap.h
 * @brief  A heap optimized to reduce the likelihood of false sharing.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XPHEAP_H_
#define _XPHEAP_H_

#include "xdefines.h"
#include "sassert.h"
#include "ansiwrapper.h"
#include "kingsleyheap.h"
#include "adapt.h"
#include "sllist.h"
#include "dllist.h"
#include "sanitycheckheap.h"
#include "zoneheap.h"
#include "objectheader.h"

#include "spinlock.h"

template <class SourceHeap>
class NewSourceHeap : public SourceHeap {
public:
  void * malloc (size_t sz) {

    void * ptr = SourceHeap::malloc (sz + sizeof(objectHeader));
    if (!ptr) {
      return NULL;
    }
   
    //fprintf(stderr, "sz is %x - %d\n", sz, sz); 
#if 0 // ALIGN_TO_PAGE
    if (sz >= 4096 - sizeof(objectHeader)) {
      ptr = (void *) (((((size_t) ptr) + 4095) & ~4095) - sizeof(objectHeader));
    }
#endif
    objectHeader * o = new (ptr) objectHeader (sz);
    void * newptr = getPointer(o);

    assert (getSize(newptr) >= sz);
    return newptr;
  }
  void free (void * ptr) {
    SourceHeap::free ((void *) getObject(ptr));
  }
  size_t getSize (void * ptr) {
    objectHeader * o = getObject(ptr);
    size_t sz = o->getSize();
    if (sz == 0) {
      PRFATAL ("Object size error, can't be 0");
    }
    return sz;
  }
private:

  static objectHeader * getObject (void * ptr) {
    objectHeader * o = (objectHeader *) ptr;
    return (o - 1);
  }

  static void * getPointer (objectHeader * o) {
    return (void *) (o + 1);
  }
};

template <class SourceHeap, int Chunky>
class KingsleyStyleHeap :
  public 
  HL::ANSIWrapper<
  HL::StrictSegHeap<Kingsley::NUMBINS,
		    Kingsley::size2Class,
		    Kingsley::class2Size,
		    HL::AdaptHeap<HL::SLList, NewSourceHeap<SourceHeap> >,
		    NewSourceHeap<HL::ZoneHeap<SourceHeap, Chunky> > > >
{
private:

  typedef 
  HL::ANSIWrapper<
  HL::StrictSegHeap<Kingsley::NUMBINS,
		    Kingsley::size2Class,
		    Kingsley::class2Size,
		    HL::AdaptHeap<HL::SLList, NewSourceHeap<SourceHeap> >,
		    NewSourceHeap<HL::ZoneHeap<SourceHeap, Chunky> > > >
  SuperHeap;

public:
  KingsleyStyleHeap (void) {
  }

  void * malloc (size_t sz) {
    void * ptr = SuperHeap::malloc (sz);
    return ptr;
  }
};

template <int NumHeaps,
	  class TheHeapType>
class PerProcessHeap : public TheHeapType {
public:
  PerProcessHeap (void)
  {
  }

  void * malloc (int ind, size_t sz)
  {
    // Try to get memory from the local heap first.
    void * ptr = _heap[ind].malloc (sz);
    return ptr;
  }
 
  // Here, we will give one block of memory back to the originated process related heap. 
  void free (int ind, void * ptr)
  { 
    //int ind = getHeapId(ptr);
    if(ind >= NumHeaps) {
      PRERR("wrong free status\n");
    }
    _heap[ind].free (ptr);
    //fprintf(stderr, "now first word is %lx\n", *((unsigned long*)ptr));
  }

  void lock(int ind) {
	  _lock[ind].acquire();
  }

  void unlock(int ind) {
	  _lock[ind].release();
  }	

private:
  int getHeapId(void * ptr) {
    return random()%NumHeaps;
  }

  spinlock _lock[NumHeaps];
  TheHeapType _heap[NumHeaps];
};

#include "pageowner.h"

// Protect heap 
template <class SourceHeap>
class xpheap : public SourceHeap 
{
  typedef PerProcessHeap<xdefines::NUM_HEAPS, KingsleyStyleHeap<SourceHeap, xdefines::PHEAP_CHUNK> >
  SuperHeap;

public: 
  xpheap() {
    int  metasize = sizeof(SuperHeap);

    char * base;

    // Allocate a share page to hold all heap metadata so that all threads can access them
    // without any problem.
    base = (char *)MMAP_SHARED(metasize);
    if(base == NULL) {
      PRFATAL("Failed to allocate memory for heap metadata.");
    }
    fprintf(stderr, "xpheap with size %d base %p\n", metasize, base);
    _heap = new (base) SuperHeap;
  }

  void * malloc(int heapid, int size) {
    _heap->malloc(heapid, size);
  }

  void free(int heapid, void * ptr) {
    _heap->free(heapid, ptr);
  }

  size_t getSize (void * ptr) {
    return _heap->getSize (ptr);
  }
 
private:
  SuperHeap * _heap; 
};

#endif // _XPHEAP_H_

