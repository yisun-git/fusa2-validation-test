#ifndef KEYLOCKER_H
#define KEYLOCKER_H

#define MSR_IA32_COPY_LOCAL_TO_PLATFORM		0xD91
#define MSR_IA32_COPY_PLATFORM_TO_LOCAL		0xD92
#define MSR_IA32_COPY_STATUS			0x990
#define MSR_IA32_IWKEYBACKUP_STATUS		0x991

#define CPUID_07_KL				(1 << 23)
#define CPUID_19_AESKLE                         (1 << 0)
#define CR4_KL                                  (1ULL << 19)


#endif

