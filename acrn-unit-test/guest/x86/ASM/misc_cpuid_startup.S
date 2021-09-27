#include "../misc_cpuid.h"
mov $MSR_IA32_MISC_ENABLE, %ecx
rdmsr
mov %eax, IA32_MISC_ENABLE_STARTUP_LOW_ADDR
mov %edx, IA32_MISC_ENABLE_STARTUP_HIGH_ADDR
