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

#include "../../../interflop-stdlib/interflop_stdlib.h"

#define INTERFLOP_CANCELLATION_API(name) interflop_cancellation_##name

/* Interflop context */
typedef struct {
  IUint64_t seed;
  int tolerance;
  IBool choose_seed;
  IBool warning;
} cancellation_context_t;

#endif /* __INTERFLOP_CANCELLATION_H__ */