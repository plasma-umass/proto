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
 * @file   xscheduler.h
 * @brief  Thread scheduler.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef __XSCHEDULER_H__
#define __XSCHEDULER_H__

#include <errno.h>
#include <malloc.h>
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define  THREAD_SWITCH_DEBUG(old,new) \
         PRLOG("%d: Swtiching from %d to %d",getpid(), old->tid, new->tid);
 //        fprintf(stderr, "%d: Swtiching from %d to %d\n",getpid(), old->tid, new->tid);
//         PRWRN("%d: Swtiching from %d to %d",getpid(), old->tid, new->tid);

#define THREAD_SWITCH(old,new) \
    THREAD_SWITCH_DEBUG(old, new) \
    swapcontext(&((old)->ctx.context), &((new)->ctx.context));


// FIXME in the future
#define CALC_TIME(a, b) (a+b)
#define SET_TIME_NOW(t1) \
do { \
    struct timeval tt; \
    gettimeofday(&tt); \
    *(t1) = CALC_TIME(tt.tv_sec, tt.tv_usec); \
} while (0)

typedef long long rtime_t;

extern "C" {
// Main entry of a process
void schedulerThread(void);

// Yield to someone
class xthread;
void threadYieldHoldingLock(spinlock * lock);
void threadYieldToRunQueue(xqueue * to);
void threadYieldInitially(xqueue * to);
};

#endif
