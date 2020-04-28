#ifndef MISC_H
#define MISC_H

#define MAX_CASE_ID_LEN 201U

#define RING1_CS32_GDT_DESC	(0x00cfbb000000ffffULL)
#define RING1_CS64_GDT_DESC	(0x00afbb000000ffffULL)
#define RING1_DS_GDT_DESC	(0x00cfb3000000ffffULL)

#define RING2_CS32_GDT_DESC	(0x00cfdb000000ffffULL)
#define RING2_CS64_GDT_DESC	(0x00afdb000000ffffULL)
#define RING2_DS_GDT_DESC	(0x00cfd3000000ffffULL)

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

extern volatile bool ring1_ret;
extern volatile bool ring2_ret;
extern volatile bool ring3_ret;
extern unsigned char kernel_entry;
extern unsigned char kernel_entry1;
extern unsigned char kernel_entry2;

extern void set_page_control_bit(void *gva,
	page_level level, page_control_bit bit, u32 value, bool is_invalidate);

extern int write_cr4_exception_checking(unsigned long val);
extern int wrmsr_checking(u32 MSR_ADDR, u64 value);
extern int rdmsr_checking(u32 MSR_ADDR, u64 *result);
extern void send_sipi();
extern uint32_t get_lapic_id(void);
extern int do_at_ring1(void (*fn)(const char *), const char *arg);
extern int do_at_ring2(void (*fn)(const char *), const char *arg);
extern int do_at_ring3(void (*fn)(const char *), const char *arg);
extern void setup_ring_env();
#endif

