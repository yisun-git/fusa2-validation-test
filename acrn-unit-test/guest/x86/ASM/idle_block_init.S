#include "../idle_block.h"
/*MSR_IA32_MISC_ENABLE		0x000001a0*/
mov $0x000001a0, %ecx
rdmsr
mov %eax, (IA32_MISC_ENABLE_INIT1_ADDR)
mov %edx, (IA32_MISC_ENABLE_INIT1_ADDR + 4)
/* IA32_MONITOR_FILTER_SIZE		0x6*/
mov $0x6, %ecx
rdmsr
mov %eax, (IA32_MONITOR_FILTER_SIZE_INIT_ADDR)
mov %edx, (IA32_MONITOR_FILTER_SIZE_INIT_ADDR + 4)
