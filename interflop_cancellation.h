/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *  Copyright (c) 2019-2022                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#ifndef __INTERFLOP_CANCELLATION_H__
#define __INTERFLOP_CANCELLATION_H__

#include "interflop-stdlib/interflop_stdlib.h"

#define INTERFLOP_CANCELLATION_API(name) interflop_cancellation_##name

/* define default environment variables and default parameters */
#define CANCELLATION_TOLERANCE_DEFAULT 1
#define CANCELLATION_WARNING_DEFAULT 0
#define CANCELLATION_SEED_DEFAULT 0ULL

/* Interflop context */
typedef struct {
  IUint64_t seed;
  int tolerance;
  IBool choose_seed;
  IBool warning;
} cancellation_context_t;

typedef cancellation_context_t cancellation_conf_t;

const char *INTERFLOP_CANCELLATION_API(get_backend_name)(void);
const char *INTERFLOP_CANCELLATION_API(get_backend_version)(void);

void INTERFLOP_CANCELLATION_API(add_float)(float a, float b, float *res,
                                           void *context);
void INTERFLOP_CANCELLATION_API(sub_float)(float a, float b, float *res,
                                           void *context);
void INTERFLOP_CANCELLATION_API(mul_float)(float a, float b, float *res,
                                           void *context);
void INTERFLOP_CANCELLATION_API(div_float)(float a, float b, float *res,
                                           void *context);
void INTERFLOP_CANCELLATION_API(add_double)(double a, double b, double *res,
                                            void *context);
void INTERFLOP_CANCELLATION_API(sub_double)(double a, double b, double *res,
                                            void *context);
void INTERFLOP_CANCELLATION_API(mul_double)(double a, double b, double *res,
                                            void *context);
void INTERFLOP_CANCELLATION_API(div_double)(double a, double b, double *res,
                                            void *context);
void INTERFLOP_CANCELLATION_API(fma_float)(float a, float b, float c,
                                           float *res, _u_ void *context);
void INTERFLOP_CANCELLATION_API(fma_double)(double a, double b, double c,
                                            double *res, _u_ void *context);
void INTERFLOP_CANCELLATION_API(cast_double_to_float)(double a, float *b,
                                                      _u_ void *context);
void INTERFLOP_CANCELLATION_API(configure)(cancellation_conf_t conf,
                                           void *context);
void INTERFLOP_CANCELLATION_API(CLI)(int argc, char **argv, void *context);
void INTERFLOP_CANCELLATION_API(pre_init)(File *stream, interflop_panic_t panic,
                                          void **context);
struct interflop_backend_interface_t
    INTERFLOP_CANCELLATION_API(init)(void *context);

#endif /* __INTERFLOP_CANCELLATION_H__ */