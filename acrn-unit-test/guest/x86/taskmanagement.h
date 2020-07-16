#ifndef __TASK_MANAGEMENT_H
#define __TASK_MANAGEMENT_H

#define GDT_NEWTSS_INDEX		0x21

#define XCR_XFEATURE_ENABLED_MASK       0x00000000
#define XCR_XFEATURE_ILLEGAL_MASK       0x00000010

int test_cs;

#define MAIN_TSS_SEL (FIRST_SPARE_SEL + 0)
#define VM86_TSS_SEL (FIRST_SPARE_SEL + 8)
#define CONFORM_CS_SEL  (FIRST_SPARE_SEL + 16)
#define TASK_GATE_SEL 0x48

#define TASK_GATE		(FIRST_SPARE_SEL + 8)
#define TARGET_TSS		(FIRST_SPARE_SEL + 16)
#define ERROR_ADDR	0x5fff0000

tss32_t tss_intr_0;
//static char intr_alt_stack_0[4096];

static volatile int test_count;
static volatile unsigned int test_divider;

//static char *fault_addr;
//static ulong fault_phys;

struct task_gate {
	u16 resv_0;
	u16 selector;
	u8 resv_1 :8;
	u8 type :4;
	u8 system :1;
	u8 dpl :2;
	u8 p :1;
	u16 resv_2;
} gate;

struct tss_desc {
	u16 limit_low;
	u16 base_low;
	u8 base_middle;
	u8 type :4;
	u8 system :1;
	u8 dpl :2;
	u8 p :1;
	u8 granularity;
	u8 base_high;
} *desc_intr = NULL, *desc_main = NULL, *desc_target = NULL;

struct tss_desc desc_intr_backup, desc_cur_backup;

#ifndef __x86_64__
static void write_cr0_ts(u32 bitvalue)
{
	u32 cr0 = read_cr0();
	if (bitvalue) {
		write_cr0(cr0 | X86_CR0_TS);
	} else {
		write_cr0(cr0 & ~X86_CR0_TS);
	}
}

#endif
typedef struct ap_init_regs_struct
{
	volatile u32 cr0;
	volatile u32 cs;
	volatile u32 ftw;
	volatile u32 reserved;
	volatile u32 fpop;
} ap_init_regs_t;

typedef struct bp_startup_regs_struct
{
	volatile u32 cr0;
	volatile u32 fsw;
	volatile u32 ftw;
	volatile u32 reserved;
	volatile u32 fpop;
} ap_startup_regs_t;

#endif

