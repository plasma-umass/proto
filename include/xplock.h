#ifndef _XPLOCK_H_
#define _XPLOCK_H_

#if !defined(_WIN32)
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "internalheap.h"

/**
 * @class xplock
 * @brief A cross-process lock.
 * 
 * Note: lock must be allocated in a shared memory space in order to
 *       make it work correctly.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

class xplock {
public:

  xplock (void) {
  	/// The lock's attributes.
  	pthread_mutexattr_t attr;

    // Set up the lock with a shared attribute.
    WRAP(pthread_mutexattr_init) (&attr);
    pthread_mutexattr_setpshared (&attr, PTHREAD_PROCESS_SHARED);

    WRAP(pthread_mutex_init) (&_lock, &attr);
  }

  /// @brief Lock the lock.
  void lock (void) {
    WRAP(pthread_mutex_lock) (&_lock);
  }

  /// @brief Unlock the lock.
  void unlock (void) {
    WRAP(pthread_mutex_unlock)(&_lock);
  }

private:

  /// A pointer to the lock.
  pthread_mutex_t _lock;
};

#endif
