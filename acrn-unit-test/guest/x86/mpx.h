#ifndef __MPX_H
#define __MPX_H

#define MSR_IA32_BNDCFGS		0x00000d90
#define XCR0_MASK       0x00000000
#define XCR_XFEATURE_ENABLED_MASK       0x00000000
#define XCR_XFEATURE_ILLEGAL_MASK       0x00000010

#define PASS 0

#define REX_PREFIX "0x48, "

#define XS_BNDCFG 0x18

#define MAWAU_BIT 0x3e0000
#define MPX_BIT 0x4000


#ifdef __x86_64__
extern struct descriptor_table_ptr gdt64_desc;
#endif
static __unused int check_cpuid_1_ecx(unsigned int bit)
{
	return (cpuid(1).c & bit) != 0;
}
#endif
