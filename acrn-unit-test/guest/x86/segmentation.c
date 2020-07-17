#include "segmentation.h"
#include "instruction_common.h"
#include "debug_print.h"
#include "desc.h"
#include <regdump.h>
#include <asm/spinlock.h>
#define USE_HAND_EXECEPTION

/* bit test */
#define BIT_IS(NUMBER, N) ((((uint64_t)NUMBER) >> N) & (0x1))

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

typedef void (*trigger_func)(void *data);
typedef void (*call_gate_fun)(void);

void lss_index_0(void *data)
{
	struct lseg_st lss;

	lss.offset = 0xffffffff;
	/*index =0 RPL =0 */
	lss.selector = SELECTOR_INDEX_0H;

	asm volatile(
		"lss  %0, %%eax\t\n"
		::"m"(lss)
	);
}

void lss_rpl_2_index_0(void *data)
{
	struct lseg_st lss;

	lss.offset = 0xffffffff;
	/*index =0 RPL =2 */
	lss.selector = SELECTOR_INDEX_2H;

	asm volatile(
		"lss  %0, %%eax\t\n"
		::"m"(lss)
	);
}

void lss_index_48(void *data)
{
	struct lseg_st lss;

	lss.offset = 0xffffffff;
	/* index =0x30 RPL=0 */
	lss.selector = SELECTOR_INDEX_180H;

	asm volatile(
		"lss  %0, %%eax\t\n"
		::"m"(lss)
	);
}

void lss_index_80(void *data)
{
	struct lseg_st lss;

	lss.offset = 0xffffffff;
	/* index =0x50 RPL=0*/
	lss.selector = SELECTOR_INDEX_280H;

	asm volatile(
		"lss  %0, %%eax\t\n"
		::"m"(lss)
	);
}

void lss_index_rpl_1_80(void *data)
{
	struct lseg_st lss;

	lss.offset = 0xffffffff;
	/*index =0x50 RPL =1 */
	lss.selector = SELECTOR_INDEX_281H;

	asm volatile(
		"lss  %0, %%eax\t\n"
		::"m"(lss)
	);
}

void lss_index_1024(void *data)
{
	struct lseg_st lss;

	lss.offset = 0xffffffff;
	/*max index 1024, RPL =0 */
	lss.selector = SELECTOR_INDEX_2000H;

	asm volatile(
		"lss  %0, %%eax\t\n"
		::"m"(lss)
	);
}

void lfs_index_48(void *data)
{
	struct lseg_st lfs;

	lfs.offset = 0xffffffff;
	/*index =0x30 RPL =0 */
	lfs.selector = SELECTOR_INDEX_180H;

	asm volatile(
		"lfs  %0, %%eax\t\n"
		::"m"(lfs)
	);
}

void lfs_index_80(void *data)
{
	struct lseg_st lfs;

	lfs.offset = 0xffffffff;
	/*index =0x50 RPL =0 */
	lfs.selector = SELECTOR_INDEX_280H;

	asm volatile(
		"lfs  %0, %%eax\t\n"
		::"m"(lfs)
	);
}

void lfs_rpl_3_index_48(void *data)
{
	struct lseg_st lfs;

	lfs.offset = 0xffffffff;
	/* index = 0x30 RPL =3 */
	lfs.selector = SELECTOR_INDEX_183H;

	asm volatile(
		"lfs  %0, %%eax\t\n"
		::"m"(lfs)
	);
}

void lfs_rpl_3_index_80(void *data)
{
	struct lseg_st lfs;

	lfs.offset = 0xffffffff;
	/* index = 0x50 RPL =3 */
	lfs.selector = SELECTOR_INDEX_283H;

	asm volatile(
		"lfs  %0, %%eax\t\n"
		::"m"(lfs)
	);
}

void lfs_index_1024(void *data)
{
	struct lseg_st lfs;

	lfs.offset = 0xffffffff;
	/*max index 1024, RPL =0 */
	lfs.selector = SELECTOR_INDEX_2000H;

	asm volatile(
		"lfs  %0, %%eax\t\n"
		::"m"(lfs)
	);
}

#if !defined(IN_NATIVE)
#ifdef __x86_64__
/*test case which should run under 64bit  */
#include "64/segmentation_fn.c"
#elif __i386__
/*test case which should run  under 32bit  */
#include "32/segmentation_fn.c"
#endif

static void print_case_list(void)
{
#ifndef __x86_64__	/*386*/
	print_case_list_32();
#else			/*_x86_64__*/
	print_case_list_64();
#endif
}

