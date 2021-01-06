/* 
 * Copyright (C) 2014, Galois, Inc.
 * This sotware is distributed under a standard, three-clause BSD license.
 * Please see the file LICENSE, distributed with this software, for specific
 * terms and conditions.
 */
#include <stdlib.h>

char *getenv(const char *name __attribute__((unused)))
{
  return NULL; // There is no environment, so this always returns "not found"
}
