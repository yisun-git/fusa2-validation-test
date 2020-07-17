/*segmentation.h*/
#ifndef _SEGMENTATION_H_
#define _SEGMENTATION_H_
#include "libcflat.h"
#include "processor.h"
#include "desc.h"
#include "alloc.h"
#include "apic-defs.h"
#include "apic.h"
#include "vm.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "isr.h"
#include "atomic.h"
#include "types.h"
#include "misc.h"

/* GDT description info */
#define SEGMENT_LIMIT                                           0xFFFF
#define SEGMENT_LIMIT2                                          0xF
#define SEGMENT_LIMIT_ALL										0xFFFFF

#define SEGMENT_PRESENT_SET                                     0x80
#define SEGMENT_PRESENT_CLEAR                                   0x00
#define DESCRIPTOR_PRIVILEGE_LEVEL_0                            0x00
#define DESCRIPTOR_PRIVILEGE_LEVEL_1                            0x20
#define DESCRIPTOR_PRIVILEGE_LEVEL_2                            0x40
#define DESCRIPTOR_PRIVILEGE_LEVEL_3                            0x60
#define DESCRIPTOR_TYPE_SYS                                     0x00
#define DESCRIPTOR_TYPE_CODE_OR_DATA                            0x10

#define SEGMENT_TYPE_DATE_READ_ONLY                             0x0
#define SEGMENT_TYPE_DATE_READ_ONLY_ACCESSED                    0x1
#define SEGMENT_TYPE_DATE_READ_WRITE                            0x2
#define SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED                   0x3
#define SEGMENT_TYPE_DATE_READ_ONLY_EXP_DOWN                    0x4
#define SEGMENT_TYPE_DATE_READ_ONLY_EXP_DOWN_ACCESSED           0x5
#define SEGMENT_TYPE_DATE_READ_WRITE_EXP_DOWN                   0x6
#define SEGMENT_TYPE_DATE_READ_WRITE_EXP_DOWN_ACCESSED          0x7

#define SEGMENT_TYPE_CODE_EXE_ONLY                              0x8
#define SEGMENT_TYPE_CODE_EXE_ONLY_ACCESSED                     0x9
#define SEGMENT_TYPE_CODE_EXE_READ                              0xA
#define SEGMENT_TYPE_CODE_EXE_READ_ACCESSED                     0xB
#define SEGMENT_TYPE_CODE_EXE_ONLY_CONFORMING                   0xC
#define SEGMENT_TYPE_CODE_EXE_ONLY_CONFORMING_ACCESSED          0xD
#define SEGMENT_TYPE_CODE_EXE_READ_CONFORMING                   0xE
#define SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED     0xF

#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_LDT               0x2
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TASKGATE          0x5
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_CALLGATE          0x0C
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TSSG              0xB

#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_INTERGATE         0xE
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TRAPGATE          0xF

#define GRANULARITY_SET                                         0x80
#define GRANULARITY_CLEAR                                       0x00
#define DEFAULT_OPERATION_SIZE_16BIT_SEGMENT                    0x00
#define DEFAULT_OPERATION_SIZE_32BIT_SEGMENT                    0x40
#define L_64_BIT_CODE_SEGMENT                                   0x20
#define AVL0                                                    0x0

#define UNUSED                          0
#define IST_TABLE_INDEX                 0

/*index:0, RPL:0 GDT*/
#define SELECTOR_INDEX_0H				0x0

/*index:0, RPL:2 GDT*/
#define SELECTOR_INDEX_2H				0x2

/*index:0x1, RPL:0 GDT*/
#define SELECTOR_INDEX_8H				0x8

/*index:0x2, RPL:0 GDT*/
#define SELECTOR_INDEX_10H				0x10

/*index:0x7, RPL:0 GDT*/
#define SELECTOR_INDEX_38H				0x38

/*index:0x30, RPL:0 GDT*/
#define SELECTOR_INDEX_180H				0x180

/*index:0x30, RPL:3 GDT*/
#define SELECTOR_INDEX_183H				0x183

/*index:0x31, RPL:0 GDT*/
#define SELECTOR_INDEX_188H				0x188

/*index:0x33, RPL:0 GDT*/
#define SELECTOR_INDEX_198H				0x198

/*index:0x40, RPL:0 GDT*/
#define SELECTOR_INDEX_200H				0x200

/*index:0x50, RPL:0 GDT*/
#define SELECTOR_INDEX_280H				0x280

/*index:0x50, RPL:1 GDT*/
#define SELECTOR_INDEX_281H				0x281

/*index:0x50, RPL:3 GDT*/
#define SELECTOR_INDEX_283H				0x283

/*index:0x400, RPL:0 GDT*/
#define SELECTOR_INDEX_2000H			0x2000


#define CR4_FSGSBASE (1UL << 16U)

typedef enum {
	REG_CS = 0,
	REG_DS,
	REG_ES,
	REG_FS,
	REG_GS,
	REG_SS,
	REG_CR4,
} DUMPED_REG;

typedef enum {
	E_IA32_SYSENTER_CS,
	E_IA32_SYSENTER_ESP,
	E_IA32_SYSENTER_EIP,

	E_IA32_EFER,
	E_IA32_STAR,
	E_IA32_LSTAR,
	E_IA32_CSTAR,
	E_IA32_FMASK,
	E_IA32_FS_BASE,
	E_IA32_GS_BASE,
	E_IA32_KERNEL_GS_BASE,
} DUMPED_MSR;

/* for GDTR in protect mode */
struct gdtr32_t {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

#ifndef __x86_64__
struct gate_descriptor {
	unsigned gd_looffset:16;	/* gate offset (lsb) */
	unsigned gd_selector:16;	/* gate segment selector */
	unsigned gd_stkcpy:5;		/* number of stack wds to cpy */
	unsigned gd_xx:3;			/* unused */
	unsigned gd_type:5;			/* segment type */
	unsigned gd_dpl:2;			/* segment descriptor priority level */
	unsigned gd_p:1;			/* segment descriptor present */
	unsigned gd_hioffset:16;	/* gate offset (msb) */
} __attribute__((packed));
#else //__x86_64__
struct gate_descriptor {
	uint64_t gd_looffset:16;	/* gate offset (lsb) */
	uint64_t gd_selector:16;	/* gate segment selector */
	uint64_t gd_ist:3;			/* IST table index */
	uint64_t gd_xx:5;			/* unused */
	uint64_t gd_type:5;			/* segment type */
	uint64_t gd_dpl:2;			/* segment descriptor priority level */
	uint64_t gd_p:1;			/* segment descriptor present */
	uint64_t gd_hioffset:48;	/* gate offset (msb) */
	uint64_t sd_xx1:32;
} __attribute__((packed));
#endif

#endif
