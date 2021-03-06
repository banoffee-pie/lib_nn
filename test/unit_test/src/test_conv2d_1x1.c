// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nn_op_helper.h"
#include "nn_operator.h"
#include "tst_common.h"
#include "unity.h"
#include "xs3_vpu.h"

#define DO_PRINT_EXTRA ((DO_PRINT_EXTRA_GLOBAL) && 0)

#if CONFIG_SYMMETRIC_SATURATION_conv2d_1x1
#define NEG_SAT_VAL (-127)
#else
#define NEG_SAT_VAL (-128)
#endif

static void Check_Y(const nn_image_t y_exp, const nn_image_t* Y,
                    const nn_image_params_t* y_params, const unsigned row,
                    const unsigned col, const unsigned chn,
                    const unsigned line) {
  char str_buff[200];

  nn_image_t y = Y[IMG_ADDRESS_VECT(y_params, row, col, chn)];

  if (y != y_exp) {
    sprintf(str_buff, "Y[%u][%u][%u] was wrong [line %u]", row, col, chn, line);
  }

  TEST_ASSERT_EQUAL_MESSAGE(y_exp, y, str_buff);
}

#define CHANS_IN (VPU_INT8_EPV)
#define CHANS_OUT (VPU_INT8_ACC_PERIOD)
#define HEIGHT (2)
#define WIDTH (2)
#define CHANS_OUT_CEIL                       \
  ((CHANS_OUT + (VPU_INT8_ACC_PERIOD - 1)) & \
   (0xFFFFFFFF << VPU_INT8_ACC_PERIOD_LOG2))
