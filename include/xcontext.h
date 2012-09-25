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
 * @file:   xcontext.h
 * @brief:  Thread context. 
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef XCONTEXTH
#define XCONTEXTH

#include <ucontext.h>

#include "xdefines.h"
#include "xatomic.h"
#include "memwrapper.h"
#include "list.h"

extern "C" {
  typedef void * (threadFunction)(void *);

  void threadRunning(void * func, void * arg);
};

// Swapcontext
class xcontext {
public:
  // Stack related information
  void * stack;
  void * stackbottom;
  int    stacksize;
   
  // User context
  ucontext_t context;
  
  // Task status
  e_thread_status status;

  xcontext() {
    // get context for current program
    // This step is necessary before you try to makecontext
    // Otherwise, some register will failed to be initialized.
    getcontext(&context);

    stacksize = xdefines::STACK_SIZE;

    // Create the stack for this task.
    stack = MALLOC_SHARED(stacksize);
    stackbottom = (void *)((intptr_t)stack + stacksize);   
 
    //fprintf(stderr, "%d: stack %p ~ 0x%x\n", getpid(), stack, stackbottom);

    // Cleanup the stack
    memset(stack, 0, stacksize);

    //PRDBG("%d: stack %p ~ 0x%x after memset\n",  getpid(), stack, stackbottom);
    // Set context for this thread. 
    context.uc_link = 0;
    context.uc_stack.ss_sp = stack;
    context.uc_stack.ss_size = stacksize;
    context.uc_stack.ss_flags = 0;
  }

  ~xcontext() {
    // Free the memory for the stack.
    FREE_SHARED(stack);
  }

  void freeStack(void) {
    FREE_SHARED(stack);
  }

  // FIXME: we have to try to create the a new context.
  // We may use a general function
  void makeContext(void * wrapFunc, void * threadFunc, void * arg) {
#if 0
    int i;
    for(i =0; i < 18; i++) {
      fprintf(stderr, "before register %d: %p \n", i,   (void*)context.uc_mcontext.gregs[i]);
    }
#endif
    // We will set to thread::
    makecontext(&context, (void(*)(void))wrapFunc, 2, threadFunc, arg);
    //makecontext( &context, (void(*)(void))func, 1, arg);
  }

  void getContext(void) {
    getcontext(&context);
  }

  void setContext(void) {
    
    setcontext(&context);
    assert(0); 
  }

  void switchContext(void * newcontext) {
    memcpy(&context, newcontext, sizeof(ucontext_t));
  }

  // Reset all stack related field.
  void resetStack(void * newstack, int size) {
    void * oldstack = stack;
    
    stacksize = size;

    // Reset the stack for this task.
    stack = newstack;
    stackbottom = (void *)((intptr_t)stack + stacksize);   
 
    // Set context for this thread. 
    context.uc_stack.ss_sp = stack;
    context.uc_stack.ss_size = stacksize;
    
    // Free the old stack.
    FREE_SHARED(oldstack);
  }

  void * getStackBottom(void) {
    return stackbottom; 
  }

  void checkStack(void) {
    int esp = context.uc_mcontext.gregs [REG_EBP];
    if(esp < (int)((intptr_t)stack)) {
      PRERR("thread %d: esp %x stack %pStack is overflowed!!!!!!\n", pthread_self(), esp, stack);
      PRERR("Stack is overflowed!!!!!!\n");
      abort();
    } 

  }

  ucontext_t * myContext(void) {
    return &context;
  }

  static void setCr2Register(ucontext_t *context) {
    context->uc_mcontext.cr2 = 0;
  }

  static void printRegisters(ucontext_t *context) {
     fprintf(stderr, "\nContext: 0x%08lx\n", (unsigned long) context);
  fprintf (stderr, "    gs: 0x%08x   fs: 0x%08x   es: 0x%08x   ds: 0x%08x\n"
               "   edi: 0x%08x  esi: 0x%08x  ebp: 0x%08x  esp: 0x%08x\n"
               "   ebx: 0x%08x  edx: 0x%08x  ecx: 0x%08x  eax: 0x%08x\n"
               "  trap:   %8u  err: 0x%08x  eip: 0x%08x   cs: 0x%08x\n"
               "  flag: 0x%08x   sp: 0x%08x   ss: 0x%08x  cr2: 0x%08lx\n",
               context->uc_mcontext.gregs [REG_GS],     context->uc_mcontext.gregs [REG_FS],   context->uc_mcontext.gregs [REG_ES],  context->uc_mcontext.gregs [REG_DS],
               context->uc_mcontext.gregs [REG_EDI],    context->uc_mcontext.gregs [REG_ESI],  context->uc_mcontext.gregs [REG_EBP], context->uc_mcontext.gregs [REG_ESP],
               context->uc_mcontext.gregs [REG_EBX],    context->uc_mcontext.gregs [REG_EDX],  context->uc_mcontext.gregs [REG_ECX], context->uc_mcontext.gregs [REG_EAX],
               context->uc_mcontext.gregs [REG_TRAPNO], context->uc_mcontext.gregs [REG_ERR],  context->uc_mcontext.gregs [REG_EIP], context->uc_mcontext.gregs [REG_CS],
               context->uc_mcontext.gregs [REG_EFL],    context->uc_mcontext.gregs [REG_UESP], context->uc_mcontext.gregs [REG_SS],  context->uc_mcontext.cr2
  );
  }
};

#endif 
