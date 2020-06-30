#include "../pmu_fu.h"

mov %cr4, %eax
mov %eax, (0x8000)

#platform_info
info = MSR_PLATFORM_INFO
mov $info, %ecx
rdmsr
mov %eax, (BP_PLATFORM_INFO_ADDR)
mov %edx, (BP_PLATFORM_INFO_ADDR + 4)
