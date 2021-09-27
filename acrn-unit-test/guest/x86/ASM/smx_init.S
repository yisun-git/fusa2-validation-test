#include "../smx.h"
#include "msr.h"

mov $(MSR_IA32_FEATURE_CONTROL),%ecx
rdmsr
mov %eax, (INIT_IA32_FEATURE_CONTROL)

mov %cr4, %eax
mov %eax, INIT_CR4_ADDR

