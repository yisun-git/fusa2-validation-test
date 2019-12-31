#ifndef MISC_H
#define MISC_H

#define MAX_CASE_ID_LEN 201U

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
extern void set_page_control_bit(void *gva,
	page_level level, page_control_bit bit, u32 value, bool is_invalidate);

extern int write_cr4_exception_checking(unsigned long val);
extern int wrmsr_checking(u32 MSR_ADDR, u64 value);
extern int rdmsr_checking(u32 MSR_ADDR, u64 *result);
extern void send_sipi();
#endif

