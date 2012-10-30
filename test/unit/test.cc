#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>

#include <errno.h>
#include <pthread.h>
#include <stdint.h>

#include "gtest/gtest.h"

pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile signed num_threads = 1;
const signed max_threads = 8;

static uint32_t partition(uint32_t* arr, uint32_t left, uint32_t right, uint32_t pivot) {
  uint32_t pivot_val = arr[pivot];
  arr[pivot] = arr[right];
  arr[right] = pivot_val;
  uint32_t cursor = left;
  for (uint32_t i = left; i < right; ++i) {
    if (arr[i] < pivot_val) {
      uint32_t tmp = arr[i];
      arr[i] = arr[cursor];
      arr[cursor] = tmp;
      cursor += 1;
    }
  }
  arr[right] = arr[cursor];
  arr[cursor] = pivot_val;
  return cursor;
}

void p_quicksort(uint32_t* arr, uint32_t left, uint32_t right);

static void *p_qsort_bootstrap(void* args) {
  uint32_t* int_args = (uint32_t*)args;
  uint32_t* arr = (uint32_t*)int_args[0];
  uint32_t left = int_args[1];
  uint32_t right = int_args[2];
  p_quicksort(arr, left, right);
  pthread_mutex_lock(&threads_mutex);
  num_threads--;
  pthread_mutex_unlock(&threads_mutex);
  return NULL;
}

void p_quicksort(uint32_t* arr, uint32_t left, uint32_t right) {
  uint32_t args[3];
  pthread_t child;
  if (left < right) {
    uint32_t pivot = left;
    pivot = partition(arr, left, right, pivot);

    bool fork = false;

    pthread_mutex_lock(&threads_mutex);
    if (num_threads < max_threads) {
      num_threads++;
      pthread_mutex_unlock(&threads_mutex);
      fork = true;
    } else {
      pthread_mutex_unlock(&threads_mutex);
    }

    // we need to avoid underflow for pivot.
    if (pivot > 1) {
      // only bifurcate once per quicksort.
      // Children will maintain themselves
      if (fork) {
        args[0] = (uint32_t)arr;
        args[1] = left;
        args[2] = pivot - 1;
        int status = pthread_create(&child,
                                    NULL,
                                    p_qsort_bootstrap,
                                    (void*)args);
        ASSERT_EQ(0, status);
      } else {
        p_quicksort(arr, left, pivot - 1);
      }
    }
    p_quicksort(arr, pivot + 1, right);
    if (fork) {
      pthread_join(child, NULL);
    }
  }
}

static void randomize_vector(std::vector<uint32_t>* vec, uint32_t n) {
  srandom(time(NULL));
  for (uint32_t i = 0; i < n; ++i) {
    vec->push_back((uint32_t)random());
  }
}

TEST(QuicksortTest, Singleton) {
  uint32_t arr[] = {42};
  {
    SCOPED_TRACE("p_quicksort");
    p_quicksort(arr, 0, 0);
  }
}

TEST(QuicksortTest, Ten) {
  uint32_t sizes[] = {1 << 5, 1 << 10, 1 << 20};
  for (uint32_t size_i = 0; size_i < (sizeof(sizes) / sizeof(*sizes)); ++size_i) {
    uint32_t n = sizes[size_i];

    std::vector<uint32_t> a;
    randomize_vector(&a, n);
    std::vector<uint32_t> b(a);

    std::sort(a.begin(), a.end());

    {
      SCOPED_TRACE("p_quicksort");
      p_quicksort(&b[0], 0, n - 1);
    }
   
    ASSERT_EQ(0, memcmp((void*)&a[0], (void*)&b[0], n * sizeof(uint32_t)));
  }
}

