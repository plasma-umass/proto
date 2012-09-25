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
 * @file:   memmap.h
 * @brief:  The file to handle the mmap system call.
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

/* 
 How to handle the mmap calls in the process-as-threads system.

 The basic idea is to change "MMAP_PRIVATE" to "MMAP_SHARED", so that
 we can make all processes to share the same mapping(physical pages). 

 But that is not easy:

 (1) First, we have to check whether we already creat processes or not.
 If yes, then it is much harder to do, we must populate the same
 mapping from one process to other processes so that all processes will see
 the same mapping. The basic idea is to do mmap() first in one process, then
 we can signal other processes so that they can call mmap() to the same mapping.
 However, in order to mmap to the same place, we have to utilize one file to backup.
 That is, we can create a large file mapping in the beginning, then we are mapping to
 different place later.

 In the first phase, we will skip this implementation since it is too complicated.
 
 (2) Secondly, some times we may have a situation which can't be changed the mapping easily.
 For example, the case like the following is valid in a sigle thread program:
   fd = open(xxx, O_RDONLY);
   mmap(.., PROT_READ|PROT_WRITE, MMAP_PRIVATE, **, fd);

 It is completely valid and smart trick. The user program won't change original file. 
 However, it is impossible to do in a process-as-threads environment. 

 (1) If we don't change to "MMAP_SHARED" (still using "MMAP_PRIVATE"), then the modification
 on one process cannot be seen by other process. For example, the problem of word_count is caused
 by this problem. 

 (2) If we change the mapping from "MMAP_PRIVATE" to "MMAP_SHARED", the property of this mmap won't 
 be held anymore. 
  (a) First, we can't change the mapping only. For this case, mmap will fail because the file is "RDONLY", thus it can be mmaped as PROT_READ and PROT_WRITE. So we must change the protection of file to 
   "O_RDWR". 
  (b) The original behavior is not going to hold after we are doing this, the original file will be affected if we change it. But it seems to me that we cannot do much on this. 
   

*/
#ifndef _MEMMAP_H_
#define _MEMMAP_H_

#include <unistd.h>
#include <fcntl.h>
#include "memwrapper.h"

class memmap {
public:
  memmap()
   : fd(0)
  { 
  }

  // A singleton shared by different processes.
  static memmap& getInstance(void) {
    static memmap * theObject = NULL;
    if(!theObject) {
      void * buf = MALLOC_SHARED(sizeof(memmap));
      theObject = new(buf) memmap();
    }
    return * theObject;
  }

  // Created by different process. 
  void createAnonMapping(void) {
  
    // Open a common file to save mapping.

  } 


  int mmap(void *addr, size_t length, int prot,
         int flags,  int fd, off_t offset) {
    int newflags = flags;
    bool privatemap = false;
    void * ptr;
  
    // Check whether the flags is including MMAP_PRIVATE
    if(flags & MAP_PRIVATE) {
  		newflags = (flags & ~MAP_PRIVATE) | MAP_SHARED;

      // Check whether we need to change the file status
      if((fd != -1) && (prot & PROT_WRITE)) {
        int status;
        
        // Change the file status.
        
      }

    }
      
    // Calling the actual mmap operation

  
    // Different situation for mmap.
    // If we are mapping to file or we do not
    // If we mmaped to file, then it is possible to use MAP_PRIVATE.
    // If we don't mmap to file, then possibly that we have to change to MAP_SHARED
    // Any other possibilities?
    // Do we have to wait until the one process has been mapped successfully?
    // How to make different process to call mmap based the address returned??
    
    // Replace those MAP_PRIVATE with MAP_SHARED, otherwise the modification can't be shared
    // by different processes.

    // Check the flags of existing file.
    
    ptr = WRAP(mmap)(addr, length, prot, newflags, fd, offset);
    if(ptr == MAP_FAILED) {
      PRERR("Mmap failed.");
    }
    
    // Now we are trying to make other process to mmap to the same fd and same address again.
    // Calling xrun's corresponding function to populate this mmap effect
//    xrun::getInstance().populateMmap(ptr, length, prot, newflags | MAP_FIXED, fd, offset);
     
    if(needPopulated) {


    }

  }


private:
  int fd;   // common file for anonymous mapping after process creation.
  bool needPopulated; // Whether we need to populate the mapping from one process to other processes.
};
#endif /* _ */
