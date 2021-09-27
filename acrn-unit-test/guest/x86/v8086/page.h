#ifndef __PAGE_H__
#define __PAGE_H__

typedef u32 bool;
typedef u32 pteval_t;
typedef u32 pmdval_t;
typedef u32 pgdval_t;
typedef struct { pteval_t pte; } pte_t;
typedef struct { pgdval_t pgd; } pgd_t;
typedef struct { pteval_t pgprot; } pgprot_t;

#define NULL ((void *)(0))
#define __AC(X, Y)	(X##Y)
#define _AC(X, Y)	__AC(X, Y)

#define PAGE_SHIFT		12
#define PAGE_SIZE		(_AC(1, UL) << PAGE_SHIFT)
#define PAGE_MASK		(~(PAGE_SIZE-1))

#define	PAGE_LEVEL	2
#define	PGDIR_WIDTH	10
#define	PGDIR_MASK	1023

#define PGDIR_BITS(lvl)        (((lvl) - 1) * PGDIR_WIDTH + PAGE_SHIFT)
#define PGDIR_OFFSET(va, lvl)  (((va) >> PGDIR_BITS(lvl)) & PGDIR_MASK)

typedef enum page_control_bit {
	PAGE_P_FLAG = 0,
	PAGE_WRITE_READ_FLAG = 1,
	PAGE_USER_SUPER_FLAG = 2,
	PAGE_PWT_FLAG = 3,
	PAGE_PCM_FLAG = 4,
	PAGE_PS_FLAG = 7,
	PAGE_PTE_GLOBAL_PAGE_FLAG = 8,
	PAGE_XD_FLAG = 63,
} page_control_bit;

typedef enum page_level {
	PAGE_PTE = 1,
	PAGE_PDE,
	PAGE_PDPTE,
	PAGE_PML4,
} page_level;


#endif