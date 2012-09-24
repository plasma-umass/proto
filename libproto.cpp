// -*- C++ -*-

/*
  Copyright (c) 2011, University of Massachusetts Amherst.

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

/**
 * @file libsheriff.cpp
 * @brief Interface with outside library.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *
 */

#if !defined(_WIN32)
#include <dlfcn.h>
#endif

#include <stdarg.h>
#include "memwrapper.h"
#include "log.h"
#include "streambuffer.h"
#include "xrun.h"

extern "C" {

#if defined(__GNUG__)
  void initializer (void) __attribute__((constructor));
  void finalizer (void)   __attribute__((destructor));
#endif

#define INITIAL_MALLOC_SIZE 81920
  //static bool *initialized;
  static bool initialized = false;
  static char * tempmalloc = NULL;
  static int remainning = 0;
  // Bug for proto: we are created processes xrun::getInstance().initialize(),
  // if initialized is in the "library-related" global segment, 
  // setting "initialized" won't affect other processes at all.
  // Thus, other processes will skip all synchronization since initialized is still false.
  // To resolve this problem, we can have two approaches.:
  // First, we divide the initalize to two phases, thus, creating children proceses after the setting.
  // Second, we are using a share memory.
  void initializer (void) {
    // Using globals to provide allocation
    // before initialized.
    // We can not use stack variable here since different process
    // may use this to share information. 
    static char tempbuf[INITIAL_MALLOC_SIZE];

    // temprary allocation
    tempmalloc = (char *)tempbuf;
    remainning = INITIAL_MALLOC_SIZE;

    // Try to get those library functions address
    init_real_functions();

    xrun::getInstance().initialize();
    initialized = true;
   
    // Now we can start processes and switch stack now.
    //xrun::getInstance().postinit(); 

    //printf ("tempmalloc is %p.\n", tempmalloc); fflush (stdout);
    // Start our first transaction.
#ifndef NDEBUG
//    printf ("we're gonna begin now.\n"); fflush (stdout);
#endif
  }

  void finalizer (void) {
  //  fprintf(stderr, "%d: Finalizer\n", getpid());
    initialized = false;
    xrun::getInstance().finalize();
  }

  static void * mytempmalloc(int size) {
    void * ptr = NULL;
    if(remainning < size) {
      // complaining. Tried to set to larger
      fprintf(stderr, "Not enought temporary buffer, size %d remainning %d\n", size, remainning);
      exit(-1);
    }
    else {
      ptr = (void *)tempmalloc;
      tempmalloc += size;
      remainning -= size;
    }
    return ptr; 
  }

  bool isInitialized(void) {
    return initialized;
  }

  /// Functions related to memory management.
  void * proto_malloc (size_t sz) {
    void * ptr;
    if (!isInitialized()) {
      // Now we can't use normall malloc and mmap,
      // we have to use some temporary memory to allocate
      ptr = mytempmalloc(sz);
    } else {
      ptr = xrun::getInstance().malloc (sz);
    }
 
    if (ptr == NULL) {
      fprintf (stderr, "Out of memory when initialization is %d!\n", isInitialized());
      ::abort();
    }
    return ptr;
  }
  
  void * proto_calloc (size_t nmemb, size_t sz) {
    void * ptr = proto_malloc(sz *nmemb);
    if (isInitialized()) {
	    memset(ptr, 0, sz*nmemb);
    }
    return ptr;
  }

  void proto_free (void * ptr) {
    if (isInitialized()) {
      xrun::getInstance().free (ptr);
    }
  }

  size_t proto_malloc_usable_size(void * ptr) {
    if(isInitialized()) {
      return xrun::getInstance().getSize(ptr);
    }
    return 0;
  }

  void * proto_memalign (size_t boundary, size_t size) {
	  fprintf(stderr, "%d : proto don't support memalign. boundary %d size %d\n", getpid(), boundary, size);
    ::abort();
    return NULL;
  }

  void * proto_realloc (void * ptr, size_t sz) {
    return xrun::getInstance().realloc (ptr, sz);
  }

#if 0 
  void * malloc (size_t sz) throw() {
    return proto_malloc(sz);
  }

  void * calloc (size_t nmemb, size_t sz) throw() {
    return proto_calloc(nmemb, sz);
  }

  void free(void *ptr) throw () {
    proto_free(ptr);
  }
  
  void* realloc(void * ptr, size_t sz) {
    return proto_realloc(ptr, sz);
  }

  void * memalign(size_t boundary, size_t sz) { 
    return proto_memalign(boundary, sz);
  }
#endif
  /// Threads's synchronization functions.
  // Mutex related functions 
  int pthread_mutex_init (pthread_mutex_t * mutex, const pthread_mutexattr_t* attr) {    
    if (isInitialized()) 
      return xrun::getInstance().mutex_init (mutex);
    else 
      return 0;
  }
  
  int pthread_mutex_lock (pthread_mutex_t * mutex) {   
    if (isInitialized()) { 
  //    PRWRN("proto: mutex_lock\n");
      xrun::getInstance().mutex_lock (mutex);
    }

    return 0;
  }

  // FIXME: add support for trylock
  int pthread_mutex_trylock(pthread_mutex_t * mutex) {
    return 0;
  }
  
  int pthread_mutex_unlock (pthread_mutex_t * mutex) {    
    if (isInitialized()) 
      xrun::getInstance().mutex_unlock (mutex);

    return 0;
  }

  int pthread_mutex_destroy (pthread_mutex_t * mutex) {    
    if (isInitialized()) 
      return xrun::getInstance().mutex_destroy (mutex);
    else
      return 0;
  }
  
  // Condition variable related functions 
  int pthread_cond_init (pthread_cond_t * cond, const pthread_condattr_t* condattr)
  {
    if (isInitialized()) 
      xrun::getInstance().cond_init (cond);
    return 0;
  }

  int pthread_cond_broadcast (pthread_cond_t * cond)
  {
    if (isInitialized()) 
      xrun::getInstance().cond_broadcast (cond);
    return 0;
  }

  int pthread_cond_signal (pthread_cond_t * cond) {
    if (isInitialized()) 
      xrun::getInstance().cond_signal (cond);
    return 0;
  }

  int pthread_cond_wait (pthread_cond_t * cond, pthread_mutex_t * mutex) {
    if (isInitialized()) 
      xrun::getInstance().cond_wait (cond, mutex);
    return 0;
  }

  int pthread_cond_destroy (pthread_cond_t * cond) {
	if (isInitialized()) 
    	xrun::getInstance().cond_destroy (cond);
    return 0;
  }

  // Barrier related functions 
  int pthread_barrier_init(pthread_barrier_t  *barrier,  const pthread_barrierattr_t* attr, unsigned int count) {
    if (isInitialized()) 
      return xrun::getInstance().barrier_init (barrier, count);
    else
      return 0;
  }

  int pthread_barrier_destroy(pthread_barrier_t *barrier) {
    if (isInitialized()) 
      return xrun::getInstance().barrier_destroy (barrier);
    else
      return 0;
  }
  
  int pthread_barrier_wait(pthread_barrier_t *barrier) {
    if (isInitialized()) 
      return xrun::getInstance().barrier_wait (barrier);
    else
      return 0;
  }  

  int pthread_cancel (pthread_t thread) {
    xrun::getInstance().cancel((int)thread);
    return 0;
  }
  
  int sched_yield (void) 
  {
    return 0;
  }

  void pthread_exit (void * value_ptr) {
    // FIXME?
    _exit (0);
    // This should probably throw a special exception to be caught in spawn.
  }

  // Here, getpid() will return the process id to run current
  // thread. But there is no relatation with specific thread. 
  int getpid(void) {
    return xrun::getInstance().getpid();
  }
 

  int pthread_setconcurrency (int) {
    return 0;
  }

  int pthread_attr_init (pthread_attr_t *) {
    return 0;
  }

  int pthread_attr_destroy (pthread_attr_t *) {
    return 0;
  }

  // Return the tid of current thread
  pthread_t pthread_self (void) 
  {
    return xrun::getInstance().gettid();
  }

  int pthread_kill (pthread_t thread, int sig) {
    // FIX ME
    xrun::getInstance().thread_kill((void*)thread, sig);
    return 0;
  }

#if 0
  int pthread_rwlock_destroy (pthread_rwlock_t * rwlock) NOTHROW
  {
    return 0;
  }

  int pthread_rwlock_init (pthread_rwlock_t * rwlock,
			   const pthread_rwlockattr_t * attr) NOTHROW
  {
    return 0;
  }

  int pthread_detach (pthread_t thread) NOTHROW
  {
    return 0;
  }

  int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) NOTHROW
  {
    return 0;
  }

  int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock) NOTHROW
  {
    return 0;
  }


  int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) NOTHROW
  {
    return 0;
  }

  int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock) NOTHROW
  {
    return 0;
  }

  int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) NOTHROW
  {
    return 0;
  }

