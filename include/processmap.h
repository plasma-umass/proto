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
 * @file:   processmap.h
 * @brief:  Mapping between core id, pqueue id and private queue pointer. 
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _PROCESSMAP_H_
#define _PROCESSMAP_H_

#include <ucontext.h>

#include "xdefines.h"
#include "xatomic.h"
#include "xplock.h"

class processmap {

  class pqmap{
  public:
    pid_t pid;
    xqueue * pqueue;
  };  

public:
  processmap(void) {

  }

  // It is an actual sigleton which are shared by multiple threads.
  static processmap& getInstance (void) {
#if 0
    static processmap * pmapObject = NULL;
    if(!pmapObject) {
      void *buf = MMAP_SHARED(sizeof(processmap));
      pmapObject = new(buf) processmap();
    }
    return *pmapObject;
#else
  static char buf[sizeof(processmap)];
  static processmap * pmapObject = new(buf) processmap;
  return * pmapObject;
#endif
  }

  void initPrivateQueues(void) {
    void * ptr;

    // Malloc a block of share memory to hold private queues.
    ptr = MALLOC_SHARED(sizeof(xqueue) * CPU_CORES);
    for(int i = 0; i < CPU_CORES; i++) {
      void * qptr;
      qptr = (void *)((intptr_t)ptr + i * sizeof(xqueue));
//      fprintf(stderr, "core %d: pqueue %p\n", i, qptr);
      map[i].pqueue = new (qptr) xqueue;
    }
  }

  // This function will be called only by the main pqueue
  // So there is no need for a lock.
  void registerProcess(int coreid, int pid) {
    map[coreid].pid = pid;
//    fprintf(stderr, "coreid %d with process %d\n", coreid, pid);
  }

  pid_t getPid(int coreid) {
    return map[coreid].pid;
  }

  int getCoreid(int pid) {
    int i;

    for(i = 0; i < CPU_CORES; i++) {
      if(map[i].pid == pid) {
        return i;
      }
    }
    
    // should not come here.
    assert(0);
    return 0;
  }

  xqueue * getPQueue(int coreid) {
    return map[coreid].pqueue;
  }
 
  
private:
  // We have to use map_shared area, instead of 
  // globals of library since different process may
  // request this map to find out corresponding pqueue later.
  // If we are using the globals, then some processes may seen some process id haven't been
  // setted since only the initial process will set this map when it creates
  // new processes.
  pqmap map[CPU_CORES];
};
#endif /* _ */
