#include "../machine_check.h"
#indlude "msr.h"
mov $(MSR_IA32_FEATURE_CONTROL),%ecx
rdmsr
mov %eax, (INIT_IA32_FEATURE_CONTROL)

mov $(MSR_IA32_MCG_CAP),%ecx
rdmsr
mov %eax, (INIT_IA32_MCG_CAP_LOW_ADDR)
mov %edx, (INIT_IA32_MCG_CAP_HIGH_ADDR)

mov $(MSR_IA32_MCG_STATUS),%ecx
rdmsr
mov %eax, (INIT_IA32_MCG_STATUS_LOW_ADDR)
mov %edx, (INIT_IA32_MCG_STATUS_HIGH_ADDR)

mov $(MSR_IA32_P5_MC_TYPE),%ecx
rdmsr
mov %eax, (INIT_IA32_P5_MC_TYPE_LOW_ADDR)
mov %edx, (INIT_IA32_P5_MC_TYPE_HIGH_ADDR)

mov $(MSR_IA32_P5_MC_ADDR),%ecx
rdmsr
mov %eax, (INIT_IA32_P5_MC_ADDR_LOW_ADDR)
mov %edx, (INIT_IA32_P5_MC_ADDR_HIGH_ADDR)

mov %cr4, %eax
mov %eax, (INIT_CR4_REG_ADDR)
