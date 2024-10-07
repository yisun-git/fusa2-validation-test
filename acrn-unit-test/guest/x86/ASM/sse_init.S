#include "../sse.h"
mov %cr4, %eax
mov %eax, INIT_CR4_SAVE_ADDR
/*MSR_FEATURE_CONFIG       0x13C*/
mov $0x13C, %ecx
rdmsr
mov %eax, INIT_MSR_FEATURE_CONFIG_LOW_ADDR
mov %edx, INIT_MSR_FEATURE_CONFIG_HIGH_ADDR
fxsave INIT_SSE_XSAVE_ADDR