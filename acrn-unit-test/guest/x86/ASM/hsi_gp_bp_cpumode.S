#include "../hsi_gp_cpumode.h"

movl $MSR_IA32_EFER,%ecx
rdmsr
movl %eax, (SAVE_IA32_EFER)

mov %cr0, %eax
mov %eax, (SAVE_CR0)

