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
 * @file   process.cpp
 * @brief  The main engine for process management, etc.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#include <errno.h>
#include "process.h"
#include "xmemory.h"
#include "xmap.h"
#include "processmap.h"
#include "xrun.h"
#include "xscheduler.h"

void process::initialize(int pid, int coreid) {
    // Set process id for this process,
    // this will affect getpid result.
    setpid(coreid, pid);
  
    // Set core id for this process
    setCoreId(coreid);
    
    // Bind to specific core
    bindToCpu(coreid);

    // Set all queues for this process
    setQueues(coreid);
}

void process::bindToCpu(int cpuid) {
  cpu_set_t cpuset;

  /* set affinity attribute */
  CPU_ZERO(&cpuset);
  CPU_SET(cpuid, &cpuset);

  if(sched_setaffinity(0, sizeof (cpu_set_t), &cpuset) != 0) {
    PRFATAL("can't set effinitity for core %d\n", cpuid);
  }
}

// Create a new process
void process::create(int coreid) {
  //PRLOG(stderr, "%d: creating %d process\n", getpid(), id);
  int child = forkShareFS();
  //fprintf(stderr, "%d: creating %d process %d\n", getpid(), coreid, child);
  if(child == 0) {
    // make the system call to get actual pid here.
    pid_t mypid = syscall(SYS_getpid);

    //fprintf(stderr, "%d before initialize\n", mypid);   
    initialize(mypid, coreid);
   
    //fprintf(stderr, "%d before protect\n", mypid);   
    // Now we have to setup all memory
    xmemory::getInstance().protectAllMemory();

    ucontext_t current;

    getcontext(&current);

    //fprintf(stderr, "%d after getcontext\n", mypid);   
    // Create a new scheduler thread on current process
//    PRLOG(stderr, "Creating child process %d before spawning scheduelr\n", mypid);
    xthread * scheduler = spawnSchedulerThread();
    PRLOG("Creating child process %d after spawning scheduelr\n", mypid);
    PRLOG("Creating child process %d: stack %p\n", mypid, &mypid);
   // fprintf(stderr, "Creating child process %d: stack %p\n", mypid, &mypid);

    // Now we are running the scheduler thread!!!!
    // Note: we have to switch to scheduler thread otherwise
    // scheduler are still using current process's stack.
    setcontext(&scheduler->ctx.context);
   // swapcontext(&current, &scheduler->ctx.context);
  }
  else {
    // Register this process.
    // Set process id to the global processmap.
    processmap::getInstance().registerProcess(coreid, child);  
  }
}

// Spawn a scheduler thread for this process.
xthread * process::spawnSchedulerThread(void) {
  int tid = xmap::getInstance().allocTid();

  // Create a block of memory to hold this task.
  void * ptr = MALLOC_SHARED(sizeof(xthread));

  // Create a block of memory to hold this task.
  xthread *  thread = new (ptr)xthread(tid);

  PRDBG("%d: Creating spawning thread %p tid %d\n", getpid(), thread, tid);
  //fprintf(stderr, "%d: Spawning scheduler thread %p tid %d\n", getpid(), thread, tid);
  // Set scheduler thread.
  setSchedulerThread(thread);

  // Register this thread
  xmap::getInstance().registerThread(tid, thread, false);

  // Set the starting function 
  thread->spawn((void *)&schedulerThread, NULL);
  //PRDBG("%d: Creating spawning thread %p tid %d. after spawn the scheduler\n", getpid(), thread, tid);

  return thread;
}

// Setup private queues and share queue for current process.
void process::setQueues(int id) {
  pqueue = xrun::getInstance().getPrivateQueue(id);
  squeue = xrun::getInstance().getShareQueue();
  //dqueue = xrun::getInstance().getDeadQueue();
}

// Set corresponding pid    
void process::setpid(int coreid, pid_t pid) {
  _pid = pid;

  // Set current pid to xrun in order to speedup the getpid()
  xrun::getInstance().setpid(coreid, pid);
}

// Set corresponding coreid, which is actual heap id.    
void process::setCoreId(int id) {
  _coreid = id;
  
  // Set heapid to corresponding core id. 
  xrun::getInstance().setHeapid(id);
}

