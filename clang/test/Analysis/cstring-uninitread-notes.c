// RUN: %clang_analyze_cc1 -verify %s \
// RUN:   -analyzer-checker=core,alpha.unix.cstring \
// RUN:   -analyzer-output=text

#include "Inputs/system-header-simulator.h"

// Inspired by a report on ffmpeg, libavcodec/tiertexseqv.c, seq_decode_op1().
int coin();

void maybeWrite(const char *src, unsigned size, int *dst) {
  if (coin())
    memcpy(dst, src, size);
}

void returning_without_writing_to_memcpy(const char *src, unsigned size) {
  int block[8 * 8];
  maybeWrite(src, size, block);

  int buf[8 * 8];
  memcpy(buf, &block[0], 8);
}
