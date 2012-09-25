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
 * @file:   xcondvar.h
 * @brief:  The file to handle the conditional variable. 
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XCONDVAR_H_
#define _XCONDVAR_H_

class xcondvar{
  enum { MAGIC = 0xCAFEBABE };

public:
  xcondvar() {
  }

  // initialize the conditional variable.
  void condInit(void) {
    // Initialize the waitlist.
    listInit(&waitlist);
 
    // Initialize the spinlock.
    lck.init();

    // save some information
    //waiters = 0;
    mutex = NULL;
 
    setInited();
  }

  /* 
   Copying from the POSIX specification about this function:
     This function atomically release mutex and cause the calling thread to block on the
     condition variable cond. That  is, if another thread is able to acquire the mutex
     after the about-to-block thread has released it, then a subsequent call to
     pthread_cond_broadcast()  or  pthread_cond_signal()  in  that thread shall
     behave as if it were issued after the about-to-block thread has blocked.
    
     Upon  successful  return,  the  mutex shall have been locked and shall be owned 
     by the  calling thread.
   */
  void condWait(pthread_mutex_t *mx, xthread * current) {
    // Acquire the spin lock
    lock();

    // Check whether the cond var has been initialized or not.
    if(!isInited()) {
      condInit();
    } 

    // Whether we are using the same mutex. Complaining if not, it is
    // a bad user bug!!!
    if(mutex) {
      if(mutex != mx) {
        PRWRN("Saved mutex %p != current mx %p, it is a user bug!!!", mutex, mx); 
      }
    }
    else {
      mutex = mx;
      //PRWRN("*******Saved mutex to %p**********!!!", mx); 
    }
    
    // Put current thread into the waiting list.
    enqueueWaitlist(current);

    // Release the user mutex
    //fprintf(stderr, "releasing mutex:%p related with condvar %p\n", mx, &lck); 
    pthread_mutex_unlock(mx);
    //PRWRN("releasing current mutex"); 
   
    // Yielding so other threads can proceed
    threadYieldHoldingLock(getLock());

    //fprintf(stderr, "acquiring current mutex:%p\n", mx); 
    // Grab the lock before return.
    pthread_mutex_lock(mx);
  }

  // Simply wakeup one of waiters.
  void condSignal(xthread * current, xqueue *queue) {
    xthread * thread = NULL;
      
    lock();

    // Check the waiters;
    if(hasWaiters()) {
      // dequeue the first item
      thread = dequeueWaitlist(); 
    }

    unlock();

    // Enqueue the thread after unlock() to avoid possible deadlock
    // The corresponding queue have locks to avoid contention
    if(thread) {
      // Put the waiter into the corresponding pqueue
      // Note: donot put it into the shared queue since normally
      // current process will own this page.
      // We can possibly improve the performance by doing so.
      queue->enqueue(thread);
    }

    return;
  }

  // Wakeup all waiters. Can we put all waiters into 
  // our current queue? Whether it is too crowded?
  // Currently, let's do this at first
  void condBroadcast(xthread * current, xqueue *queue) {
    bool insertWaiter = false;
    struct lnode head;

    lock();

    // Check how many waiters here
    if(hasWaiters()) {
      // Move queue completely, we simply get the
      // the first item and the last item of the waiting list.
      head.prev = waitlist.prev;
      head.next = waitlist.next;
  
      // Initialize the waitlist, now it is empty.
      listInit(&waitlist);
      
      insertWaiter = true;
      assert(hasWaiters() != true);
    }

    unlock();

    if(insertWaiter) {
      // Add all items into the specified queue 
      queue->enqueueAllList(&head);
    }
  }

  // Destory a mutex by simply set it to un-initialized status
  // Note: no need to free the memory, user should take care this
  void condDestroy(void) {
    if(hasWaiters()) {
      PRERR("WRONG!!! Conditional variable should not have waiters when destroying.!\n");
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

  // Check whether the mutex is inited or not.
  inline bool isInited(void) {
    return init == MAGIC;
  }

  // Put a thread to the waiting list of current lock
  // Note: lock must be held to call this function
  inline void enqueueWaitlist(xthread * current) {
    current->setThreadCondWaiting();
   // waiters++;
    listInsertTail(&current->toqueue, &waitlist);
  }

  // FIXME: we should remove this redundancy 
  // Dequeue the first thread on the waiting list
  // and put them into the corresponding running queue
  // Note: lock must be held to call this function
  inline xthread * dequeueWaitlist(void) {
    // Remove the first node from the wailist.
    lnode * node = waitlist.next;
    listRemoveNode(node);
 
    xthread * thread = container_of(node, xthread, toqueue);
  //  waiters--;
   // thread->setThreadLockRunning();
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

  spinlock * getLock(void) {
    return &lck;
  }

  spinlock lck;
 // int  waiters;    // How many waiters on this lock
  struct lnode  waitlist;
  void * mutex;
  int    init;
};

#endif /* _ */
