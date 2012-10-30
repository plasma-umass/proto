// -*- C++ -*-

/*
 Author: Emery Berger, http://www.cs.umass.edu/~emery
 
 Copyright (c) 2007-8 Emery Berger, University of Massachusetts Amherst.

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
 * @class xatomic
 * @brief A wrapper class for basic atomic hardware operations.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XATOMIC_H_
#define _XATOMIC_H_

extern "C" {
#define cmpxchg(ptr, o, n)            \
  ((__typeof__(*(ptr)))__cmpxchg((ptr), (unsigned long)(o), \
               (unsigned long)(n),    \
               sizeof(*(ptr))))

#if defined(__i386__)
struct __xchg_dummy {
  unsigned long a[100];
};
#define __xg(x) ((struct __xchg_dummy *)(x))

static inline unsigned long __cmpxchg(volatile void *ptr, unsigned long old,
              unsigned long newval, int size)
{
  unsigned long prev;
  switch(size) {
  case 1:
    asm volatile("cmpxchgb %b1,%2"
           : "=a"(prev)
           : "q"(newval), "m"(*__xg(ptr)), "0"(old)
           : "memory");
    return prev;
  case 2:
    asm volatile("cmpxchgw %w1,%2"
           : "=a"(prev)
           : "r"(newval), "m"(*__xg(ptr)), "0"(old)
           : "memory");
    return prev;
  case 4:
    asm volatile("cmpxchgl %1,%2"
           : "=a"(prev)
           : "r"(newval), "m"(*__xg(ptr)), "0"(old)
           : "memory");
    return prev;
  }
  return old;
}
#elif defined(__x86_64__)
#define __xg(x) ((volatile long *)(x))
static inline unsigned long __cmpxchg(volatile void *ptr, unsigned long old,
              unsigned long newval, int size)
{
  unsigned long prev;
  switch (size) {
  case 1:
    asm volatile("cmpxchgb %b1,%2"
           : "=a"(prev)
           : "q"(newval), "m"(*__xg(ptr)), "0"(old)
           : "memory");
    return prev;
  case 2:
    asm volatile("cmpxchgw %w1,%2"
           : "=a"(prev)
           : "r"(newval), "m"(*__xg(ptr)), "0"(old)
           : "memory");
    return prev;
  case 4:
    asm volatile("cmpxchgl %k1,%2"
           : "=a"(prev)
           : "r"(newval), "m"(*__xg(ptr)), "0"(old)
           : "memory");
  case 8:
    asm volatile("cmpxchgq %1,%2"
           : "=a"(prev)
           : "r"(newval), "m"(*__xg(ptr)), "0"(old)
           : "memory");
    return prev;
  }
  return old;
}
#endif

};

class xatomic {
public:

  inline static unsigned long exchange(volatile unsigned long * oldval,
      unsigned long newval) {
#if defined(__i386__)
    asm volatile ("lock; xchgl %0, %1"
        : "=r" (newval)
        : "m" (*oldval), "0" (newval)
        : "memory");
#elif defined(__sparc)
    asm volatile ("swap [%1],%0"
        :"=r" (newval)
        :"r" (oldval), "0" (newval)
        : "memory");
#elif defined(__x86_64__)
    // Contributed by Kurt Roeckx.
    asm volatile ("lock; xchgq %0, %1"
        : "=r" (newval)
        : "m" (*oldval), "0" (newval)
        : "memory");
#else
#error "Not supported on this architecture."
#endif
    return newval;
  }

  // Atomic increment 1 and return the original value.
  static inline int increment_and_return(volatile unsigned long * obj) {
    int i = 1;
    asm volatile("lock; xaddl %0, %1"
        : "+r" (i), "+m" (*obj)
        : : "memory");
    return i;
  }

  static inline void increment(volatile unsigned long * obj) {
    asm volatile("lock; incl %0"
        : "+m" (*obj)
        : : "memory");
  }

  static inline void add(int i, volatile unsigned long * obj) {
    asm volatile("lock; addl %0, %1"
        : "+r" (i), "+m" (*obj)
        : : "memory");
  }

  static inline void decrement(volatile unsigned long * obj) {
    asm volatile("lock; decl %0;"
        : :"m" (*obj)
        : "memory");
  }

  // Atomic decrement 1 and return the original value.
  static inline int decrement_and_return(volatile unsigned long * obj) {
    int i = -1;
    asm volatile("lock; xaddl %0, %1"
        : "+r" (i), "+m" (*obj)
        : : "memory");
    return i;
  }

  static inline void atomic_set(volatile unsigned long * oldval,
      unsigned long newval) {
#if defined(__i386__)
    asm volatile ("lock; xchgl %0, %1"
        : "=r" (newval)
        : "m" (*oldval), "0" (newval)
        : "memory");
#else
    asm volatile ("lock; xchgq %0, %1"
        : "=r" (newval)
        : "m" (*oldval), "0" (newval)
        : "memory");
#endif
    return;
  }

  static inline int test_and_set(volatile unsigned long *w) {
    int ret;
    //fprintf(stderr, "Before lock: value is %d\n", *w);
#if defined(__i386__)
    asm volatile("lock; xchgl %0, %1":"=r"(ret), "=m"(*w)
       :"0"(1), "m"(*w)
       :"memory");
#else
    asm volatile("lock; xchgq %0, %1":"=r"(ret), "=m"(*w)
       :"0"(1), "m"(*w)
       :"memory");
#endif
    return ret;
  }

  static inline int atomic_read(const volatile unsigned long *obj) {
    return (*obj);
  }

  static inline void memoryBarrier(void) {
    // Memory barrier: x86 only for now.
    __asm__ __volatile__ ("mfence": : :"memory");
  }

  static inline void cpuRelax(void) {
    asm volatile("pause\n": : :"memory");
  }


};

#endif
