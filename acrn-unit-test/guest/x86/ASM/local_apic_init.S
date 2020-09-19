#include "apic-defs.h"
#include "msr.h"
#include "../local_apic.h"

#ifndef IN_NATIVE
mov $MSR_IA32_APICBASE, %ecx
rdmsr
mov %eax, (IA32_APIC_BASE_INIT_ADDR)

/*bts $10, %eax
wrmsr*/

mov $MSR_APIC_CMCI, %ecx
rdmsr
mov %eax, (LVT_CMCI_INIT_ADDR)

mov $MSR_APIC_LVT0, %ecx
rdmsr
mov %eax, (LINT0_INIT_ADDR)

mov $MSR_APIC_LVT1, %ecx
rdmsr
mov %eax, (LINT1_INIT_ADDR)

mov $MSR_APIC_LVTERR, %ecx
rdmsr
mov %eax, (LVT_ERROR_INIT_ADDR)

mov $MSR_APIC_LVTTHMR, %ecx
rdmsr
mov %eax, (LVTTHMR_INIT_ADDR)

mov $MSR_APIC_LVTPC, %ecx
rdmsr
mov %eax, (LVTPC_INIT_ADDR)

mov $MSR_APIC_LVTT, %ecx
rdmsr
mov %eax, (LVT_TIMER_INIT_ADDR)

mov $MSR_APIC_ESR, %ecx
rdmsr
mov %eax, (ESR_INIT_ADDR)

mov $MSR_APIC_SPIV, %ecx
rdmsr
mov %eax, (SVR_INIT_ADDR)

mov $MSR_APIC_ICR, %ecx
rdmsr
mov %eax, (ICR_INIT_ADDR)

mov $MSR_APIC_TASKPRI, %ecx
rdmsr
mov %eax, (TPR_INIT_ADDR)

mov $MSR_APIC_PROCPRI, %ecx
rdmsr
mov %eax, (PPR_INIT_ADDR)

mov $MSR_APIC_IRR, %ecx
rdmsr
mov %eax, (IRR_INIT_ADDR)

mov $MSR_APIC_ISR, %ecx
rdmsr
mov %eax, (ISR_INIT_ADDR)

mov $MSR_APIC_TMR, %ecx
rdmsr
mov %eax, (TMR_INIT_ADDR)

mov $MSR_APIC_TMICT, %ecx
rdmsr
mov %eax, (TICR_INIT_ADDR)

mov $MSR_APIC_TMCCT, %ecx
rdmsr
mov %eax, (TCCR_INIT_ADDR)

mov $MSR_APIC_TDCR, %ecx
rdmsr
mov %eax, (TDCR_INIT_ADDR)

mov $MSR_IA32_TSCDEADLINE, %ecx
rdmsr
mov %eax, (IA32_TSC_DEADLINE_INIT_ADDR)
#endif
