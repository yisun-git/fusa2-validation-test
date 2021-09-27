#ifndef _HSI_H_
#define _HSI_H_


#define MSR_IA32_APIC_BASE                      0x0000001BU
#define MSR_IA32_TSC_DEADLINE                   0x000006E0U
#define LAPIC_CPUID_APIC_CAPABILITY             1
#define LAPIC_CPUID_APIC_CAP_X2APCI             (0x1U << 21)
#define EN_x2APIC                               (0x1U << 10)
#define EN_xAPIC                                (0x1U << 11)


#endif