#endif
  int pthread_attr_getstacksize (const pthread_attr_t *, size_t * s) {
    *s = 1048576UL; // really? FIX ME
    return 0;
  }

  int pthread_mutexattr_destroy (pthread_mutexattr_t *) { return 0; }
  int pthread_mutexattr_init (pthread_mutexattr_t *)    { return 0; }
  int pthread_mutexattr_settype (pthread_mutexattr_t *, int) { return 0; }
  int pthread_mutexattr_gettype (const pthread_mutexattr_t *, int *) { return 0; }
  int pthread_attr_setstacksize (pthread_attr_t *, size_t) { return 0; }

  int pthread_create (pthread_t * tid,
		      const pthread_attr_t * attr,
		      void *(*start_routine) (void *),
		      void * arg) 
  {
    *tid = (pthread_t)xrun::getInstance().spawn ((void *)start_routine, arg);
    return 0;
  }

  int pthread_join (pthread_t tid, void ** val) {
    xrun::getInstance().join (tid, val);
    return 0;
  }

#if 0
  ssize_t read (int fd, void * buf, size_t count) {
    int * start = (int *)buf;
    long pages = (((intptr_t)buf & xdefines::PAGE_SIZE_MASK)+count)/xdefines::PageSize;

    // Trying to read on those pages, thus there won't be a segmenation fault in 
    // the system call.
    for(long i = 0; i < pages; i++) {
        start[i * 1024] = 0;
    }
    start[count/sizeof(int)-1] = 0;

    return WRAP(read)(fd, buf, count);
  }

  ssize_t write (int fd, const void * buf, size_t count) {
    int * start = (int *)buf;
    long pages = (((intptr_t)buf & xdefines::PAGE_SIZE_MASK)+count)/xdefines::PageSize;
    volatile int temp;


    // Trying to read on those pages, thus there won't be a segmenation fault in 
    // the system call.
    for(long i = 0; i < pages; i++) {
        temp = start[i * 1024];
    }
    temp = start[count/sizeof(int)-1];
    return WRAP(write)(fd, buf, count);
  }
