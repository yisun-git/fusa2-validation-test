#include "../machine_check.h"
#indlude "msr.h"
mov $(MSR_IA32_FEATURE_CONTROL),%ecx
rdmsr
mov %eax, (INIT_IA32_FEATURE_CONTROL)
