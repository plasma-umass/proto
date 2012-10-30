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
 * @file:   xthread.cpp
 * @brief:  This file handle thread management. 
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <ucontext.h>
#include <setjmp.h>

#include "xthread.h"
#include "process.h"
#include "xrun.h"
#include "xscheduler.h"
#include "xmap.h"
#include "processmap.h"

void xthread::removeFromDeadQueue(void) {
  xqueue * dqueue = xrun::getInstance().getDeadQueue();
  dqueue->dequeue();  
}

void xthread::putJoineeQueue(xthread * joinee) {
  listInsertHead(&toqueue, &joinee->joinqueue);
}


// Put all events into the specified list and re-initialize the event queue
void xthread::getAllEvents(lnode * list) {
  lock();

  listRetrieveAllItems(list, &eventqueue);

  unlock();
}

// Put myself into the global dead queue.
void xthread::putDeadqueue(xthread * thread) {
  xqueue * dqueue = xrun::getInstance().getDeadQueue();
  dqueue->enqueue(thread);
}

// Spawn a new thread. 
void xthread::spawn(void * actualFunc, void * arg) {
    PRLOG("%d: Spawning ....\n", getpid());
    // Make context
    ctx.makeContext((void *)&xthread::threadRun, actualFunc, arg);   
 
    // set the parent to current thread.
    parent = process::getInstance().getCurrent();
    parent->setUnBounded();
}


