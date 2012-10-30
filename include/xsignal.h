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
 * @file:   xsignal.h
 * @brief:  This file defines some user messages which is used to communicate
 *          between different threades. Noramally, these messages are passed around
 *          use the signal mechanism. For example, when there is pthread_cancel
 *          call, we have to send a signal to the target process to stop the current thread
 *          immediately.
 *          We could reference /nfs/cm/scratch1/tonyliu/grace/trunk/srccondvar/xrun.h. 
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XSIGNAL_H_
#define _XSIGNAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "memwrapper.h"

class xsignal{
private:
  typedef enum{
    E_SIGNAL_KILL, // to handle pthread_kill, specific message are passed in the actual
    E_SIGNAL_CANCEL, // to handle pthread_cancel
  } e_signal_type;

public:
  xsignal() {

  }


  // Notice that xsignal is a singleton here. 
  // However, it is not a actual singleton for the whole system
  // Since xsignal is allocated in the globals space of this library, which can't 
  // be seen by other process, thus we can store some thread-local
  // variables here.
  static xsignal& getInstance (void) {
    static char buf[sizeof(xsignal)];
    static xsignal * theOneTrueObject = new (buf) xsignal();
    return *theOneTrueObject;
  }


  // Install the signal handling in the same function. 
  void initialize() {
    installSignalHandler();
  }


  // Basically, we will copy some implementation from xsignal.h and put it here.
  // There is no need to put into the xsignal.h since it is nothing related to 
  // memory.
  static void handleSigusr2 (int signum,
          siginfo_t * siginfo,
          void * context)
  {
    // Now we got the signal, unsafe to call printf but good for write according "man 2 signal".
    const char * str = "caught signal...";
    write(1, str, strlen(str));

    // how we can handle it? 
    union sigval sig = siginfo->si_value;
    
    //If current process needs to be waken up, then it should not pause again
    // Since it is possible that one process can receive signal before it can pause actually
    // that is why we need to do this.
    if(sig.sival_int & E_SIGNAL_KILL) {
      _exit(0);
    } 

    // Since every process will have a signal handler, we have to send signal 
    // to specified thread.

    // Now we may have to free the 
   // printf("%d get signal\n", getpid());
  }

  /// @brief Install a handler for SEGV signals.
  void installSignalHandler (void) {
    stack_t         sigstack;
 
   // Set up an alternate signal stack.
    sigstack.ss_sp = MMAP_PRIVATE(SIGSTKSZ);
    sigstack.ss_size = SIGSTKSZ;
    sigstack.ss_flags = 0;
    sigaltstack (&sigstack, (stack_t *) 0);

    printf("signal handler stack %p to 0x%x\n", sigstack.ss_sp, (intptr_t)sigstack.ss_sp + SIGSTKSZ); 
   // Now set up a signal handler for SIGSEGV events.
    struct sigaction siga;
    sigemptyset (&siga.sa_mask);

    // Set the following signals to a set 
    sigaddset (&siga.sa_mask, SIGUSR2);

    sigprocmask (SIG_BLOCK, &siga.sa_mask, NULL);

    // Point to the handler function.
    siga.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART | SA_NODEFER;

    // If we specify SA_SIGINFO, then signal handler will take three parameter
    // and we will set sa_sigaction instead of sa_handler.
    siga.sa_sigaction = xsignal::handleSigusr2;
    if (sigaction (SIGUSR2, &siga, NULL) == -1) {
      printf ("sigusr1.\n");
      exit (-1);
    }

    sigprocmask (SIG_UNBLOCK, &siga.sa_mask, NULL);
  }

  void signalKill(int pid) {
     union sigval sig;
     
     sig.sival_int = E_SIGNAL_KILL;
     sigqueue(pid, SIGUSR2, sig);
  }

private:


  // Check whether there is some signal mechanism of DTHREADS. For example, xthread_kill
  // which may end up to use the signal mechanism.

  // Another mechanism that we have to add is to keep track of which pid a thread 
  // is running on. Also, we need to keep track of the status of a thread. 
  // For example, when one thread is putting the running queue of one process, 
  // We mark those hostid to the corresponding process id and change the status to READY.
  // When a thread is running , then we should change the status to RUNNING. 
  // If one thread is not running, it can still receive some signals, however, those signals
  // maybe be queued in corresponding queue, which depends on which signals. 
  // For example, "KILL" a thread should be handled immediately so there is no need to be queued
  // no matter whether the thread is running or not. But we should provide a mechanism to queue
  // signals if not that important, those signals will be handled when threads are started.  

private:
  void signal(int pid);
};

#endif /* _ */