static void test_segmentation(void)
{
#ifndef __x86_64__	/*386*/
	test_segmentation_32();
#else				/*x86_64__*/
	test_segmentation_64();
#endif
}

#define MSR_BASE 10
#define GDTR_BASE 40
#define GDT_BASE 50
#define ACCESS_BASE 80

#define DUMP_MAGIC  0xDEADBEAF

#if defined(BP_STARTUP_CHECK)
static void verify_bp_dump_info(void)
{
	uint32_t *dump = (u32 *)0x6000;

	printf("Verify Segmentation Initialization Check TCs for BP:\n");

	if (dump[0] != DUMP_MAGIC) {
		printf("dumped magic damaged (0x%x!=0x%x), all initialial values cases could be failed!\n",
			dump[0], DUMP_MAGIC);
		return;
	}
	printf("dump buffer magic number (0x%x) correct!\n", DUMP_MAGIC);

	uint32_t *p_reg = dump + 1;

	printf("check regs, dumped @%p, cs = 0x%x, ds = 0x%x\n", dump, p_reg[REG_CS], p_reg[REG_DS]);

	report("Test Case: 29056", p_reg[REG_CS] == 0x10); //0X10
	report("Test Case: 29053", p_reg[REG_DS] == 0x18); //0x18
	report("Test Case: 29050", p_reg[REG_ES] == 0x18);
	report("Test Case: 29026", p_reg[REG_FS] == 0x00);
	report("Test Case: 29047", p_reg[REG_GS] == 0x00);
	report("Test Case: 29024", p_reg[REG_SS] == 0x18);
	report("Test Case: 26110", (p_reg[REG_CR4] & CR4_FSGSBASE) == 0);

	uint64_t *p_msr = (uint64_t *)(dump + MSR_BASE);

	printf("\ncheck MSRs, dumped @%p\n", p_msr);
	report("Test Case: 29532", p_msr[E_IA32_SYSENTER_CS] == 0x00);
	report("Test Case: 29535", p_msr[E_IA32_SYSENTER_ESP] == 0x00);
	report("Test Case: 29537", p_msr[E_IA32_SYSENTER_EIP] == 0x00);

	report("Test Case: 29530", (p_msr[E_IA32_EFER] & EFER_SCE) == 0x00);

	report("Test Case: 29539", p_msr[E_IA32_STAR] == 0x00);
	report("Test Case: 29541", p_msr[E_IA32_LSTAR] == 0x00);
	report("Test Case: 29546", p_msr[E_IA32_CSTAR] == 0x00);
	report("Test Case: 29544", p_msr[E_IA32_FMASK] == 0x00);

	report("Test Case: 26139", p_msr[E_IA32_FS_BASE] == 0x00);
	report("Test Case: 27143", p_msr[E_IA32_GS_BASE] == 0x00);
	report("Test Case: 27133", p_msr[E_IA32_KERNEL_GS_BASE] == 0x00);


	struct gdtr32_t *gdtr32 = (struct gdtr32_t *)(dump + GDTR_BASE);

	printf("\ndumped GDTR@%p: base=0x%x, limit=%d\n", gdtr32, gdtr32->base, gdtr32->limit);
	printf("LDTR selector = 0x%x\n", (u16)dump[GDTR_BASE + 3]);
	printf("dump GDT rep args: ecx = %d, esi=0x%x, edi=0x%x, (check:%p)\n", dump[GDTR_BASE + 4],
		dump[GDTR_BASE + 5], dump[GDTR_BASE + 6], dump + GDT_BASE);

	for (uint32_t i = 0; i < ((gdtr32->limit + 1) / 4); i += 2) {
		printf("GDT[%d]: %08x %08x\n", i/2, dump[GDT_BASE + i + 1], dump[GDT_BASE + i]);
	}

	//for LDTR test cases
	report("Test Case: 29065", (u16)dump[GDTR_BASE + 3] == 0);

	//for GDTR test cases, verify its base and limit
	u32 *p_base = (u32 *)(u64)(gdtr32->base);
	u32 orig_base_val = *p_base;
	*p_base = 0x55AAAA55;
	report("Test Case: 29512", (*p_base == 0x55AAAA55) && (gdtr32->limit == 0x1F)); //on ACRN it is 0x1F
	*p_base = orig_base_val;

	uint64_t *p_1st_entry = (uint64_t *)&dump[GDT_BASE]; // first entry of GDT
	uint64_t *p_2nd_entry = (uint64_t *)&dump[GDT_BASE + 2]; // second entry of GDT
	uint64_t *p_3rd_entry = (uint64_t *)&dump[GDT_BASE + 4];
	uint64_t *p_4th_entry = (uint64_t *)&dump[GDT_BASE + 6];
	//target CS: base=0, limit=f ffff, type=1011b, S=1b, DPL=00b, P=1b, AVL=0b, L=0b, G=1b, DB=1b
	uint64_t target_cs = 0x00cf9b000000ffff;
	  //target DS: base=0, limit=f ffff, type=0011b, S=1b, DPL=00b, P=1b, AVL=0b, L=0b, G=1b, DB=1b
	uint64_t target_ds = 0x00cf93000000ffff;

	// The 3rd item should be same as target_cs; the 4th item should be same as target_ds
	report("Test Case: 29528", (*p_1st_entry == 0) && (*p_2nd_entry == 0)
		&& (*p_3rd_entry == target_cs) && (*p_4th_entry == target_ds));

#if 0 //not used now
	//the case for check regment access
	uint32_t *p_access_seg = dump + ACCESS_BASE;
	report("Test Case: XXXX", (p_access_seg[0] == DUMP_MAGIC) && (p_access_seg[1] == DUMP_MAGIC) &&
		(p_access_seg[2] == DUMP_MAGIC) && (p_access_seg[3] == DUMP_MAGIC) &&
		(p_access_seg[4] == DUMP_MAGIC) && (p_access_seg[5] == DUMP_MAGIC));
#endif
}
#endif

