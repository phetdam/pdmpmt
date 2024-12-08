/*******************************************************************************
* multistream.c: this file is an example for the usage of the prand library.
 
* prand: C library for generating random numbers with multiple streams.

* Github repository:
        https://github.com/cheng-zhao/prand

* Copyright (c) 2020 Cheng Zhao <zhaocheng03@gmail.com>
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

*******************************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include "prand.h"

#define NUM_STREAM      5
#define NUM_STEP        100000
#define RNG_TYPE        PRAND_RNG_MT19937
#define SEED            1

#define CHECK_ERROR(err)                        \
  if (PRAND_IS_ERROR(err)) {                   \
    printf("Error: %s\n", prand_errmsg(err));  \
    return err;                                 \
  }

int main(void) {
  int err = 0;
  double z, max = 0;
  const uint64_t step = NUM_STEP;
  const int nstream = NUM_STREAM;

  prand_t *rng;

  /* single stream */
  rng = prand_init(RNG_TYPE, SEED, 0, 0, &err);
  CHECK_ERROR(err);
  printf("-> Single stream:\n");
  for (int i = 0; i < nstream; i++) {
    printf("%" PRIu64 "-th number: %lf\n",
        i * step, rng->get_double(rng->state));
    for (uint64_t j = 1; j < step; j++) {
      z = rng->get_double(rng->state);
      if (max < z) max = z;
    }
  }
  prand_destroy(rng);

  /* multiple streams */
  rng = prand_init(RNG_TYPE, SEED, nstream, step, &err);
  CHECK_ERROR(err);
  printf("-> %d streams with step size %" PRIu64 ":\n", nstream, step);

  for (int i = 0; i < nstream; i++) {
    printf("starting number of %d-th stream: %lf\n",
        i, rng->get_double(rng->state_stream[i]));
  }
  prand_destroy(rng);

  return 0;
}

