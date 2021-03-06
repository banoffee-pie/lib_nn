// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

#include "meas_common.h"
#include "nn_operator.h"
#include "xs3_vpu.h"

void benchmark_vpu_memcpy(int argc, char** argv) {
  assert(argc >= 1);

  for (int k = 0; k < argc; k++) {
    unsigned buff_size = atoi((char*)argv[k]);

    uint8_t* src = malloc(buff_size);
    uint8_t* dst = malloc(buff_size);

    assert(src);
    assert(dst);

    vpu_memcpy(dst, src, buff_size);

    free(src);
    free(dst);
  }
}