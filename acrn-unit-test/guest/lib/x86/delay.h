#ifndef __X86_DELAY__
#define __X86_DELAY__

#include "libcflat.h"

#ifdef __x86_64__
#define TSC_PER_SECOND	2100000000UL
#else
#define TSC_PER_SECOND	2100000000ULL
#endif
void delay(u64 count);
void test_delay(int time);

#endif
