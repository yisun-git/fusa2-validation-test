#include "../pmu_fu.h"

mov %cr4, %eax
mov %eax, (0x8100)

#platform_info
info = MSR_PLATFORM_INFO
mov $info, %ecx
rdmsr
mov %eax, (AP_PLATFORM_INFO_ADDR)
mov %edx, (AP_PLATFORM_INFO_ADDR + 4)