#endif

  // Intercepting fopen calls.
  // We DONOT need to intercept setbbuf function at all. 
  // setvbuf function can be called only after fopen, that is,
  // we will call setvbuf first.
  // If user are trying to call setvbuf, it will change the buffer to
  // heap, stack or mmap area, which we can intercept.
  // So there is no need to worry about it.
  FILE *fopen(const char *path, const char *mode){
    FILE * file;
    void * ptr;

    // Call normal fopen function.
    file = WRAP(fopen)(path, mode);

    if(isInitialized()) {
      // MMAP a block of MAP_SHARED area
      ptr = MMAP_SHARED(xdefines::FILE_BUFFER_SIZE);

      // setvbuf to a MAP_SHARED buffer if possible.
      // This part of memory will be freed when fclose.
      if(ptr != MAP_FAILED) {
//        PRWRN("fopen, using %p as a buffer\n", ptr);
        streambuffer::getInstance().recordBuffer(file, ptr);

        setvbuf(file, (char *)ptr, _IOFBF, xdefines::FILE_BUFFER_SIZE);
      }
      else {
        setvbuf(file, NULL, _IONBF, 0);
      }
    }
    else {
      PRWRN("fopen before initialize\n");
      setvbuf(file, NULL, _IONBF, 0);
    }
    return file;
  }

  int fclose(FILE *file) {
    fflush(file);

    if(isInitialized()) {
      // Free the shared buffer entry
      streambuffer::getInstance().removeBuffer(file);
    }

    return WRAP(fclose)(file);
  }

  // Intercept the mmap() and call our special mmap
  // We have to change MMAP_PRIVATE to MMAP_SHARED
  // If fopen(O_RDONLY), then we must change the at
  void * mmap(void *addr, size_t length, int prot, 
		     int flags,  int fd, off_t offset) {
	  void * ptr = NULL;
    int newflags = flags;
 
    if(isInitialized()) {
   //   WRAP(mmap)(addr, length, prot, flags, fd, offset);
     ptr = xrun::getInstance().mmap(addr, length, prot, flags, fd, offset);
    } 
    else {
      PRFATAL("We can't call mmap before initialization");
    }
    return ptr;
  }
}


