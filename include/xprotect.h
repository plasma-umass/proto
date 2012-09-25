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

/**
 * @class  xprotect
 * @brief  Handling the creation and trap on protected memory.
 *
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XPROTECT_H_
#define _XPROTECT_H_

#include <set>
#include <list>
#include <vector>

#if !defined(_WIN32)
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xdefines.h"
//#include "process.h"
#include "pageowner.h"

#define USING_FILE_BACKUP 0
class xprotect {
public:

  /// @arg startaddr: the optional starting address of the local memory.
  /// Globals will pass a startaddr
  xprotect (void * startaddr = 0, size_t startsize = 0)
  : _isProtected (false)
  {
    // Do some initialization
    if(startsize == 0) {
      _size = xdefines::PHEAP_SIZE;
      _startaddr = (void *) xdefines::PHEAP_START_ADDR;
      _isHeap = true;
    }
    else {
      // Normally, startaddr should be page aligned
      if(!isPageAligned(startaddr)) {
        printf("Global start address %p is not page aligned\n");
        ::abort();       
      }

      _size = calcPages(startaddr, startsize) * xdefines::PageSize;
      _startaddr = startaddr;
      _isHeap = false; 
    }
    _totalpages = _size/xdefines::PageSize;
    _endaddr = (void *) ((intptr_t)_startaddr + _size);

#ifdef USING_FILE_BACKUP    
    // Get a temporary file name (which had better not be NFS-mounted...).
    char backingFname[L_tmpnam];
    sprintf (backingFname, "protoMXXXXXX");
    int backingFd = mkstemp (backingFname);
    if (backingFd == -1) {
      printf("Failed to make persistent file.\n");
      ::abort();
    }
   
    // Set the files to the sizes of the desired object.
    if(ftruncate(backingFd, xdefines::PHEAP_SIZE)) { 
      printf("Mysterious error with ftruncate.\n");
      ::abort();
    }
   
    // Get rid of the files when we exit.
    unlink (backingFname);
#endif

    // Set the attributes of memory region.
    void * temp = NULL;
    void * ptr = NULL;
    if(!_isHeap) {
      // It is the globals, save the original contents of globals  
      // Get a block of private memory to hold it.
      temp = MMAP_PRIVATE(_size);
      if(temp == MAP_FAILED) {
        printf("Can't allocate to hold globals.\n");
        ::abort();
      }

      // Create a copy of all globals initialized content.
      memcpy(temp, _startaddr, startsize);
    }

    // We have to mmaped to specified address
    ptr = WRAP(mmap)(_startaddr, _size, PROT_READ | PROT_WRITE,
#ifdef USING_FILE_BACKUP
                     MAP_SHARED | MAP_FIXED , backingFd, 0);
#else
                     MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
#endif
    if(ptr == MAP_FAILED) {
      printf("Can't allocate memory for globals.\n");
      ::abort();
    } 

    if(!_isHeap) {
      // Copy back those initialized data of globals.
      memcpy(_startaddr, temp, startsize);

      // Released the tempory memory
      MUNMAP(temp, _size); 
    }

    fprintf(stderr, "owner page with totalpages %d, ptr %p\n", _totalpages, ptr); 
    // Now initialize pageowner;
    _ownning.initialize(_totalpages); 
//    printf("XProtect: Allocate memory %p and size %x\n", _startaddr, size());
  }

  ~xprotect (void) {
  }
  
  void initialize(void) {
  }

  void finalize(void) {
    // Now it is time to close all protection.
    stopProtection(); 
  }

  void setMemoryUnowned(void) {
    if(!_isHeap) {
      _ownning.setPagesUnowned(0, _totalpages);
    }
    // For heap pages, we don't need to do anything
    // since all pages are set to be un-owned initially
  }

  // Set all pages owner for one block of heap memory.
  void setPagesOwner(void * addr, int size);

  // Start the protection from now on
  void startProtection (void) {
    //fprintf(stderr, "%d: isHeap %d\n", getpid(), _isHeap);
    //printf(" ");
    if(_isHeap) {
      mprotect(base(), size(), PROT_NONE);
    }
    else {
//      mprotect(base(), size(), PROT_NONE);
#if 1 
     // protecting the second page.
     mprotect((void *)base(), xdefines::PageSize, PROT_NONE);
     mprotect((void *)((intptr_t)base()+xdefines::PageSize), xdefines::PageSize, PROT_NONE);
     mprotect((void *)((intptr_t)base()+ 2* xdefines::PageSize), xdefines::PageSize, PROT_NONE);
     mprotect((void *)((intptr_t)base()+ 3* xdefines::PageSize), xdefines::PageSize, PROT_NONE);
#endif
    }
    //fprintf(stderr, "%d: base %p size() %lx \n", getpid(), base(), size());
  
    //fprintf(stderr, "base %p size() %lx\n", base(), size());
    // Now memory is protected because we are trying to create multiple threads.
    // When there are multiple threads, we must keep track of page ownerships.
    _isProtected = true;
  }

  void stopProtection(void) {
    mprotect(base(), size(), PROT_READ | PROT_WRITE);
    _isProtected = false;
  }

  // Remove the page protection, now page is readable and writable
  // for current process
  void removePageProtect(void * addr) {
    void * start = getPageStartAddr(addr);
    mprotect(start, xdefines::PageSize, PROT_READ | PROT_WRITE);
  }

  /// @return true iff the address is in this space.
  inline bool inRange (void * addr) {
    if (addr >= _startaddr && addr <= _endaddr) {
      return true;
    } else {
      return false;
    }
  }

  /// @return the start of the memory region being managed.
  inline void * base (void) const {
    return _startaddr;
  }

  /// @return the size in bytes of the underlying object.
  inline int size (void) const {
	  return _size;
  }

  /// @brief Set all pages to be unowned initially
  /// @brief Handle the page trap 
  void handleAccessTrap (void * addr, void * context);

private:
  inline void * getPageStartAddr(void * addr) {
    return (void *)((intptr_t)addr & ~xdefines::PAGE_SIZE_MASK);
  }

  inline int computePage (void * addr) {
    unsigned long offset = ((intptr_t)addr - (intptr_t)base());

    return offset/xdefines::PageSize;
  }

  inline bool isPageAligned(void *addr)  {
    return (!((intptr_t)addr & xdefines::PAGE_SIZE_MASK));
  }

  // Calculate pages acrossed by this range
  inline int calcPages(void * addr, int size) {
    // Calcuate the start address of this 
    void * start = getPageStartAddr(addr);
    void * end = (void *)((intptr_t)addr + size);
    int offset = (intptr_t)end - (intptr_t)start;

    int pages = offset/xdefines::PageSize;
    return (offset%xdefines::PageSize == 0) ? pages : (pages+1);
  }

  // Start address and end address
  void * _startaddr;
  void * _endaddr;
  size_t _size;

  // Dividing of functionality between this class and 
  int _totalpages;
  
  pageowner _ownning; 

  bool _isProtected; // Whether the page protection is on?

  bool _isHeap;  
};

#endif
