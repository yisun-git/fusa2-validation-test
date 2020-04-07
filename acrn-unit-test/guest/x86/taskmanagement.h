#ifndef __TASK_MANAGEMENT_H
#define __TASK_MANAGEMENT_H

#define XCR0_MASK       0x00000000
#define XCR_XFEATURE_ENABLED_MASK       0x00000000
#define XCR_XFEATURE_ILLEGAL_MASK       0x00000010

#define CPUID_1_ECX_FMA					(1<<12)
#define CPUID_1_ECX_XSAVE			(1 << 26)
#define CPUID_1_ECX_OSXSAVE			(1 << 27)
#define CPUID_1_ECX_AVX	    (1 << 28)

#define X86_CR4_OSXSAVE					(1<<18)

#define XSTATE_FP       0x1
#define XSTATE_SSE      0x2
#define XSTATE_YMM      0x4


#define PASS 0

#define STATE_X87_BIT			0
#define STATE_SSE_BIT			1
#define STATE_AVX_BIT			2
#define STATE_MPX_BNDREGS_BIT		3
#define STATE_MPX_BNDCSR_BIT		4
#define STATE_AVX_512_OPMASK		5
#define STATE_AVX_512_Hi16_ZMM_BIT	7
#define STATE_PT_BIT			8
#define STATE_PKRU_BIT			9
#define STATE_HDC_BIT			13

#define STATE_X87		(1 << STATE_X87_BIT)
#define STATE_SSE		(1 << STATE_SSE_BIT)
#define STATE_AVX		(1 << STATE_AVX_BIT)
#define STATE_MPX_BNDREGS	(1 << STATE_MPX_BNDREGS_BIT)
#define STATE_MPX_BNDCSR	(1 << STATE_MPX_BNDCSR_BIT)
#define STATE_AVX_512		(0x7 << STATE_AVX_512_OPMASK)
#define STATE_PT		(1 << STATE_PT_BIT)
#define STATE_PKRU		(1 << STATE_PKRU_BIT)
#define STATE_HDC		(1 << STATE_HDC_BIT)
#define KBL_NUC_SUPPORTED_XCR0	0x1F
#define XCR0_BIT10_BIT63	0xFFFFFFFFFFFFFC00

#define ALIGNED(x) __attribute__((aligned(x)))
typedef unsigned __attribute__((vector_size(16))) sse128;
typedef float __attribute__((vector_size(16))) avx128;
typedef unsigned __attribute__((vector_size(32))) sse256;


typedef union {
	sse128 sse;
	unsigned u[4];
} sse_union;

typedef union {
	sse256 sse;
	unsigned long u[4];
} avx_union;


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
static int xsetbv_checking(u32 index, u64 value)
{
	u32 eax = value;
	u32 edx = value >> 32;

	asm volatile(ASM_TRY("1f")
		".byte 0x0f,0x01,0xd1\n\t" /* xsetbv */
		"1:"
		: : "a" (eax), "d" (edx), "c" (index));
	return exception_vector();
}

static void write_cr0_ts(u32 bitvalue)
{
	u32 cr0 = read_cr0();
	if (bitvalue) {
		write_cr0(cr0 | X86_CR0_TS);
	} else {
		write_cr0(cr0 & ~X86_CR0_TS);
	}
}

static int write_cr4_checking(unsigned long val)
{
	asm volatile(ASM_TRY("1f")
		"mov %0, %%cr4\n\t"
		"1:" : : "r" (val));
	return exception_vector();
}
static int write_cr4_osxsave(u32 bitvalue)
{
	u32 cr4;

	cr4 = read_cr4();
	if (bitvalue) {
		return write_cr4_checking(cr4 | X86_CR4_OSXSAVE);
	} else {
		return write_cr4_checking(cr4 & ~X86_CR4_OSXSAVE);
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

