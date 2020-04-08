#include "../machine_check.h"
#indlude "msr.h"
movl $MSR_IA32_FEATURE_CONTROL,%ecx
rdmsr
movl %eax, (STARTUP_IA32_FEATURE_CONTROL)
