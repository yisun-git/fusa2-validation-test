#include "../../lib/x86/msr.h"
#include "../vmx_check.h"

/*read cr4*/
mov %cr4, %eax
mov %eax, (STARTUP_CR4_SAVE_ADDR)
/*read msr MSR_IA32_FEATURE_CONTROL*/
mov $(MSR_IA32_FEATURE_CONTROL), %ecx
rdmsr
mov %edx, (STARTUP_MSR_IA32_FEATURE_CONTROL_MSB_32bit_ADDR)
mov %eax, (STARTUP_MSR_IA32_FEATURE_CONTROL_LSB_32bit_ADDR)



