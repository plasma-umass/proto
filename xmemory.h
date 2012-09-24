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
 * @file   xmemory.h
 * @brief  Memory management.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _XMEMORY_H_
#define _XMEMORY_H_

#include <signal.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <set>

#include "xglobals.h"

#include "xheap.h"
#include "xoneheap.h"
#include "xpheap.h"

// Heap Layers
#include "privateheap.h"
#include "stlallocator.h"

#include "xfilemap.h"

class xmemory {
private:

  // Private on purpose. See getInstance(), below.
  xmemory (void) 
  {
  }

public:

  void initialize(void) {
    // Intercept SEGV signals (used for trapping initial reads and
    // writes to pages).
    installSignalHandler();
	
    // Call _pheap initialization so xheap.h can be initialized at first 
    // then it can work normally.
    fprintf(stderr, "heap initialize\n");  
  _pheap.initialize();
	  _globals.initialize();
    fprintf(stderr, "xmemory initialize\n");  
  }

  void finalize(void) {
	  _globals.finalize();
	  _pheap.finalize();
  }

  // Notice that xmemory is a singleton here. 
  // However, it is not a actual singleton for the whole system
  // Since xmemory is allocated in the globals space of this library, which can't 
  // be seen by other process, thus we can store some thread-local
  // variables here.
  static xmemory& getInstance (void) {
    static char buf[sizeof(xmemory)];
    static xmemory * theOneTrueObject = new (buf) xmemory();
    return *theOneTrueObject;
  }

  // Setup memory mappings
  inline void setupMemoryMappings(void) {
    // 1. All pages should not be owned initially.
    setAllMemoryUnowned();   

    // 2. Protect all memory
    protectAllMemory();
  }

  // Cleanup memory mappings. FIXME:
  // How to notify other processes to cleanup mappings
  inline void cleanupMemoryMappings(void) {
    setAllMemoryUnowned();

    unprotectAllMemory();    
    // Post events to other processes so that other processes
    // can do unprotection on all memory.
    // FIXME   
  }

  inline void setAllMemoryUnowned(void) {
    _globals.setMemoryUnowned();     
    _pheap.setMemoryUnowned();     
  }  
  
  // Protect all memory.
  // This will be called before the main process tried to 
  // create children processes and when a process is created
  inline void protectAllMemory(void) {
    //fprintf(stderr, "%d: global start protection\n", getpid());
  //  _globals.startProtection();
    //fprintf(stderr, "%d: heap start protection\n", getpid());
  //  _pheap.startProtection();
  }

  inline void unprotectAllMemory(void) {
  //  _globals.stopProtection();
  //  _pheap.stopProtection();
  }

  // Handle traps of access
  void handleAccessTrap(void * addr, void * context) {
    // Check what is the type of this address.
    // If this page is normal memory, global or heap
    if(_pheap.inRange(addr)) {
      _pheap.handleAccessTrap(addr, context);
    }
    else if(_globals.inRange(addr)) {
      _globals.handleAccessTrap(addr, context);
    }
    else if(_filemaps.inRange(addr)) {
      _filemaps.handleAccessTrap(addr, context);
    }
    else {
      // Print error if this address doesn't belong to 
      // global, heap or filemapping
      PRERR("addr %p is not valid, something wrong happens\n");
      abort();
    }
  }

  // Handling memory mapping.
  // We will differentiate the file mapping and anonymous mapping
  // in the future. 
  // For anonymous mapping, we will simply change MAP_PRIVATE to MAP_SHARED if
  // it is before the process creation. If the mmap is called after process creation,
  // We might resort to the help of file to shared across different process.
  
  // For file mapping, we will resort to a file mappi  
  void * mmap(bool isAfterCreation, void *addr, size_t length, int prot,
         int flags,  int fd, off_t offset) {
    void * ptr;

#if 0
    if(postinitialized) {
      PRWRN("calling mmap after creation of processes\n");
    }
    else {
      PRERR("calling mmap before creation of processes\n");
    }
#endif
    
    ptr = WRAP(mmap)(addr, length, prot, flags, fd, offset); 
//   fprintf(stderr, "calling mmap with ptr %p and size %x\n", ptr, length);
    return ptr;
  }


  // Handling memory allocation and free  
  inline void *malloc (int heapid, size_t sz) {
  	void * ptr;
    ptr = _pheap.malloc(heapid, sz);
    //fprintf(stderr, "malloc ptr %p sz %d\n", ptr, sz);
    return ptr;
  }

  // FIXME: how to find corresponding heap and free it to corresponding heap
  inline void free (int heapid, void * ptr) {
    if(ptr) {
		  _pheap.free(heapid, ptr);
    }
  }

  /// @return the allocated size of a dynamically-allocated object.
  inline size_t getSize (void * ptr) {
    // Just pass the pointer along to the heap.
    return _pheap.getSize (ptr);
  }
 
public:

  /* Signal-related functions for tracking page accesses. */
  /// @brief Signal handler to trap SEGVs.
  static void segvHandle (int signum,
		      siginfo_t * siginfo,
		      void * context) 
  {
    
    void * addr = siginfo->si_addr; // address of access

    char str[256];
   //PRERR("Segment fault address %p and signum %p\n", addr, &signum);
   sprintf(str, "%d: segment fault address %p\n", getpid(), &addr);
   write(1, str, strlen(str));
  
    // Check if this was a SEGV that we are supposed to trap.
    if (siginfo->si_code == SEGV_ACCERR) {
      xmemory::getInstance().handleAccessTrap(addr, context);
    } else if (siginfo->si_code == SEGV_MAPERR) {
      PRFATAL("%d : MAP error with addr %p.\n", getpid(), addr);
      ::abort();
    } else {
      PRFATAL("%d : other access error with addr %p.\n", getpid(), addr);
    }
  }

  /// @brief Install a handler for SEGV signals.
  void installSignalHandler (void) {
    stack_t         sigstack;
 
   // Set up an alternate signal stack.
    sigstack.ss_sp = MMAP_PRIVATE(SIGSTKSZ);
    sigstack.ss_size = SIGSTKSZ;
    sigstack.ss_flags = 0;
    sigaltstack (&sigstack, (stack_t *) 0);

    fprintf(stderr, "signal handler stack %p to 0x%x\n", sigstack.ss_sp, (intptr_t)sigstack.ss_sp + SIGSTKSZ); 
   // Now set up a signal handler for SIGSEGV events.
    struct sigaction siga;
    sigemptyset (&siga.sa_mask);

    // Set the following signals to a set 
    sigaddset (&siga.sa_mask, SIGSEGV);

    sigprocmask (SIG_BLOCK, &siga.sa_mask, NULL);

    // Point to the handler function.
    siga.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART | SA_NODEFER;

    // If we specify SA_SIGINFO, then signal handler will take three parameter
    // and we will set sa_sigaction instead of sa_handler.
    siga.sa_sigaction = xmemory::segvHandle;
    if (sigaction (SIGSEGV, &siga, NULL) == -1) {
      printf ("sfug.\n");
      exit (-1);
    }

    sigprocmask (SIG_UNBLOCK, &siga.sa_mask, NULL);
  }

private:

  /// The globals region.
  xglobals   _globals;

  xpheap<xoneheap<xheap > > _pheap;

  xfilemap _filemaps;
};

#endif
