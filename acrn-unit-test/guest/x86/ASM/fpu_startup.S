#include "../fpu.h"
mov % cr0, % eax
mov % eax, (STARTUP_CR0_REG_ADDR)
