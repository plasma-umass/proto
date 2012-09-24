/*
  Author: Tongping Liu, http://www.cs.umass.edu/~tonyliu
 
  Copyright (c) 2011-12 Tongping Liu, University of Massachusetts Amherst.

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
 * @file   darray.h
 * @brief  The dynamic array (per thread) is always expanding. 
 *         If the entries are not enough, a reallocMemory() will be triggered. 
 *         After a while, we will reclaim all entries at one time but we don't release those memory. 
 *         Althought it is good to slink the size of array, but we didn't do this now for performance reason.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <errno.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#endif

#include <stdlib.h>

#include "xdefines.h"
#include "privateheap.h"


template <int EntryNumb, int EntryUnitSize, bool Shared> 

class DArray {
public:

	DArray()
	{
    // Initialize it
		_start = NULL;
		_cur = 0;
    _totalEntryNumb = EntryNumb;

    if(Shared) {
      _flags = MAP_PRIVATE | MAP_ANONYMOUS;
    }
    else {
      _flags = MAP_SHARED | MAP_ANONYMOUS;
    }
    
    // Malloc a block of memory.
    reallocMemory(0, _totalEntryNumb);
    
    cleanup();
	}

  void reallocMemory(int origsize, int newsize) {
    assert(origsize != newsize);

    // Malloc a block of memory from the private heap.
    void * ptr;  
		ptr = mmap (NULL,
         			  newsize * EntryUnitSize,
         			  PROT_READ | PROT_WRITE,
                _flags,
                -1,
                0);

    if(ptr == NULL) {
      fprintf(stderr, "Memory allocation failed. Origize %d newsize %d\n", origsize, newsize);
      exit(-1);
    }

    // If we are expanding the memory, we have to 
    void * oldptr = _start;
       
    if(origsize != 0) {
      // Copy from the old place to the new place.
      memcpy(ptr, oldptr, origsize * EntryUnitSize);

      // Release the old memory
      munmap(oldptr, origsize * EntryUnitSize);
    }
    
    // Set the start address  
    _start = ptr; 

    return; 
  }

  void * getEntry(int index) {
    void * ptr;

    ptr =(void *)((intptr_t)_start + index * EntryUnitSize);

    return ptr;
  }

  int getEntriesNumb(void) {
    return _cur;
  }
  
	void * allocEntry(void) {
		void * entry = NULL;

		if(_cur >= _totalEntryNumb) {
      int oldsize = _totalEntryNumb; 

			// There is no enough entry now, re-allocate new entries now.
			fprintf(stderr, "NO enough page entry, now _cur %x, _totalEntryNumb %x!!!\n", _cur, _totalEntryNumb);
      
      // We always double the size if not enough.
      _totalEntryNumb *= 2;
      
      reallocMemory(oldsize, _totalEntryNumb);
		}

    return getEntry(_cur++);
  }

	void cleanup(void) {
		_cur = 0;
	}

private:
	// How many entries in total.
	int _totalEntryNumb;

	// Current index of entry that need to be allocated.
  int _flags;
	int _cur;
	
	void * _start;
};

#endif
