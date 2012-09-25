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
 * @file:   xbarr.h
 * @brief:  The file to handle the barrier. 
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XBARR_H_
#define _XBARR_H_

class xbarr{

  enum { MAGIC = 0xCAFEBABE };

public:
  xbarr() {}

  void barrInit(unsigned count) {
    // Initialize the waitlist.
    listInit(&waitlist);
 
    // Initialize the spinlock.
    lck.init();

    maxthreads = count;
    waiters = 0;
  }

  // barrier wait
  void barrWait(xthread * current, xqueue * queue) {
    lock();

    waiters++;
  
    // check whether we have enough waiters. 
    if(waiters >= maxthreads) {
      struct lnode head;
      bool toEnqueue = false;

      // If maxthreads is 1, then there
      // is no need to check the waitlist.    
      if(maxthreads > 1) {
        listRetrieveAllItems(&head, &waitlist);        
 
        toEnqueue = true;
      }
        
      // Now waiters is 0.
      waiters = 0;

      unlock();
      
      // FIXME: add all items into the global shared queue
      // however, it is not efficient on cache usage since it is better to 
      // put the thread back the orignal process 
      if(toEnqueue) {
//        fprintf(stderr, "%d: Barrier wait, release all threads %d\n", getpid(), maxthreads-1);
        queue->enqueueAllList(&head);
      }
    } else {
      // Put myself into the waitlist.
      enqueueWaitlist(current);

      // Yielding to scheduler while holding lock.
      threadYieldHoldingLock(getLock());
    }

    return;
  }

  // FIXME: if someone is waiting here, complainning it. 
  void barrDestroy(void) {
    if(waiters > 0) {
      PRERR("Some one is waiting on barrier when detroying????\n");
      assert(waiters == 0);
    } 
  }
  
private:
  // Put a thread to the waiting list of current lock
  // Note: lock must be held to call this function
  inline void enqueueWaitlist(xthread * current) {
    listInsertTail(&current->toqueue, &waitlist);
  }

  void lock(void) {
    lck.acquire();
  }

  void unlock(void) {
    lck.release();
  }

  spinlock * getLock(void) {
    return &lck;
  }

private:
  spinlock lck;     // spin lock used to 
  struct lnode   waitlist;
  unsigned int   maxthreads;
  unsigned int   waiters; // How many waiters are waiting on this barrier.
 // bool             arrivalphase;
};

#endif /* _ */
