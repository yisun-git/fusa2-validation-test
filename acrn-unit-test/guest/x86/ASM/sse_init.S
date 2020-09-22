#include "../sse.h"
mov %cr4, %eax
mov %eax, INIT_CR4_SAVE_ADDR
fxsave INIT_SSE_XSAVE_ADDR