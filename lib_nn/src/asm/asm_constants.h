// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#pragma once

#ifndef __ASSEMBLER__

#include <stdint.h>

#include "xs3_vpu.h"

typedef struct {
  // Word offset = 0
  int16_t vec_0x007F[VPU_INT8_ACC_PERIOD];
  // Word offset = 8
  int8_t vec_0x01[VPU_INT8_ACC_PERIOD];
  // Word offset = 12
  int16_t vec_0x0002[VPU_INT8_ACC_PERIOD];
  // Word offset = 20
  int8_t vec_0x80[VPU_INT8_EPV];
  // Word offset = 28

} vpu_constants_t;

extern const vpu_constants_t vpu_vects;

extern const uint32_t vpu_vect_zero[VPU_INT32_EPV];
extern const int16_t vpu_vect_0x007F[VPU_INT16_EPV];
extern const int8_t vpu_vect_0x01[VPU_INT8_EPV];
extern const int8_t vpu_vect_0x02[VPU_INT8_EPV];
extern const int8_t vpu_vect_0x80[VPU_INT8_EPV];

#endif  // __ASSEMBLER__

#define VPU_MODE_32BIT 0x0000
#define VPU_MODE_16BIT 0x0100
#define VPU_MODE_8BIT 0x0200

#define VPU_VEC_0x007F (0)
#define VPU_VEC_0x01 (8)
#define VPU_VEC_0x0002 (12)
#define VPU_VEC_0x80 (20)
