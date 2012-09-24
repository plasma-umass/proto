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
 * @file:   xmap.h
 * @brief:  Managing the mapping between tid and xthread pointer. 
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XTHREADMAP_H_
#define _XTHREADMAP_H_

#include <ucontext.h>

#include "xdefines.h"
#include "xatomic.h"
#include "xplock.h"
#include "xthread.h"

class xmap {
  
public:
  xmap(void) {
    max = xdefines::MAX_THREADS;

    // Cleanup the whole map.
    memset(&map[0], 0 , sizeof(xthread *)*max);

    // Initialize: at least one thread-main thread, tid = 0
    first = true;
    now = 1;
    threads = 0;
  }

  // It is an actual sigleton which are shared by multiple threads.
  static xmap& getInstance (void) {
    static xmap * threadmapObject = NULL;
    if(!threadmapObject) {
      void *buf = MMAP_SHARED(sizeof(xmap));
      threadmapObject = new(buf) xmap();
    }
    return * threadmapObject;
  }

  int allocTid(void) {
    int tid;
    
    lock();

    // Try tid at first. 
    if(!first) {
      // We are using a slow way to find an available id.
      while(map[now] != NULL) {
        now++;
      } 
    }

    tid = now;
    
    // update the now after allocation.
    now = (now + 1)%max;
    if(now == 0) {
      first = false;
    }
    
    unlock();
   
    return tid; 
  } 

  void registerThread(int tid, xthread * thread, bool userthread) {
    lock();
    
    // We only count user threads. 
    if(userthread) {
      threads++;
    //  printf("threadtid %d is a user thread, now threads %d\n", tid, threads);
    }

    //fprintf(stderr, "******thread %d at %p queue %p********\n", tid, thread, &thread->toqueue);
    map[tid] = thread;
    unlock();
  }

  void deregisterThread(int tid) {
    lock();
    map[tid] = NULL;

    // Since deregisterThread can only be called by user thread,
    // we don't need to check whether it is a user thread.
    // Simply do the decrement here.
    threads--;
    unlock();
  }

  // Get corresponding thread structure according
  // to tid.
  xthread * getThread(int tid) {
    xthread * thread = NULL;

    lock();

    if(tid >= 0 && tid <= max)
      thread = map[tid]; 
    unlock();

    return thread;
  }

  // Get the number of active threads
  int numThreads(void) {
    return threads;
  }
 
  bool hasOneThreadOnly(void) {
   // printf("threads is %d\n", threads);
    return (threads == 1);
  }
 
private:

  void lock(void) {
    plock.lock();
  }  

  void unlock(void) {
    plock.unlock();
  }

  // Totally how much threads can be 
  int max; // max thread id 
  int now; // current available thread id
  bool first; // whether the first run has been finished
  int threads; // total number of active threads
  xplock plock;

  // We are having a global map
  xthread * map[xdefines::MAX_THREADS];   
};
#endif /* _ */
