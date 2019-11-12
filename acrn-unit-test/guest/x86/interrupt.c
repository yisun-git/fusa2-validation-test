/*
 * Test for x86 interrupt & exception handling
 *
 * Copyright (c) 2015 intel
 *
 * Authors:
 *  Jiang Yong <sevekwl@qq.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */
#include "alloc.h"
#include "libcflat.h"
#include "desc.h"
#include "apic-defs.h"
#include "apic.h"
#include "processor.h"
#include "vm.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "isr.h"
#include "atomic.h"
#include "types.h"

#ifdef USED
#udef USED
#endif

#ifndef SECTION
#undef SECTION
#endif

#ifdef WEAK
#undef WEAK
#endif

#define WEAK 				__attribute__((weak))
#define USED				__attribute__((used))
#define SECTION(x)			__attribute__((section(x)))

#define __TESTCASE_DEBUG__
#ifdef __TESTCASE_DEBUG__
#define DEBUG(fmt, args...)     printf(fmt, ## args)
#define INFOR(fmt, args...)     printf(fmt, ## args)
#define ERROR(fmt, args...)     printf(fmt, ## args)

#define IRQ_DEBUG(fmt, args...) printf(fmt, ## args)
#define IRQ_INFOR(fmt, args...) printf(fmt, ## args)
#define IRQ_ERROR(fmt, args...) printf(fmt, ## args)

#ifdef ASSERT
#undef ASSERT
#endif

#define ASSERT(cond, fmt, args...)						\
	do {									\
		if (!(cond)) {							\
			printf("[%04d]%s(%s): " fmt "\n",			\
			(int)__LINE__, __FILE__, #cond, ## args);		\
		}								\
	} while (0)

#else
#define DEBUG(fmt, args...)
#define INFOR(fmt, args...)
#define ERROR(fmt, args...) 

#define IRQ_DEBUG(fmt, args...)
#define IRQ_INFOR(fmt, args...)
#define IRQ_ERROR(fmt, args...)
#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(cond, fmt, args...)
#endif

#define RESULT_OK		0
#define RESULT_TIMEOUT		-1
#define RESULT_OUTRANGE		-2
#define RESULT_NOMEM		-3
#define RESULT_FAULT		-4
#define RESULT_INVALID_OP	-5
#define RESULT_BUG		-6

#define ASM_TRIGGER_DE()	asm volatile(				\
					"push" W " %" R "ax\n"		\
					"push" W " %" R "bx\n"		\
					"push" W " %" R "dx\n"		\
					"mov" W " $1, %" R "ax\n"	\
					"mov" W " $0, %" R "bx\n"	\
					"div" W " %" R "bx\n"		\
					"pop" W " %" R "dx\n"		\
					"pop" W " %" R "bx\n"		\
					"pop" W " %" R "ax\n")
#define ASM_TRIGGER_INT0x80()	asm volatile("int $0x80\n")
#define ASM_TRIGGER_UD()	asm volatile("ud2\n")

/**
 * See SDM Vol 3 - 3.4.2 Segment Selectors
 */
#define SHIFT_SEL_RPL		0
#define SHIFT_SEL_TI		2
#define SHIFT_SEL_INDEX		3

#define MASK_SEL_RPL		(3u << SHIFT_SEL_RPL)
#define MAKE_SEL_RPL(rpl)	(((rpl) << SHIFT_SEL_RPL) & MASK_SEL_RPL)
#define TAKE_SEL_RPL(sel)	(((sel) & MASK_SEL_RPL) >> SHIFT_SEL_RPL)

#define MASK_SEL_TI		(1u << SHIFT_SEL_TI)
#define MAKE_SEL_TI(ti)		(((ti) << SHIFT_SEL_TI) & MASK_SEL_TI)
#define TAKE_SEL_TI(sel)	(((sel) & MASK_SEL_TI) >> SHIFT_SEL_TI)

#define MASK_SEL_INDEX		(0x1FFFu << SHIFT_SEL_INDEX)
#define MAKE_SEL_INDEX(idx)	(((idx) << SHIFT_SEL_INDEX) & MASK_SEL_INDEX)
#define TAKE_SEL_INDEX(sel)	(((sel) & MASK_SEL_INDEX) >> SHIFT_SEL_INDEX)

#define MAKE_SEL(i, t, r)	(MAKE_SEL_INDEX(i) | MAKE_SEL_TI(t) | MAKE_SEL_RPL(r))

/**
 * GDT Descriptor
 */

/**
 * See SDM Vol 3 - 3.4.5.1 Code- and Data-Segment Descriptor Types
 */
#define SEGCD_T_DR		0ul		/**< Data, Read, not access */
#define SEGCD_T_DRA		1ul		/**< Data, Read, accessed */
#define SEGCD_T_DW		2ul		/**< Data, Write, not access */
#define SEGCD_T_DWA		3ul		/**< Data, Write, accessed */
#define SEGCD_T_DER		4ul		/**< Data, Read, expand-down, not access */
#define SEGCD_T_DERA		5ul		/**< Data, Read, expand-down, accessed */
#define SEGCD_T_DEW		6ul		/**< Data, Write, expand-down, not access */
#define SEGCD_T_DEWA		7ul		/**< Data, Write, expand-down, accessed */
#define SEGCD_T_CO		8ul		/**< Code, Execute-Only not access */
#define SEGCD_T_COA		9ul		/**< Code, Execute-Only, accessed*/
#define SEGCD_T_CR		10ul		/**< Code, Execute, Read, not access */
#define SEGCD_T_CRA		11ul		/**< Code, Execute, Read, accessed */
#define SEGCD_T_COCA		13ul		/**< Code, Execute only, conforming, accessed */
#define SEGCD_T_COC		12ul		/**< Code, Execute only, conforming, not access */
#define SEGCD_T_CRC		14ul		/**< Code, Execute, Read, conforming, not access */
#define SEGCD_T_CRCA		15ul		/**< Code, Execute, Read conforming, accessed */

/**
 * See SDM Vol 3 - 3.5 SYSTEM DESCRIPTOR TYPES
 * See SDM Vol 3 - Table 3-2. System-Segment and Gate-Descriptor Types
 */
#define SEG32_T_0		0		/**< Reserved */
#define SEG32_T_TSS16_A		1		/**< 16-bit TSS (Available) */
#define SEG32_T_LDT		2		/**< LDT */
#define SEG32_T_TSS16_B		3		/**< 16-bit TSS (Busy) */
#define SEG32_T_CALL16		4		/**< 16-bit Call Gate */
#define SEG32_T_TASK		5		/**< Task Gate */
#define SEG32_T_INT16		6		/**< 16-bit Interrupt Gate */
#define SEG32_T_TRAP16		7		/**< 16-bit bit Trap Gate */
#define SEG32_T_8		8		/**< Reserved */
#define SEG32_T_TSS32_A		9		/**< 32-bit TSS (Available) */
#define SEG32_T_10		10		/**< Reserved */
#define SEG32_T_TSS32_B		11		/**< 32-bit TSS (Busy) */
#define SEG32_T_CALL32		12		/**< 32-bit Call Gate */
#define SEG32_T_13		13		/**< Reserved */
#define SEG32_T_INT32		14		/**< 32-bit Interrupt Gate */
#define SEG32_T_TRAP32		15		/**< 32-bit Trap Gate */

#define SEG64_T_0		0		/**< Reserved */
#define SEG64_T_1	  	1		/**< Reserved */
#define SEG64_T_LDT		2		/**< LDT */
#define SEG64_T_3		3		/**< Reserved */
#define SEG64_T_4		4		/**< Reserved */
#define SEG64_T_5		5		/**< Reserved */
#define SEG64_T_6		6		/**< Reserved */
#define SEG64_T_7		7		/**< Reserved */
#define SEG64_T_8		8		/**< Reserved */
#define SEG64_T_TSS64_A		9		/**< 64-bit TSS (Available) */
#define SEG64_T_10		10		/**< Reserved */
#define SEG64_T_TSS64_B		11		/**< 64-bit TSS (Busy) */
#define SEG64_T_CALL64		12		/**< 64-bit Call Gate */
#define SEG64_T_13		13		/**< Reserved */
#define SEG64_T_INT64		14		/**< 64-bit Interrupt Gate */
#define SEG64_T_TRAP64		15		/**< 64-bit Trap Gate */

#ifdef __x86_64__
#define SEG_T_TSS		SEG64_T_TSS64_A
#define SEG_T_INT		SEG64_T_INT64
#define SEG_T_TRAP		SEG64_T_TRAP64
#else
#define SEG_T_TSS		SEG32_T_TSS32_A
#define SEG_T_INT		SEG32_T_INT32
#define SEG_T_TRAP		SEG32_T_TRAP32
#endif

/* See SDM Vol 3 - 3.4.5 Segment Descriptors */
#define SHIFT_GDT_BASE2		56		/** B56~B63 : BASE2 */
#define SHIFT_GDT_G		55
#define SHIFT_GDT_DB		54
#define SHIFT_GDT_L		53
#define SHIFT_GDT_AVL		52
#define SHIFT_GDT_LIMIT1	48		/** B48~B51 : LIMIT1 */
#define SHIFT_GDT_P		47
#define SHIFT_GDT_DPL		45		/** B45~B46 : DPL */
#define SHIFT_GDT_S		44
#define SHIFT_GDT_TYPE		40		/** B40~B43 : TYPE */
#define SHIFT_GDT_BASE1		32
#define SHIFT_GDT_BASE0		16
#define SHIFT_GDT_LIMIT0	0

#define MAKE_GDT_MASK(v, s)	(((gdt_descriptor_t) (v)) << (s))
#define TAKE_GDT(e, m, r, l)	(((e) & (m)) >> ((r) - (l)))
#define MAKE_GDT(v, m, l, r)	((((gdt_descriptor_t) (v)) << ((l) - (r))) & (m))

#define MASK_GDT_DPL		MAKE_GDT_MASK(0x3, SHIFT_GDT_DPL)
#define TAKE_GDT_DPL(e)		TAKE_GDT(e, MASK_GDT_DPL, SHIFT_GDT_DPL, 0)
#define MAKE_GDT_DPL(v)		MAKE_GDT(v, MASK_GDT_DPL, SHIFT_GDT_DPL, 0)

#define MASK_GDT_TYPE		MAKE_GDT_MASK(0xF, SHIFT_GDT_TYPE)
#define TAKE_GDT_TYPE(e)	TAKE_GDT(e, MASK_GDT_TYPE, SHIFT_GDT_TYPE, 0)
#define MAKE_GDT_TYPE(v)	MAKE_GDT(v, MASK_GDT_TYPE, SHIFT_GDT_TYPE, 0)

#define MASK_GDT_LIMIT0		MAKE_GDT_MASK(0xFFFF, SHIFT_GDT_LIMIT0)
#define MASK_GDT_LIMIT1		MAKE_GDT_MASK(0xF, SHIFT_GDT_LIMIT1)
#define MASK_GDT_LIMIT		(MASK_GDT_LIMIT0 | MASK_GDT_LIMIT1)
#define TAKE_GDT_LIMIT0(e)	TAKE_GDT(e, MASK_GDT_LIMIT0, SHIFT_GDT_LIMIT0, 0)
#define TAKE_GDT_LIMIT1(e)	TAKE_GDT(e, MASK_GDT_LIMIT1, SHIFT_GDT_LIMIT1, 16)
#define TAKE_GDT_LIMIT(e)	(TAKE_GDT_LIMIT0(e) | TAKE_GDT_LIMIT1(e))
#define MAKE_GDT_LIMIT0(v)	MAKE_GDT(v, MASK_GDT_LIMIT0, SHIFT_GDT_LIMIT0, 0)
#define MAKE_GDT_LIMIT1(v)	MAKE_GDT(v, MASK_GDT_LIMIT1, SHIFT_GDT_LIMIT1, 16)
#define MAKE_GDT_LIMIT(v)	(MAKE_GDT_LIMIT0(v) | MAKE_GDT_LIMIT1(v))

