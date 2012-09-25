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
 * @file   xrun.h
 * @brief  The main engine for all stuff.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XRUN_H_
#define _XRUN_H_

#include <sys/time.h>
#include <sys/resource.h>
#include <iostream>
#include <fstream>

#include "xdefines.h"

#include "process.h"

// Task management
#include "xthread.h"

#include "xmap.h"

// memory
#include "xmemory.h"

// Heap Layers
#include "util/sassert.h"

// utilities
#include "xatomic.h"

#include "processmap.h"

// Syncrhonizations
#include "xmutex.h"
#include "xcondvar.h"
#include "xbarr.h"
#include "xsignal.h"
#include "log.h"

class xrun {

private:
  xrun (void)
  : postinitialized(false),
    maxprocs(CPU_CORES),
    threadsmap (xmap::getInstance()),
    proc (process::getInstance()),
    procmap (processmap::getInstance())
  {
  }

public:

  // xrun is not an actual singleton in the whole system,
  // Every thread will have one copy of xrun. 
  // So we can save some process-specific data there: current, pid or others 
  static xrun& getInstance (void) {
    static char buf[sizeof(xrun)];
    static xrun * theOneTrueObject = new (buf) xrun();
    return *theOneTrueObject;
  }

  /// @brief Initialize the system.
  void initialize (void)
  {
    pid_t  pid;
    void * ptr;
    
    // Get pid for this process.
    pid = syscall(SYS_getpid);

    // Setup the initial thread
    mainthread = setupInitialThread();
 
    // Initialize the memory (install the memory handler)
    xmemory::getInstance().initialize();

    // Initialize the signal handling 
    //xsignal::getInstance().initialize();

    // Initialize the global run queue
    ptr = MALLOC_SHARED(sizeof(xqueue));
    squeue = new (ptr) xqueue;

    // Initialize the global dead queue
    ptr = MALLOC_SHARED(sizeof(xqueue));
    dqueue = new (ptr) xqueue;

    //fprintf(stderr, "squeue %p and dqueue %p\n", squeue, dqueue);
    //while(1) ;
    // Initialize the private queues. Since we may involve  
    // load balance in the future. Now those private queues
    // are allocated in the shared space use mmap. 
    procmap.initPrivateQueues(); 
    
    // Initialize the first process
    proc.initialize(pid, 0);

    // changing the globals of libraries to MAP_SHARED
    //shareLibraryGlobals(); 
  }

