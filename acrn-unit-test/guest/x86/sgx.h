#ifndef SGX_H
#define SGX_H

#define PASS 0
#ifdef __x86_64__
#define uint64_t unsigned long
#else
#define uint64_t unsigned long long
#endif
#define MSR_UNCORE_PRMRR_PHYS_MASK		0x000002f5
#define MSR_UNCORE_PRMRR_PHYS_BASE		0x000002f4
#define MSR_PRMRR_VALID_CONFIG			0x000001fb
#define MSR_PRMRR_PHYS_MASK			0x000001f5
#define MSR_PRMRR_PHYS_BASE			0x000001f4	
#define MSR_SGXOWNEREPOCH1            		0x00000301
#define MSR_SGXOWNEREPOCH0             		0x00000300
#define IA32_SGX_SVN_STATUS            		0x00000500
#define IA32_SGXLEPUBKEYHASH3              	0x0000008f
#define IA32_SGXLEPUBKEYHASH2              	0x0000008e
#define IA32_SGXLEPUBKEYHASH1              	0x0000008d
#define IA32_SGXLEPUBKEYHASH0              	0x0000008c
#define SGX_CPUID_ID				0x12
#define IA32_FEATURE_CONTROL			0x0000003a

#define SGX_ENABLE_BIT				(1ul << 18)
#define SGX_LAUCH_BIT				(1ul << 17)
#define CPUID_07_SGX				(1 << 30)
#define VALUE_TO_WRITE_MSR			0x12
#define IA32_FEATURE_CONTROL_STARTUP_ADDR	(0x6000)
#define IA32_FEATURE_CONTROL_INIT1_ADDR		(0x8000)
#define IA32_FEATURE_CONTROL_INIT2_ADDR		(0x7000)

#endif