#define MASK_GDT_BASE0		MAKE_GDT_MASK(0xFFFF, SHIFT_GDT_BASE0)
#define MASK_GDT_BASE1		MAKE_GDT_MASK(0xFF, SHIFT_GDT_BASE1)
#define MASK_GDT_BASE2		MAKE_GDT_MASK(0xFF, SHIFT_GDT_BASE2)
#define MASK_GDT_BASE		(MASK_GDT_BASE0 | MASK_GDT_BASE1 | MASK_GDT_BASE2)
#define TAKE_GDT_BASE0(e)	TAKE_GDT(e, MASK_GDT_BASE0, SHIFT_GDT_BASE0, 0)
#define TAKE_GDT_BASE1(e)	TAKE_GDT(e, MASK_GDT_BASE1, SHIFT_GDT_BASE1, 16)
#define TAKE_GDT_BASE2(e)	TAKE_GDT(e, MASK_GDT_BASE2, SHIFT_GDT_BASE2, 24)
#define TAKE_GDT_BASE(e)	(TAKE_GDT_BASE0(e) | TAKE_GDT_BASE1(e) | TAKE_GDT_BASE2(e))
#define MAKE_GDT_BASE0(v)	MAKE_GDT(v, MASK_GDT_BASE0, SHIFT_GDT_BASE0, 0)
#define MAKE_GDT_BASE1(v)	MAKE_GDT(v, MASK_GDT_BASE1, SHIFT_GDT_BASE1, 16)
#define MAKE_GDT_BASE2(v)	MAKE_GDT(v, MASK_GDT_BASE2, SHIFT_GDT_BASE2, 24)
#define MAKE_GDT_BASE(v)	(MAKE_GDT_BASE0(v) | MAKE_GDT_BASE1(v) | MAKE_GDT_BASE2(v))

#define GDT_GET_S(e, s)		gdt_get_bit(e, s, SHIFT_GDT_S)
#define GDT_GET_P(e, s)		gdt_get_bit(e, s, SHIFT_GDT_P)
#define GDT_GET_G(e, s)		gdt_get_bit(e, s, SHIFT_GDT_G)
#define GDT_GET_AVL(e, s)	gdt_get_bit(e, s, SHIFT_GDT_AVL)
#define GDT_GET_DB(e, s)	gdt_get_bit(e, s, SHIFT_GDT_DB)
#define GDT_GET_L(e, s)		gdt_get_bit(e, s, SHIFT_GDT_L)
#define GDT_GET_DPL(e, s)	gdt_get_dpl(e, s)
#define GDT_GET_BASE(e, s)	gdt_get_base(e, s)
#define GDT_GET_TYPE(e, s)	gdt_get_type(e, s)
#define GDT_GET_LIMIT(e, s)	gdt_get_limit(e, s)

#define GDT_SET_S(e, s, v)	gdt_set_bit(e, s, v, SHIFT_GDT_S)
#define GDT_SET_P(e, s, v)	gdt_set_bit(e, s, v, SHIFT_GDT_P)
#define GDT_SET_G(e, s, v)	gdt_set_bit(e, s, v, SHIFT_GDT_G)
#define GDT_SET_AVL(e, s, v)	gdt_set_bit(e, s, v, SHIFT_GDT_AVL)
#define GDT_SET_DB(e, s, v)	gdt_set_bit(e, s, v, SHIFT_GDT_DB)
#define GDT_SET_L(e, s, v)	gdt_set_bit(e, s, v, SHIFT_GDT_L)
#define GDT_SET_DPL(e, s, d)	gdt_set_dpl(e, s, d)
#define GDT_SET_BASE(e, s, b)	gdt_set_base(e, s, b)
#define GDT_SET_TYPE(e, s, t)	gdt_set_type(e, s, t)
#define GDT_SET_LIMIT(e, s, l)	gdt_set_limit(e, s, l)

typedef unsigned long vir_addr_t;
typedef u64 gdt_descriptor_t;

static inline unsigned int gdt_get_bit(gdt_descriptor_t *gdt,
				       unsigned int sel,
				       unsigned int shift)
{
	unsigned int index = TAKE_SEL_INDEX(sel);
	return ((gdt[index] >> shift) & 1);
}

static inline void gdt_set_bit(gdt_descriptor_t *gdt,
				  unsigned int sel,
				  bool value,
				  unsigned int shift)
{
	unsigned int index = TAKE_SEL_INDEX(sel);

	if (value) {
		gdt[index] |= (1ull<<shift);
	}
	else {
		gdt[index] &= (~(1ull<<shift));
	}
}

static inline void gdt_set_base(gdt_descriptor_t *gdt,
				unsigned int sel,
				uint32_t base)
{
	unsigned int index = TAKE_SEL_INDEX(sel);

	gdt[index] &= (~MASK_GDT_BASE);
	gdt[index] |= MAKE_GDT_BASE(base);
}

static inline uint32_t gdt_get_base(gdt_descriptor_t *gdt,
				    unsigned int sel)
{
	unsigned int index = TAKE_SEL_INDEX(sel);
	return (uint32_t)TAKE_GDT_BASE(gdt[index]);
}

static inline void gdt_set_type(gdt_descriptor_t *gdt,
				unsigned int sel,
				unsigned int type)
{
	unsigned int index = TAKE_SEL_INDEX(sel);
	gdt[index] &= (~MASK_GDT_TYPE);
	gdt[index] |= MAKE_GDT_TYPE(type);
}

static inline uint32_t gdt_get_type(gdt_descriptor_t *gdt,
				    unsigned int sel)
{
	unsigned int index = TAKE_SEL_INDEX(sel);
	return TAKE_GDT_TYPE(gdt[index]);
}

static inline void gdt_set_limit(gdt_descriptor_t *gdt,
				 unsigned int sel,
				 unsigned int limit)
{
	unsigned int index = TAKE_SEL_INDEX(sel);
	gdt[index] &= (~ MASK_GDT_LIMIT);
	gdt[index] |= MAKE_GDT_LIMIT(limit);
}

static inline uint32_t gdt_get_limit(gdt_descriptor_t *gdt,
					 unsigned int sel,
					 unsigned int limit)
{
	unsigned int index = TAKE_SEL_INDEX(sel);
	return TAKE_GDT_LIMIT(gdt[index]);
}

static inline void gdt_set_dpl(gdt_descriptor_t *gdt,
			       unsigned int sel,
			       unsigned int dpl)
{
	unsigned int index = TAKE_SEL_INDEX(sel);
	gdt[index] &= (~ MASK_GDT_DPL);
	gdt[index] |= MAKE_GDT_DPL(dpl);
}

static inline uint32_t gdt_get_dpl(gdt_descriptor_t *gdt,
				   unsigned int sel)
{
	unsigned int index = TAKE_SEL_INDEX(sel);
	return TAKE_GDT_DPL(gdt[index]);
}

#define	BASE0		0
#define LIMIT0		0
#define AVL0		0
#define AVL1		1
#define L0		0
#define L1		1
#define DB0		0
#define DB1		1
#define G0		0
#define G1		1

#define P0		0
#define P1		1
#define S0		0
#define S1		1
#define DPL0		0
#define DPL1		1
#define DPL2		2
#define DPL3		3

#define PREPARE_SEGMENT(sel, base, limit, type, s, dpl, p, avl, l, db, g)			\
	construct_segment_descriptor(sel, base, limit, type, s, dpl, p, avl, l, db, g)

#ifdef __x86_64__
#define PREPARE_DEFAULT_SEGMENT(sel, type, s, dpl, p, db, g)					\
	PREPARE_SEGMENT(sel,			/* selector */					\
			BASE0,			/* base */					\
			0xFFFFF,		/* limit */					\
			type,			/* type */					\
			s,			/* s */						\
			dpl,			/* dpl */					\
			p,			/* p */						\
			AVL0,			/* avl */					\
			L1,			/* l */						\
			db,			/* db */					\
			g)			/* g */

#define PREPARE_CODE_SEGMENT(sel, dpl, p, db, g)						\
	PREPARE_SEGMENT(sel,			/* selector */					\
			BASE0,			/* base */					\
			0xFFFFF,		/* limit */					\
			SEGCD_T_CO,		/* type */					\
			S1,			/* s */						\
			dpl,			/* dpl */					\
			p,			/* p */						\
			AVL0,			/* avl */					\
			L1,			/* l */						\
			db,			/* db */					\
			g)			/* g */
#define PREPARE_KERNEL_CODE_SEGMENT(sel, p)							\
	PREPARE_CODE_SEGMENT(sel, DPL0, p, DB0, G1)
#define PREPARE_USER_CODE_SEGMENT(sel, p)							\
	PREPARE_CODE_SEGMENT(sel, DPL3, p, DB0, G1)
#else
#define PREPARE_DEFAULT_SEGMENT(sel, type, s, dpl, p, db, g)					\
	PREPARE_SEGMENT(sel,			/* selector */					\
			BASE0,			/* base */					\
			0xFFFFF,		/* limit */					\
			type,			/* type */					\
			s,			/* s */						\
			dpl,			/* dpl */					\
			p,			/* p */						\
			AVL0,			/* avl */					\
			L0,			/* l */						\
			db,			/* db */					\
			g)			/* g */
#define PREPARE_TSS_SEGMENT(sel, s, dpl, p, db, g)						\
	PREPARE_SEGMENT(sel,			/* selector */					\
			(ulong)&tss_intr,	/* base */					\
			sizeof(tss_intr) - 1,	/* limit */					\
			SEG_T_TSS,		/* type */					\
			s,			/* s */						\
			dpl,			/* dpl */					\
			p,			/* p */						\
			AVL0,			/* avl */					\
			L0,			/* l */						\
			db,			/* db */					\
			g)			/* g */
#endif

static void construct_segment_descriptor(unsigned int sel,
					 uint32_t base, uint32_t limit, uint16_t type,
					 bool s, uint16_t dpl, bool p,
					 bool avl, bool l, bool db, bool g)
{
	struct descriptor_table_ptr gdtr;
	gdt_descriptor_t *gdt;

	sgdt(&gdtr);

	gdt = (gdt_descriptor_t*)gdtr.base;

	ASSERT(TAKE_SEL_INDEX(sel) < (gdtr.limit + 1) / sizeof(gdt_descriptor_t),
		"BUG: sel %u out of range %u.\n",
		sel, (unsigned int)((gdtr.limit + 1) / sizeof(gdt_descriptor_t)));

	GDT_SET_BASE(gdt, sel, base);
	GDT_SET_LIMIT(gdt, sel, limit);
	GDT_SET_TYPE(gdt, sel, type);
	GDT_SET_S(gdt, sel, s);
	GDT_SET_DPL(gdt, sel, dpl);
	GDT_SET_P(gdt, sel, p);
	GDT_SET_AVL(gdt, sel, avl);
	GDT_SET_L(gdt, sel, l);
	GDT_SET_DB(gdt, sel, db);
	GDT_SET_G(gdt, sel, g);
}

#define PREPARE_NEWGDT(oldgdtr, newgdtr)	prepare_newgdt(oldgdtr, newgdtr)
#define RECOVERY_OLDGDT(oldgdtr, newgdtr)	recovery_oldgdt(oldgdtr, newgdtr)

static void * prepare_newgdt(struct descriptor_table_ptr *oldgdtr,
			     struct descriptor_table_ptr *newgdtr)
{
	const size_t new_size = PAGE_SIZE *2;
	
	sgdt(oldgdtr);

	newgdtr->base = (ulong)malloc(new_size);
	newgdtr->limit = new_size - 1;

	memset((void*)newgdtr->base, 0, new_size);
	memcpy((void*)newgdtr->base, (void*)oldgdtr->base, oldgdtr->limit + 1);
	memcpy((void*)(newgdtr->base + PAGE_SIZE), (void*)oldgdtr->base, oldgdtr->limit + 1);

	lgdt(newgdtr);

	return (void*)newgdtr->base;
}

static inline void recovery_oldgdt(struct descriptor_table_ptr *oldgdtr,
				   struct descriptor_table_ptr *newgdtr)
{
	lgdt(oldgdtr);
	free((void*)newgdtr->base);
}

/*
 * Test for x86 interrupt & exception handling
 *
 * Copyright (c) 2015 intel
 * File   :  idt.h
 * Author :  Jiang Yong
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

/* See SDM Vol 3 - 6.14.1 Interrupt Descriptors */
#ifndef __INTERRUPT_LIB_IDT_H__
#define __INTERRUPT_LIB_IDT_H__
#include "desc.h"
#include "types.h"

#define SHIFT_IDT_OFFSET2	64		/** B64~B95 : Offset 32~63*/
#define SHIFT_IDT_OFFSET1	48		/** B48~B63 : Offset Offset 16~31*/
#define SHIFT_IDT_P		47
#define SHIFT_IDT_DPL		45		/** B45~B46 : DPL */
#define SHIFT_IDT_S		44
#define SHIFT_IDT_TYPE		40		/** B40~B43 : TYPE */
#define SHIFT_IDT_SEL		16
#define SHIFT_IDT_OFFSET0	0		/** B00~B15 : Offset 00~15 */