void xthread::shareInitialStack(void * start, int stacksize) {
  void * ptr;
   
  PRDBG("start %p and stacksize %x\n", start, stacksize);

  fprintf(stderr, "ShareinitStack: start %x size %x\n", start, stacksize); 
  // MMAP current stack to shared.
  ptr = WRAP(mmap)(start, stacksize, PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
  if(ptr != start) {
    PRERR("Can't make inital stack to be shared.\n");
    _exit(-1);
  }
}

// @brief: recover current stack to original place and set it to context.
void xthread::resetStack(void * origStackStart, int size) {

  // Reset stack address to specified stack.
  ctx.resetStack(origStackStart,size);
}

bool xthread::isThreadDead(void) {
  return status == THREAD_STATUS_DEAD ? true : false;
}

void xthread::freeThread(xthread * thread) {
  int tid = thread->getTid();

  // Cleanup the xmap about this thread.
  xmap::getInstance().deregisterThread(tid);
 
  // Free the stack for this thread.
  thread->ctx.freeStack();

  // Free the memory for this thread.
  FREE_SHARED(thread);
}

/**
 * pthread_cancel: cancel a given thread.
 * The implementation is very similar to the join operation.
 * The additional step is to send the target thread a signal.
 */

void xthread::cancel(xthread * current, int tid) {
  xthread * target = xmap::getInstance().getThread(tid);
 
  // Find out the process that the target thread is running on.

  // Send a signal to the target process.

 
  // 
  void * result;


  // Calling the join operation, that is, current thread must wait until 
  // the target thread is cleaned up tidy in order to move forward.  
  join(current, tid, &result);
 
}

bool xthread::hasPendingEvents(void) {
  return (!isListEmpty(&eventqueue)); 
}

/**
 * pthread_join a thread with given tid.
 */
void xthread::join(xthread * current, pthread_t tid, void ** result) {
  xthread * joinee;

  // Since now I am calling join, let's set this thread to bounded from now on.
  // This is terrible, maybe we should check whether there is only one thread or not???
  current->setBounded();

  joinee = xmap::getInstance().getThread(tid);

  if(current == joinee) {
    PRERR("Thread %d cannot join myslef (tid %d)\n", current->getTid(), (int)tid);
    abort();
  }

  if(joinee == NULL) {
    PRERR("Thread %d is not existing\n", (int)tid);
    abort();
  }

//  fprintf(stderr, "%d: acquire the thread %d's lock\n", getpid(), (int)tid);
  // Acquire the joinee's lock!!! 
  joinee->lock();

  // If the thread already finished, then we have to cleanup the 
  // child thread.
  if(!joinee->isThreadDead()) {
    /* Thread is still running normally, not in the deadqueue. */
    // Add myself to the joinee's waiting list. 
    // Now the joinee should  guarantee to put me onto my bounded core
    putJoineeQueue(joinee);       
    
    // If I am yielding, then I won't come back here until 
    // the thread I am waiting put me into the run queue again.
    threadYieldHoldingLock(joinee->getLock());
    
    PRDBG("the joining thread is running on process %d\n", getpid());
    //PRWRN("the joining thread is running on process %d\n", getpid());
    joinee->lock();
  }

  if(!joinee->isThreadDead()) {
    PRERR("Joinee should be putted into the dead queue, tid %d status is %d\n", tid, joinee->status);
    abort();
  }

  // Release the lock before we free the thread
  joinee->unlock();
  PRDBG("Joining thread %d, before remove queue\n", tid);

  // Remove joinee from its queue.
  joinee->removeFromDeadQueue();

  PRDBG("Joining thread %d, after remove queue\n", tid);

  // Set the result to 0
  if(result) {
    *result = joinee->retval;
  }
   
  // Then we can free this control block. 
  freeThread(joinee);
  
  // Check whether I am the only thread
  // If yes, then I should be working on my bounded core.
  if(xmap::getInstance().hasOneThreadOnly()) {
    int coreid = current->getBoundCore();

    // FIXME, must cooperate with others parts if the process isnot going to 
    xmemory::getInstance().cleanupMemoryMappings();

    // Switch to bounded core if not.
    if(process::getInstance().getCoreId() != coreid) { 
      xqueue * pqueue = processmap::getInstance().getPQueue(coreid);
      threadYieldToRunQueue(pqueue);
    }
  }
}

bool xthread::hasJoiner(void) {
  return isListEmpty(&joinqueue) ? false : true;
}

xthread * xthread::getJoiner(void) {
  xthread * thread = NULL;
  lnode * node = NULL;

  node = joinqueue.next;

  PRDBG("node is %p\n", node);
#if 0
  if(node == NULL) {
    while(1) ;
  }
#endif
  
  assert(node != NULL);
  listRemoveNode(node);

  // Get corresponding thread for this thread
  if(node) {
    thread = container_of(node, xthread, toqueue);
    PRDBG("node is %p. Get thread %p\n", node, thread);
  }
  else {
    // Should not be here.
    assert(0);
  }

  return thread;
}

// Put a thread into a private queue
static xqueue * getPrivateQueue(int coreid) {
  xqueue * pqueue = processmap::getInstance().getPQueue(coreid);
  return pqueue;
}      

// @brief: thread exit function
// 1. Check whether the parent thread is waiting on my exit.
//    Wakeup the parent thread and put the parent thread into
//    the global queue. 
// 2. Put myself into the dead queue. Thus this thread
//    can be cleanedup by one thread to join me.
//    According to posix standard, there is no require
//    of "parent-chilren" relation to call pthread_join
void xthread::exit(void * retval) {
  xthread * thread = process::getInstance().getCurrent();

  PRDBG("NNNNNOW %d: exit function: tid %d", getpid(), thread->getTid()); 
 
  // Acquire myself's lock in order to avoid the joiner to put into
  // my joinqueue in the same time. 
  thread->lock(); 
 
  thread->retval = retval;

  // Set current thread to exit status
  thread->status = THREAD_STATUS_DEAD;
   
  // Put myself to global dead queue
  thread->putDeadqueue(thread);

  // Check the joinqueue
  if(thread->hasJoiner()) {

    // Get the joiner.
    xthread * joiner = thread->getJoiner();
  
    // Change the joiner status.
    joiner->status = THREAD_STATUS_RUNNING;
 
    // if the joiner is bounded, we have to put it into specified private queue.
    xqueue * queue;

    if(joiner->isBounded()) {
      queue = getPrivateQueue(joiner->getBoundCore());      
    } 
    else {
      queue = process::getInstance().getSQueue();
    }

    // Put the joiner to the current run queue.
    queue->enqueue(joiner);

    // Release the lock
    threadYieldHoldingLock(thread->getLock());
  }
  else {
    // Yield and never return. Since I am now in the deadqueue
    threadYieldHoldingLock(thread->getLock()); 
    
    PRERR("NNNNNOW %d: should not be here after yield ", getpid()); 

    assert(0);
  }
}

// I could use a class function, however, 
void xthread::threadRun(void * threadFunc, void * arg) {
  
  // Run the actual thread function.
  void * retval = ((threadFunction*)threadFunc)(arg);

  xthread::exit(retval);
}