  // Postinit will be called before we creates children initially.
  // The function will be called only before the first pthread_create
  // to avoid some problems causing by mmap()
  void postinit(void) {
    // Set current process id.
    xthread * scheduler;
    long   newesp, newebp;
    long   esp, ebp;
    unsigned long  stacksize;
    unsigned long  copystacksize;
    void * newstackBottom;

    // We are trying to get stack information by analyzing corresponding map file.  
    stacksize = getStackInformation();

    // Setup memory mappings
    // All heap and globals should be MAP_PRIVATE and PROT_NONE
    xmemory::getInstance().setupMemoryMappings();

    // Saving the stack, set initial stack to SHARED and switch the stack.
    // Note: we don't want to switch stack completely since that needs to 
    // change all previous stack frames to new stack, that is error prone:-)
    // Otherwise, it will jump back after the poping out of current frame.
    // Also, we only want a private stack since we will fork new proceses soon.
    // It is impossible to make it right if we are using a shared stack, since
    // multiple process will modify the shared stack simultaneously:-)
    allocPrivateStack(stacksize);
  
    // Save the stack to the private stack and switch to this private stack. 
    // Get the new stack bottom
    newstackBottom = (void *)((intptr_t)privateStack + stacksize);

    // Get current esp and ebp
    asm volatile(
      "movl %%ebp, %0 \n"
      "movl %%esp, %1 \n"
      : "=r" (ebp), "=r" (esp)
    );

    // Create the scheduler thread here for the main process
    scheduler = proc.spawnSchedulerThread();

    /*** Calculating the stack related part ***/
    // We only need to copy those stacks that existing now
    copystacksize = ((intptr_t)stackEnd - (intptr_t)esp);
    
    // Calculate the new esp and ebp after switching.
    newesp = ((intptr_t)newstackBottom - ((intptr_t)stackEnd - esp));
    newebp = ((intptr_t)newstackBottom - ((intptr_t)stackEnd - ebp));
 
//    fprintf(stderr, "Before switching, %d (getpid %d): mainthread %p\n", pid, getpid(), mainthread);
//    fprintf(stderr, "before switching, pstackbottom %p pstackStart %p pstackTop %p esp 0x%x ebp 0x%x\n", stackEnd, stackStart, esp, esp, ebp); 
//    fprintf(stderr, "before switching, newstackbottom %p newesp 0x%x newebp 0x%x, distance %x.\n", newstackBottom, newesp, newebp, copystacksize); 

    // Copy the whole stack to the new stack (private). We could copy back after forkings
    memcpy((void *)((intptr_t)newstackBottom - copystacksize), (void *)esp, copystacksize);

    // Switch the stack manually here!!!!
    // It is important to switch here (not use a function call), otherwise, the lowest
    // level of frame will be poped out and the stack will return back to the original one
    // Then techniquely we didn't switch successfully. 
    // What we want is that new frames are using the new stack, but we will recover
    // the stack in the same function later to void problems!!!
    asm volatile(
      // Set ebp and esp to new pointer
      "movl %0, %%ebp\n"
      "movl %1, %%esp\n"
      : : "r" (newebp), "r" (newesp)
    );
 //   fprintf(stderr, "After switching, %d (getpid %d): mainthread %p\n", pid, getpid(), mainthread);
       
   // PRDBG("after switching, newstackbottom %p newesp 0x%x newebp 0x%x, distance %x.\n", newstackBottom, newesp, newebp, distance); 
    // Set the initial stack to be shared by all processes.
    // Do this before forking thus children processes can see this stack (MAP_SHARED).
    // The initial thread may be run on other processes later, you never know:-)
    PRDBG("share initial stack\n");
    mainthread->shareInitialStack(stackStart, stacksize);

    // Now we can set postinitialized
    // Note: we should set this before we are doing memcpy operation, otherwise,
    // the result won't show up in the new stack.
    postinitialized = true;

    // Recover the stack to initial shared stack.
    memcpy((void *)esp, (void *)((intptr_t)newstackBottom - copystacksize), copystacksize);

    
    //while(1) ;
    // Create other processes and binding them to specific core
    createOtherProcesses(CPU_CORES);  

    //fprintf(stderr, "%d: after create other processes\n", getpid());
    // Note: we can't do this before forking processes to avoid stack smashing.
    // switching back the original stack.
    asm volatile(
      // Set ebp and esp to new pointer
      "movl %0, %%ebp\n"
      "movl %1, %%esp\n"
      : : "r" (ebp), "r" (esp)
    );
  
    // Reset the stack for initial thread context.
    fprintf(stderr, "%d: after create other processes and switch back, now postinitialized %d\n", getpid(), postinitialized);
    mainthread->resetStack(stackStart, stacksize);

    // Now make the scheduler thread to have a chance to run, otherwise switching to
    // scheduler thread can cause problems later.
    threadYieldInitially(getCurrentPQueue()); 
  }

  // We may use a inode attribute to analyze whether we need to do this.
  bool toHandleLibraryEntry(std::string & mapentry) {
    bool toHandle = false;
    using namespace std;
    static bool lastLibraryEntry = false;
    // Check whether it is library, normally library's inode is "08:06".
    if(mapentry.find(" 08:06 ", 0) != string::npos) {
      lastLibraryEntry = true;
       
     // if(mapentry.find("rw-p", 0) != string::npos) {
      // We don't need to handle libc.so. I am not sure why?
      // But it will cause the problem of streamcluster. stol will fail.
      if(mapentry.find("rw-p", 0) != string::npos && mapentry.find("libc.so", 0) == string::npos) {
        toHandle = true;
      }
    }
    else if((mapentry.find(" 00:00 ", 0) != string::npos) 
         && (lastLibraryEntry == true) 
         && (mapentry.find("rw-p", 0) != string::npos)) {
      toHandle = true;
      lastLibraryEntry = false;
      // check the permissions of this entry
      // Normally, globals should have the permission like "rw-p".
    }
    else {
      lastLibraryEntry = false;
    }

    return toHandle;
  }
 
