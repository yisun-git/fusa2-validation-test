#include "../fpu.h"
fstcw (INIT_FPU_CWS_ADDR)
fxsave INIT_FPU_SAVE_ADDR
fsave INIT_FPU_FSAVE_ADDR
