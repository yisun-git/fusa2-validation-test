#include "../fpu.h"
fstcw (INIT_FPU_CWS_ADDR)
fstsw (INIT_FPU_SWS_ADDR)
fxsave INIT_FPU_SAVE_ADDR
fsave INIT_FPU_FSAVE_ADDR
mov % cr0, % eax
mov % eax, (INIT_CR0_REG_ADDR)
