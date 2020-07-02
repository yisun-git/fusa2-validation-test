#include "../machine_check.h"
#indlude "msr.h"
mov $MSR_IA32_FEATURE_CONTROL,%ecx
rdmsr
mov %eax, (STARTUP_IA32_FEATURE_CONTROL)

mov $MSR_IA32_MCG_CAP,%ecx
rdmsr
mov %eax, (STARTUP_IA32_MCG_CAP)

mov $MSR_IA32_MCG_STATUS,%ecx
rdmsr
mov %eax, (STARTUP_IA32_MCG_STATUS)

mov $MSR_IA32_P5_MC_TYPE,%ecx
rdmsr
mov %eax, (STARTUP_IA32_P5_MC_TYPE_LOW_ADDR)
mov %edx, (STARTUP_IA32_P5_MC_TYPE_HIGH_ADDR)

mov $MSR_IA32_P5_MC_ADDR,%ecx
rdmsr
mov %eax, (STARTUP_IA32_P5_MC_ADDR_LOW_ADDR)
mov %edx, (STARTUP_IA32_P5_MC_ADDR_HIGH_ADDR)

mov %cr4, %eax
mov %eax, (STARTUP_CR4_REG_ADDR)

#ifdef IN_SAFETY_VM
#dump MSR IA32_MCi_STATUS(0<=i<=9))
mov $MSR_IA32_MC0_STATUS,%ecx
mov $STARTUP_IA32_MC0_STATUS_ADDR,%ebx
dump_mci_status:
   rdmsr
   mov %eax, (%ebx)
   add $0x04, %ebx
   mov %edx, (%ebx)
   add $0x04, %ecx
   add $0x04, %ebx
   cmp $STARTUP_IA32_MC9_STATUS_ADDR,%ebx
   jle dump_mci_status

#dump MSR IA32_MCi_CTL2(0<=i<=3)
mov $MSR_IA32_MC0_CTL2, %ecx
mov $STARTUP_IA32_MC0_CTL2_ADDR,%ebx
dump_mci_ctl2:
   rdmsr
   mov %eax, (%ebx)
   add $0x04, %ebx
   mov %edx, (%ebx)
   add $0x01, %ecx
   add $0x04, %ebx
   cmp $STARTUP_IA32_MC3_CTL2_ADDR, %ebx
   jle dump_mci_ctl2
#endif