#define MAKE_IDT_MASK(v, s)	(((idt_descriptor_t) (v)) << (s))
#define TAKE_IDT(e, m, r, l)	(((e) & (m)) >> ((r) - (l)))
#define MAKE_IDT(v, m, l, r)	((((idt_descriptor_t) (v)) << ((l) - (r))) & (m))

#define MASK_IDT_DPL		MAKE_IDT_MASK(0x3, SHIFT_IDT_DPL)
#define TAKE_IDT_DPL(e)		TAKE_GDT(e, MASK_IDT_DPL, SHIFT_IDT_DPL, 0)
#define MAKE_IDT_DPL(v)		MAKE_GDT(v, MASK_IDT_DPL, SHIFT_IDT_DPL, 0)

#define MASK_IDT_TYPE		MAKE_IDT_MASK(0xF, SHIFT_IDT_TYPE)
#define TAKE_IDT_TYPE(e)	TAKE_IDT(e, MASK_IDT_TYPE, SHIFT_IDT_TYPE, 0)
#define MAKE_IDT_TYPE(v)	MAKE_IDT(v, MASK_IDT_TYPE, SHIFT_IDT_TYPE, 0)

#define MASK_IDT_SEL		MAKE_IDT_MASK(0xFFFF, SHIFT_IDT_SEL)
#define TAKE_IDT_SEL(e)		TAKE_IDT(e, MASK_IDT_SEL, SHIFT_IDT_SEL, 0)
#define MAKE_IDT_SEL(v)		MAKE_IDT(v, MASK_IDT_SEL, SHIFT_IDT_SEL, 0)

#define MASK_IDT_OFFSET0	MAKE_IDT_MASK(0xFFFF, SHIFT_IDT_OFFSET0)
#define MASK_IDT_OFFSET1	MAKE_IDT_MASK(0xFFFF, SHIFT_IDT_OFFSET1)

#ifdef __x86_64__
#define MASK_IDT_OFFSET2	MAKE_IDT_MASK(0xFFFFFFFF, SHIFT_IDT_OFFSET2)
#define MASK_IDT_OFFSET		(MASK_IDT_OFFSET0 | MASK_IDT_OFFSET1 | MASK_IDT_OFFSET2)
#else
#define MASK_IDT_OFFSET		(MASK_IDT_OFFSET0 | MASK_IDT_OFFSET1)
#endif

#define TAKE_IDT_OFFSET0(e)	TAKE_IDT(e, MASK_IDT_OFFSET0, SHIFT_IDT_OFFSET0, 0)
#define TAKE_IDT_OFFSET1(e)	TAKE_IDT(e, MASK_IDT_OFFSET1, SHIFT_IDT_OFFSET1, 16)
#ifdef __x86_64__
#define TAKE_IDT_OFFSET2(e)	TAKE_IDT(e, MASK_IDT_OFFSET2, SHIFT_IDT_OFFSET2, 32)
#define TAKE_IDT_OFFSET(e)	(TAKE_IDT_OFFSET0(e) | TAKE_IDT_OFFSET1(e) | TAKE_IDT_OFFSET2(e))
#else
#define TAKE_IDT_OFFSET(e)	(TAKE_IDT_OFFSET0(e) | TAKE_IDT_OFFSET1(e))
#endif

#define MAKE_IDT_OFFSET0(v)	MAKE_IDT(v, MASK_IDT_OFFSET0, SHIFT_IDT_OFFSET0, 0)
#define MAKE_IDT_OFFSET1(v)	MAKE_IDT(v, MASK_IDT_OFFSET1, SHIFT_IDT_OFFSET1, 16)
#ifdef __x86_64
#define MAKE_IDT_OFFSET2(v)	MAKE_IDT(v, MASK_IDT_OFFSET2, SHIFT_IDT_OFFSET2, 32)
#define MAKE_IDT_OFFSET(v)	(MAKE_IDT_OFFSET0(v) | MAKE_IDT_OFFSET1(v) | MAKE_IDT_OFFSET2(v))
#else
#define MAKE_IDT_OFFSET(v)	(MAKE_IDT_OFFSET0(v) | MAKE_IDT_OFFSET1(v))
#endif

#define IDT_SET_OFFSET(i, v, o)	idt_set_offset(i, v, o)
#define IDT_SET_SEL(i, v, s)	idt_set_selector(i, v, s)
#define IDT_SET_P(i, v, p)	idt_set_bit(i, v, p, SHIFT_IDT_P)
#define IDT_SET_DPL(i, v, dpl)	idt_set_dpl(i, v, dpl)
#define IDT_SET_S(i, v, s)	idt_set_bit(i, v, s, SHIFT_IDT_S)
#define IDT_SET_TYPE(i, v, t)	idt_set_type(i, v, t)

#define IDT_GET_OFFSET(i, v)	((void*)TAKE_IDT_OFFSET(idt[v]))
#define IDT_GET_SEL(i, v)	((uint16_t)TAKE_IDT_SEL(idt[v]))
#define IDT_GET_P(i, v)		((idt[v] & (1ul << SHIFT_IDT_P)) != 0)
#define IDT_GET_DPL(i, v)	((unsigned int)TAKE_IDT_DPL(idt[v]))
#define IDT_GET_S(i, v)		((idt[v] & (1ul << SHIFT_IDT_S)) != 0)
#define IDT_GET_TYPE(i, v)	((unsigned int)TAKE_IDT_TYPE(idt[v]))

#define PREPARE_INTERRUPT_GATE(vector, selector, handler, p, dpl, s)				\
	construct_interrupt_descriptor(								\
		vector,			/* Which entry in idt */				\
		selector,		/* selector */						\
		handler,		/* offset */						\
		p, 			/* P bit */						\
		dpl, 			/* DPL */						\
		s, 			/* S bit */						\
		SEG_T_INT 		/* TYPE */						\
		)

#define PREPARE_TRAP_GATE(vector, selector, handler, p, dpl, s)					\
	construct_interrupt_descriptor(								\
		vector,			/* Which entry in idt */				\
		selector,		/* selector */						\
		handler,		/* offset */						\
		p, 			/* P bit */						\
		dpl, 			/* DPL */						\
		s, 			/* S bit */						\
		SEG_T_TRAP 		/* TYPE */						\
		)
#define PREPARE_TASK_GATE(vector, selector, handler, p, dpl, s)					\
	construct_interrupt_descriptor(								\
		vector,			/* Which entry in idt */				\
		selector,		/* selector */						\
		handler,		/* offset */						\
		p, 			/* P bit */						\
		dpl, 			/* DPL */						\
		s, 			/* S bit */						\
		SEG32_T_TASK 		/* TYPE */						\
		)

#define RECOVERY_INTERRUPT_GATE(vector, selector, handler)					\
	construct_interrupt_descriptor(								\
		vector,			/* Which entry in idt */				\
		selector,		/* selector */						\
		handler,		/* offset */						\
		1, 			/* P bit */						\
		0, 			/* DPL */						\
		0, 			/* S bit */						\
		SEG_T_INT 		/* TYPE */						\
		)

#ifdef __x86_64__
typedef unsigned __int128 idt_descriptor_t;
#else
typedef u64 idt_descriptor_t;
#endif

typedef struct ex_regs irq_regs_t;
#define exp_regs_t irq_regs_t

#ifdef __cplusplus
	extern "c" {
#endif
void idt_set_offset(idt_descriptor_t *idt,
		    unsigned int vector,
		    void *offset);
void idt_set_selector(idt_descriptor_t *idt,
		      unsigned int vector,
		      unsigned int selector);
void idt_set_bit(idt_descriptor_t *idt,
		 unsigned int vector,
		 bool value,
		 unsigned int shift);
void idt_set_dpl(idt_descriptor_t *idt,
		 unsigned int vector,
		 unsigned int dpl);
void idt_set_type(idt_descriptor_t *idt,
		  unsigned int vector,
		  unsigned int type);
void construct_interrupt_descriptor(
	int vec, unsigned int selector, void *offset,
	bool p, int dpl, bool s, unsigned int type);
#ifdef __cplusplus
	}
#endif

#endif /* __INTERRUPT_LIB_IDT_H__ */

/*
 * Test for x86 interrupt & exception handling
 *
 * Copyright (c) 2015 intel
 * File   :  idt.c
 * Author :  Jiang Yong
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

void idt_set_offset(idt_descriptor_t *idt,
		    unsigned int vector,
		    void *offset)
{
	idt[vector] &= (~MASK_IDT_OFFSET);
	idt[vector] |= MAKE_IDT_OFFSET((vir_addr_t)offset);
}

void idt_set_selector(idt_descriptor_t *idt,
		      unsigned int vector,
		      unsigned int selector)
{
	idt[vector] &= (~MASK_IDT_SEL);
	idt[vector] |= MAKE_IDT_SEL(selector);
}


void idt_set_bit(idt_descriptor_t *idt,
		 unsigned int vector,
		 bool value,
		 unsigned int shift)
{
	if (value) {
		idt[vector] |= (((idt_descriptor_t)1)<<shift);
	}
	else {
		idt[vector] &= (~(((idt_descriptor_t)1)<<shift));
	}
}

void idt_set_dpl(idt_descriptor_t *idt,
		 unsigned int vector,
		 unsigned int dpl)
{
	idt[vector] &= (~MASK_IDT_DPL);
	idt[vector] |= MAKE_IDT_DPL(dpl);
}

void idt_set_type(idt_descriptor_t *idt,
		  unsigned int vector,
		  unsigned int type)
{
	idt[vector] &= (~MASK_IDT_TYPE);
	idt[vector] |= MAKE_IDT_TYPE(type);
}

void construct_interrupt_descriptor(
	int vec, unsigned int selector, void *offset,
	bool p, int dpl, bool s, unsigned int type)
{
	struct descriptor_table_ptr idtr;
	idt_descriptor_t *idtp;

	sidt(&idtr);
	idtp = (idt_descriptor_t*)idtr.base;
	memset(idtp + vec, 0, sizeof(idt_descriptor_t));

	IDT_SET_SEL(idtp, vec, selector);
	IDT_SET_OFFSET(idtp, vec, offset);
	IDT_SET_P(idtp, vec, p);
	IDT_SET_DPL(idtp, vec, dpl);
	IDT_SET_S(idtp, vec, s);
	IDT_SET_TYPE(idtp, vec, type);
}

/** DEBUG */
#define DEBUG_GDT_SHOW_COUNT(c)			debug_gdt_show_count(c)
#define DEBUG_GDT_SHOW_ENTRY(i)			debug_gdt_show_entry(i)
#define DEBUG_GDT_SHOW_FIELDS()			debug_gdt_show_fields()
#define DEBUG_IDT_SHOW_COUNT(c)			debug_idt_show_count(c)
#define DEBUG_IDT_SHOW_FIELDS()			debug_idt_show_fields()
#define DEBUG_IDT_SHOW_ENTRY(i)			debug_idt_show_entry(i)
#define DEBUG_TSS_SHOW_ENTRY(tss)		debug_tss_show_entry(tss)

struct segment_desc {
	uint16_t limit1;
	uint16_t base1;
	uint8_t  base2;
	union {
		uint16_t  type_limit_flags;      /* Type and limit flags */
		struct {
			uint16_t type:4;
			uint16_t s:1;
			uint16_t dpl:2;
			uint16_t p:1;
			uint16_t limit:4;
			uint16_t avl:1;
			uint16_t l:1;
			uint16_t db:1;
			uint16_t g:1;
		} __attribute__((__packed__));
	} __attribute__((__packed__));
	uint8_t  base3;
} __attribute__((__packed__));

struct interrupt_desc{
    unsigned short offset0;
    unsigned short selector;
    unsigned short ist : 3;
    unsigned short reserved0: 5;
    unsigned short type : 4;
    unsigned short s: 1;
    unsigned short dpl : 2;
    unsigned short p : 1;
    unsigned short offset1;
#ifdef __x86_64__
    unsigned offset2;
    unsigned reserved1;
#endif
} ;

static inline void debug_gdt_show_fields(void)
{
#ifdef __TESTCASE_DEBUG__
    DEBUG("  %s ", "id");
    DEBUG("%7s%s%7s", " ", "base", " ");
    DEBUG("%2s%s%2s", " ", "limit", " ");
    DEBUG("%s", "type");
    DEBUG("  %s ", "s");
    DEBUG("%s", "dpl");
    DEBUG(" %s ", "p");
    DEBUG("%s", "avl");
    DEBUG(" %s ", "l");
    DEBUG("%s ", "db");
    DEBUG(" %s \n", "g");
#endif
}

