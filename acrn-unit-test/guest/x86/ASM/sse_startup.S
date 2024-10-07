#include "../sse.h"
mov %cr4, %eax
mov %eax, STARTUP_CR4_SAVE_ADDR
/*MSR_FEATURE_CONFIG       0x13C*/
mov $0x13C, %ecx
rdmsr
mov %eax, STARTUP_MSR_FEATURE_CONFIG_LOW_ADDR
mov %edx, STARTUP_MSR_FEATURE_CONFIG_HIGH_ADDR

fxsave STARTUP_SSE_XSAVE_ADDR