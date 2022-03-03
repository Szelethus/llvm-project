// RUN: %clang_analyze_cc1 -verify %s \
// RUN:   -triple i386-apple-darwin10 -Wno-objc-root-class -fblocks \
// RUN:   -analyzer-output=text \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=osx.cocoa.NilArg \
// RUN:   -analyzer-checker=osx.cocoa.RetainCount \
// RUN:   -analyzer-checker=alpha.core.ReverseNull

#include "Inputs/system-header-simulator-objc.h"

typedef const struct __CFDictionary * CFDictionaryRef;
const void *CFDictionaryGetValue(CFDictionaryRef theDict, const void *key);

// From NSString.m:
NSString* f11(CFDictionaryRef dict, const char* key) {
  NSString* s = (NSString*) CFDictionaryGetValue(dict, key);
  // expected-note@-1{{'s' initialized here}}
  [s retain];
  // expected-note@-1{{Pointer assumed non-null here}}
  if (s) {
    // expected-warning@-1{{Pointer is unconditionally non-null here [alpha.core.ReverseNull]}}
    // expected-note@-2{{Pointer already constrained nonnull}}
    [s release];
  }
  return 0;
}