static inline void debug_gdt_show_entry(unsigned int sel)
{
	#ifdef __TESTCASE_DEBUG__
	struct descriptor_table_ptr gdtr;
	struct segment_desc *gdt;
	gdt_descriptor_t *gdtl;
	unsigned long base;
	unsigned int limit;
	unsigned int i;

	sgdt(&gdtr);
	gdt   = (struct segment_desc*)gdtr.base;
	gdtl  = (gdt_descriptor_t*)gdtr.base;
	i = (sel >> 3);

	base  = (gdt[i].base1 << 0)
	      + (gdt[i].base2 << 16)
	      + (gdt[i].base3 << 24);
	limit = (gdt[i].limit1 << 0)
	      + (gdt[i].limit << 16);

	DEBUG(" %03u ", i);
	DEBUG(" %016lx ", base);
	DEBUG(" %08x ", limit);
	DEBUG(" %2u ", (unsigned int)gdt[i].type);
	DEBUG(" %u ", (unsigned int)gdt[i].s);
	DEBUG(" %u ", (unsigned int)gdt[i].dpl);
	DEBUG(" %u ", (unsigned int)gdt[i].p);
	DEBUG(" %u ", (unsigned int)gdt[i].avl);
	DEBUG(" %u ", (unsigned int)gdt[i].l);
	DEBUG(" %u ", (unsigned int)gdt[i].db);
	DEBUG(" %u ", (unsigned int)gdt[i].g);
#ifdef __x86_64__
	DEBUG("0x%016lx\n", gdtl[i]);
#else
	DEBUG("0x%016llx\n", gdtl[i]);
#endif
#endif
}

static inline void debug_idt_show_fields(void)
{
#ifdef __TESTCASE_DEBUG__
	DEBUG("  %s ", "ID");
	DEBUG("%s%s%s", " ", "SELR", " ");
	DEBUG("%6s%s%6s", " ", "OFFSET", " ");
	DEBUG(" %s ", "TYPE");
	DEBUG(" %s ", "S");
	DEBUG("%s", "DPL");
	DEBUG("  %s \n", "P");
#endif
}

static inline void debug_idt_show_entry(unsigned int vector)
{
#ifdef __TESTCASE_DEBUG__
	struct descriptor_table_ptr idtr;
	struct interrupt_desc *idt;
	unsigned long offset = 0;

	sidt(&idtr);

	idt    = (struct interrupt_desc *)idtr.base;

	offset = (((unsigned long)idt[vector].offset0) << 0)
	       + (((unsigned long)idt[vector].offset1) << 16);

#ifdef __x86_64__
	offset += (((unsigned long)idt[vector].offset2) << 32);
#endif
	DEBUG(" %03u ", vector);
	DEBUG(" %04x ", idt[vector].selector);
	DEBUG(" %016lx ", offset);
	DEBUG(" %04u ", (unsigned int)idt[vector].type);
	DEBUG(" %u ", (unsigned int)idt[vector].s);
	DEBUG(" %02u ", (unsigned int)idt[vector].dpl);
	DEBUG(" %u \n", (unsigned int)idt[vector].p);
#ifdef __x86_64__
	//DEBUG("0x%016lx  ", idtl[vector * 2]);
	//DEBUG("0x%016lx\n", idtl[vector * 2 + 1]);
#else
	//DEBUG("0x%016llx\n", idtl[vector * 2]);
#endif
#endif
}

static inline void debug_gdt_show_count(unsigned int count)
{
#ifdef __TESTCASE_DEBUG__
	unsigned int i;

	DEBUG("\n");

	DEBUG_GDT_SHOW_FIELDS();

	for (i = 0; i < count; i++)
	{
		DEBUG_GDT_SHOW_ENTRY(i << 3);
	}
#endif
}

static inline void debug_idt_show_count(unsigned int count)
{
#ifdef __TESTCASE_DEBUG__
	unsigned int i;

	DEBUG("\n");
	DEBUG_IDT_SHOW_FIELDS();

	for (i = 0; i < count; i++)
	{
		DEBUG_IDT_SHOW_ENTRY(i);
	}
#endif
}

#ifdef __x86_64__
static USED void debug_tss_show_entry(tss64_t *tss)
{

}
#else
static USED void debug_tss_show_entry(tss32_t *tss)
{
	DEBUG("IOMAP &    T : 0x%04x, 0x%04x\n", tss->iomap_base, tss->t);
	DEBUG("RES10 &  LDT : 0x%04x, 0x%04x\n", tss->res11, tss->ldt);
	DEBUG("RES10 &   GS : 0x%04x, 0x%04x\n", tss->res10, tss->gs);
	DEBUG("RES9  &   FS : 0x%04x, 0x%04x\n", tss->res9, tss->fs);
	DEBUG("RES8  &   DS : 0x%04x, 0x%04x\n", tss->res8, tss->ds);
	DEBUG("RES7  &   SS : 0x%04x, 0x%04x\n", tss->res7, tss->ss);
	DEBUG("RES6  &   CS : 0x%04x, 0x%04x\n", tss->res6, tss->cs);
	DEBUG("RES5  &   ES : 0x%04x, 0x%04x\n", tss->res5, tss->es);
	DEBUG("         EBX : 0x%08x\n", tss->edi);
	DEBUG("         EBX : 0x%08x\n", tss->esi);
	DEBUG("         EBX : 0x%08x\n", tss->ebp);
	DEBUG("         EBX : 0x%08x\n", tss->esp);
	DEBUG("         EBX : 0x%08x\n", tss->ebx);
	DEBUG("         EDX : 0x%08x\n", tss->edx);
	DEBUG("         ECX : 0x%08x\n", tss->ecx);
	DEBUG("         EAX : 0x%08x\n", tss->eax);
	DEBUG("         EIP : 0x%08x\n", tss->eip);
	DEBUG("         CR3 : 0x%08x\n", tss->cr3);
	DEBUG("RES4  &  SS2 : 0x%04x, 0x%04x\n", tss->res4, tss->ss2);
	DEBUG("        ESP2 : 0x%08x\n", tss->esp2);
	DEBUG("RES3  &  SS1 : 0x%04x, 0x%04x\n", tss->res3, tss->ss1);
	DEBUG("        ESP1 : 0x%08x\n", tss->esp1);
	DEBUG("RES2  &  SS0 : 0x%04x, 0x%04x\n", tss->res2, tss->ss0);
	DEBUG("        ESP0 : 0x%08x\n", tss->esp0);
	DEBUG("RES1  & PREV : 0x%04x, 0x%04x\n", tss->res1, tss->prev);
}
#endif

/**
 * IRQ Counter
 * This array counts all interrupts:
 * Befor every testing, we must be call irqcounter_initialize() to sets all counter
 * to 0 first, then irq counter can be help us to testing.
 * If an interrupt/exception happened, we can call irqcounter_incre(vector) to 
 * incre the counter of this interrupt/exception in the interrupt handler, and 
 * after interrupt handler, we can call irqcounter_query(vector) to get the
 * counter and check it if is success or failure.
 * in the test interrupt handler,
 */

#define EXCEPTION_COUNTER_QUERY(no)								\
	DEBUG("#%s count = %u\n", exception_name(no), irqcounter_query(no));

static volatile unsigned int g_irqcounter[256] = { 0 };

#define PREPARE_INTERRUPT_MONITOR()		irqcounter_initialize()

static inline void irqcounter_initialize(void)
{
	memset((void*)g_irqcounter, 0, sizeof(g_irqcounter));
}

static inline void irqcounter_incre(unsigned int vector)
{
	g_irqcounter[vector]++;
}

static inline unsigned int irqcounter_query(unsigned int vector)
{
	return g_irqcounter[vector];
}

#define PAGE_TYPE_PTE			1
#define PAGE_TYPE_PDE			2
#define PAGE_TYPE_RDPTE			3
#define PAGE_TYPE_PML4			4

#define SHIFT_PAGE_P			0
#define SHIFT_PAGE_WR			1
#define SHIFT_PAGE_US			2
#define SHIFT_PAGE_PWT			3
#define SHIFT_PAGE_PCM			4
#define SHIFT_PAGE_PS			6

#define PREPARE_PAGE(page, s, v)						\
	page_control_set_bit(page, PAGE_TYPE_PTE, s, v)
/**
 * Page control
 */
void page_control_set_bit(void *gva, unsigned int level,
			  unsigned int shift, u32 value)
{
	if (gva == NULL) {
		ERROR("this address is NULL!\n");
		return;
	}

	ulong cr3 = read_cr3();
#ifdef __x86_64__
	u32 pdpte_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_TYPE_RDPTE);
	u32 pml4_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_TYPE_PML4);
	u32 pd_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_TYPE_PDE); 
	u32 pt_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_TYPE_PTE);
	pteval_t *pml4 = (pteval_t *)cr3;

	pteval_t *pdpte = (pteval_t *)(pml4[pml4_offset] & PAGE_MASK);
	pteval_t *pd = (pteval_t *)(pdpte[pdpte_offset] & PAGE_MASK);
	pteval_t *pt = (pteval_t *)(pd[pd_offset] & PAGE_MASK);

	switch (level) {
	case PAGE_TYPE_PML4:
		if (value) {
			pml4[pml4_offset] |= (1 << shift);		
		} else {
			pml4[pml4_offset] &= ~(1 << shift);					
		}
		break;
	case PAGE_TYPE_RDPTE:
		if (value) {
			pdpte[pdpte_offset] |= (1 << shift);		
		} else {
			pdpte[pdpte_offset] &= ~(1 << shift);					
		}
		break;
	case PAGE_TYPE_PDE:
		if (value) {
			pd[pd_offset] |= (1 << shift);		
		} else {
			pd[pd_offset] &= ~(1 << shift);					
		}
		break;	
	case PAGE_TYPE_PTE:
		if (value) {
			pt[pt_offset] |= (1 << shift);		
		} else {
			pt[pt_offset] &= ~(1 << shift);					
		}
		break;	
	}

	if (value) {
		pml4[pml4_offset] |= (1 << shift);		
		pdpte[pdpte_offset] |= (1 << shift);		
		pd[pd_offset] |= (1 << shift);		
		pt[pt_offset] |= (1 << shift);		
	}
#else 
	u32 pde_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_TYPE_PDE);
	u32 pte_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_TYPE_PTE);
	pteval_t *pde = (pgd_t *)cr3;

	pteval_t *pte = (pteval_t *)(pde[pde_offset] & PAGE_MASK);

	if (level == PAGE_TYPE_PDE) {
		if (value) {
			pde[pde_offset] |= (1 << shift);		
		} else {
			pde[pde_offset] &= ~(1 << shift); 				
		}
	} else {
		if (value) {
			pte[pte_offset] |= (1 << shift);		
		} else {
			pte[pte_offset] &= ~(1 << shift); 				
		}
	}
#endif
	asm volatile("invlpg %0\n\t"
			"nop\n\t" : : "m"(*((uintptr_t *)gva)): "memory"); 
}

/**
 * Default Exception and interrupt handlers
 * All exception default handler is common_exception_handler(), if we no need to
 * check the other exception, we need set the handler to common_exception_handler.
 * If a not expected exception happend, the processor will be enter to the fault
 * function common_exception_fault(), then the cpu will be put the error infor to
 * the monitor,  and  dead loop in this function.
 */

#define CSO_VECTOR			9
#ifndef XM_VECTOR
#define	XM_VECTOR			19
#endif

#ifndef VE_VECTOR
#define VE_VECTOR			20
#endif

#ifndef AC_VECTOR
#define AC_VECTOR			17
#endif

#define E15_VECTOR			15
#define E21_VECTOR			21
#define E22_VECTOR			22
#define E23_VECTOR			23
#define E24_VECTOR			24
#define E25_VECTOR			25
#define E26_VECTOR			26
#define E27_VECTOR			27
#define E28_VECTOR			28
#define E29_VECTOR			29
#define E30_VECTOR			30
#define E31_VECTOR			31
#define	X20_VECTOR			32
#define X21_VECTOR			33

#define X80_VECTOR			128
#define XFF_VECTOR			255

#define TICK_VECTOR			X20_VECTOR
#define IPI_VECTOR			X21_VECTOR
#define SYSCALL_VECTOR			X80_VECTOR

