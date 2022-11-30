/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *                                                                           *\
 *  Copyright (c) 2015                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *     CMLA, Ecole Normale Superieure de Cachan                              *\
 *                                                                           *\
 *  Copyright (c) 2018                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *                                                                           *\
 *  Copyright (c) 2019-2022                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
// Changelog:
//
// 2021-10-13 Switched random number generator from TinyMT64 to the one
// provided by the libc. The backend is now re-entrant. Pthread and OpenMP
// threads are now supported.
// Generation of hook functions is now done through macros, shared accross
// backends.

#include <argp.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>

#include "common/float_const.h"
#include "common/float_struct.h"
#include "common/float_utils.h"
#include "common/logger.h"
#include "common/options.h"
#include "common/rng/vfc_rng.h"
#include "interflop.h"
#include "interflop_cancellation.h"
#include "interflop_stdlib.h"

/* define default environment variables and default parameters */
#define TOLERANCE_DEFAULT 1
#define WARNING_DEFAULT 0

#define max(a, b) ((a) > (b) ? (a) : (b))

static void _set_tolerance(int tolerance, void *context) {
  cancellation_context_t *ctx = (cancellation_context_t *)context;
  ctx->tolerance = tolerance;
}

static void _set_warning(bool warning, void *context) {
  cancellation_context_t *ctx = (cancellation_context_t *)context;
  ctx->warning = warning;
}

/* global thread identifier */
static pid_t global_tid = 0;

/* helper data structure to centralize the data used for random number
 * generation */
static __thread rng_state_t rng_state;

/* noise = rand * 2^(exp) */
static inline double _noise_binary64(const int exp, rng_state_t *rng_state) {
  const double d_rand = get_rand_double01(rng_state, &global_tid) - 0.5;
  binary64 b64 = {.f64 = d_rand};
  b64.ieee.exponent += exp;
  return b64.f64;
}

/* cancell: detects the cancellation size; and checks if its larger than the
 * chosen tolerance. It reports a warning to the user and adds a MCA noise of
 * the magnitude of the cancelled bits. */
#define cancell(X, Y, Z, CTX, RNG_STATE)                                       \
  {                                                                            \
    cancellation_context_t *TMP_CTX = (cancellation_context_t *)CTX;           \
    const int32_t e_z = GET_EXP_FLT(*Z);                                       \
    /* computes the difference between the max of both operands and the        \
     * exponent of the result to find the size of the cancellation */          \
    int cancellation = max(GET_EXP_FLT(X), GET_EXP_FLT(Y)) - e_z;              \
    if (cancellation >= TMP_CTX->tolerance) {                                  \
      if (TMP_CTX->warning) {                                                  \
        logger_info("cancellation of size %d detected\n", cancellation);       \
      }                                                                        \
      /* Add an MCA noise of the magnitude of cancelled bits.                  \
       * This particular version in the case of cancellations does not use     \
       * extended quad types */                                                \
      const int32_t e_n = e_z - (cancellation - 1);                            \
      _init_rng_state_struct(&RNG_STATE, TMP_CTX->choose_seed, TMP_CTX->seed,  \
                             false);                                           \
      *Z += _noise_binary64(e_n, &RNG_STATE);                                  \
    }                                                                          \
  }

#define _u_ __attribute__((unused))

/* Cancellations can only happen during additions and substractions */
void INTERFLOP_CANCELLATION_API(add_float)(float a, float b, float *res,
                                           void *context) {
  *res = a + b;
  cancell(a, b, res, context, rng_state);
}
void INTERFLOP_CANCELLATION_API(sub_float)(float a, float b, float *res,
                                           void *context) {
  *res = a - b;
  cancell(a, b, res, context, rng_state);
}

void INTERFLOP_CANCELLATION_API(mul_float)(float a, float b, float *res,
                                           _u_ void *context) {
  *res = a * b;
}

void INTERFLOP_CANCELLATION_API(div_float)(float a, float b, float *res,
                                           _u_ void *context) {
  *res = a / b;
}

void INTERFLOP_CANCELLATION_API(add_double)(double a, double b, double *res,
                                            void *context) {
  *res = a + b;
  cancell(a, b, res, context, rng_state);
}
void INTERFLOP_CANCELLATION_API(sub_double)(double a, double b, double *res,
                                            void *context) {
  *res = a - b;
  cancell(a, b, res, context, rng_state);
}

void INTERFLOP_CANCELLATION_API(mul_double)(double a, double b, double *res,
                                            _u_ void *context) {
  *res = a * b;
}

void INTERFLOP_CANCELLATION_API(div_double)(double a, double b, double *res,
                                            _u_ void *context) {
  *res = a / b;
}