#if defined(AP_INIT_CHECK)
static void verify_ap_dump_info(void)
{
	uint32_t *dump = (u32 *)0x7000;

	printf("Verify Segmentation Initialization Check TCs for AP:\n");

	if (dump[0] != DUMP_MAGIC) {
		printf("dumped magic damaged (0x%x!=0x%x), all initialial values cases could be failed!\n",
			dump[0], DUMP_MAGIC);
		return;
	}

	printf("dump buffer magic number (0x%x) correct!\n", DUMP_MAGIC);

	uint32_t *p_reg = dump + 1;

	printf("check regs, dumped @%p, cs = 0x%x, ds = 0x%x\n", dump, p_reg[REG_CS], p_reg[REG_DS]);

	//report("Test Case: 29056", p_reg[REG_CS] == 0); //0X10

	report("Test Case: 27194", p_reg[REG_DS] == 0); //0x18
	report("Test Case: 27190", p_reg[REG_ES] == 0);
	report("Test Case: 27189", p_reg[REG_FS] == 0);
	report("Test Case: 27187", p_reg[REG_GS] == 0);
	report("Test Case: 27186", p_reg[REG_SS] == 0);
	report("Test Case: 29064", (p_reg[REG_CR4] & CR4_FSGSBASE) == 0);

	uint64_t *p_msr = (uint64_t *)(dump + MSR_BASE);

	printf("\ncheck MSRs, dumped @%p\n", p_msr);
	report("Test Case: 29534", p_msr[E_IA32_SYSENTER_CS] == 0x00);
	report("Test Case: 29536", p_msr[E_IA32_SYSENTER_ESP] == 0x00);
	report("Test Case: 29538", p_msr[E_IA32_SYSENTER_EIP] == 0x00);

	report("Test Case: 29531", (p_msr[E_IA32_EFER] & EFER_SCE) == 0x00);

	report("Test Case: 29540", p_msr[E_IA32_STAR] == 0x00);
	report("Test Case: 29542", p_msr[E_IA32_LSTAR] == 0x00);
	report("Test Case: 29547", p_msr[E_IA32_CSTAR] == 0x00);
	report("Test Case: 29545", p_msr[E_IA32_FMASK] == 0x00);

	report("Test Case: 29061", p_msr[E_IA32_FS_BASE] == 0x00);
	report("Test Case: 29059", p_msr[E_IA32_GS_BASE] == 0x00);
	report("Test Case: 29055", p_msr[E_IA32_KERNEL_GS_BASE] == 0x00);


	struct gdtr32_t *gdtr32 = (struct gdtr32_t *)(dump + GDTR_BASE);

	printf("\ndumped GDTR@%p: base=0x%x, limit=%d\n", gdtr32, gdtr32->base, gdtr32->limit);
//	printf("LDTR = %d\n", dump[GDTR_BASE + 3]);

	//for GDTR test cases:
	report("Test Case: 29527", (gdtr32->base == 0) && (gdtr32->limit == 0xFFFF));

}
#endif