asm("DE_VECTOR = 0\n\t");
asm("DB_VECTOR = 1\n\t");
asm("NMI_VECTOR = 2\n\t");
asm("BP_VECTOR = 3\n\t");
asm("OF_VECTOR = 4\n\t");
asm("BR_VECTOR = 5\n\t");
asm("UD_VECTOR = 6\n\t");
asm("NM_VECTOR = 7\n\t");
asm("DF_VECTOR = 8\n\t");
asm("CSO_VECTOR = 9\n\t");
asm("TS_VECTOR = 10\n\t");
asm("NP_VECTOR = 11\n\t");
asm("SS_VECTOR = 12\n\t");
asm("GP_VECTOR = 13\n\t");
asm("PF_VECTOR = 14\n\t");
asm("E15_VECTOR = 15\n\t");
asm("MF_VECTOR = 16\n\t");
asm("AC_VECTOR = 17\n\t");
asm("MC_VECTOR = 18\n\t");
asm("XM_VECTOR = 19\n\t");
asm("VE_VECTOR = 20\n\t");
asm("E21_VECTOR = 21\n\t");
asm("E22_VECTOR = 22\n\t");
asm("E23_VECTOR = 23\n\t");
asm("E24_VECTOR = 24\n\t");
asm("E25_VECTOR = 25\n\t");
asm("E26_VECTOR = 26\n\t");
asm("E27_VECTOR = 27\n\t");
asm("E28_VECTOR = 28\n\t");
asm("E29_VECTOR = 29\n\t");
asm("E30_VECTOR = 30\n\t");
asm("E31_VECTOR = 31\n\t");
asm("X20_VECTOR = 32\n\t");
asm("X21_VECTOR = 33\n\t");
asm("X80_VECTOR = 128\n\t");
asm("XFF_VECTOR = 255\n\t");
asm("TICK_VECTOR = X20_VECTOR\n\t");
asm("IPI_VECTOR = X21_VECTOR\n\t");
asm("SYSCALL_VECTOR = X80_VECTOR\n\t");
asm("false = 0\n\t");
asm("true = 1\n\t");

typedef struct
{
	const ulong	 vector;
	void		*handler;
	const char 	*comment;
	const char	*name;
}exception_infor_t;

/**
 * IRQ/EXCEPTION Entry function
 */
#ifdef __x86_64__
#define R_HANDLER	"%r15"
#else
#define R_HANDLER	"%edi"
#endif

static void exception_befor_handler(exp_regs_t *regs, int ncase, int incre);
static void exception_common_handler(exp_regs_t *regs);

#define DEFAULT(NAME)			NAME##_entry_0
#define HANDLER(NAME, CASE)		NAME##_entry_##CASE
#define PREPARE_INTERRUPT_HANDLER_EX(NAME, CASE, VECTOR, INC, FUNC, COMM, CMT)	\
        extern void NAME##_entry_##CASE(void);					\
        asm (".pushsection .text \n\t"						\
		#NAME"_entry_"#CASE ": \n\t"					\
		"push %" R "ax \n\t"						\
		"mov $"#VECTOR", %" R "ax\n\t"					\
		"cmp $DF_VECTOR, %" R "ax\n\t"					\
		"jz _"#NAME#CASE".1\n\t"					\
		"cmp $TS_VECTOR, %" R "ax\n\t"					\
		"jz _"#NAME#CASE".1\n\t"					\
		"cmp $NP_VECTOR, %" R "ax\n\t"					\
		"jz _"#NAME#CASE".1\n\t"					\
		"cmp $SS_VECTOR, %" R "ax\n\t"					\
		"jz _"#NAME#CASE".1\n\t"					\
		"cmp $GP_VECTOR, %" R "ax\n\t"					\
		"jz _"#NAME#CASE".1\n\t"					\
		"cmp $PF_VECTOR, %" R "ax\n\t"					\
		"jz _"#NAME#CASE".1\n\t"					\
		"cmp $PF_VECTOR, %" R "ax\n\t"					\
		"jz _"#NAME#CASE".1\n\t"					\
		"cmp $AC_VECTOR, %" R "ax\n\t"					\
		"jz _"#NAME#CASE".1\n\t"					\
		"pop %" R "ax\n\t"						\
		"push" W " $0\n\t"						\
		"push %" R "ax\n\t"						\
		"_"#NAME#CASE".1:\n\t"						\
		"pop %" R "ax\n\t"						\
		"push" W " $"#VECTOR" \n\t"					\
		"push " R_HANDLER "\n\t"					\
		"mov $" #NAME "_handler_" #CASE ", " R_HANDLER "\n\t"		\
		"jmp __interrupt_comm_entry \n\t"				\
		".popsection\n\t");						\
										\
		asm(".pushsection .data.ex.comm."#VECTOR"."#CASE"\n");		\
		static USED const exception_infor_t g_##VECTOR##CASE##_infor = {\
			VECTOR,							\
			(void*)NAME##_entry_##CASE,				\
			CMT,							\
			#NAME,							\
		};								\
		asm(".popsection\n");						\
										\
		static USED void NAME##_handler_##CASE(exp_regs_t *regs)	\
		{								\
			exception_befor_handler(regs, CASE, INC);		\
			FUNC(regs);						\
		}

#define INTERRUPT_DEFAULT_HEAD(VECTOR, VAR)					\
	static const exception_infor_t *VAR = &g_##VECTOR##0_infor
#define INTERRUPT_DEFAULT_TAIL(VECTOR, VAR)					\
	static const exception_infor_t *VAR = &g_##VECTOR##0_infor

#define DEFINE_INTERRUPT_DEFAULT(NAME, VECTOR, COMMENT)				\
	PREPARE_INTERRUPT_HANDLER_EX(NAME, 0, VECTOR, false, 			\
				     exception_common_handler,			\
				     true, COMMENT)

#define PREPARE_INTERRUPT_HANDLER_NOINCRE(NAME, CASE, VECTOR, FUNC)		\
	PREPARE_INTERRUPT_HANDLER_EX(NAME, CASE, VECTOR, false, FUNC, false, "")

#define PREPARE_INTERRUPT_HANDLER(NAME, CASE, VECTOR, FUNC)			\
	PREPARE_INTERRUPT_HANDLER_EX(NAME, CASE, VECTOR, true, FUNC, false, "")

#define PREPARE_INTERRUPT_HANDLER_COMMON(NAME, CASE, VECTOR)			\
	PREPARE_INTERRUPT_HANDLER(NAME, CASE, VECTOR, exception_common_handler)

#define DEFINE_SPECIAL_HANDLER(NAME, VECTOR, CODE)				\
        extern void NAME##_entry(void);						\
        asm (".pushsection .text \n\t"						\
		#NAME"_entry: \n\t"						\
		"push" W " $0 \n\t"						\
		"push" W " $"#VECTOR" \n\t"					\
		"push " R_HANDLER " \n\t"					\
		"mov $" #NAME "_handler, " R_HANDLER "\n\t"			\
		"jmp __interrupt_comm_entry \n\t"				\
		".popsection");							\
										\
		static USED void NAME##_handler(exp_regs_t *regs)		\
		{								\
			CODE							\
		}

#define SPECIAL(NAME)			NAME##_entry

asm (".pushsection .text \n\t"
	"__interrupt_comm_entry: \n"
#ifdef __x86_64__
 	"push %r14; push %r13; push %r12 \n\t"
	"push %r11; push %r10; push %r9; push %r8 \n\t"
	"push %"R "di; push %"R "si; push %"R "bp; sub $"S", %"R "sp \n\t"
	"push %"R "bx; push %"R "dx; push %"R "cx; push %"R "ax \n\t"
#else
	"push %"R "si; push %"R "bp; sub $"S", %"R "sp \n\t"
	"push %"R "bx; push %"R "dx; push %"R "cx; push %"R "ax \n\t"
#endif
	
#ifdef __x86_64__
	"mov %"R "sp, %"R "di \n\t"
	"call" W " *%r15 \n\t"
#else
	"mov %" R "sp, %" R "bp \n\t"
	"push %" R "bp\n\r"
	"call" W " *%" R "di \n\t"
	"pop %" R "bp\n\r"
#endif
	"pop %"R "ax; pop %"R "cx; pop %"R "dx; pop %"R "bx \n\t"
	"add $"S", %"R "sp; pop %"R "bp; pop %"R "si; pop %"R "di \n\t"
#ifdef __x86_64__
	"pop %r8; pop %r9; pop %r10; pop %r11 \n\t"
	"pop %r12; pop %r13; pop %r14; pop %r15 \n\t"
#endif
	"add $"S", %"R "sp \n\t"
	"add $"S", %"R "sp \n\t"
	"iret"W" \n\t"
);

DEFINE_INTERRUPT_DEFAULT(DE , 0x00, "#DE - Divide Error Exception");
DEFINE_INTERRUPT_DEFAULT(DB , 0x01, "#DB - Debug Exception");
DEFINE_INTERRUPT_DEFAULT(NMI, 0x02, "NMI - NMI Interrupt");
DEFINE_INTERRUPT_DEFAULT(BP , 0x03, "#BP - Breakpoint Exception");
DEFINE_INTERRUPT_DEFAULT(OF , 0x04, "#OF - Overflow Exception");
DEFINE_INTERRUPT_DEFAULT(BR , 0x05, "#BR - BOUND Range Exceeded Exception");
DEFINE_INTERRUPT_DEFAULT(UD , 0x06, "#UD - Invalid Opcode Exception");
DEFINE_INTERRUPT_DEFAULT(NM , 0x07, "#NM - Device Not Available Exception");
DEFINE_INTERRUPT_DEFAULT(DF , 0x08, "#DF - Double Fault Exception");
DEFINE_INTERRUPT_DEFAULT(CSO, 0x09, "CSO - Coprocessor Segment Overrun");
DEFINE_INTERRUPT_DEFAULT(TS , 0x0a, "#TS - Invalid TSS Exception");
DEFINE_INTERRUPT_DEFAULT(NP , 0x0b, "#NP - Segment Not Present");
DEFINE_INTERRUPT_DEFAULT(SS , 0x0c, "#SS - Stack Fault Exception");
DEFINE_INTERRUPT_DEFAULT(GP , 0x0d, "#GP - General Protection Exception");
DEFINE_INTERRUPT_DEFAULT(PF , 0x0e, "#PF - Page Fault Exception");
DEFINE_INTERRUPT_DEFAULT(E15, 0x0f, "E15 - Exception 15 reserved");
DEFINE_INTERRUPT_DEFAULT(MF , 0x10, "#MF - x87 FPU Floating Point Error");
DEFINE_INTERRUPT_DEFAULT(AC , 0x11, "#AC - Alignment Check Exception");
DEFINE_INTERRUPT_DEFAULT(MC , 0x12, "#MC - Machine Check Exception");
DEFINE_INTERRUPT_DEFAULT(XM , 0x13, "#XM - SIMD Floating-Point Exception");
DEFINE_INTERRUPT_DEFAULT(VE , 0x14, "#VE - Virtualization Exception");
DEFINE_INTERRUPT_DEFAULT(E21, 0x15, "E21 - Exception 21 reserved");
DEFINE_INTERRUPT_DEFAULT(E22, 0x16, "E22 - Exception 22 reserved");
DEFINE_INTERRUPT_DEFAULT(E23, 0x17, "E23 - Exception 23 reserved");
DEFINE_INTERRUPT_DEFAULT(E24, 0x18, "E24 - Exception 24 reserved");
DEFINE_INTERRUPT_DEFAULT(E25, 0x19, "E25 - Exception 25 reserved");
DEFINE_INTERRUPT_DEFAULT(E26, 0x1a, "E26 - Exception 26 reserved");
DEFINE_INTERRUPT_DEFAULT(E27, 0x1b, "E27 - Exception 27 reserved");
DEFINE_INTERRUPT_DEFAULT(E28, 0x1c, "E28 - Exception 28 reserved");
DEFINE_INTERRUPT_DEFAULT(E29, 0x1d, "E29 - Exception 29 reserved");
DEFINE_INTERRUPT_DEFAULT(E30, 0x1e, "E30 - Exception 30 reserved");
DEFINE_INTERRUPT_DEFAULT(E31, 0x1f, "E31 - Exception 31 reserved");
DEFINE_INTERRUPT_DEFAULT(X20, 0x20, "X20 - Systick interrupt");
DEFINE_INTERRUPT_DEFAULT(X21, 0x21, "X21 - IPI interrupt");
DEFINE_INTERRUPT_DEFAULT(X80, 0x80, "X80 - System call interrupt");
DEFINE_INTERRUPT_DEFAULT(XFF, 0xff, "XFF - Interrupt 255");

