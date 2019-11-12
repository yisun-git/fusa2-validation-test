#ifndef SMX_H
#define SMX_H

#define PASS 0
#define CPUID_1_SMX_SUPPORTED	(1ul << 6)
#define X86_CR4_SMX		(1ul << 14)
#define IA32_FEATURE_CONTROL	0x0000003a
#define SMX_SENTER_LOCAL_FUNCTON_ENABLE		0x7f00ul
#define SMX_SENTER_GLOBAL_ENABLE		(1UL << 15)
#ifdef __x86_64__
#define uint64_t unsigned long
#else
#define uint64_t unsigned long long
#endif
struct emulate_register { u32 a, b, c, d; };

#endif

