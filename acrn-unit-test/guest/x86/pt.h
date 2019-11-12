#ifndef PT_H
#define PT_H

#define PASS 0
#ifdef __x86_64__
#define uint64_t unsigned long
#else
#define uint64_t unsigned long long
#endif
#define IA32_RTIT_OUTPUT_BASE		0x00000560
#define IA32_RTIT_OUTPUT_MASK_PTRS	0x00000561
#define IA32_RTIT_CTL			0x00000570
#define IA32_RTIT_STATUS		0x00000571
#define IA32_RTIT_CR3_MATCH		0x00000572
#define IA32_RTIT_ADDR0_A              0x00000580
#define IA32_RTIT_ADDR0_B              0x00000581
#define IA32_RTIT_ADDR1_A              0x00000582
#define IA32_RTIT_ADDR1_B              0x00000583
#define IA32_RTIT_ADDR2_A              0x00000584
#define IA32_RTIT_ADDR2_B              0x00000585
#define IA32_RTIT_ADDR3_A              0x00000586
#define IA32_RTIT_ADDR3_B              0x00000587

#define MSR_VALUE			0x11
#define CPUID_07_PT_BIT			(1 << 25)
#define CPUID_14H_PT			0x14
#endif