  // makeMappingShared between "begin" and "end"
  void makeMappingShared(void * begin, void * end) {
    void * ptr;
    int size;
    void * temp;

    size = (intptr_t)end - (intptr_t)begin;

    if(size > 0x10000) {
      PRWRN("mapping from begin %p and %p, size %x is too large\n", begin, end, size); 
    }

    if(size == 0 || size > 0x10000)
      return;

    // Malloc a block of memory.
    temp = malloc(size);;

    // Store something in this temporary place. 
    memcpy(temp, begin, size);

    // We have to mmaped to specified address
    ptr = WRAP(mmap)(begin, size, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    if(ptr != begin) {
      PRERR("Can't allocate memory for library globals between %p and %p\n", begin, end);
      abort();
    }

    // Copy back those initialized data of globals.
    memcpy(ptr, temp, size);

    free(temp);
    return;
  }
 
  // Trying to get stack information. 
  long getStackInformation(void) {
    using namespace std;
    ifstream iMapfile;
    string mapentry;

    try {
      iMapfile.open("/proc/self/maps");
    } catch(...) {
      PRERR("can't open the maps file, exit now\n");
      abort();
    } 

    // Now we analyze each line of this maps file.
    while(getline(iMapfile, mapentry)) {

      // Check whether it is library, normally library name is end with ".so".
      if(mapentry.find("[stack]", 0) != string::npos) {
        string::size_type pos = 0;
        string::size_type endpos = 0;
        // Get the starting address and end address of this entry.
        // Normally the entry will be like this
        // ffce9000-ffcfe000 rw-p 7ffffffe9000 00:00 0   [stack]
        string beginstr, endstr;

        while(mapentry[pos] != '-') pos++;
          beginstr = mapentry.substr(0, pos);
  
          // Skip the '-' character
          pos++;
          endpos = pos;
        
          // Now pos should point to a space or '\t'.
          while(!isspace(mapentry[pos])) pos++;
          endstr = mapentry.substr(endpos, pos-endpos);

          // Now we get the start address of stack and end address
          stackStart = (void *)strtoul(beginstr.c_str(), NULL, 16);
          stackEnd = (void *)strtoul(endstr.c_str(), NULL, 16);
   
        fprintf(stderr, "stackStart %p stackEnd %p\n", stackStart, stackEnd);
      }
    }
    iMapfile.close();

    // Return the size of stack.
    return((intptr_t)stackEnd - (intptr_t)stackStart);
  } 

  // Change all globals of libraries to MAP_SHARED
  void shareLibraryGlobals(void) {
    // Those libraries are already loaded now. 
    // We can simply analyze the /proc/self/maps to find out those 
    // writable private segement and change to MAP_SHARED
    using namespace std;
    ifstream iMapfile;
    string mapentry;

    try {
      iMapfile.open("/proc/self/maps");
    } catch(...) {
      PRERR("can't open the maps file, exit now\n");
      abort();
    } 

    // Now we analyze each line of this maps file.
    while(getline(iMapfile, mapentry)) {

      // Check whether it is library, normally library name is end with ".so".
      if(toHandleLibraryEntry(mapentry)) {
        string::size_type pos = 0;
        string::size_type endpos = 0;
        // Get the starting address and end address of this entry.
        // Normally the entry will be like this
        // "00797000-00798000 rw-p ...."
        string beginstr, endstr;

        while(mapentry[pos] != '-') pos++;
        beginstr = mapentry.substr(0, pos);
  
        // Skip the '-' character
        pos++;
        endpos = pos;
        
        // Now pos should point to a space or '\t'.
        while(!isspace(mapentry[pos])) pos++;
        endstr = mapentry.substr(endpos, pos-endpos);

        void * startaddr, * endaddr;

        //startaddr = (void *)(atoi(beginstr.c_str()));
        //endaddr = (void *)(atoi(endstr.c_str()));
    
        startaddr = (void *)strtoul(beginstr.c_str(), NULL, 16);
        endaddr = (void *)strtoul(endstr.c_str(), NULL, 16);
      //  cout << "current entry:" << mapentry << "." << "Start:" << beginstr << ".end:" << endstr<< endl;
        //cout << "Start:" << startaddr << ".end:" << endaddr << "." << endl;
        makeMappingShared(startaddr, endaddr);    
        //mapShared();
      }
    }
    
    iMapfile.close();
  }

  void finalize (void)
  {
    fflush(stdout);

    // If the tid was set, it means that this instance was
    // initialized: end the transaction (at the end of main()).
    xmemory::getInstance().finalize();

    //fprintf(stderr, "%d: xrun exit in finalize function\n", getpid());

    if(!postinitialized) {
      return;
    }

    // Terminiate all proceses when we are here. 
    // Now we should guarantee that now I am in the initial process.
    int coreid = process::getInstance().getCurrent()->getBoundCore();

    // Switch to bounded core if not.
    if(process::getInstance().getCoreId() != coreid) {
      xqueue * pqueue = processmap::getInstance().getPQueue(coreid);
      threadYieldToRunQueue(pqueue);
    }
   
    // Check whether I am running on the bounded core.
    assert(process::getInstance().getCoreId() == coreid);

    for(int i = 1; i < CPU_CORES; i++) {
      pid_t id = procmap.getPid(i);
      kill(id, SIGKILL);
      //if(id > 0)
      //xsignal::getInstance().signalKill(id);
    }
  }

  /// @brief create processs and binding them to different cores.
  void createOtherProcesses(int cores) {
    int i;

    // Create other processes and binding them to specific cores
    for(i = 1; i < cores; i++) {
      proc.create(i);
    }
  }

  /// @return the "thread" id.
  inline int gettid (void) {
    xthread * thread = proc.getCurrent();
   
    return thread->getTid();
  }

  // Set process id for current thread
  inline pid_t getpid(void) {
    return pid;
  }
  
  inline void setpid(int coreid, pid_t pid) {
    this->pid = pid;
  }

  // @brief: allocate private stack before creating other processes.
  void allocPrivateStack(long size) {
    privateStack = MMAP_PRIVATE(size);
  }

  // @brief: free the private stack after usage.
  // Since the allocation happened before forkings, 
  // every process should called this after no usage
  void freePrivateStack(void) {
    long size = (intptr_t)stackEnd - (intptr_t)stackStart;

    MUNMAP(privateStack, size);
  }

  /// @brief Save context for current thread
  /// We will create a context for current thread with specified
  inline xthread * setupInitialThread(void) {
    int tid = 0;
 
    // Create a block of memory to hold this task.
    void * ptr = MMAP_SHARED(sizeof(xthread));
    
    //printf("starting initial thread\n");
    xthread * thread = new (ptr)xthread(tid);

    threadsmap.registerThread(tid, thread, true);

    // Bounding the initial thread to initial process pid.
    thread->setBoundCore(0);
 
    // Set current thread
    proc.setCurrent(thread);
    return thread; 
  }

  // Insert a thread to the global queue
  void insertGlobalQueue(xthread * thread) {
    thread->setThreadRunning();
    squeue->enqueue(thread);
  }
 
  /// @brief Spawn a thread.
  /// @return an opaque object used by sync.
  inline pthread_t spawn (void * threadFunc, void * arg)
  {
    // check whether we have call postinit
    if(postinitialized == false) {
      postinit();
    }

    int tid = threadsmap.allocTid();
    
    void * ptr = MALLOC_SHARED(sizeof(xthread));
 
    // Create a block of memory to hold this task.
    PRDBG("%d: spawning user thread %p. ptr %p to 0x%x\n", getpid(), threadFunc, ptr, (intptr_t)ptr + sizeof(xthread));
    //printf("spawn new thread %d\n", tid);
    xthread *  thread = new (ptr)xthread(tid);
 
    // Register this thread block
    threadsmap.registerThread(tid, thread, true);
   
    // Spawn a thread 
    thread->spawn(threadFunc, arg);

    // Insert this thread into the global queue, then
    // an idle process will pickup this thread and try to run that.
    //PRWRN("%d: spawning user thread %p (tid %d). ptr %p to 0x%x\n", getpid(), threadFunc, tid, ptr, (intptr_t)ptr + sizeof(xthread));
    insertGlobalQueue(thread);

    return tid;
  }

  /// @brief Wait for a thread.
  inline void join (pthread_t tid, void ** result) {
    // Now we have to find out which thread we are going to join
    xthread * thread = proc.getCurrent();
    //PRWRN("thread join tid %d\n", tid); 
    thread->join(thread, tid, result);
    PRDBG("thread join tid %d\n", tid); 
  }

  /// @brief Do a pthread_cancel
  inline void cancel(int tid) {
    //fprintf(stderr, "%d: cancel a thread %d\n", getpid(), tid);
   // xthread * thread = proc.getCurrent();
    //PRWRN("thread join tid %d\n", tid);
    // In the implementation of spawn, we actually return 
    // the threadid (int) to the user. So the user can only
    // utilize this tid. That is, the  
   // thread->cancel(thread, tid);
  }

  inline void thread_kill(void *v, int sig) {
    //_thread.thread_kill(this, v, sig);
  } 

  /* Heap-related functions. */
  inline void * malloc (size_t sz) {
    void * ptr = xmemory::getInstance().malloc (heapid, sz);
    return ptr;
  }

  // In fact, we can delay to open its information about heap.
  inline void free (void * ptr) {
    xmemory::getInstance().free (heapid, ptr);
  }

  inline size_t getSize (void * ptr) {
    return xmemory::getInstance().getSize (ptr);
  }

  inline void * realloc(void * ptr, size_t sz) {
    void * newptr;
    //PRDBG("realloc ptr %p sz %x\n", ptr, sz);
    if (ptr == NULL) {
      newptr = malloc(sz);
      return newptr;
    }
    if (sz == 0) {
      free (ptr);
      return NULL;
    }

    // Do the normal realloc operation.
    size_t s = getSize (ptr);
    newptr =  malloc(sz);
    if (newptr && s != 0) {
      size_t copySz = (s < sz) ? s : sz;
      memcpy (newptr, ptr, copySz);
    }

    free(ptr);
    return newptr;
  }

  // Handling the mmap system call.
  void * mmap(void *addr, size_t length, int prot,
         int flags,  int fd, off_t offset) {
    return xmemory::getInstance().mmap(postinitialized, addr, length, prot, flags, fd, offset); 
  }

  /// Save an actual mutex address in original mutex.
  /// Mutex function calls. Here, we are using a totally different mechanism with
  /// sheriff or dthreads. In sheriff, we will have a private mapping
  /// for those synchronizations, thus it can't be used to synchronize different processes.
  /// However, proto will share a mapping between different processes. 
  /// Thus, we can re-utilize the memory allocated from user space. 
  int mutex_init(pthread_mutex_t * mutex) {
    xmutex * mx = (xmutex *)mutex;
    mx->mutexInit();
    return 0;
  }

  void printMutex(void * mutex) {
    int i;
    int * ptr = (int *)mutex;

    // check mutex;
    for(i = 0; i < sizeof(pthread_mutex_t)/sizeof(int);i++) {
      fprintf(stderr, "xrun %d: MUTEX %d: 0x%x (at %p)\n", getpid(), i, ptr[i], &ptr[i]);
    }
  }

  int mutex_lock(pthread_mutex_t * mutex) {
    xmutex * mx = (xmutex *)mutex;
  //  fprintf(stderr, "mutex lock on %p\n", mutex);
    mx->mutexLock(getCurrent());
    return 0;
  }

  int mutex_unlock(pthread_mutex_t * mutex) {
    xmutex * mx = (xmutex *)mutex;
    xthread * current = getCurrent();
    xqueue  * pqueue = getCurrentPQueue();
    mx->mutexUnlock(current, pqueue);
    return 0;
  }

  int mutex_destroy(pthread_mutex_t * mutexptr) {
    xmutex * mutex = (xmutex *)mutexptr;
    mutex->mutexDestroy();
    return 0;
  }
  
  ///// conditional variable functions.
  void cond_init (pthread_cond_t * condptr) {
    xcondvar * cond = (xcondvar *)condptr;

    cond->condInit();
  }

  void cond_destroy (pthread_cond_t * condptr) {
    xcondvar * cond = (xcondvar *)condptr;

    cond->condDestroy();
  }

  /// FIXME: whether we can using the order like this.
  void cond_wait(pthread_cond_t * condptr, pthread_mutex_t * mutexptr) {
    xcondvar * cond = (xcondvar *)condptr;
    xthread * current = getCurrent();
    //PRWRN("thread %d is waiting: condptr %p mutexptr %p\n", current->getTid(), condptr, mutexptr);
    cond->condWait(mutexptr, current);
   // PRERR("thread %d: condptr %p mutexptr %p\n", current->getTid(), condptr, mutexptr);
  }

  void cond_broadcast (pthread_cond_t * condptr) {
    xcondvar * cond = (xcondvar *)condptr;
    xqueue  * queue = getShareQueue();
    xthread * current = getCurrent();
    cond->condBroadcast(current, queue);
  }

  void cond_signal (pthread_cond_t * condptr) {
    xcondvar * cond = (xcondvar *)condptr;
    xqueue  * queue = getShareQueue();
    xthread * current = getCurrent();
    cond->condSignal(current, queue);
  }

  // Barrier support
  int barrier_init(pthread_barrier_t  *barrier, unsigned int count) {
    xbarr * barr = (xbarr *)barrier;

    barr->barrInit(count);
    return 0;
  }

  int barrier_destroy(pthread_barrier_t *barrier) {
    xbarr * barr = (xbarr *)barrier;

    barr->barrDestroy();
    return 0;
  }


  int barrier_wait(pthread_barrier_t *barrier) {
    xbarr * barr = (xbarr *)barrier;
    xqueue  * queue = getShareQueue();
    xthread * current = getCurrent();

    barr->barrWait(current, queue);
    return 0;
  }

  void setHeapid(int id) {
    heapid = id;
  }

  xqueue * getShareQueue(void) {
    return squeue;
  }

  xqueue * getPrivateQueue(int coreid) {
    return procmap.getPQueue(coreid);
  }

  xqueue * getDeadQueue(void) {
    return dqueue;
  }
private:
  int allocTid(void) {
    return threadsmap.allocTid();
  } 

  xthread * getCurrent(void) {
    return proc.getCurrent();
  }

  xqueue * getCurrentPQueue(void) {
    return proc.getPQueue();
  }

  xmap    &threadsmap;

  xthread * mainthread;

  // Should we interfere the process management?
  // If not, then we could put "process" on the stack.
  process & proc;

  // stack information.
  void * stackStart;
  void * stackEnd;

  // Process 
  processmap & procmap;

  // Global barrier
  pthread_cond_t * gcondvar; 

  // Private stack used by different processes.
  void * privateStack; 

  // The global run queue for those threads which don't have preferred
  // cores, for example, those newly created threads
  xqueue * squeue;
  xqueue * dqueue; // Deadqueue, one thread's exit will put it to the dqueue.
  pid_t    pid; //Current process id.
  int      heapid; //Which heap we are going to use.
  int      maxprocs; // Max process

  bool     postinitialized;
};


#endif
