// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nn_operator.h"
#include "tst_common.h"
#include "xs3_vpu.h"

// #include "dsp_xs3_vector.h"
#include "unity.h"

#define DO_PRINT_EXTRA ((DO_PRINT_EXTRA_GLOBAL) && 0)

#define DEBUG_ON (0 || TEST_DEBUG_ON)
#define MAX_LEN 128
#define REPS 50
void test_lookup8_case0() {
  srand(12312314);

  PRINTF("%s...\n", __func__);

  uint8_t WORD_ALIGNED src[MAX_LEN] = {0};

  uint8_t WORD_ALIGNED lut[256];

  uint8_t WORD_ALIGNED dst[MAX_LEN] = {0};

  pseudo_rand_bytes((char*)src, MAX_LEN);
  pseudo_rand_bytes((char*)lut, 256);

  for (int k = 0; k < REPS; k++) {
    PRINTF("\trep %d...\n", k);

    uint32_t start = pseudo_rand_uint32() % MAX_LEN;
    uint32_t size = pseudo_rand_uint32() % (MAX_LEN - start);

    memset(dst, 0xCC, sizeof(dst));

    lookup8(dst, src, lut, start, size);

    for (int i = 0; i < MAX_LEN; i++) {
      TEST_ASSERT_EQUAL((i >= start && i < start + size) ? lut[src[i]] : 0xCC,
                        dst[i]);
    }
  }
}
#undef REPS
#undef MAX_LEN
#undef DEBUG_ON

void test_lookup8() {
  UNITY_SET_FILE();

  RUN_TEST(test_lookup8_case0);
}