INTERRUPT_DEFAULT_HEAD(0xff, g_exception_infor_head);
INTERRUPT_DEFAULT_TAIL(0x00, g_exception_infor_tail);

static const exception_infor_t *exception_infor(unsigned int vector)
{
	const exception_infor_t *infor = g_exception_infor_head;

	for (infor = g_exception_infor_head; 
	     infor <= g_exception_infor_tail;
	     infor++) {
		if (infor->vector == vector) {
			return infor;
		}
	}

	return NULL;
}

static inline const char *exception_name(unsigned int vector)
{
	const exception_infor_t *infor = exception_infor(vector);

	if (infor) {
		return infor->name;
	}

	return "UNKNOW";
}

static inline const char *exception_comment(unsigned int vector)
{
	const exception_infor_t *infor = exception_infor(vector);

	if (infor) {
		return infor->comment;
	}

	return "UNKNOW interrupt";
}

static USED void exception_common_handler(exp_regs_t *regs)
{
	/* do nothing. */
}

static USED void exception_befor_handler(exp_regs_t *regs, int ncase, int incre)
{
	if (ncase == 0){
		const exception_infor_t *infor = g_exception_infor_head;

		for (infor = g_exception_infor_head; 
		infor <= g_exception_infor_tail;
		infor++) {
			if (infor->vector == regs->vector) {
				IRQ_ERROR("***CPU %x capture unknow %s.\n", apic_id(), infor->comment);
				IRQ_ERROR("     rip:  0x%08lx\n", regs->rip);
				IRQ_ERROR("err code:  0x%08lx\n", regs->error_code);
				while(true);
			}
		}

		IRQ_ERROR("CPU %x capture unknow interrupt %ld.\n", apic_id(), regs->vector);
		IRQ_ERROR("     rip:  0x%08lx\n", regs->rip);
		IRQ_ERROR("err code:  0x%08lx\n", regs->error_code);

		while(true);
	}
	else{
		IRQ_DEBUG("%s - %ld @ CPU %x(case %d).\n",
			exception_comment(regs->vector), regs->vector, apic_id(), ncase);
		IRQ_DEBUG("      ss:  0x%08lx\n", (unsigned long)read_ss());
		IRQ_DEBUG("     rsp:  0x%08lx\n", (unsigned long)regs);
		IRQ_DEBUG("      cs:  0x%08lx\n", regs->cs);
		IRQ_DEBUG("     rip:  0x%08lx\n", regs->rip);
		IRQ_DEBUG("err code:  0x%08lx\n", regs->error_code);

		if (incre) {
			irqcounter_incre(regs->vector);
		}
	}
}

static void exception_initialize(void)
{
	const exception_infor_t *infor;

	for (infor = g_exception_infor_head; 
	     infor <= g_exception_infor_tail;
	     infor++)
	{
		DEBUG("Register %s\n", infor->comment);
		PREPARE_INTERRUPT_GATE(infor->vector, KERNEL_CS, infor->handler, P1, DPL0, S0);
	}
}


/**
 * SYSTick
 * System ticks counter, when system starting, we must be call  systick_initialize()
 * to initialize SYSTick. If it has been done, then we can call systick_clock() to
 * get current clock counter in the all testing. also systick_delay() can be help us
 * to delay any ticks if we need.
 */
static volatile unsigned long g_systick = 0;

DEFINE_SPECIAL_HANDLER(SYSTICK, X20_VECTOR,
{
	g_systick++;
	eoi();
});

static inline void systick_recovery(void)
{
	apic_write(APIC_LVTT, APIC_LVT_TIMER_PERIODIC | X20_VECTOR);
	apic_write(APIC_TDCR, APIC_TDR_DIV_1);
	apic_write(APIC_TMICT, 1000U * 1000U * 100U);
}

static inline void systick_initialize(void)
{
	irq_disable();
	g_systick = 0;

	PREPARE_INTERRUPT_GATE(X20_VECTOR, KERNEL_CS, SPECIAL(SYSTICK), P1, DPL0, S0);
	systick_recovery();
	irq_enable();
}

static inline unsigned long systick_clock(void)
{
	return g_systick;
}

static inline void systick_delay(unsigned long time)
{
	unsigned long oldtime = g_systick;
	while(g_systick < oldtime + time);
}

/**
 *  Test case code
 */

/**
 * Case name: Interrupt Exception Source Expose 000
 * Summary  : Register the handler of exception #DE(0x0) then divide zero should be 
 *            capture #DE exception.
 */
static void DE_27484_exception_handler(exp_regs_t *regs)
{
	struct descriptor_table_ptr memgdtr = { 0 };
	ASSERT(TAKE_SEL_TI(regs->cs) == 0, "Not support LDT.");
	sgdt(&memgdtr);

	if (GDT_GET_L((gdt_descriptor_t*)memgdtr.base, regs->cs)) {
		regs->rip += 3;
	}
	else if (GDT_GET_DB((gdt_descriptor_t*)memgdtr.base, regs->cs)) {
		regs->rip += 2;
	}
	else {
		DEBUG("Not support 16bits segment.\n");
	}
}

PREPARE_INTERRUPT_HANDLER(DE, 27484, DE_VECTOR, DE_27484_exception_handler);

static int exception_source_expose_000(void)
{
	DEBUG("27484.Interrupt Exception Source Expose 000(#DE) testing...\n");

	irq_disable();
	PREPARE_INTERRUPT_MONITOR();
	PREPARE_INTERRUPT_GATE(DE_VECTOR, KERNEL_CS, HANDLER(DE, 27484), P1, DPL0, S0);
	irq_enable();

	ASM_TRIGGER_DE();

	EXCEPTION_COUNTER_QUERY(DE_VECTOR);

	irq_disable();
	RECOVERY_INTERRUPT_GATE(DE_VECTOR, KERNEL_CS, DEFAULT(DE));
	irq_enable();

	if (irqcounter_query(DE_VECTOR) == 1){
		return RESULT_OK;
	}

	return RESULT_FAULT;
}

/**
 * Case name: Interrupt interrupt&exception handling expose 001
 * Summary  : Register the handler of interrupt 0x80 then execute INT 0x80 instruction
 *            should be capture the soft interrupt 0x80.
 */

PREPARE_INTERRUPT_HANDLER_COMMON(X80, 27367, X80_VECTOR);

static int interrupt_and_exception_handling_expose_001(void)
{
	DEBUG("27367.Interrupt interrupt&exception handling expose 001(INT 0x80) testing...\n");

	irq_disable();
	PREPARE_INTERRUPT_MONITOR();
	PREPARE_INTERRUPT_GATE(X80_VECTOR, KERNEL_CS, HANDLER(X80, 27367), P1, DPL0, S0);
	irq_enable();

	ASM_TRIGGER_INT0x80();

	EXCEPTION_COUNTER_QUERY(X80_VECTOR);

	irq_disable();
	RECOVERY_INTERRUPT_GATE(X80_VECTOR, KERNEL_CS, DEFAULT(X80));
	irq_enable();

	if (irqcounter_query(X80_VECTOR) == 1){
		return RESULT_OK;
	}

	return RESULT_FAULT;
}

/**
 * Case name: Interrupt NMI sources injection exprose 001
 * Summary  : If the processor meeting a NMI interrupt, the RFLAGS.IF(bit 9) 
 *            will be ignore and the processor should be process NMI interrupt.
 */
PREPARE_INTERRUPT_HANDLER_COMMON(NMI, 27341, NMI_VECTOR);

static int NMI_sources_injection_exprose_001(void)
{
	DEBUG("27341.Interrupt NMI sources injection exprose 001(#NMI) testing...\n");

	irq_disable();
	PREPARE_INTERRUPT_MONITOR();
	PREPARE_INTERRUPT_GATE(NMI_VECTOR, KERNEL_CS, HANDLER(NMI, 27341), P1, DPL0, S0);
	apic_icr_write(APIC_DEST_SELF | APIC_DM_NMI | APIC_DM_FIXED | NMI_VECTOR, 0);
	EXCEPTION_COUNTER_QUERY(NMI_VECTOR);
	RECOVERY_INTERRUPT_GATE(NMI_VECTOR, KERNEL_CS, DEFAULT(NMI));
	irq_enable();

	if (irqcounter_query(NMI_VECTOR) == 1){
		return RESULT_OK;
	}

	return RESULT_FAULT;
}


/**
 * Case name: Interrupt External interrupt sources injection exprose 001
 * Summary  : If the processor meeting a NMI interrupt, the RFLAGS.IF(bit 9) 
 *            will be ignore and the processor should be process NMI interrupt.
 */
static void X20_27327_timer_handler(irq_regs_t *regs)
{
	eoi();
}

PREPARE_INTERRUPT_HANDLER(X20, 27327, X20_VECTOR, X20_27327_timer_handler);

static int external_interrupt_sources_injection_exprose_001(void)
{
	DEBUG("27327.Interrupt External interrupt sources injection exprose 001(#TIMER) testing...\n");

	irq_disable();
	PREPARE_INTERRUPT_MONITOR();
	PREPARE_INTERRUPT_GATE(X20_VECTOR, KERNEL_CS, HANDLER(X20, 27327), P1, DPL0, S0);
	apic_write(APIC_LVTT, APIC_LVT_TIMER_ONESHOT | X20_VECTOR);
	apic_write(APIC_TDCR, APIC_TDR_DIV_16);
	apic_write(APIC_TMICT, 10000/* 10000000 */);
	irq_enable();

	while(irqcounter_query(X20_VECTOR) < 1);

	irq_disable();
	RECOVERY_INTERRUPT_GATE(X20_VECTOR, KERNEL_CS, SPECIAL(SYSTICK));
	systick_recovery();
	irq_enable();

	EXCEPTION_COUNTER_QUERY(X20_VECTOR);

	if (irqcounter_query(X20_VECTOR) == 1){
		return RESULT_OK;
	}

	return RESULT_FAULT;
}

/**
 * Case name: Second #DF 001
 * Summary  : Register the handler of interrupt 0x80 then execute INT 0x80 instruction
 *            should be capture the soft interrupt 0x80.
 */
static void DF_24211_exception_handler(exp_regs_t *regs)
{
	struct descriptor_table_ptr newgdtr;
	sgdt(&newgdtr);
	page_control_set_bit((void*)(newgdtr.base + PAGE_SIZE),
			     PAGE_TYPE_PTE, SHIFT_PAGE_P, 1);
}

PREPARE_INTERRUPT_HANDLER(DF, 24211, DF_VECTOR, DF_24211_exception_handler);
PREPARE_INTERRUPT_HANDLER_COMMON(PF, 24211, PF_VECTOR);

static int second_DF_001(void)
{
	struct descriptor_table_ptr oldgdtr;
	struct descriptor_table_ptr newgdtr;
	unsigned char *newgdt;

	DEBUG("24211.Interrupr_Second_#DF_001 testing...\n");

	irq_disable();
	PREPARE_INTERRUPT_MONITOR();
	PREPARE_INTERRUPT_GATE(DF_VECTOR, KERNEL_CS, HANDLER(DF, 24211), P1, DPL0, S0);
	PREPARE_INTERRUPT_GATE(PF_VECTOR, MAKE_SEL(513, 0, 0), HANDLER(PF, 24211), P1, DPL0, S0);
	newgdt = PREPARE_NEWGDT(&oldgdtr, &newgdtr);
	irq_enable();

	DEBUG_IDT_SHOW_FIELDS();
	DEBUG_IDT_SHOW_ENTRY(DF_VECTOR);
	DEBUG_IDT_SHOW_ENTRY(PF_VECTOR);

	DEBUG_GDT_SHOW_FIELDS();
	DEBUG_GDT_SHOW_ENTRY(KERNEL_CS);
	DEBUG_GDT_SHOW_ENTRY(MAKE_SEL(513, 0, 0));

	PREPARE_PAGE(newgdt + PAGE_SIZE, SHIFT_PAGE_P, 0);

	*(newgdt + PAGE_SIZE) = 0;

	EXCEPTION_COUNTER_QUERY(DF_VECTOR);
	EXCEPTION_COUNTER_QUERY(PF_VECTOR);

	irq_disable();
	RECOVERY_INTERRUPT_GATE(DF_VECTOR, KERNEL_CS, DEFAULT(DF));
	RECOVERY_INTERRUPT_GATE(PF_VECTOR, KERNEL_CS, DEFAULT(PF));
	RECOVERY_OLDGDT(&oldgdtr, &newgdtr);
	irq_enable();

	if (irqcounter_query(DF_VECTOR) == 1){
		return RESULT_OK;
	}

	return RESULT_FAULT;
}

