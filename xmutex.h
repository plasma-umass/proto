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
 * @file:   xmutex.h
 * @brief:  The file to handle the mutex lock. 
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XMUTEX_H_
#define _XMUTEX_H_

#include "spinlock.h"
#include "xscheduler.h"

class xmutex{

  enum { MAGIC = 0xCAFEBABE };
  typedef enum {
    MUTEX_STATUS_UNLOCKED = false,
    MUTEX_STATUS_LOCKED = true,
  } e_mutex_status;


public:

  // Normally, mutexInit is called in a single thread mode.
  void mutexInit(void) {
    // Initialize the waitlist.
    listInit(&waitlist);
 
    // Set the lock to unlocked.
    markLockFree();

    // Initialize the spinlock.
    lck.init();
 
    setInited();
  }

  void mutexLock(xthread * current) {
    // Acquire the spin lock
    lock();

    // Check whether the lock has been initialized or not.
    if(!isInited()) {
      mutexInit();
    } 

    // Check whether the mutex is lockable or not
    while(status != MUTEX_STATUS_UNLOCKED) {
      unlock();
     
checkagain: 
      // Loop for a while and check again
      while(status != MUTEX_STATUS_UNLOCKED) {
        xatomic::cpuRelax();
      }
      
      lock();  
  
      if(status == MUTEX_STATUS_UNLOCKED) {
        // Now we can get lock!!
        break;
      }  

#if 1
      //fprintf(stderr, "My god, why I still can't get the lock\n");
      unlock();
      goto checkagain;
#else

      // Otherwise, put current thread into the waiting list.
      enqueueWaitlist(current);

      // yielding will release corresponding lock. 
      threadYieldHoldingLock(&lck);

      lock();
#endif
    }

    // Now we can acquire lock safely
    // Set the status to be locked and set the owner to current thread.
    markLocked();
    owner = current->getTid();

    unlock();
  }

  // release corresponding mutex. 
  void mutexUnlock(xthread * current, xqueue * pqueue) {
    int mytid = current->getTid();
    xthread * thread = NULL;
  
    lock();
    // check the status
    if(status == MUTEX_STATUS_UNLOCKED) {
      PRFATAL("Incorrect status. Status should be locked");
    }
      
    // Check the owner
    if(mytid != owner) {
      PRFATAL("Thread %d CANNOT release a lock acquired by thread %d", mytid, owner);
    }

    // Check the waiters;
    if(hasWaiters()) {
      // dequeue the first item
      thread = dequeueWaitlist(); 
    }

    // Set the lock status
    markLockFree();

    unlock();

    // Enqueue the thread after unlock() to avoid possible deadlock
    // The corresponding queue have locks to avoid contention
    if(thread) {
      // Put the waiter into the corresponding pqueue
      // Note: donot put it into the shared queue since normally
      // current process will own this page.
      // We can possibly improve the performance by doing so.
      pqueue->enqueue(thread);
    }

    return;
  }

  // Destory a mutex by simply set it to un-initialized status
  // Note: no need to free the memory, user should take care this
  void mutexDestroy(void) {
    if(hasWaiters()) {
      PRERR("Someone is still waiting on this mutex when destroying?????\n");
      assert(hasWaiters() == false); 
    }
    setUninited();
  }

private:
  // Set this mutex to inited status.
  inline void setInited(void) {
    init = MAGIC;
  }

  // Set this mutex to uninited
  inline void setUninited(void) {
    init = 0;
  }

  inline void markLocked(void) {
    status = MUTEX_STATUS_LOCKED;
  }

  inline void markLockFree(void) {
    status = MUTEX_STATUS_UNLOCKED;
  }

  // Check whether the mutex is inited or not.
  inline bool isInited(void) {
    return init == MAGIC;
  }

  // Put a thread to the waiting list of current lock
  // Note: lock must be held to call this function
  inline void enqueueWaitlist(xthread * current) {
    current->setThreadLockWaiting();
    listInsertTail(&current->toqueue, &waitlist);
  }

  // Dequeue the first thread on the waiting list
  // and put them into the corresponding running queue
  // Note: lock must be held to call this function
  inline xthread * dequeueWaitlist(void) {
    // Remove the first node from the wailist.
    lnode * node = waitlist.next;
    listRemoveNode(node);
 
    xthread * thread = container_of(node, xthread, toqueue);
    thread->setThreadRunning();

    return thread;
  }

  bool hasWaiters(void) {
    return !isListEmpty(&waitlist); 
  }

  
private:
  void lock(void) {
    lck.acquire();
  }

  void unlock(void) {
    lck.release();
  }

/*
This is a stub: 
    void *  mx_mutex;                    
    int     mx_init;
    int     res2;
    int     mx_kind;
    int     res3;
    int     mx_init_lock;

This is an actual mutex:
struct pth_mutex_st { 
    unsigned       mx_state:8;
    short      mx_index;
    int        mx_type;
    pth_t      mx_owner;
    pid_t      mx_owner_pid;
    pth_qlock_t      mx_lock;
    pth_ringnode_t   mx_node;
    pth_list_t       mx_waitlist;
    struct rfutex_st mx_shared;
    unsigned long    mx_count;
    unsigned long    mx_waitcount;
};

*/
  // WE should make sure that the size cannot be larger than pthread_mutex_t.
  // NOTE: otherwise, one field can be modified silently, a bug we found in debugging!
  spinlock lck;   // spin lock used to 
  int      status;  // What is the status of lock
  struct lnode     waitlist;
  int      owner;   // Who is owning this lock 
  // To avoid the problems of uninitialize, we could use a 
  // magic number to check whether one mutex is initialized or not.
  int      init;    // whether the lock has been initialized or not.
};

#endif /* _ */
