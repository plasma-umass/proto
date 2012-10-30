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
 * @file   xscheduler.cpp
 * @brief  Thread scheduler.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#include <errno.h>
#include "process.h"
#include "memwrapper.h"
#include "xmemory.h"
#include "xmap.h"
#include "xrun.h"
#include "xatomic.h"
#include "xscheduler.h"
#include "xevent.h"

extern "C" {

void schedulerWait(void) {
  //FIXME: whether it is good to use pause.
//  usleep(100);
  //pause();
  xatomic::cpuRelax();
}

// Check whether current thread can be runnable on current process?
bool isRunnableThread(xthread * thread, int coreid, xqueue * queue) {
  // If the thread is not existing, return false
  if(!thread)
    return false;

  //PRWRN("thread %d is runnable??\n", thread->getTid());
#if 1
  // When the thread is bounded and current core is not the core to be bounded
  if(thread->isBounded() && (thread->isBoundCore(coreid) == false)) {
    queue->enqueue(thread); 
    return false;
  }
#endif

#if 0
  // We don't want other thread to run on 
  //if((thread->getTid() != 0 && thread->getTid() != 1) && coreid != 1) {
  if((thread->getTid() != 0 && thread->getTid() != 1) && coreid == 0) {
    queue->enqueue(thread); 
    return false;
  }
#endif

  return true;
}

static long getRegister(ucontext_t * context, int reg) {
  return context->uc_mcontext.gregs [reg];
}

static xevent * retrieveEvent(lnode * eventlist) {
  lnode * node = (lnode *)listRetrieveItem(eventlist);
  xevent * event = NULL;

  if(node) {
    event = container_of(node, xevent, toqueue);
  } 

  return event;
}

// Put a thread into the specified queue, which should
// be held the specified lock.
static void enqueueYieldThread(xthread * thread, xqueue * queue) {
  queue->enqueue(thread);
}

// handle the events of current thread.
static void handleThreadEvents(xthread * current) {


  //fprintf(stderr, "%d: HANDLE thread events\n", getpid());
  if(current->hasPendingEvents()) { 
    lnode  eventlist;
    //fprintf(stderr, "%d:*******has pending events\n", getpid());
    // Get all events inside the event queue.
    current->getAllEvents(&eventlist);
 
    // Get events of this thread.
    while(!isListEmpty(&eventlist)) {
      xevent * event = retrieveEvent(&eventlist);

      switch(event->type) {
        case E_EVENT_YIELD: 
          // Put the specified thread into the specified queue.
          enqueueYieldThread(event->args.YIELD.thread, event->args.YIELD.queue);      
          break;

        case E_EVENT_RELEASE_LOCK:
          {
            spinlock * lock = event->args.RELEASELOCK.lock;
            lock->release();
          }        
          break;

        default:
          PRERR("the event is not defined %d\n", event->type);
          break;
      }
    }
  }
}
 
// Main entry of a process
void schedulerThread(void) {
  xthread * thread;
  xqueue  * pqueue, * squeue;
  process &proc = process::getInstance();
  int coreid = proc.getCoreId();
  xthread * scheduler = proc.getScheduler();
  pid_t mypid = proc.getPid();
  int tid = scheduler->getTid();
  rtime_t runningtime;
  rtime_t snapshottime;

  // Get the queues about this process
  pqueue = proc.getPQueue();
  squeue = proc.getSQueue();

  // Release the temporary stack.
  xrun::getInstance().freePrivateStack();

  //printf("pid %d tid %d scheduler %d proc %p\n", mypid, tid, scheduler, proc);
#if 0
  long esp, ebp;

    // Get current esp and ebp
    asm volatile(
      "movl %%ebp, %0 \n"
      "movl %%esp, %1 \n"
      : "=r" (ebp), "=r" (esp)
    );

  printf("%d (tid %d)*******scheduler thread: esp %lx ebp %lx*******\n",mypid, tid, esp, ebp);
#endif
  // Initialize the snapshot time
  PRLOG("%d - %d: under process %d main_entry. PROC coreid %p. thread %p", getpid(), proc.getPid(), coreid, &proc._coreid, &thread);
  assert(getpid() == proc.getPid());

  int running_thread3 = 0;

  bool inpqueue = false;
  // An endless loop
  for(;;) {
    
    //handleThreadEvents(scheduler);

    // Whileloop is used to pick up one ready thread. 
    while(true) {
      inpqueue = false;
      // Check whether there are some work in my private queue.
      thread = pqueue->dequeue();
   
      // if one thread is bounded, only those boundative can run them.
      if(isRunnableThread(thread, coreid, squeue)) { 
        inpqueue = true;
        break;
      }
        
      // Check whether there are some work in the global queue.
      thread = squeue->dequeue(); 
      
      if(isRunnableThread(thread, coreid, squeue)) { 
        break;
      }

      // If no work need to do, sleep for a while and try again
      schedulerWait(); 
    }

    // Now we have some ready threads. 
    PRLOG("%d: thread %x (at %p) is ready now\n", getpid(), thread->getTid(), thread);
   // fprintf(stderr, "%d: thread %d (at %p) is ready now, getting from pqueue %d\n", getpid(), thread->getTid(), thread, inpqueue);

#if 1
    // Now check whether some signals for this thread
    // when current thread is not running.
    // Now it is time to handle these signals here.
    thread->lock(); 
    if(thread->hasPendingSignals()) {
      // FIXME
      // Handle those signals specifically.
      
    }
    thread->unlock();  
#endif

    PRDBG("Set thread %d to current thread before switching\n", thread->getTid());
    // Set current for new thread
    process::getInstance().setCurrent(thread);
    //proc.setCurrent(thread);


    // Update the thread time.
    // Update the scheduler time
    
    // Check the stack address. Give warning if the stack is overflow
    //thread->checkStack();

//    PRWRN("Set thread %d to current thread before switching\n", thread->getTid());
    // Now we are done and ready to switch context.
/*
    ucontext_t * mycontext =thread->myContext();
    long oesp, oebp, oeip;
  oesp = getRegister((ucontext_t *)mycontext, REG_ESP);
  oebp = getRegister((ucontext_t *)mycontext, REG_EBP);
  oeip = getRegister((ucontext_t *)mycontext, REG_EIP);
*/
  //  fprintf(stderr, "SCHEDULING %d: pick up thread %d with eip %x esp %x ebp %x\n", getpid(), thread->getTid(), oeip, oesp, oebp);
    THREAD_SWITCH(scheduler, thread);

   // fprintf(stderr, "%d SCHEDULING: scheduler thread tid %d\n", getpid(), scheduler->getTid());
   // swapcontext(&(scheduler->ctx.context), &(thread->ctx.context));
//    PRWRN("Now it is switching back to scheduler thread %d\n", scheduler->getTid());
    // Set current to Schuduler thread
    //process::getInstance().setCurrent(thread);
    proc.setCurrent(scheduler);

    handleThreadEvents(scheduler);
 
    //printf("SCHEDULING: scheduler thread\n");
    // Update the time on of previous thread 
    // About pth_sched.c 448 line
  
    // FIXME: check whether one process have one more thread 
    // is spawned, whether there are multiple threads?

    // Remove signals ?

    // Check whether some threads is on the deadqueue, if yes, kick them out
    
    // If some threads are waiting for some events, put them into waiting queue.
    
  }
}

// Add a event to the scheduler's event queue
void insertSchedulerEventQueue(xthread * scheduler, xevent * event) {

  // We are trying to add this event to the scheduler's queue.
  // So we have to acquire the scheduler's lock.
  scheduler->lock();

  listInsertTail(&event->toqueue, &scheduler->eventqueue);

  scheduler->unlock();
}


// Current thread is yielding to specified runqueue. 
void threadYieldToRunQueue(xqueue * to) {

  assert(to != NULL);

  // Get current thread information.
  process & proc = process::getInstance();
  xthread * current = proc.getCurrent();
  xthread * scheduler = proc.getScheduler();
 
  //fprintf(stderr, "%d: THREADYIELD, from userid %d to scheduler %d\n", getpid(), current->getTid(), scheduler->getTid()); 

  // Make sure that scheduler thread won't call threadYield.
  assert(scheduler != NULL);
  assert(current != scheduler);

  // Allocate a block of shared memery to 
  xevent * event;
  void * ptr = MALLOC_SHARED(sizeof(xevent));
  event = (xevent *) new (ptr) xevent;

  event->type = E_EVENT_YIELD; 
  event->args.YIELD.thread = current;
  event->args.YIELD.queue = to;

  // Add this event to the scheduler's  event queue.
  insertSchedulerEventQueue(scheduler, event);
 
  //fprintf(stderr, "%d yielding: Right before switch to scheduler\n", getpid()); 
  THREAD_SWITCH(current, scheduler);
}

// Initial thread is yielding. 
// It is safe to put current thread into current process's private queue
// Also, it is impossible to use events at first, since current scheduler is not
// stop at the checking events point. 
void threadYieldInitially(xqueue * to) {
  assert(to != NULL);
  // Get current thread information.
  process & proc = process::getInstance();
  xthread * current = proc.getCurrent();
  xthread * scheduler = proc.getScheduler();

  to->enqueue(current);
 
  //fprintf(stderr, "%d yielding: Right before switch to scheduler\n", getpid()); 
  THREAD_SWITCH(current, scheduler);
}

// Current thread is yielding to scheduler thread, here, current thread is still
// holding the lock, then it is the duty of scheduler to release lock.
// Holding the lock in switching is to avoid possible race condition:
// the same thread can not run on two processes. 
// If not holding the lock, then it is possible that the target process is running 
// this thread before current process swap out current thread.
void threadYieldHoldingLock(spinlock * lock) {
  // Get current thread information.
  process & proc = process::getInstance();
  xthread * current = proc.getCurrent();
  xthread * scheduler = proc.getScheduler();
 
  //fprintf(stderr, "%d: THREADYIELDWITHLOCK, from userid %d to scheduler %d\n", getpid(), current->getTid(), proc.getScheduler()->getTid()); 

  // Make sure that scheduler thread won't call threadYield.
  assert(scheduler != NULL);
  assert(current != scheduler);
 
  void * ptr = MALLOC_SHARED(sizeof(xevent));
  xevent * event = (xevent *) new (ptr) xevent;

  event->type = E_EVENT_RELEASE_LOCK;

  // We ask the scheduler to release the lock for me.
  event->args.RELEASELOCK.lock = lock; 
  
  // Add this event to the scheduler's  event queue.
  insertSchedulerEventQueue(scheduler, event);

  THREAD_SWITCH(current, scheduler);
}

// threadYieldToRunQueue
// threadYieldToWaitList

};