void test_conv2d_1x1_case0() {
  int8_t WORD_ALIGNED X[HEIGHT][WIDTH][CHANS_IN];

  int8_t WORD_ALIGNED K[CHANS_OUT][CHANS_IN];

  struct {
    int32_t bias[CHANS_OUT_CEIL];
    int16_t shift1[CHANS_OUT_CEIL];
    int16_t scale[CHANS_OUT_CEIL];
    int16_t offset_scale[CHANS_OUT_CEIL];
    int16_t offset[CHANS_OUT_CEIL];
    int16_t shift2[CHANS_OUT_CEIL];
  } BSO;

  int8_t WORD_ALIGNED Y[HEIGHT][WIDTH][CHANS_OUT];

  PRINTF("%s...\n", __func__);

  typedef struct {
    int8_t x;
    int8_t k;
    int32_t bias;
    int16_t shift1;
    int16_t scale;
    int16_t shift2;

    int8_t y;
    unsigned line;
  } test_case_t;

  //   Y[i] = C_in * (x * k)

  const test_case_t casses[] = {
      //  X       K           bias            shift1      scale       shift2 Y
      {0x00, 0x00, 0x00000000, 0, 0x0000, 0, 0x00, __LINE__},
      {0x00, 0x00, 0x00000000, 0, 0x0001, 0, 0x00, __LINE__},
      {0x00, 0x00, 0x00100000, 0, 0x0000, 0, 0x00, __LINE__},
      {0xCC, 0x00, 0x00000000, 0, 0x0001, 0, 0x00, __LINE__},
      {0xCC, 0xCC, 0x00000000, 0, 0x0000, 0, 0x00, __LINE__},
      {0x00, 0xCC, 0x00000000, 0, 0x0001, 0, 0x00, __LINE__},
      {0xAA, 0xBB, 0x0ABCDABC, 0, 0x0000, 0, 0x00, __LINE__},

      {0x00, 0x00, 0x00000001, 0, 0x0001, 0, 0x01, __LINE__},
      {0x00, 0x00, 0x00000011, 0, 0x0001, 0, 0x11, __LINE__},
      {0x00, 0x00, 0x0000007F, 0, 0x0001, 0, 0x7F, __LINE__},
      {0x00, 0x00, -0x0000007F, 0, 0x0001, 0, -0x7F, __LINE__},
      {0x00, 0x00, 0x00000080, 0, 0x0001, 0, 0x7F, __LINE__},
      {0x00, 0x00, -0x00000080, 0, 0x0001, 0, NEG_SAT_VAL, __LINE__},
      {0x00, 0x00, 0x0FFFFFFF, 0, 0x0001, 0, 0x7F, __LINE__},
      {0x00, 0x00, -0x0FFFFFFF, 0, 0x0001, 0, NEG_SAT_VAL, __LINE__},

      {0x00, 0x00, 0x00000002, 0, 0x0001, 1, 0x01, __LINE__},
      {0x00, 0x00, 0x00000004, 0, 0x0001, 1, 0x02, __LINE__},
      {0x00, 0x00, 0x00000004, 0, 0x0001, 2, 0x01, __LINE__},
      {0x00, 0x00, 0x00000001, 0, 0x0001, 1, 0x01, __LINE__},
      {0x00, 0x00, 0x00000003, 0, 0x0001, 1, 0x02, __LINE__},
      {0x00, 0x00, -0x00000002, 0, 0x0001, 1, -0x01, __LINE__},
      {0x00, 0x00, 0x00000002, 0, 0x0001, 1, 0x01, __LINE__},
      {0x00, 0x00, 0x00000100, 0, 0x0001, 7, 0x02, __LINE__},

      {0x00, 0x00, 0x00000001, 0, 0x0002, 0, 0x02, __LINE__},
      {0x00, 0x00, 0x00000001, 0, 0x0003, 0, 0x03, __LINE__},
      {0x00, 0x00, 0x00000001, 0, -0x0002, 0, -0x02, __LINE__},
      {0x00, 0x00, 0x00000010, 0, 0x0002, 0, 0x20, __LINE__},
      {0x00, 0x00, 0x00000010, 0, 0x0002, 0, 0x20, __LINE__},
      {0x00, 0x00, 0x00004000, 0, 0x0100, 21, 0x02, __LINE__},
      {0x00, 0x00, 0x00004000, 0, -0x0100, 21, -0x02, __LINE__},
      {0x00, 0x00, 0x00004000, 0, 0x0100, 18, 0x10, __LINE__},

      {0x00, 0x00, 0x00000002, 1, 0x0001, 0, 0x01, __LINE__},
      {0x00, 0x00, 0x00000004, 1, 0x0001, 0, 0x02, __LINE__},
      {0x00, 0x00, 0x00000004, 2, 0x0001, 0, 0x01, __LINE__},
      {0x00, 0x00, 0x00000001, 1, 0x0001, 0, 0x01, __LINE__},
      {0x00, 0x00, 0x00000003, 1, 0x0001, 0, 0x02, __LINE__},
      {0x00, 0x00, -0x00000002, 1, 0x0001, 0, -0x01, __LINE__},
      {0x00, 0x00, 0x00000002, 1, 0x0001, 0, 0x01, __LINE__},
      {0x00, 0x00, 0x00000100, 7, 0x0001, 0, 0x02, __LINE__},
      {0x00, 0x00, 0x00010000, 0, 0x0001, 10, 0x20, __LINE__},
      {0x00, 0x00, 0x00010000, 10, 0x0001, 0, 0x40, __LINE__},
      {0x00, 0x00, 0x00010000, 0, 0x0002, 10, 0x40, __LINE__},
      {0x00, 0x00, 0x00010000, 4, 0x0001, 6, 0x40, __LINE__},
      {0x00, 0x00, -0x00010000, 4, 0x0001, 6, -0x40, __LINE__},
      {0x00, 0x00, 0x00010000, 4, 0x0004, 8, 0x40, __LINE__},

      {0x01, 0x01, 0x00000000, 0, 0x0001, 0, 0x20, __LINE__},
      {0x01, 0x02, 0x00000000, 0, 0x0001, 0, 0x40, __LINE__},
      {0x02, 0x01, 0x00000000, 0, 0x0001, 0, 0x40, __LINE__},
      {0x01, -0x01, 0x00000000, 0, 0x0001, 0, -0x20, __LINE__},
      {-0x01, 0x01, 0x00000000, 0, 0x0001, 0, -0x20, __LINE__},
      {-0x01, -0x01, 0x00000000, 0, 0x0001, 0, 0x20, __LINE__},
      {0x01, 0x01, -0x00000010, 0, 0x0001, 0, 0x10, __LINE__},
      {0x02, 0x02, 0x00000000, 0, 0x0001, 1, 0x40, __LINE__},
      {0x10, 0x02, 0x00000000, 0, 0x0001, 5, 0x20, __LINE__},
      {0x10, 0x10, -0x00002020, 0, 0x0001, 0, -0x20, __LINE__},
      {0x10, 0x02, 0x00000000, 0, 0x0010, 8, 0x40, __LINE__},

      {0x40, 0x40, 0x00000000, 16, 0x0001, 0, 0x02, __LINE__},
      {0x40, 0x40, 0x00000000, 17, 0x0001, 0, 0x01, __LINE__},
      {0x40, 0x40, 0x00000000, 18, 0x0001, 0, 0x01, __LINE__},
      {0x40, 0x40, 0x00000000, 19, 0x0001, 0, 0x00, __LINE__},
      {0x40, -0x40, 0x00000000, 16, 0x0001, 0, -0x02, __LINE__},
      {-0x40, 0x40, 0x00000000, 17, 0x0001, 0, -0x01, __LINE__},
      {-0x40, -0x40, 0x00000000, 18, -0x0001, 0, -0x01, __LINE__},
      {0x40, 0x40, 0x00000000, 19, -0x0001, 0, 0x00, __LINE__},

      // SPECIAL CASES
      //  The case logic will look for this case (by x==0xED) and handle it
      //  differently.
      //  Point of the special case is to not have all X and all K be identical.
      //  That way it can make sure that, for example, it's not just always
      //  using the same input data

      // X[i][j][k] = k,   K[u][v] = 1        ->   Y[i][j][k] = (0 + 1 + ... +
      // 31) >> 4 =  (2^4 * 31) >> 4 = 31
      {0xED, 0x00, 0x00000000, 0, 0x0001, 4, 0x00, __LINE__},
      // X[i][j][k] = 1,   K[u][v] = u        ->   Y[i][j][k] = 32 * k >> 4  =
      // 2*k
      {0xED, 0x01, 0x00000000, 0, 0x0001, 4, 0x00, __LINE__},
      // X[i][j][k] = 1,   K[u][v] = v        ->   Y[i][j][k] =  (0 + 1 + ... +
      // 31) >> 3 =  (2^4 * 31) >> 3 = 62
      {0xED, 0x02, 0x00000000, 0, 0x0001, 3, 0x00, __LINE__},
  };

  const unsigned N_casses = sizeof(casses) / sizeof(test_case_t);
  const unsigned start_case = 0;
  const unsigned stop_case = -1;

  print_warns(start_case);

  for (unsigned v = start_case; v < N_casses && v < stop_case; v++) {
    const test_case_t* casse = (const test_case_t*)&casses[v];

    PRINTF("\ttest vector %u...\n", v);

    nn_image_params_t x_params = {HEIGHT, WIDTH, CHANS_IN};
    nn_image_params_t y_params = {HEIGHT, WIDTH, CHANS_OUT};

    if (((uint8_t)casse->x) != 0xED) {
      memset(X, casse->x, sizeof(X));
      memset(K, casse->k, sizeof(K));
    } else {
      switch (casse->k) {
        case 0:
          memset(K, 1, sizeof(K));
          for (int r = 0; r < HEIGHT; r++)
            for (int c = 0; c < WIDTH; c++)
              for (int i = 0; i < CHANS_IN; i++) X[r][c][i] = i;
          break;
        case 1:
          memset(X, 1, sizeof(X));
          for (int i = 0; i < CHANS_OUT; i++)
            for (int j = 0; j < CHANS_IN; j++) K[i][j] = i;
          break;
        case 2:
          memset(X, 1, sizeof(X));
          for (int i = 0; i < CHANS_OUT; i++)
            for (int j = 0; j < CHANS_IN; j++) K[i][j] = j;
          break;
        default:
          TEST_FAIL();
          break;
      }
    }

    for (int k = 0; k < CHANS_OUT; k++) {
      BSO.bias[k] = casse->bias;
      BSO.shift1[k] = casse->shift1;
      BSO.scale[k] = casse->scale;
      BSO.offset_scale[k] = 0;
      BSO.offset[k] = 0;
      BSO.shift2[k] = casse->shift2;
    }

    nn_standard_BSO_layout((nn_bso_block_t*)&BSO, (int32_t*)&BSO.bias,
                           (int16_t*)&BSO.shift1, (int16_t*)&BSO.scale,
                           (int16_t*)&BSO.offset_scale, (int16_t*)&BSO.offset,
                           (int16_t*)&BSO.shift2, NULL, CHANS_OUT);

    memset(Y, 0xCC, sizeof(Y));  // too expensive to write the whole image, so
                                 // just do the part that's in play
    conv2d_1x1((int8_t*)Y, (int8_t*)X, (int8_t*)K, (nn_bso_block_t*)&BSO,
               &x_params, &y_params);

    char str_buff[200] = {0};
    for (unsigned row = 0; row < y_params.height; row++) {
      for (unsigned col = 0; col < y_params.width; col++) {
        for (unsigned chn = 0; chn < y_params.channels; chn++) {
          int8_t y_exp = casse->y;

          if (((uint8_t)casse->x) == 0xED) {
            int8_t exps[] = {0x1F, 2 * chn, 0x3E};
            y_exp = exps[casse->k];
          }

          Check_Y(y_exp, (nn_image_t*)Y, &y_params, row, col, chn, casse->line);
        }
      }
    }
  }
}
#undef CHANS_IN
#undef CHANS_OUT
#undef HEIGHT
#undef WIDTH
#undef CHANS_OUT_CEIL

