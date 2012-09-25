// -*- C++ -*-

#ifndef _XQUEUE_H_
#define _XQUEUE_H_

#include <errno.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#endif

#include <iostream>
#include <queue>
#include <pthread.h>
#include <stdlib.h>

#include "xdefines.h"
#include "xthread.h"
#include "xcontext.h"
#include "list.h"
#include "internalheap.h"
#include "xplock.h"
#include "spinlock.h"
#include "process.h"

//#define USE_MUTEX_LOCK 1
// Looks like that the implementation of mutex is much more efficient than
// the spin lock.
class xqueue {

public:
  xqueue () {
    //fprintf(stderr, "threadqueue constructor\n");
    // Initialize the queue list
    listInit(&queue);
  } 
  
  // push a thread to queue
  void enqueue(xthread * thread) {
    lock();
    listInsertTail(&thread->toqueue, &queue); 
    //fprintf(stderr, "ENQUEUE %d: queue %p queue->prev %p queue->next %p\n", getpid(), &queue, queue.prev, queue.next);
    if(hasWork() != true) {
      fprintf(stderr, "************WRONG!!!%d (on lock %p): enqueue thread %p with tid %d. After queue.prev %p queue %p queue.next %p\n", getpid(), &qlock, &thread->toqueue, thread->getTid(), queue.prev, &queue, queue.next);
      assert(hasWork() == true);
    }
   // PRWRN("enqueue thread %p with tid %d ater hasWorkd %d\n", thread, thread->getTid(), hasWork());
    unlock();
  }

  // Add the whole list into the queue
  void enqueueAllList(lnode * list) {
   
    lock();

    // Insert the list to the tail of queue and skip "list" node.
    listInsertListTail(list, &queue);


    unlock();
  }

  // Pop a thread from the queue
  xthread * dequeue(void) {
    xthread * thread = NULL;
    lnode * node = NULL;

    lock();

   // fprintf(stderr, "DEQUEUE %d: node is %p. Now queue %p queue->prev %p queue->next %p\n", getpid(), node, &queue, queue.prev, queue.next);
    if(hasWork()) {
      node = queue.next;
      listRemoveNode(node);
    
      // Check the list.
      //listPrintItems(&queue, 8);
    }

    unlock();

    // Get corresponding thread for this thread
    if(node) {
      thread = container_of(node, xthread, toqueue);  
    }

    return thread;
  }

  // Check whether the queue is empty or not
  bool hasWork(void) {
    return (!isListEmpty(&queue));
  }

  void * getLock(void) {
    return &qlock;
  }
  
  void * getQueue(void) {
    return &queue;
  }

private:

  void lock() {
#ifdef USE_MUTEX_LOCK
    qlock.lock();
#else
    qlock.acquire();
#endif
//    fprintf(stderr, "%d: lock the queue %p\n", getpid(), &queue);
  }

  void unlock() {
#ifdef USE_MUTEX_LOCK
    qlock.unlock();
#else
    qlock.release();
#endif
 //   fprintf(stderr, "%d: unlock the queue %p\n", getpid(), &queue);
  }

  // which core is located, this will affect the heap allocation too.
#ifdef USE_MUTEX_LOCK
  xplock qlock;
#else
  spinlock qlock;
#endif

  struct lnode queue;

  // padding to avoid the false sharing problem.
  char padding[128];
};

#endif