/**
 * Case name: Interrupt Simultaneous exceptions-2nd including #PF with GDT exception 001
 * Summary  : In #UD exception meet #GP and #PF, the processor should be process the all interrupt.
 *            should be capture the soft interrupt 0x80.
 */

static void UD_26813_exception_handler(irq_regs_t *regs)
{
	regs->rip += 2;
}

static void PF_26813_exception_handler(irq_regs_t *regs)
{
	struct descriptor_table_ptr newgdtr;
	sgdt(&newgdtr);
	page_control_set_bit((void*)(newgdtr.base + PAGE_SIZE),
			     PAGE_TYPE_PTE, SHIFT_PAGE_P, 1);
}

static void GP_26813_exception_handler(irq_regs_t *regs)
{
	struct descriptor_table_ptr newgdtr;
	sgdt(&newgdtr);
	GDT_SET_S((gdt_descriptor_t*)newgdtr.base, MAKE_SEL(513, 0, 0), 1);
	DEBUG_GDT_SHOW_FIELDS();
	DEBUG_GDT_SHOW_ENTRY(KERNEL_CS);
	DEBUG_GDT_SHOW_ENTRY(MAKE_SEL(513, 0, 0));
}

PREPARE_INTERRUPT_HANDLER(UD, 26813, UD_VECTOR, UD_26813_exception_handler);
PREPARE_INTERRUPT_HANDLER(PF, 26813, PF_VECTOR, PF_26813_exception_handler);
PREPARE_INTERRUPT_HANDLER(GP, 26813, GP_VECTOR, GP_26813_exception_handler);
/**
 * Case name: Interrupt Simultaneous exceptions-2nd including #PF with GDT exception 001
 * Summary  : Construct a code segment descriptor of #UD handler at 513th entry and page out 
 *            second page of GDT will be triggering a second #PF when processor meet #UD, at
 *            the same time if the processor meeting the S bit of 513th entry is 0, then #PF
 *            #GP and #UD will be simultaneous triggering.
 */
static int simultaneous_exceptions_2nd_including_PF_with_GDT_exception_001(void)
{
	struct descriptor_table_ptr oldgdtr;
	struct descriptor_table_ptr newgdtr;
	unsigned char *newgdt;

	DEBUG("26813.Interrupt Simultaneous exceptions-2nd "
	      "including #PF with GDT exception 001 testing...\n");

	PREPARE_INTERRUPT_MONITOR();
	newgdt = PREPARE_NEWGDT(&oldgdtr, &newgdtr);
#ifdef __x86_64__
	PREPARE_DEFAULT_SEGMENT(MAKE_SEL(513, 0, 0), SEGCD_T_CR, S0, DPL0, P1, DB0, G1);
#else
	PREPARE_DEFAULT_SEGMENT(MAKE_SEL(513, 0, 0), SEGCD_T_CR, S0, DPL0, P1, DB1, G1);
#endif
	PREPARE_INTERRUPT_GATE(UD_VECTOR, MAKE_SEL(513, 0, 0), HANDLER(UD, 26813), P1, DPL0, S0);
	PREPARE_INTERRUPT_GATE(PF_VECTOR, KERNEL_CS, HANDLER(PF, 26813), P1, DPL0, S0);
	PREPARE_INTERRUPT_GATE(GP_VECTOR, KERNEL_CS, HANDLER(GP, 26813), P1, DPL0, S0);
	PREPARE_PAGE(newgdt + PAGE_SIZE, SHIFT_PAGE_P, 0);

	ASM_TRIGGER_UD();

	EXCEPTION_COUNTER_QUERY(UD_VECTOR);
	EXCEPTION_COUNTER_QUERY(PF_VECTOR);
	EXCEPTION_COUNTER_QUERY(GP_VECTOR);

	RECOVERY_INTERRUPT_GATE(GP_VECTOR, KERNEL_CS, DEFAULT(GP));
	RECOVERY_INTERRUPT_GATE(PF_VECTOR, KERNEL_CS, DEFAULT(PF));
	RECOVERY_INTERRUPT_GATE(UD_VECTOR, KERNEL_CS, DEFAULT(UD));

	RECOVERY_OLDGDT(&oldgdtr, &newgdtr);

	if (irqcounter_query(UD_VECTOR) == 1){
		return RESULT_OK;
	}

	return RESULT_FAULT;
	
}

/**
 * Case name: Interrupt IDT expose 002(64Bit Interrupt-gate)
 * Summary  : Call interrupt-gate(type = 0xE) should be trigger a interrupt, in the interrupt
 *            the processor will be automatic disable interrupt(set RFLAGS.IF to 0).
 */
static void X80_28831_interrupt_gate_handler(irq_regs_t *regs)
{
	unsigned long rflags = read_rflags();

	if (0 == (rflags & X86_EFLAGS_IF))
	{
		irqcounter_incre(regs->vector);
	}
}
PREPARE_INTERRUPT_HANDLER_NOINCRE(X80, 28831, X80_VECTOR, X80_28831_interrupt_gate_handler);

static int IDT_expose_002_64bit_interrupt_gate(void)
{
	DEBUG("28831.Interrupt IDT expose 002(64Bit Interrupt-gate) testing...\n");

	PREPARE_INTERRUPT_MONITOR();
#ifdef __x86_64__
	PREPARE_DEFAULT_SEGMENT(MAKE_SEL(10, 0, 0), SEGCD_T_CR, S1, DPL0, P1, DB0, G1);
#else
	PREPARE_DEFAULT_SEGMENT(MAKE_SEL(10, 0, 0), SEGCD_T_CR, S1, DPL0, P1, DB1, G1);
#endif
	PREPARE_INTERRUPT_GATE(X80_VECTOR, MAKE_SEL(10, 0, 0), HANDLER(X80, 28831), P1, DPL0, S0);

	ASM_TRIGGER_INT0x80();

	EXCEPTION_COUNTER_QUERY(X80_VECTOR);
	RECOVERY_INTERRUPT_GATE(X80_VECTOR, KERNEL_CS, DEFAULT(X80));

	if (irqcounter_query(X80_VECTOR) == 1){
		return RESULT_OK;
	}

	return RESULT_FAULT;
}

/**
 * Case name: Interrupt IDT expose 004(64Bit Trap-gate)
 * Summary  : Call Trap-gate should be trigger a trip-interrupt, in trip-interrupt 
 *            the processor should not be disable interrupt(RFLAGS.IF equal 1).
 */
static void X80_28833_trap_gate_handler(irq_regs_t *regs)
{
	unsigned long rflags = read_rflags();

	if (0 != (rflags & X86_EFLAGS_IF))
	{
		irqcounter_incre(regs->vector);
	}
}
PREPARE_INTERRUPT_HANDLER_NOINCRE(X80, 28833, X80_VECTOR, X80_28833_trap_gate_handler);

static int IDT_expose_004_64bit_trap_gate(void)
{
	DEBUG("28833.Interrupt IDT expose 004(64Bit Trap-gate) testing...\n");

	PREPARE_INTERRUPT_MONITOR();
#ifdef __x86_64__
	PREPARE_DEFAULT_SEGMENT(MAKE_SEL(10, 0, 0), SEGCD_T_CR, S1, DPL0, P1, DB0, G1);
#else
	PREPARE_DEFAULT_SEGMENT(MAKE_SEL(10, 0, 0), SEGCD_T_CR, S1, DPL0, P1, DB1, G1);
#endif
	PREPARE_TRAP_GATE(X80_VECTOR, MAKE_SEL(10, 0, 0), HANDLER(X80, 28833), P1, DPL0, S0);
	
	ASM_TRIGGER_INT0x80();

	EXCEPTION_COUNTER_QUERY(X80_VECTOR);
	RECOVERY_INTERRUPT_GATE(X80_VECTOR, KERNEL_CS, DEFAULT(X80));

	if (irqcounter_query(X80_VECTOR) == 1){
		return RESULT_OK;
	}

	return RESULT_FAULT;
}

#ifdef __x86_64__
/**
 * Case name: Interrupt IDT expose 004(64Bit Trap-gate)
 * Summary  : Call Trap-gate should be trigger a trip-interrupt, in trip-interrupt 
 *            the processor should not be disable interrupt(RFLAGS.IF equal 1).
 */

struct descriptor_table_ptr g_oldgdtr;
struct descriptor_table_ptr g_newgdtr;

static void X21_26113_trip_fault_trigging_function(void)
{
	char *newgdt = (char*)g_newgdtr.base;
	int i;

	DEBUG("CPU %u, Clock %lu, will be shutdown after 3 ticks.\n", apic_id(), systick_clock());
	lgdt(&g_newgdtr);

	for(i = 3; i > 0; i--) {
		printf("CPU %u, Ready %d...\n", apic_id(), i);
		systick_delay(1);
	}

	*(newgdt + PAGE_SIZE) = 0;

	while(true);
}

static USED void X21_26113_ipi_interrupt_handler(irq_regs_t *regs)
{
	regs->rip = (unsigned long) X21_26113_trip_fault_trigging_function;
}

PREPARE_INTERRUPT_HANDLER_COMMON(DF, 26113, DF_VECTOR);
PREPARE_INTERRUPT_HANDLER_COMMON(PF, 26113, PF_VECTOR);
PREPARE_INTERRUPT_HANDLER(X21, 26113, X21_VECTOR, X21_26113_ipi_interrupt_handler);

static int shutdown_mode_001(void)
{
	char *newgdt;

	DEBUG("26113.shutdown mode testing...\n");

	PREPARE_INTERRUPT_MONITOR();
	newgdt = PREPARE_NEWGDT(&g_oldgdtr, &g_newgdtr);
	PREPARE_DEFAULT_SEGMENT(MAKE_SEL(513, 0, 0), SEGCD_T_CR, S1, DPL0, P1, DB0, G1);
	PREPARE_INTERRUPT_GATE(DF_VECTOR, MAKE_SEL(513, 0, 0), HANDLER(DF, 26113), P1, DPL0, S0);
	PREPARE_INTERRUPT_GATE(PF_VECTOR, MAKE_SEL(513, 0, 0), HANDLER(PF, 26113), P1, DPL0, S0);
	PREPARE_INTERRUPT_GATE(IPI_VECTOR, KERNEL_CS, HANDLER(X21, 26113), P1, DPL0, S0);
	apic_icr_write(APIC_DEST_ALLBUT | APIC_DEST_PHYSICAL | APIC_DM_FIXED | X21_VECTOR, 0);
	PREPARE_PAGE(newgdt + PAGE_SIZE, SHIFT_PAGE_P, 0);

	while(true) {
		DEBUG("CPU %u, Clock %lu\n", apic_id(), systick_clock());
		systick_delay(1);
	}

	EXCEPTION_COUNTER_QUERY(DF_VECTOR);
	EXCEPTION_COUNTER_QUERY(PF_VECTOR);
	EXCEPTION_COUNTER_QUERY(X21_VECTOR);

	RECOVERY_INTERRUPT_GATE(DF_VECTOR, KERNEL_CS, DEFAULT(DF));
	RECOVERY_INTERRUPT_GATE(PF_VECTOR, KERNEL_CS, DEFAULT(PF));
	RECOVERY_INTERRUPT_GATE(X21_VECTOR, KERNEL_CS, DEFAULT(X21));
	RECOVERY_OLDGDT(&g_oldgdtr, &g_newgdtr);

	if (irqcounter_query(DF_VECTOR) == 1){
		return RESULT_OK;
	}

	return RESULT_FAULT;
}
#endif

#ifndef __x86_64__
/**
 * Case name: Interrupt IDT expose 001(32Bit Task-gate)
 * Summary  : In protection mode, call Task-gate should be schedule to the new task.
 */
void X80_28827_task_gate_handler(void)
{
    start:
	IRQ_DEBUG("%s - %d @ CPU %x(case %d).\n",
		exception_comment(X80_VECTOR), X80_VECTOR, apic_id(), 28827);
	IRQ_DEBUG("      ss:  0x%08lx\n", (unsigned long)read_ss());
	IRQ_DEBUG("      cs:  0x%08lx\n", (unsigned long)read_cs());
	irqcounter_incre(X80_VECTOR);
        asm volatile ("iret");
        IRQ_DEBUG("IRQ task restarts after iret.\n");
        goto start;
}