#undef _u_
static struct argp_option options[] = {
    {"tolerance", 't', "TOLERANCE", 0, "Select tolerance (TOLERANCE >= 0)", 0},
    {"warning", 'w', "WARNING", 0, "Enable warning for cancellations", 0},
    {"seed", 's', "SEED", 0, "Fix the random generator seed", 0},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  cancellation_context_t *ctx = (cancellation_context_t *)state->input;
  char *endptr;
  switch (key) {
  case 't':
    /* tolerance */
    errno = 0;
    int val = strtol(arg, &endptr, 10);
    if (errno != 0 || val < 0) {
      logger_error("--tolerance invalid value provided, must be a"
                   "positive integer.");
    } else {
      _set_tolerance(val, ctx);
    }
    break;
  case 'w':
    _set_warning(true, ctx);
    break;
  case 's':
    errno = 0;
    ctx->choose_seed = 1;
    ctx->seed = strtoull(arg, &endptr, 10);
    if (errno != 0) {
      logger_error("--seed invalid value provided, must be an integer");
    }
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, "", "", NULL, NULL, NULL};

void INTERFLOP_CANCELLATION_API(CLI)(int argc, char **argv, void *context) {
  /* parse backend arguments */
  cancellation_context_t *ctx = (cancellation_context_t *)context;
  if (interflop_argp_parse != NULL) {
    interflop_argp_parse(&argp, argc, argv, 0, 0, ctx);
  } else {
    interflop_panic("Interflop backend error: argp_parse not implemented\n"
                    "Provide implementation or use interflop_configure to "
                    "configure the backend\n");
  }
}

static void init_context(cancellation_context_t *ctx) {
  ctx->choose_seed = 0;
  ctx->seed = 0ULL;
  ctx->warning = WARNING_DEFAULT;
  ctx->tolerance = TOLERANCE_DEFAULT;
}

#define CHECK_IMPL(name)                                                       \
  if (interflop_##name == Null) {                                              \
    interflop_panic("Interflop backend error: " #name " not implemented\n");   \
  }

void _cancellation_check_stdlib(void) {
  CHECK_IMPL(malloc);
  CHECK_IMPL(exit);
  CHECK_IMPL(fopen);
  CHECK_IMPL(fprintf);
  CHECK_IMPL(getenv);
  CHECK_IMPL(gettid);
  CHECK_IMPL(sprintf);
  CHECK_IMPL(strcasecmp);
  CHECK_IMPL(strerror);
  CHECK_IMPL(vfprintf);
  CHECK_IMPL(vwarnx);
}

void INTERFLOP_CANCELLATION_API(pre_init)(File *stream, interflop_panic_t panic,
                                          void **context) {
  interflop_set_handler("panic", panic);
  _cancellation_check_stdlib();

  /* Initialize the logger */
  logger_init(stream);

  /* allocate the context */
  cancellation_context_t *ctx = (cancellation_context_t *)interflop_malloc(
      sizeof(cancellation_context_t));
  init_context(ctx);
  *context = ctx;
}

struct interflop_backend_interface_t
INTERFLOP_CANCELLATION_API(init)(void *context) {
  cancellation_context_t *ctx = (cancellation_context_t *)context;
  logger_info("interflop_cancellation: loaded backend with tolerance = %d\n",
              ctx->tolerance);

  struct interflop_backend_interface_t interflop_backend_cancellation = {
    interflop_add_float : INTERFLOP_CANCELLATION_API(add_float),
    interflop_sub_float : INTERFLOP_CANCELLATION_API(sub_float),
    interflop_mul_float : INTERFLOP_CANCELLATION_API(mul_float),
    interflop_div_float : INTERFLOP_CANCELLATION_API(div_float),
    interflop_cmp_float : NULL,
    interflop_add_double : INTERFLOP_CANCELLATION_API(add_double),
    interflop_sub_double : INTERFLOP_CANCELLATION_API(sub_double),
    interflop_mul_double : INTERFLOP_CANCELLATION_API(mul_double),
    interflop_div_double : INTERFLOP_CANCELLATION_API(div_double),
    interflop_cmp_double : NULL,
    interflop_cast_double_to_float : NULL,
    interflop_madd_float : NULL,
    interflop_madd_double : NULL,
    interflop_enter_function : NULL,
    interflop_exit_function : NULL,
    interflop_user_call : NULL,
    interflop_finalize : NULL
  };

  /* The seed for the RNG is initialized upon the first request for a random
     number */

  _init_rng_state_struct(&rng_state, ctx->choose_seed,
                         (unsigned long long int)(ctx->seed), false);

  return interflop_backend_cancellation;
}

struct interflop_backend_interface_t interflop_init(void *context)
    __attribute__((weak, alias("interflop_cancellation_init")));

void interflop_pre_init(File *stream, interflop_panic_t panic, void **context)
    __attribute__((weak, alias("interflop_cancellation_pre_init")));

void interflop_CLI(int argc, char **argv, void *context)
    __attribute__((weak, alias("interflop_cancellation_CLI")));