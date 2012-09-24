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
 * @file:   pageowner.h
 * @brief:  Handle the ownership of pages.
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *          Should we introduce the locks to manage those ownership?
 *          If the lock is not owned initially, it is good to use cmpxchg to
 *          change the value.
 *          However, if a page is already owned by one process, there are several
            steps involving the change of ownership.
            1. The consumer should notify the owner. 
            2. Original Owner will issue a mprotect() to change the ownership.
            3. Original Owner will change the ownership to current user.
            4. The consumer detects the change of ownership, then issue a mprotect() again
               to change this page to PROT_READ and PROT_WRITE.

            That is, changing the ownership owning by others is involved two mprotect() calls and 
            a signal handling.
 */

#ifndef _PAGEOWNER_H_
#define _PAGEOWNER_H_

#include "xdefines.h"
#include "xatomic.h"
#include "memwrapper.h"

class pageowner {
  enum { OWNER_NONE = 0xFFFFFFFF };

  struct pageentry {
    unsigned long coreid;
    // We may keep track of accesser information
  };
public:

  pageowner() {
    _owner = NULL;
  }

  // Initialize should be done by two steps in order to save time.
  // For example,     
  void initialize(int totalpages) {
    void * ptr;
    int size = totalpages * sizeof(pageentry); 

    // MMAP a block of shared memory
    ptr = MMAP_SHARED(size);
//    fprintf(stderr, "pageowner with size %d\n", size);
    
    _owner = (pageentry *)ptr;
  }

  int getOwner(int pageNo) {
    // Calculate the pageNo
    pageentry * entry = &_owner[pageNo];
 
    // Get the owner
    return entry->coreid;
  }

  // Set pages owner for a bunch of pages
  // Lock must be acquired when it happens or 
  // we know that no other processes are trying to work on 
  // specific pages
  void setPagesOwner(int pageNo, int pages, int coreid) {
    pageentry * entry = &_owner[pageNo];

    while(pages) {
      entry->coreid = coreid;
      pages--;    
      entry++;  
    }
  }

  // Set pages unowned.
  void setPagesUnowned(int pageNo, int pages) {
    setPagesOwner(pageNo, pages, OWNER_NONE);
  } 
 
  bool isPageOwned(int pageNo) {
    pageentry * entry = &_owner[pageNo];
    return(entry->coreid != OWNER_NONE);
  }

  bool acquireOwnership(int pageNo, int coreid) {
    bool result = false;
    pageentry * entry = &_owner[pageNo];
    // Using cmpxchg instruction.
    // x86/include/asm/cmpxchg_64.h
    // If a page is not owned, then own it.
    // cmpxchg will return the previous value before CAS.
    // If the value is equal to OWNER_NONE, acquiring is successful.
    return (cmpxchg(&entry->coreid, OWNER_NONE, coreid) == OWNER_NONE);  
  }
  

private:
  pageentry * _owner;
};
#endif /* _ */