static int IDT_expose_001_32bit_task_gate(void)
{
	DEBUG("28827.IDT expose 001(32BIT Task-gate) testing...\n");
	
	PREPARE_INTERRUPT_MONITOR();

	setup_tss32();

	tss_intr.cs   = read_cs();
	tss_intr.eip  = (unsigned long)X80_28827_task_gate_handler;

	PREPARE_TSS_SEGMENT(MAKE_SEL(10, 0, 0), S0, DPL0, P1, DB0, G0);
	PREPARE_TASK_GATE(X80_VECTOR, MAKE_SEL(10, 0, 0), NULL, P1, DPL0, S0);

	ASM_TRIGGER_INT0x80();

	EXCEPTION_COUNTER_QUERY(X80_VECTOR);
	RECOVERY_INTERRUPT_GATE(X80_VECTOR, KERNEL_CS, DEFAULT(X80));

	if (irqcounter_query(X80_VECTOR) == 1){
		return RESULT_OK;
	}

	return RESULT_FAULT;

}
#endif

#define DS_USER 			MAKE_SEL(11, 0, DPL3)
#define CS_USER				MAKE_SEL(12, 0, DPL3)
#define SS_USER				MAKE_SEL(11, 0, DPL3)
#define ENTER_TO_RING3(fn, p)		enter_to_ring3(fn, p)

typedef int (*ring3_routine_fn_t)(void *p);
static unsigned char g_user_stack[4096] __attribute__((aligned(4096))) = { 0 };

int enter_to_ring3(ring3_routine_fn_t fn, void *param)
{
	int ret = 0;
	extern void kernel_entry();

	set_idt_entry(X80_VECTOR, &kernel_entry, DPL3);

	asm volatile (
#ifdef __x86_64__
	"push %r15; push %r14; push %r13; push %r12 \n\t"
	"push %r11; push %r10; push %r9; push %r8 \n\t"
#endif
	"push %"R "di; push %"R "si; push %"R "bp;\n\t"
	"push %"R "bx; push %"R "dx; push %"R "cx;\n\t"
	);

    asm volatile (
	"mov %%" R "sp, %%" R "cx\n\t"		// kernel stack top >> rcx
	"mov %[user_ds], %%dx\n\t"
	"mov %%dx, %%es\n\t"
	"mov %%dx, %%ds\n\t"
	"mov %%dx, %%fs\n\t"
	"mov %%dx, %%gs\n\t"
	"mov %[user_ss], %%dx\n\t"
	"push" W " %%" R "dx \n\t"		// push ss
	"lea %[user_stack], %%" R "dx \n\t"
	"push" W " %%" R "dx \n\t"		// push rsp
	"pushf" W "\n\t"			// pushfq
	"push" W " %[user_cs] \n\t"		// push cs
	"push" W " $1f \n\t"			// push rip
	"iret" W "\n\t"				// iretq
	"1:\n\t"
	"push %%" R "cx\n\t"			// kernel stack top >> [rsp]
#ifndef __x86_64__
	"push %[param]\n\t"
#else
        "movq %[param],  %%" R "di\n\r"
#endif
        "call *%[fn]\n\t"
#ifndef __x86_64__
	"pop %%ecx\n\t"
#endif
	"pop %%" R "cx\n\t"
	"mov $1f, %%" R "dx\n\t"
	"int %[kernel_entry_vector]\n\t"
	".section .text.entry \n\t"
	"kernel_entry: \n\t"
	"mov %%" R "cx, %%" R "sp \n\t"
	"mov %[kernel_ds], %%cx\n\t"
	"mov %%cx, %%ds\n\t"
	"mov %%cx, %%es\n\t"
	"mov %%cx, %%fs\n\t"
	"mov %%cx, %%gs\n\t"
	"jmp *%%" R "dx \n\t"
	".section .text\n\t"
	"1:\n\t"
	:  [ret] "=&a" (ret)
	:  [user_cs] "i" (CS_USER),
	   [user_ss] "i" (SS_USER),
	   [user_ds] "i" (DS_USER),
	   [kernel_ds] "i" (KERNEL_DS),
	   [user_stack] "m" (g_user_stack[sizeof g_user_stack]),
	   [fn]"r"(fn),
	   [param]"D"(param),
	   [kernel_entry_vector]"i"(X80_VECTOR)
	:  "rcx", "rdx");

	asm volatile (
	"pop %"R "cx; pop %"R "dx; pop %"R "bx \n\t"
	"pop %"R "bp; pop %"R "si; pop %"R "di \n\t"
#ifdef __x86_64__
	"pop %r8; pop %r9; pop %r10; pop %r11 \n\t"
	"pop %r12; pop %r13; pop %r14; pop %r15 \n\t"
#endif
	);
	return ret;
}

/**
 * Case name: Interrupt EFLAGS.AC Expose 001
 * Summary  : Enable CR0.AM and RFLAGS.AC then access an unaligned data in RING3
 *            will be generating #AC exception.
 */
#define ENABLE_AM_BIT_IN_CR0()		write_cr0(read_cr0() | X86_CR0_AM);
#define DISABLE_AM_BIT_IN_CR0()		write_cr0(read_cr0() & (~X86_CR0_AM));
#define ENABLE_AC_BIT_IN_RFLAGS()	write_rflags(read_rflags() | X86_EFLAGS_AC);
#define DISABLE_AC_BIT_IN_RFLAGS()	write_rflags(read_rflags() & (~X86_EFLAGS_AC));

static int ring3_24209_routine(void *param)
{
	unsigned long *qbuffer = (unsigned long*)&g_user_stack[1];

	DEBUG("Starting case #AC USER code, 0x%08lx...\n", (unsigned long) qbuffer);

	*qbuffer = 0x0;

	return 0;
}

static void AC_24209_exception_handler(irq_regs_t *regs)
{
	DISABLE_AM_BIT_IN_CR0();
	DISABLE_AC_BIT_IN_RFLAGS();
}
PREPARE_INTERRUPT_HANDLER(AC, 24209, AC_VECTOR, AC_24209_exception_handler);

static int EFLAGS_AC_expose_001(void)
{
	DEBUG("24209.Interrupt EFLAGS.AC Expose 001 testing...\n");
	
	PREPARE_INTERRUPT_MONITOR();
	ENABLE_AM_BIT_IN_CR0();
	ENABLE_AC_BIT_IN_RFLAGS();
	PREPARE_INTERRUPT_GATE(AC_VECTOR, KERNEL_CS, HANDLER(AC, 24209), P1, DPL3, S0);
#ifdef __x86_64__
	PREPARE_DEFAULT_SEGMENT(CS_USER, SEGCD_T_CR, S1, DPL3, P1, DB0, G1);
	PREPARE_DEFAULT_SEGMENT(DS_USER, SEGCD_T_DW, S1, DPL3, P1, DB0, G1);
#else
	PREPARE_DEFAULT_SEGMENT(CS_USER, SEGCD_T_CR, S1, DPL3, P1, DB1, G1);
	PREPARE_DEFAULT_SEGMENT(DS_USER, SEGCD_T_DW, S1, DPL3, P1, DB1, G1);
#endif
	DEBUG("RFLAGS : 0x%08lx\n", read_rflags());
	DEBUG("CR0    : 0x%08lx\n", read_cr0());

	ENTER_TO_RING3(ring3_24209_routine, NULL);

	EXCEPTION_COUNTER_QUERY(AC_VECTOR);
	RECOVERY_INTERRUPT_GATE(AC_VECTOR, KERNEL_CS, DEFAULT(AC));

	if (irqcounter_query(AC_VECTOR) == 1){
		return RESULT_OK;
	}

	return RESULT_FAULT;
}

#define BP_HAVE_INIT		((unsigned int*)0x8000)
#define AP_CPU_COUNT		((unsigned int*)0x8008)
#define BP_INIT_RFLAGS		((unsigned int*)0x800c)
#define AP_STARTUP_RFLAGS	((unsigned int*)0x8010)

static int EFLAGS_AC_following_init_001(void)
{
	unsigned int *bp_init_rflags = BP_INIT_RFLAGS;

	DEBUG("24136.Interrupt EFLAGS.AC following init 001 testing...\n");
	DEBUG("EFLAGS.AC of BP is 0x%08x.\n", *bp_init_rflags);

	if (0 != (*bp_init_rflags & X86_EFLAGS_AC)) {
		return RESULT_FAULT;
	}

	return RESULT_OK;
}

static int EFLAGS_AC_following_startup_001(void)
{
	unsigned int *ap_startup_rflags = AP_STARTUP_RFLAGS;
	unsigned int *ap_cpu_count = AP_CPU_COUNT;
	unsigned int i;

	DEBUG("24134.Interrupt EFLAGS.AC following start-up 001 testing...\n");

	for (i = 0; i < *ap_cpu_count; i++) {
		DEBUG("EFLAGS.AC of AP(%d) is 0x%08x.\n", i, ap_startup_rflags[i]);

		if (0 != (ap_startup_rflags[i] & X86_EFLAGS_AC)) {
			return RESULT_FAULT;
		}
	}

	return RESULT_OK;
}

void system_initialize(void)
{
	irq_disable();
	setup_idt();
	setup_vm();
	irqcounter_initialize();
	exception_initialize();
	systick_initialize();
	irq_enable();
}

void save_unchanged_reg()
{
	irq_disable();
	irq_enable();
}



int main(int argc, char *argv[])
{
	unsigned int *ap_startup_rflags = AP_STARTUP_RFLAGS;
	unsigned int *bp_init_rflags = BP_INIT_RFLAGS;
	unsigned int *ap_cpu_count = AP_CPU_COUNT;
	unsigned int *bp_have_init = BP_HAVE_INIT;
	unsigned int i;
	int result = 0;

	system_initialize();

	DEBUG("bp_have_init       =  0x%08x\n", *bp_have_init);
	DEBUG("ap_cpu_count       =  0x%08x\n", *ap_cpu_count);
	DEBUG("bp_init_rflags     =  0x%08x\n", *bp_init_rflags);

	for (i = 0; i < *ap_cpu_count; i++)
	{
		DEBUG("ap_startup_rflags[%u]   =  0x%08x\n", i, ap_startup_rflags[i]);
	}

	if (*bp_have_init == 0x0f0fa55a)
	{
		result = EFLAGS_AC_following_init_001();
		report("24136.Interrupt EFLAGS.AC following init 001, result 0x%08x\n",
			result == 0, result);

		result = EFLAGS_AC_following_startup_001();
		report("24136.Interrupt EFLAGS.AC following init 001, result 0x%08x\n",
			result == 0, result);
	}

	result = exception_source_expose_000();
	report("27484.Interrupt Exception Source Expose 000(#DE), result 0x%08x",
		result == 0, result);

	result = external_interrupt_sources_injection_exprose_001();
	report("27327.Interrupt External interrupt sources injection exprose 001(#TIMER), result 0x%08x",
		result == 0, result);

	result = interrupt_and_exception_handling_expose_001();
	report("27367.Interrupt interrupt&exception handling expose 001(INT 0x80), result 0x%08x",
		result == 0, result);

	result = NMI_sources_injection_exprose_001();
	report("27341.Interrupt NMI sources injection exprose 001(#NMI), result 0x%08x",
		result == 0, result);

	result = second_DF_001();
	report("24211.Second #DF 001, result 0x%08x", result == 0, result);

	result = simultaneous_exceptions_2nd_including_PF_with_GDT_exception_001();
	report("26813.Interrupt Simultaneous exceptions-2nd including #PF with GDT exception 001, result 0x%08x",
		result == 0, result);

	result = IDT_expose_002_64bit_interrupt_gate();
	report("28831.Interrupt IDT expose 002(64Bit Interrupt-gate), result 0x%08x", result == 0, result);

	result = IDT_expose_004_64bit_trap_gate();
	report("28833.Interrupt IDT expose 004(64Bit Trap-gate), result 0x%08x", result == 0, result);

#ifndef __x86_64__
	result = IDT_expose_001_32bit_task_gate();
	report("28827.IDT expose 001(32BIT Task-gate), result 0x%08x", result == 0, result);
#endif

	result = EFLAGS_AC_expose_001();
	report("24209.Interrupt EFLAGS.AC Expose 001, result 0x%08x", result == 0, result);

#ifdef __x86_64__
	result = shutdown_mode_001();
	report("26113.shutdown mode, result 0x%08x", result == 0, result);
#endif
	while(true);
}
