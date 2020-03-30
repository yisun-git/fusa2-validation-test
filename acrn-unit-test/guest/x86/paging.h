#ifndef PAGING_H
#define PAGING_H

#define PASS		0U
#define TYPE_TLB_01	0x01
#define TYPE_TLB_03	0x03
#define TYPE_TLB_63	0x63
#define TYPE_TLB_76	0x76
#define TYPE_TLB_B6	0xb6
#define TYPE_STLB_C3	0xc3
#define TYPE_PREFECTH_F0	0xf0
#define TYPE_GENEAL_FF		0xff

#define MSR_EFER_ME		(1ul << 8)
#define EDX_PAT			(1ul << 16)
#define MSR_IA32_CR_PAT_TEST 0x00000277
#define X86_IA32_EFER_NXE (1ul << 11)
#define X86_CR4_SMEP (1ul << 20)
#define X86_CR4_PGE (1ul << 7)
#define X86_CR0_CD		(1UL << 30)

#define USER_BASE		(1ul << 25)
#define PHYSICAL_ADDRESS_WIDTH	39
#define LINEAR_ADDRESS_WIDTH	48
#define PAGE_PTE_RESERVED_BIT	51
#define ERROR_CODE_INIT_VALUE	0x0000

#define MAX_OFFSET_OF_4K_PAGING	0xfff
#define PAGING_IA32_EFER          0xc0000080

#define STARTUP_CR0_ADDR	(0x6000)
#define STARTUP_CR2_ADDR	(0x6004)
#define STARTUP_CR4_ADDR	(0x6008)
#define STARTUP_EFLAGS_ADDR	(0x600c)
#define STARTUP_IA32_EFER_LOW_ADDR		(0x6010)
#define STARTUP_IA32_EFER_HIGH_ADDR		(0x6014)
#define STARTUP_CR3_ADDR			(0x6018)

#define INIT_CR0_ADDR		(0x7000)
#define INIT_CR2_ADDR		(0x7004)
#define INIT_CR4_ADDR		(0x7008)
#define INIT_EFLAGS_ADDR	(0x700c)
#define INIT_IA32_EFER_LOW_ADDR		(0x7010)
#define INIT_IA32_EFER_HIGH_ADDR	(0x7014)
#define INIT_CR3_ADDR			(0x7018)

#define WRITE_INITIAL_VALUE			0x1

#define	ERROR_CODE_P_BIT		(1ul << 0)
#define	ERROR_CODE_WR_BIT		(1ul << 1)
#define ERROR_CODE_US_BIT		(1UL << 2)
#define	ERROR_CODE_RSVD_BIT		(1UL << 3)
#endif

