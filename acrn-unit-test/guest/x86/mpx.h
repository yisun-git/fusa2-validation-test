#ifndef __MPX_H
#define __MPX_H

#define MSR_IA32_BNDCFGS		0x00000d90
#define XCR0_MASK       0x00000000
#define XCR_XFEATURE_ENABLED_MASK       0x00000000
#define XCR_XFEATURE_ILLEGAL_MASK       0x00000010

#define REX_PREFIX "0x48, "

#define XS_BNDCFG 0x18

#define MAWAU_BIT 0x3e0000
#define MPX_BIT 0x4000


#ifdef __x86_64__
extern struct descriptor_table_ptr gdt64_desc;
#endif
#endif
