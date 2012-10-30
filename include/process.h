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
 * @file   process.h
 * @brief  Process management, etc.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XPROCESS_H_
#define _XPROCESS_H_

#include <errno.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#endif

#include <iostream>
#include <queue>
#include <pthread.h>
#include <stdlib.h>
#include <sched.h>

#include "xdefines.h"
#include "xthread.h"
#include "xqueue.h"
#include "xmemory.h"
#include "internalheap.h"

class process {
public:
  process () {
  } 

  // process is not an actual singleton in the whole system,
  // So we can save some process-specific data there: current, pid or others 
  static process& getInstance (void) {
    static char pbuf[sizeof(process)];
    static process * theProcessObject = new (pbuf) process();
    return *theProcessObject;
  }

  void initialize(int pid, int coreid);

  void bindToCpu(int cpuid);

  // Run specified thread.
  static void runTask(process & proc, xthread * task);
  
  // Main entry of a process
  //static void schedulerThread(void);

  xthread *spawnSchedulerThread(void);

  // Register a new process
  void registerProcess(pid_t pid, int coreid);

  // Create a new process
  void create(int id);

  // Setup private and share queues for current process   
  void setQueues(int id);

  // Set current thread
  void setCurrent(xthread * thread) {
    current = thread;
    
  }

  // Get current thread.
  xthread * getCurrent(void) {
    return current;
  }
 
  xthread * getScheduler(void) {
    return scheduler;
  }
 
  xqueue * getPQueue(void) {
    return pqueue;
  }

  xqueue * getSQueue(void) {
    return squeue;
  }

#if 0
  xqueue * getDQueue(void) {
    return dqueue;
  }
#endif

private:
  int forkShareFS(void) {
    return syscall(SYS_clone, CLONE_FS|CLONE_FILES|SIGCHLD, (void*) 0 );
  } 

public:  
  void setCoreId(int id);

  int getCoreId(void) {
    return _coreid;
  }

  int getPid(void) {
    return _pid;
  }

  void setSchedulerThread(xthread * thread) {
    PRDBG("%d: Coreid %d: set scheduler %p\n", getpid(), _coreid, thread);
    scheduler = thread;
  }

  void setpid(int coreid, pid_t pid);

  // which core is located, this will affect the heap allocation too.
  int     _coreid;
  pid_t   _pid;

  xqueue * pqueue;
  xqueue * squeue;
  //xqueue * dqueue;

  // Current thread??
  xthread * current;

  xthread * scheduler; // One scheduler thread
};

#endif
