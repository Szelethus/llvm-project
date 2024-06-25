// RUN: %clang_analyze_cc1 -verify %s \
// RUN: -analyzer-checker=core,alpha.unix.cstring \
// RUN: -analyzer-output=text

#include "Inputs/system-header-simulator.h"

void memcpy_array_only_init_portion(FILE *f, char *dst) {
  char buf[10];
  fread(dst, sizeof(buf[0]), sizeof(buf)/sizeof(buf[0]), f);
  memcpy(dst, buf, 1);
  (void)buf;
}