#define MAX_CHANS_IN (3 * VPU_INT8_EPV)
#define MAX_CHANS_OUT (3 * VPU_INT8_ACC_PERIOD)
#define HEIGHT (2)
#define WIDTH (2)
#define CHANS_OUT_CEIL                           \
  ((MAX_CHANS_OUT + (VPU_INT8_ACC_PERIOD - 1)) & \
   (0xFFFFFFFF << VPU_INT8_ACC_PERIOD_LOG2))
void test_conv2d_1x1_case1() {
  int8_t WORD_ALIGNED X[HEIGHT][WIDTH][MAX_CHANS_IN];

  int8_t WORD_ALIGNED K[MAX_CHANS_OUT][MAX_CHANS_IN];

  struct {
    int32_t bias[CHANS_OUT_CEIL];
    int16_t shift1[CHANS_OUT_CEIL];
    int16_t scale[CHANS_OUT_CEIL];
    int16_t offset_scale[CHANS_OUT_CEIL];
    int16_t offset[CHANS_OUT_CEIL];
    int16_t shift2[CHANS_OUT_CEIL];
  } BSO;

  nn_bso_block_t bso[BSO_BLOCK_COUNT(MAX_CHANS_OUT)];

  int8_t Y_exp[MAX_CHANS_OUT];

  int8_t WORD_ALIGNED Y[HEIGHT][WIDTH][MAX_CHANS_OUT];

  PRINTF("%s...\n", __func__);

  int8_t* K_flat = (int8_t*)K;
  int8_t* X_flat = (int8_t*)X;

  int8_t* Y_flat = (int8_t*)Y;

  typedef struct {
    unsigned C_in;
    unsigned C_out;
    unsigned start[2];
    unsigned out_pixels;
    unsigned line;
  } test_case_t;

  const test_case_t casses[] = {
      //      C_in    C_out       //Start         //Out
      {32, 16, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {32, 32, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {32, 48, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {64, 16, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {96, 16, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {64, 32, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {16, 16, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {8, 16, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {4, 16, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {32, 8, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {32, 12, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {32, 4, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {36, 16, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {48, 16, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {80, 16, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {32, 24, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {32, 20, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {32, 36, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {16, 8, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {8, 8, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {48, 24, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {40, 12, {0, 0}, WIDTH * HEIGHT, __LINE__},
      {32, 16, {0, 0}, 1, __LINE__},
      {32, 32, {0, 0}, 2, __LINE__},
      {32, 48, {0, 0}, 3, __LINE__},
      {64, 16, {0, 1}, 1, __LINE__},
      {96, 16, {0, 1}, 2, __LINE__},
      {64, 32, {0, 1}, 3, __LINE__},
      {16, 16, {1, 0}, 1, __LINE__},
      {8, 16, {1, 0}, 2, __LINE__},
      {4, 16, {1, 1}, 1, __LINE__},
      {32, 8, {1, 1}, 1, __LINE__},

  };

  const unsigned N_casses = sizeof(casses) / sizeof(test_case_t);
  const unsigned start_case = 0;
  const unsigned stop_case = -1;
  print_warns(start_case);

  const int8_t x_val = 0x01;
  memset(X, x_val, sizeof(X));

  for (unsigned v = start_case; v < N_casses && v < stop_case; v++) {
    const test_case_t* casse = (const test_case_t*)&casses[v];
    PRINTF("\ttest vector %u...\n", v);

    const unsigned C_out = casse->C_out;
    const unsigned C_in = casse->C_in;

    for (int k = 0; k < C_out; k++) {
      int a = k % 2;
      // int b = (k>>1) % 2;
      // int c = (k % 5) - 2;
      int c = a;
      int d = 0;

      memset(&K_flat[k * C_in], c, sizeof(int8_t) * C_in);

      int32_t bias = 0;
      int16_t shift1 = d;
      int16_t scale = 1;
      int16_t offset_scale = 1;
      int16_t offset = 1;
      int16_t shift2 = 0;

      BSO.bias[k] = bias;
      BSO.shift1[k] = shift1;
      BSO.scale[k] = scale;
      BSO.offset_scale[k] = offset_scale;
      BSO.offset[k] = offset;
      BSO.shift2[k] = shift2;

      Y_exp[k] = ((x_val * c * C_in) >> d) + 1;
    }

    nn_image_params_t x_params = {HEIGHT, WIDTH, C_in};
    nn_image_params_t y_params = {HEIGHT, WIDTH, C_out};

    memset(
        Y, 0xCC,
        sizeof(int8_t) * y_params.height * y_params.width * y_params.channels);

    nn_standard_BSO_layout(bso, (int32_t*)&BSO.bias, (int16_t*)&BSO.shift1,
                           (int16_t*)&BSO.scale, (int16_t*)&BSO.offset_scale,
                           (int16_t*)&BSO.offset, (int16_t*)&BSO.shift2, NULL,
                           y_params.channels);

    nn_conv2d_1x1_job_params_t job_params[1] = {
        {{casse->start[0], casse->start[1], 0},
         {casse->out_pixels, y_params.channels}},
    };

    conv2d_1x1_ext((int8_t*)Y, (int8_t*)X, (int8_t*)K, (nn_bso_block_t*)&bso,
                   &x_params, &y_params, &job_params[0], 0);

    unsigned pix_start = casse->start[0] * y_params.width + casse->start[1];
    unsigned pix_end = pix_start + casse->out_pixels;

    for (unsigned row = 0; row < y_params.height; row++) {
      for (unsigned col = 0; col < y_params.width; col++) {
        for (unsigned chn = 0; chn < y_params.channels; chn++) {
          int pdex = row * y_params.width + col;
          int8_t y_exp =
              (pdex >= pix_start && pdex < pix_end) ? Y_exp[chn] : 0xCC;

          Check_Y(y_exp, (nn_image_t*)Y, &y_params, row, col, chn, casse->line);
        }
      }
    }
  }
}
#undef MAX_CHANS_IN
#undef MAX_CHANS_OUT
#undef HEIGHT
#undef WIDTH
#undef CHANS_OUT_CEIL

#define MAX_CHANS_IN (3 * VPU_INT8_EPV)
#define MAX_CHANS_OUT (3 * VPU_INT8_ACC_PERIOD)
#define HEIGHT (2)
#define WIDTH (1)
#define CHANS_OUT_CEIL                           \
  ((MAX_CHANS_OUT + (VPU_INT8_ACC_PERIOD - 1)) & \
   (0xFFFFFFFF << VPU_INT8_ACC_PERIOD_LOG2))
#define REPS (50)
void test_conv2d_1x1_case2() {
  srand(2341234);
  int8_t WORD_ALIGNED X[HEIGHT][WIDTH][MAX_CHANS_IN];

  int8_t WORD_ALIGNED K[MAX_CHANS_OUT][MAX_CHANS_IN];

  struct {
    int32_t bias[CHANS_OUT_CEIL];
    int16_t shift1[CHANS_OUT_CEIL];
    int16_t scale[CHANS_OUT_CEIL];
    int16_t offset_scale[CHANS_OUT_CEIL];
    int16_t offset[CHANS_OUT_CEIL];
    int16_t shift2[CHANS_OUT_CEIL];
  } BSO;
  nn_bso_block_t bso[BSO_BLOCK_COUNT(MAX_CHANS_OUT)];

  int8_t Y_exp[MAX_CHANS_OUT];

  int8_t WORD_ALIGNED Y[HEIGHT][WIDTH][MAX_CHANS_OUT];

  int8_t* K_flat = (int8_t*)K;
  int8_t* X_flat = (int8_t*)X;

  int8_t* Y_flat = (int8_t*)Y;

  PRINTF("%s...\n", __func__);

  const int8_t x_val = 1;

  memset(X, x_val, sizeof(X));

  // For debugging purposes, burn a number of pseudo_rands equal to this many
  //  test cases. This skips the earlier tests without changing the result of
  //  the one that was bugged.
#define SKIP_REPS (0)
  unsigned skip_reps = SKIP_REPS;

  for (unsigned rep = SKIP_REPS; rep < REPS; rep++) {
    unsigned C_in = 4 * (pseudo_rand_uint32() % ((MAX_CHANS_IN >> 2) - 1)) + 4;
    unsigned C_out =
        4 * (pseudo_rand_uint32() % ((MAX_CHANS_OUT >> 2) - 1)) + 4;
    if (rep == 0) {
      C_in = VPU_INT8_EPV;
      C_out = VPU_INT8_ACC_PERIOD;
    }

    PRINTF("\trep %u...(%u, %u)\n", rep, C_in, C_out);

    for (int k = 0; k < C_out; k++) {
      int8_t k_val = pseudo_rand_uint32();

      memset(&K_flat[k * C_in], k_val, sizeof(int8_t) * C_in);

      const int32_t dot_prod = k_val * x_val * C_in;

      int32_t bias = pseudo_rand_int32() >> 8;

      const int32_t acc32 = bias + dot_prod;

      int l2_ub = ceil_log2(acc32);

      int16_t shift1 = 0;
      if (l2_ub > 15) shift1 = (pseudo_rand_uint32() % 4) + (l2_ub - 15);

      int16_t prescale = vlsat_single_s16(acc32, shift1);

      int16_t scale = pseudo_rand_uint32();
      scale = 2;

      int32_t postscale = prescale * ((int32_t)scale);
      l2_ub = ceil_log2(postscale < 0 ? -postscale : postscale);

      int16_t shift2 = 0;
      if (l2_ub > 7 && !(postscale * postscale == 1 << 14)) {
        shift2 = l2_ub - 7 + pseudo_rand_uint32() % (3);
      }

      int8_t output =
          vlsat_single_s8(postscale, shift2, NEG_SAT_VAL, VPU_INT8_MAX);

      BSO.bias[k] = bias;
      BSO.shift1[k] = shift1;
      BSO.scale[k] = scale;
      BSO.offset_scale[k] = 0;
      BSO.offset[k] = 0;
      BSO.shift2[k] = shift2;

      // if(!skip_reps)
      //     PRINTF("%d:\t% 4d\t0x%08X\t0x%08X\n", k, k_val, bias, acc32);

      Y_exp[k] = output;
    }

    // Here we've done all our pseudo_rand's, so we can continue without
    // changing the behavior.
    if (skip_reps) {
      PRINTF("\t\t(skipped)\n");
      skip_reps--;
      continue;
    }

    nn_image_params_t x_params = {HEIGHT, WIDTH, C_in};
    nn_image_params_t y_params = {HEIGHT, WIDTH, C_out};

    memset(
        Y, 0xCC,
        sizeof(int8_t) * y_params.height * y_params.width * y_params.channels);

    nn_standard_BSO_layout(bso, (int32_t*)&BSO.bias, (int16_t*)&BSO.shift1,
                           (int16_t*)&BSO.scale, (int16_t*)&BSO.offset_scale,
                           (int16_t*)&BSO.offset, (int16_t*)&BSO.shift2, NULL,
                           y_params.channels);

    conv2d_1x1((int8_t*)Y, (int8_t*)X, (int8_t*)K, (nn_bso_block_t*)&bso,
               &x_params, &y_params);

    for (unsigned row = 0; row < y_params.height; row++) {
      for (unsigned col = 0; col < y_params.width; col++) {
        for (unsigned chn = 0; chn < y_params.channels; chn++) {
          int8_t y_exp = Y_exp[chn];

          Check_Y(y_exp, (nn_image_t*)Y, &y_params, row, col, chn, __LINE__);
        }
      }
    }
  }
}
#undef CHANS_IN
#undef CHANS_OUT
#undef HEIGHT
#undef WIDTH
#undef CHANS_OUT_CEIL
#undef REPS

#define CHANS_OUT (20)
#define CHANS_IN (36)
#define HEIGHT (2)
#define WIDTH (2)
void test_conv2d_1x1_case3() {
  PRINTF("%s...\n", __func__);

  srand(2341234);

  nn_image_t WORD_ALIGNED X[HEIGHT][WIDTH][CHANS_IN];
  memset(X, 0, sizeof(X));

  nn_tensor_t WORD_ALIGNED K[CHANS_OUT][CHANS_IN];
  memset(K, 0, sizeof(K));

  struct {
    int32_t bias[CHANS_OUT];
    int16_t shift1[CHANS_OUT];
    int16_t scale[CHANS_OUT];
    int16_t offset_scale[CHANS_OUT];
    int16_t offset[CHANS_OUT];
    int16_t shift2[CHANS_OUT];
  } BSO;
  nn_bso_block_t bso[BSO_BLOCK_COUNT(CHANS_OUT)];

  nn_image_t Y_exp[CHANS_OUT];

  nn_image_t WORD_ALIGNED Y[HEIGHT][WIDTH][CHANS_OUT];

  for (int k = 0; k < CHANS_OUT; k++) {
    BSO.bias[k] = (k % 2 == 0) ? -128 : -127;
    BSO.shift1[k] = 0;
    BSO.scale[k] = 1;
    BSO.offset_scale[k] = 0;
    BSO.offset[k] = 0;
    BSO.shift2[k] = 0;
  }

  nn_image_params_t x_params = {HEIGHT, WIDTH, CHANS_IN};
  nn_image_params_t y_params = {HEIGHT, WIDTH, CHANS_OUT};

  nn_standard_BSO_layout(bso, (int32_t*)&BSO.bias, (int16_t*)&BSO.shift1,
                         (int16_t*)&BSO.scale, (int16_t*)&BSO.offset_scale,
                         (int16_t*)&BSO.offset, (int16_t*)&BSO.shift2, NULL,
                         y_params.channels);

  conv2d_1x1((int8_t*)Y, (int8_t*)X, (int8_t*)K, (nn_bso_block_t*)&bso,
             &x_params, &y_params);

  for (unsigned row = 0; row < y_params.height; row++) {
    for (unsigned col = 0; col < y_params.width; col++) {
      for (unsigned chn = 0; chn < y_params.channels; chn++) {
        nn_image_t y_exp = (chn % 2 == 0) ? NEG_SAT_VAL : -127;

        Check_Y(y_exp, (nn_image_t*)Y, &y_params, row, col, chn, __LINE__);
      }
    }
  }
}
#undef CHANS_IN
#undef CHANS_OUT
#undef HEIGHT
#undef WIDTH

#define CHANS_OUT (20)
#define CHANS_IN (36)
#define HEIGHT (4)
#define WIDTH (4)
void test_conv2d_1x1_case4() {
  PRINTF("%s...\n", __func__);

  nn_image_t WORD_ALIGNED X[HEIGHT][WIDTH][CHANS_IN];
  memset(X, 1, sizeof(X));

  nn_tensor_t WORD_ALIGNED K[CHANS_OUT][CHANS_IN];
  memset(K, 0, sizeof(K));

  struct {
    int32_t bias[CHANS_OUT];
    int16_t shift1[CHANS_OUT];
    int16_t scale[CHANS_OUT];
    int16_t offset_scale[CHANS_OUT];
    int16_t offset[CHANS_OUT];
    int16_t shift2[CHANS_OUT];
  } BSO;
  nn_bso_block_t bso[BSO_BLOCK_COUNT(CHANS_OUT)];

  nn_image_t Y_exp[CHANS_OUT];

  nn_image_t WORD_ALIGNED Y[HEIGHT][WIDTH][CHANS_OUT];

  for (int k = 0; k < CHANS_OUT; k++) {
    K[k][0] = 1;
    BSO.bias[k] = k - 100;
    BSO.shift1[k] = 0;
    BSO.scale[k] = 1;
    BSO.offset_scale[k] = 0;
    BSO.offset[k] = 0;
    BSO.shift2[k] = 0;
  }

  nn_image_params_t x_params = {HEIGHT, WIDTH, CHANS_IN};
  nn_image_params_t y_params = {HEIGHT, WIDTH, CHANS_OUT};

  nn_standard_BSO_layout(bso, (int32_t*)&BSO.bias, (int16_t*)&BSO.shift1,
                         (int16_t*)&BSO.scale, (int16_t*)&BSO.offset_scale,
                         (int16_t*)&BSO.offset, (int16_t*)&BSO.shift2, NULL,
                         y_params.channels);

  for (int r = 0; r < HEIGHT; r++)
    for (int c = 0; c < WIDTH; c++)
      for (int ch = 0; ch < CHANS_IN; ch++) X[r][c][ch] = r + c;

  memset(Y, 0xCC, sizeof(Y));

  nn_conv2d_1x1_job_params_t job_params[4] = {
      {{0, 0, 0}, {1, 20}},
      {{0, 1, 0}, {3, 20}},
      {{1, 0, 0},
       {12, 16}},  // Note, channels 16+ of the second row are not calculated!
      {{2, 0, 16}, {8, 4}},
  };

  for (int i = 0; i < 4; i++)
    conv2d_1x1_ext((int8_t*)Y, (int8_t*)X, (int8_t*)K, (nn_bso_block_t*)&bso,
                   &x_params, &y_params, &job_params[i], 0);

  for (unsigned row = 0; row < y_params.height; row++) {
    for (unsigned col = 0; col < y_params.width; col++) {
      for (unsigned chn = 0; chn < y_params.channels; chn++) {
        nn_image_t y_exp = row + col + (chn - 100);

        if (row == 1 && chn >= 16) y_exp = 0xCC;

        Check_Y(y_exp, (nn_image_t*)Y, &y_params, row, col, chn, __LINE__);
      }
    }
  }
}
#undef CHANS_IN
#undef CHANS_OUT
#undef HEIGHT
#undef WIDTH

#define CHANS_OUT (40)
#define CHANS_IN (36)
#define HEIGHT (4)
#define WIDTH (4)
void test_conv2d_1x1_case5() {
  PRINTF("%s...\n", __func__);

  nn_image_t WORD_ALIGNED X[HEIGHT][WIDTH][CHANS_IN];
  memset(X, 1, sizeof(X));

  nn_tensor_t WORD_ALIGNED K[CHANS_OUT][CHANS_IN];
  memset(K, 0, sizeof(K));
  memset(&K[16][0], 1, CHANS_IN * 16);

  struct {
    int32_t bias[CHANS_OUT];
    int16_t shift1[CHANS_OUT];
    int16_t scale[CHANS_OUT];
    int16_t offset_scale[CHANS_OUT];
    int16_t offset[CHANS_OUT];
    int16_t shift2[CHANS_OUT];
  } BSO;
  nn_bso_block_t bso[BSO_BLOCK_COUNT(CHANS_OUT)];

  nn_image_t Y_exp[CHANS_OUT];

  nn_image_t WORD_ALIGNED Y[HEIGHT][WIDTH][CHANS_OUT];

  for (int k = 0; k < CHANS_OUT; k++) {
    BSO.bias[k] = (k >= 16 && k < 32) ? 28 : 1234637;
    BSO.shift1[k] = 0;
    BSO.scale[k] = 1;
    BSO.offset_scale[k] = 0;
    BSO.offset[k] = 0;
    BSO.shift2[k] = 0;
  }

  nn_image_params_t x_params = {HEIGHT, WIDTH, CHANS_IN};
  nn_image_params_t y_params = {HEIGHT, WIDTH, CHANS_OUT};

  nn_standard_BSO_layout(bso, (int32_t*)&BSO.bias, (int16_t*)&BSO.shift1,
                         (int16_t*)&BSO.scale, (int16_t*)&BSO.offset_scale,
                         (int16_t*)&BSO.offset, (int16_t*)&BSO.shift2, NULL,
                         y_params.channels);

  nn_conv2d_1x1_job_params_t job_params = {{0, 0, 16}, {HEIGHT * WIDTH, 16}};

  // First, without the slice flag.
  memset(Y, 0xCC, sizeof(Y));

  conv2d_1x1_ext((int8_t*)Y, (int8_t*)X, (int8_t*)K, bso, &x_params, &y_params,
                 &job_params, 0);

  for (unsigned row = 0; row < y_params.height; row++) {
    for (unsigned col = 0; col < y_params.width; col++) {
      for (unsigned chn = 0; chn < y_params.channels; chn++) {
        nn_image_t y_exp = (chn >= 16 && chn < 32) ? 64 : 0xCC;

        Check_Y(y_exp, (nn_image_t*)Y, &y_params, row, col, chn, __LINE__);
      }
    }
  }

  // Then with the slice flag.
  memset(Y, 0xCC, sizeof(Y));

  nn_tensor_t* K_start = &K[16][0];
  nn_bso_block_t* bso_start = &bso[1];

  conv2d_1x1_ext((int8_t*)Y, (int8_t*)X, K_start, bso_start, &x_params,
                 &y_params, &job_params, CONV2D_1X1_FLAG_SLICED_K);

  for (unsigned row = 0; row < y_params.height; row++) {
    for (unsigned col = 0; col < y_params.width; col++) {
      for (unsigned chn = 0; chn < y_params.channels; chn++) {
        nn_image_t y_exp = (chn >= 16 && chn < 32) ? 64 : 0xCC;

        Check_Y(y_exp, (nn_image_t*)Y, &y_params, row, col, chn, __LINE__);
      }
    }
  }
}
#undef CHANS_IN
#undef CHANS_OUT
#undef HEIGHT
#undef WIDTH

void test_conv2d_1x1() {
  UNITY_SET_FILE();

  RUN_TEST(test_conv2d_1x1_case0);
  RUN_TEST(test_conv2d_1x1_case1);
  RUN_TEST(test_conv2d_1x1_case2);
  RUN_TEST(test_conv2d_1x1_case3);
  RUN_TEST(test_conv2d_1x1_case4);
  RUN_TEST(test_conv2d_1x1_case5);
}