static void check_cpuid(void)
{
	printf("check CPUID:\n");

	uint32_t ebx = cpuid(0x07).b;

	report("Test Case: 29552", ((ebx & 0x01) == 0x01));

	uint32_t edx = cpuid(0x01).d;
	report("Test Case: 29553", ((edx & (1 << 11)) != 0));

	edx = cpuid(0x80000001).d;
	report("Test Case: 29554", ((edx & (1 << 11)) != 0));
}


#endif


#if defined(IN_NATIVE)
#if defined(__x86_64__)
static inline void wrfsbase(uint64_t val)
{
	asm volatile ("wrfsbase %0" : : "r"(val) : "memory");
}

static inline uint64_t rdfsbase(void)
{
	uint64_t ret;
	asm volatile ("rdfsbase %0" : "=r"(ret) : : "memory");

	return ret;
}

static inline void wrgsbase(uint64_t val)
{
	asm volatile ("wrgsbase %0" : : "r"(val) : "memory");
}

static inline uint64_t rdgsbase(void)
{
	uint64_t ret;
	asm volatile ("rdgsbase %0" : "=r"(ret) : : "memory");

	return ret;
}

static void verify_fsgs_base(void)
{
	//for FSGSBASE, cpuid.07.ebx.bit0 Supports RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE if 1
	//CR4: bit16 to enable/disable it
	uint32_t ebx = cpuid(0x07).b;
	uint32_t cr4 = read_cr4();

	write_cr4(cr4 | (1 << 16)); // to enable fsgsbase

	uint64_t orig_fsbase = rdfsbase();
	wrfsbase(10);
	uint64_t changed_fsbase = rdfsbase(); //rdmsr(MSR_FS_BASE) same effect
	wrfsbase(orig_fsbase);

	uint64_t orig_gsbase = rdgsbase();
	wrgsbase(10);
	uint64_t changed_gsbase = rdgsbase(); //rdmsr(MSR_GS_BASE) same effect
	wrgsbase(orig_fsbase);


	printf("orig fsbase: 0x%lx, changed to: 0x%lx\n", orig_fsbase, changed_fsbase);
	printf("orig gsbase: 0x%lx, changed to: 0x%lx\n", orig_gsbase, changed_gsbase);

	printf("CPUID-07-EBX: 0x%x, CR4:0x%x, changed to 0x%x\n", ebx, cr4, (uint32_t)read_cr4());
	report("Test Case: 35200", ((ebx & 0x01) == 0x01) && (changed_fsbase == 10) &&
		(changed_gsbase == 10));

}
#endif

#if defined(__x86_64__)
//test syscall/sysret (64bits)
static u64 ring3_rsp, ring3_rip;
static bool syscall_ret_worked = false;

void syscall_test_func(void)
{
	printf("syscall in now cs=0x%x, ss=0x%x, sysret: rsp:0x%lx, rip:0x%lx\n",
		read_cs(), read_ss(), ring3_rsp, ring3_rip);

	syscall_ret_worked = true;
}

asm("syscall_door: \n"
	"call syscall_test_func\n"
	"mov ring3_rip, %rcx\n"
	"rex.w sysret\n"
	);
void syscall_door(void);

static void sys_call(const char *args)
{
	printf("current in ring3 cs=0x%x, ss=0x%x\n", read_cs(), read_ss());

	//before sysenter, need save rsp and eip here to restore when come back.
	asm volatile(
		"mov %%rsp, %0\n"
		"mov $1f, %%rdi\n"
		"mov %%rdi, %1\n"
		"syscall\n"
		"1:\n"
		::"m"(ring3_rsp), "m"(ring3_rip)
	);

	printf("after syscall, back to ring3 cs=0x%x, ss=0x%x\n", read_cs(), read_ss());
}

static void verify_syscall_sysret(void)
{
	printf("before syscall, in ring0 cs=0x%x, ss=0x%x\n", read_cs(), read_ss());

	u64 cs_ss = ((u64)KERNEL_CS64 << 32) | ((u64)(USER_CS64 - 16) << 48);
	wrmsr(IA32_STAR, cs_ss);
	wrmsr(IA32_LSTAR, (u64)(syscall_door));

	printf("IA32_FMASK = 0x%lx\n", rdmsr(IA32_FMASK));

	do_at_ring3(sys_call, NULL);

	report("syscall/ret work: Test Case: 37959", syscall_ret_worked);
	printf("after do_at_ring3, in ring0 cs=0x%x, ss=0x%x\n", read_cs(), read_ss());

}

