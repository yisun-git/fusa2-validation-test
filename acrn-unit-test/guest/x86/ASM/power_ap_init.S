#include "../power_thermal.h"

misc = IA32_MISC_ENABLE
mov $misc, %ecx
rdmsr
mov %eax, (AP_IA32_MISC_ENABLE_ADDR1)
mov %edx, (AP_IA32_MISC_ENABLE_ADDR2)
