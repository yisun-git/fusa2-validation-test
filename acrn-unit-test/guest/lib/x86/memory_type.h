#ifndef __MEMORY_TYPE_H__
#define __MEMORY_TYPE_H__
#include "processor.h"
#include "vm.h"
#include "misc.h"

#define CR0_BIT_NW					29
#define CR0_BIT_CD					30
#define CR0_BIT_PG					31

#define CR3_BIT_PWT					3
#define CR3_BIT_PCD					4

#define CR4_BIT_PAE					5
#define CR4_BIT_PGE					7

#define IA32_PAT_MSR				0x00000277
#define IA32_MISC_ENABLE			0x000001A0
#define IA32_MTRR_DEF_TYPE			0x000002FF
#define IA32_MTRRCAP_MSR			0x000000FE
#define IA32_SMRR_PHYSBASE_MSR		0x000001F2
#define IA32_SMRR_PHYSMASK_MSR		0x000001F3

#define IA32_MTRR_PHYSBASE0			0x00000200
#define IA32_MTRR_PHYSMASK0			0x00000201
#define IA32_MTRR_PHYSBASE1			0x00000202
#define IA32_MTRR_PHYSMASK1			0x00000203
#define IA32_MTRR_PHYSBASE2			0x00000204
#define IA32_MTRR_PHYSMASK2			0x00000205
#define IA32_MTRR_PHYSBASE3			0x00000206
#define IA32_MTRR_PHYSMASK3			0x00000207
#define IA32_MTRR_PHYSBASE4			0x00000208
#define IA32_MTRR_PHYSMASK4			0x00000209
#define IA32_MTRR_PHYSBASE5			0x0000020A
#define IA32_MTRR_PHYSMASK5			0x0000020B
#define IA32_MTRR_PHYSBASE6			0x0000020C
#define IA32_MTRR_PHYSMASK6			0x0000020D
#define IA32_MTRR_PHYSBASE7			0x0000020E
#define IA32_MTRR_PHYSMASK7			0x0000020F
#define IA32_MTRR_PHYBASE(i)		(IA32_MTRR_PHYSBASE0+i*2)
#define IA32_MTRR_PHYMASK(i)		(IA32_MTRR_PHYSMASK0+i*2)

#define IA32_MTRR_FIX64K_00000		0x00000250
#define IA32_MTRR_FIX16K_80000		0x00000258
#define IA32_MTRR_FIX4K_C0000		0x00000268

#define PT_PWT						3
#define PT_PCD						4
#define PT_PAT						7
#define PT_PAT_LARGE_PAGE			12

#define PT_PWT_MASK					(1ull << (PT_PWT))
#define PT_PCD_MASK					(1ull << (PT_PCD))
#define PT_PAT_MASK					(1ull << (PT_PAT))

/* init pat to 0x0000000001040506*/
#define PT_MEMORY_TYPE_MASK0		0	/* wb */
#define PT_MEMORY_TYPE_MASK1		(PT_PWT_MASK)	/* wp */
#define PT_MEMORY_TYPE_MASK2		(PT_PCD_MASK)	/* wt */
#define PT_MEMORY_TYPE_MASK3		(PT_PWT_MASK|PT_PCD_MASK)	/* wc */
#define PT_MEMORY_TYPE_MASK4		(PT_PAT_MASK)		/* uc */
#define PT_MEMORY_TYPE_MASK5		(PT_PAT_MASK|PT_PWT_MASK)	/* uc */
#define PT_MEMORY_TYPE_MASK6		(PT_PAT_MASK|PT_PCD_MASK)	/* uc */
#define PT_MEMORY_TYPE_MASK7		(PT_PAT_MASK|PT_PCD_MASK|PT_PWT_MASK)	/* uc */

/* default PAT entry value 0007040600070406 */
#define IA32_PAT_STARTUP_VALUE	0x0007040600070406
#ifdef __x86_64__
#define CACHE_TYPE_UC	0x0UL;
#define CACHE_TYPE_WB	0x0606060606060600UL;
#define CACHE_TYPE_WC	0x0101010101010100UL;
#define CACHE_TYPE_WT	0x0404040404040400UL;
#define CACHE_TYPE_WP	0x0505050505050500UL;

#define CACHE_LINE_SIZE		64UL
#define CACHE_4k_SIZE		0x200UL		/* 4k/8 */
#define CACHE_L1_SIZE		0x800UL		/* 16K/8 */
#define CACHE_L2_SIZE		0x4000UL	/* 128K/8 */
#define CACHE_L3_SIZE		0x80000UL	/* 4M/8 */
#define CACHE_OVE_L3_SIZE	0x200000UL	/* 16M/8 */
#define CACHE_MALLOC_SIZE	0x200000UL	/* 16M/8 */
#elif defined(__i386__)
#define CACHE_TYPE_UC	0x0UUL;
#define CACHE_TYPE_WB	0x0606060606060600ULL;
#define CACHE_TYPE_WC	0x0101010101010100ULL;
#define CACHE_TYPE_WT	0x0404040404040400ULL;
#define CACHE_TYPE_WP	0x0505050505050500ULL;

#define CACHE_LINE_SIZE		64ULL
#define CACHE_4k_SIZE		0x200ULL		/* 4k/8 */
#define CACHE_L1_SIZE		0x800ULL		/* 16K/8 */
#define CACHE_L2_SIZE		0x4000ULL		/* 128K/8 */
#define CACHE_L3_SIZE		0x80000ULL		/* 4M/8 */
#define CACHE_OVE_L3_SIZE	0x200000ULL		/* 16M/8 */
#define CACHE_MALLOC_SIZE	0x200000ULL		/* 16M/8 */
#endif
#define CACHE_PAT_UC		0x0
#define CACHE_PAT_WC		0x1
#define CACHE_PAT_WT		0x4
#define CACHE_PAT_WP		0x5
#define CACHE_PAT_WB		0x6

void asm_mfence();
void asm_read_access_memory(u64 *p);
void asm_clwb(void *target);
void asm_wbinvd();
void asm_wbinvd_lock(void *data);
void asm_invd();
void asm_invd_lock(void *data);
void asm_write_access_memory(unsigned long address, unsigned long value);
void asm_clflush(long unsigned int addr);
void asm_clflush_lock(long unsigned int addr);
void asm_clflushopt_lock(long unsigned int addr);
void asm_clflushopt_lock_f2(long unsigned int addr);
void asm_clflushopt_lock_f3(long unsigned int addr);
unsigned long long asm_read_tsc(void);
void asm_mfence_wbinvd();
void asm_mfence_invd();
void write_cr0_bybit(u32 bit, u32 bitvalue);
void set_memory_type_pt(void *address, u64 type, u64 size);
void flush_tlb();
void mem_cache_reflush_cache();
void set_mem_cache_type_addr(void *address, u64 cache_type, u64 size);
void set_mem_cache_type_all(u64 cache_type);
__attribute__((aligned(64))) u64 read_mem_cache_test_addr(u64 *address, u64 size);
__attribute__((aligned(64))) u64 write_mem_cache_test_addr(u64 *address, u64 size);

#endif
