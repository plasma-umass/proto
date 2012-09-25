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
 * @file:   streambuffer.h
 * @brief:  Keeping the active mapping between FILE and buffer.  
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _STREAMBUFFER_H_
#define _STREAMBUFFER_H_

#include <stdio.h>
#include "list.h"
#include "spinlock.h"
#include "memwrapper.h"

class streambuffer {
  class bufferentry{
    public:
      FILE * file;
      void * buffer;
      struct lnode list;
  };

public:
  streambuffer() {
    // Initialize the list.
    listInit(&list);
  }

  // It is an actual sigleton which are shared by multiple threads.
  static streambuffer& getInstance (void) {
    static streambuffer * threadmapObject;
    if(!threadmapObject) {
      void *buf = MALLOC_SHARED(sizeof(streambuffer));
      threadmapObject = new(buf) streambuffer();
    }
    return * threadmapObject;
  }

 
  // Save a mapping 
  void recordBuffer(FILE * file, void * buffer) {
    bufferentry * entry;

    entry = (bufferentry *)MALLOC_SHARED(sizeof(bufferentry));
    if(entry == NULL) {
      PRERR("No shared memory to save entry.\n");
      abort();
    }

    entry->file = file;
    entry->buffer = buffer;

    // Add this entry to the global list
    lock();
    listInsertTail(&entry->list, &list);
    unlock(); 
  }


  // Remove a mapping before fclose
  void removeBuffer(FILE * file) {
    bufferentry * entry = NULL;
    bool isFound = false;

    lock();

    // If list is not empty
    if(!isListEmpty(&list)) {
      entry = container_of(list.next, bufferentry, list);
    }

    while(entry) {
      if(entry->file == file) {
        isFound = true;
        break;
      }

      // Make entry pointing to next available entry.
      if(!isListTail(&entry->list, &list)) {
        entry = container_of(entry->list.next, bufferentry, list);
      }
      else {
        entry = NULL;
      }
    } 
    
    // Remove this entry from the list if it is found.
    if(isFound) {
      listRemoveNode(&entry->list);
    }
  
    unlock();

    if(isFound) {
      assert(entry != NULL);

      // Unmap corresponding buffer.
      MUNMAP(entry->buffer, xdefines::FILE_BUFFER_SIZE);
    
      // Free corresponding entry.
      FREE_SHARED(entry);
    }
    
  } 

  
private:
  void lock() {
    mylock.acquire();
  }

  void unlock() {
    mylock.release();
  }

  // which core is located, this will affect the heap allocation too.
  spinlock mylock;
  struct lnode list;
};


#endif /* _ */
