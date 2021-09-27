#include "../fpu.h"
mov % cr0, % eax
mov % eax, (STARTUP_CR0_REG_ADDR)
fstcw (STARTUP_FPU_CWS_ADDR)
fxsave (STARTUP_FPU_XSAVE_ADDR)
fsave STARTUP_FPU_FSAVE_ADDR
