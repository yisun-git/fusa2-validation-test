#include "../smx.h"
#include "msr.h"

movl $MSR_IA32_FEATURE_CONTROL,%ecx
rdmsr
movl %eax, (STARTUP_IA32_FEATURE_CONTROL)

mov %cr4, %eax
mov %eax, STARTUP_CR4_ADDR

