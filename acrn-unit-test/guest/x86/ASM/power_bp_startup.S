#include "../power_thermal.h"

misc = IA32_MISC_ENABLE
mov $misc, %ecx
rdmsr
mov %eax, (BP_IA32_MISC_ENABLE_ADDR1)
mov %edx, (BP_IA32_MISC_ENABLE_ADDR2)
