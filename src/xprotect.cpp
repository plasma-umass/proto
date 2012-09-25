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

/**
 * @class  xprotect.cpp
 * @brief  Handling the creation and trap on protected memory.
 *
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#include <signal.h>
#include "xprotect.h"
#include "process.h"
#include "xcontext.h"
#include "processmap.h"

static long getRegister(ucontext_t * context, int reg) {
  return context->uc_mcontext.gregs [reg];
}

static void changeRegister(ucontext_t * ncxt, ucontext_t * ocxt, int reg) {
  ncxt->uc_mcontext.gregs[reg] = ocxt->uc_mcontext.gregs[reg];
}
/// @brief: switch to the scheduler thread of this process.
//  However, we DONOT actually do switch NOW in the signal handler,
//  since we donot know how to take care the signal stack
//  Calling setcontext() can cause program to abort().
static void switchContext(ucontext_t * destcontext, ucontext_t * srccontext) {
  // Since the process will set the context to "context" parameter
  // here, what we need to do is to override "context" with the 
  // scheduler's context, then after signal handler, 
  // the process will switch to scheduler thread.
  long oesp, oebp, oeip;
  long nesp, nebp, neip;
  oesp = getRegister((ucontext_t *)destcontext, REG_ESP);
  oebp = getRegister((ucontext_t *)destcontext, REG_EBP);
  oeip = getRegister((ucontext_t *)destcontext, REG_EIP);
  nesp = getRegister((ucontext_t *)srccontext, REG_ESP);
  nebp = getRegister((ucontext_t *)srccontext, REG_EBP);
  neip = getRegister((ucontext_t *)srccontext, REG_EIP);

#if 1 
  // Overlapping current context with the target context
  changeRegister(destcontext, srccontext, REG_ESP);
  changeRegister(destcontext, srccontext, REG_EBP);
  changeRegister(destcontext, srccontext, REG_EIP);
  changeRegister(destcontext, srccontext, REG_EAX);
  changeRegister(destcontext, srccontext, REG_EBX);
  changeRegister(destcontext, srccontext, REG_ECX);
  changeRegister(destcontext, srccontext, REG_EDX);
  changeRegister(destcontext, srccontext, REG_ESI);
  changeRegister(destcontext, srccontext, REG_EDI);
  changeRegister(destcontext, srccontext, REG_FS);
  memcpy(&destcontext->uc_mcontext.fpregs, &srccontext->uc_mcontext.fpregs, sizeof(srccontext->uc_mcontext.fpregs));
#endif
}

/// @brief Handle the page trap 
void xprotect::handleAccessTrap (void * addr, void * context) {
  // Compute the page number of this item
  int pageNo = computePage (addr);
   
  int coreid = process::getInstance().getCoreId();

  assert(pageNo < _totalpages);

  // check the owner of this page
  if(_ownning.isPageOwned(pageNo)) {
Handle_OwnedPage:
    //removePageProtect(addr);
#if 1
    // Page is owned.
    // Get the owner of this page
    int ownerid = _ownning.getOwner(pageNo);
    xqueue * pqueue = processmap::getInstance().getPQueue(ownerid);    

    // Put this thread to the corresponding queue
    xthread * current = process::getInstance().getCurrent();

    int tid = current->getTid();

    // Make context for this thread
    current->switchContext(context);
    //switchContext(current->myContext(), (ucontext_t *)context);
    
    // Add this thread to the process owning this page
    pqueue->enqueue(current);

    // Switch to the scheduler thread after the handler
    ucontext_t * srccontext = process::getInstance().getScheduler()->myContext();
   
    switchContext((ucontext_t *)context, srccontext);
#endif
  }
  else {
    // Page is stil unowned.
    // Try to get the ownership
    if(_ownning.acquireOwnership(pageNo, coreid)) {
      // If the acquiring of ownership is successfull
      // Unprotect this page since I am the owner
      removePageProtect(addr);
    }
    else {
      // Unforunately, another process has acquired the ownership before me.
      // It is an unlikely event, but can happen:-)
      goto Handle_OwnedPage;
    }
  } 
}

void xprotect::setPagesOwner(void * addr, int size) {
    void * start = getPageStartAddr(addr);
    int pageNo = computePage(addr);
    int pages = calcPages(addr, size);
    int coreid = process::getInstance().getCoreId();

    // Only the heap will call this function.
    assert(_isHeap == true);

    if(_isProtected) {
      _ownning.setPagesOwner(pageNo, pages, coreid);
      
      // Change the protection of this block.
      // Since they are owned by current process, 
      // make all pages Readable and Writable by current process
      mprotect(addr, size, PROT_READ | PROT_WRITE);
    }
    else {
      // There is only one process if _isProtected is false . 
      // All the pages are set to Readable and Writeable
      // and all pages are not owned initially. 
      // We start to track of pages owner after creatio of children.
      _ownning.setPagesUnowned(pageNo, pages);
    }
}
