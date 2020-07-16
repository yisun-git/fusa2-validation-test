#ifndef __X86_DELAY__
#define __X86_DELAY__

#include "libcflat.h"

void delay(u64 count);
void test_delay(int time);

#endif
