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
 * @file:   xthread.h
 * @brief:  This file handle thread management. 
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XTHREAD_H_
#define _XTHREAD_H_
#include <sys/time.h>
#include <sys/resource.h>
#include <ucontext.h>

#include "xstatus.h"
#include "xdefines.h"
#include "xatomic.h"
#include "xcontext.h"
#include "spinlock.h"

// User thread: we will save all status about each thread here.
class xthread {

public:
  xthread(int tid) {
    this->tid = tid;
    this->isbounded = false;
    this->status = THREAD_STATUS_INITIAL; 

    // Initialize corresponding queue
    listInit(&toqueue);
    listInit(&joinqueue);
    listInit(&eventqueue);
  }

  // Creating a thread. 
  void spawn(void * actualFunc, void * arg);
  void join(xthread * current, pthread_t tid, void ** result);

  // Get all events from the event queue. 
  bool hasPendingEvents(void);
  void getAllEvents(lnode * list);

  void putJoineeQueue(xthread* joinee);
  void putDeadqueue(xthread * thread);

  // set up the stack for initial thread.
  void shareInitialStack(void * start, int stacksize);
  void resetStack(void * origStackStart, int size);

  // Cancel a thread.
  void cancel(xthread * current, int tid);

  // Set to bounded;
  void setBounded(void) {
    isbounded = true;
  }

  void setUnBounded(void) {
    isbounded = false;
  }

  // SetBoundedProcess
  void setBoundCore(int coreid) {
    boundcore = coreid;
  }

  bool isBounded(void) {
    return isbounded;
  }

  bool isBoundCore(int coreid) {
    return ((boundcore == coreid) ? true : false); 
  }

  int getBoundCore(void) {
    return boundcore;
  }

  void setThreadRunning(void) {
    status = THREAD_STATUS_RUNNING; 
  }
  
  // Now thread is waiting on locks, cond_wait or barrier wait
  void setThreadLockWaiting(void) {
    status = THREAD_STATUS_LOCK_WAITING; 
  }
  void setThreadCondWaiting(void) {
    status = THREAD_STATUS_COND_WAITING; 
  }
  void setThreadBarrierWaiting(void) {
    status = THREAD_STATUS_BARRIER_WAITING; 
  }

  void checkStack(void) {
    ctx.checkStack();
  }

  ucontext_t * myContext(void) {
    return ctx.myContext();
  }


  bool hasPendingSignals(void) {
    return sigpendcnt > 0 ? true : false; 
  }

  void setContext(void) {
    ctx.setContext();
  }

  // Make the context of current thread to specified context 
  void switchContext(void * context) {
    ctx.switchContext(context);
  }

  void getContext(void) {
    ctx.getContext();
  }

  // Check whether one thread has been exited or not.
  bool isThreadDead(void);

  int getTid(void) {
    return tid;
  }

  // exit handler attaching to different process
  static void exit(void * retval);

//  // The actual running function of each spawning function
  static void threadRun(void * threadFunc, void * arg);

  void lock(void) {
    lck.acquire();
  }

  void unlock(void) {
    lck.release();
  }

  spinlock * getLock(void) {
    return &lck;
  }

  void removeFromDeadQueue(void);
  void freeThread(xthread *thread);
  bool hasJoiner(void);
  xthread * getJoiner(void);

public:
  // A lock to be used when modify the thread structure.
  spinlock lck;

  /// The thread id.
  pthread_t tid;
    
  // thread context: it is important for the thread scheduling
  xcontext ctx;
  // parent information. When one thread exit, it is attached to
  // its parent. It is the parent's responsibilty to reclaim
  // any resource holding by the children.
  // Who creates me? It is helpful if we are going to exit. 
  xthread  * parent; 

  /// Put myself into some queues
  lnode  toqueue;
  
  // Some threads can wait here in order to wait me to be joined.
  lnode  joinqueue;

  // Singal handling
  /* per-thread signal handling */
  int     sigpending[2];/* internal set for 64 pending signals         */
  int     sigpendcnt;   /* number of pending signals                   */
  lnode   sigthread;    /* thread who sent signal      */

  /** 
   * The process to host current thread. The thread can be still in the runqueue.
   * It can be in different status. But this is the process to handle the 
   * all singals.   
   */
  int     hostpid;      
  e_thread_status status;

  // The event queue passed from other threads
  lnode  eventqueue;

  // The main thread should be bounded to its original process in the end,
  // otherwise, all children processes can not be reaped. 
  bool isbounded;  
  pid_t boundcore;
  
  void * retval;
  char buf[64]; // padding to avoid false sharing problem.
};
#endif /* _ */
