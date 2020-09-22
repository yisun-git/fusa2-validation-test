#include "../sse.h"
mov %cr4, %eax
mov %eax, STARTUP_CR4_SAVE_ADDR
fxsave STARTUP_SSE_XSAVE_ADDR