#else //!defined(__x86_64__)
static u32 ring3_esp, ring3_eip;
static bool sysenter_exit_worked = false;

void sysenter_test_func(void)
{
	printf("sysenter now in ring0: cs=0x%x, ss=0x%x\n, sysexit: esp:0x%x, eip:0x%x\n",
		read_cs(), read_ss(), ring3_esp, ring3_eip);

	sysenter_exit_worked = true;
}

asm("sysenter_call_door: \n"
	"call sysenter_test_func\n"
	"mov ring3_esp, %ecx\n"
	"mov ring3_eip, %edx\n"
	"sysexit\n"
	);
void sysenter_call_door(void);

static void call_sysenter(const char *args)
{
	printf("before sysenter: in ring3 cs=0x%x, ss=0x%x\n", read_cs(), read_ss());

	//before sysenter, need save rsp and eip here to restore when come back.
	asm volatile(
		"mov %%esp, %0\n"
		"mov $1f, %%edi\n"
		"mov %%edi, %1\n"
		"sysenter\n"
		"1:\n"
		::"m"(ring3_esp), "m"(ring3_eip)
	);

	printf("after sysenter, back to ring3 cs=0x%x, ss=0x%x\n", read_cs(), read_ss());
}

static void verify_sysenter_sysexit(void)
{
	static unsigned char user_stack[4096] __attribute__((aligned(16)));

	printf("before call do_at_ring3, in ring0 cs=0x%x, ss=0x%x\n", read_cs(), read_ss());

	wrmsr(IA32_SYSENTER_CS, KERNEL_CS16);
	wrmsr(IA32_SYSENTER_ESP, (u32)(user_stack + 4096));
	wrmsr(IA32_SYSENTER_EIP, (u32)sysenter_call_door);

	do_at_ring3(call_sysenter, NULL);

	report("sysenter/exit work: Test Case: 37949", sysenter_exit_worked);
	printf("after do_at_ring3, in ring0 cs=0x%x, ss=0x%x\n", read_cs(), read_ss());
}
#endif

static void verify_constraints(void)
{
#if defined(__x86_64__)
	verify_fsgs_base();

	// CPUID with EAX = 01, check return EDX.SEP (bit 11)
	uint32_t edx = cpuid(0x01).d;
	report("Test Case: 35198", ((edx & (1 << 11)) != 0));

	//SYSCALL and SYSRET are available (CPUID.80000001H.EDX[bit 11] = 1)
	edx = cpuid(0x80000001).d;
	report("Test Case: 35199", ((edx & (1 << 11)) != 0));

	//SYSCALL Enable: IA32_EFER.SCE (R/W) (bit0) Enables SYSCALL/SYSRET instructions in 64-bit mode.
	uint32_t orig_efer = rdmsr(0xC0000080); //IA32_EFER;
	wrmsr(0xC0000080, orig_efer | 0x01);
	uint32_t changed_efer = rdmsr(0xC0000080);
	printf("IA32_EFER: orig=0x%x, changed to 0x%x\n", orig_efer, changed_efer);

	report("Test Case: 37960", ((orig_efer & 0X01) == 0) && ((changed_efer & 0x01) == 0x01));

	verify_syscall_sysret();
#else
	verify_sysenter_sysexit();
#endif
}
#endif

static volatile bool bp_test_done = false;

int main(void)
{
	setup_vm();

	setup_ring_env();

	setup_idt();

#if defined(IN_NATIVE)
	verify_constraints();

#else

#if defined(BP_STARTUP_CHECK)
	verify_bp_dump_info();
#endif

	check_cpuid();

	print_case_list();
	test_segmentation();

#endif
	report_summary();

	bp_test_done = true;

	return 0;
}

static struct spinlock ap_lock;

void ap_main(void)
{
	static bool run_once = false; //just run test code on one AP

	//wait BP test done, then test AP
	while (!bp_test_done);

	spin_lock(&ap_lock);

	if (!run_once) {
		run_once = true;
#if defined(AP_INIT_CHECK) && !defined(IN_NATIVE)
		printf("AP-test start:\n");
		verify_ap_dump_info();
		report_summary();
#endif
	}

	spin_unlock(&ap_lock);
}
