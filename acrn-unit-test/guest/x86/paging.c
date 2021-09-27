#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "paging.h"
#include "vm.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "alloc_phys.h"
#include "alloc.h"
#include "misc.h"
#include "register_op.h"
#include "fwcfg.h"
#include "delay.h"

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;
static volatile u64 cr3;

__unused void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}

__unused static void notify_modify_and_read_init_value(int case_id)
{
	cur_case_id = case_id;
	need_modify_init_value = 1;
	/* will change INIT value after AP reboot */
	send_sipi();
	wait_ap_ready();
	/* Will check INIT value after AP reboot again */
	send_sipi();
	wait_ap_ready();
}

#if (defined(IN_NATIVE) || defined(__i386__))
void ap_main(void)
{
	asm volatile ("pause");
}

#elif __x86_64__
typedef void (*ap_init_value_modify)(void);
static void map_first_32m_supervisor_pages();

__unused static void ap_init_value_process(ap_init_value_modify modify_init_func)
{
	if (need_modify_init_value) {
		need_modify_init_value = 0;
		modify_init_func();
		wait_ap = 1;
	} else {
		wait_ap = 1;
	}
}

__unused static void set_paging_efer_nxe()
{
	wrmsr(MSR_EFER, rdmsr(MSR_EFER) | X86_IA32_EFER_NXE);
}

__unused static void set_paging_eflags_ac()
{
	write_rflags(read_rflags() | X86_EFLAGS_AC);
}

__unused static void set_paging_cr4_smep()
{
	/*Enables supervisor-mode execution prevention*/
	map_first_32m_supervisor_pages();
	write_cr4(read_cr4() | X86_CR4_SMEP);
}

__unused static void set_paging_cr4_smap()
{
	/*Enables supervisor-mode access prevention*/
	map_first_32m_supervisor_pages();
	write_cr4(read_cr4() | X86_CR4_SMAP);
}

__unused static void set_paging_cr4_pse()
{
	write_cr4(read_cr4() | X86_CR4_PSE);//
}

__unused static void set_paging_cr4_pge()
{
	write_cr4(read_cr4() | X86_CR4_PGE);
}

__unused static void set_paging_cr4_pae()
{
	write_cr4(read_cr4() | X86_CR4_PAE);
}

__unused static void set_paging_cr0_wp()
{
	write_cr0(read_cr0() | X86_CR0_WP);
}

__unused static void set_paging_cr0_pg()
{
	write_cr0(read_cr0() | X86_CR0_PG);
}

__unused static void set_paging_cr2()
{
	write_cr2(0x100);
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 32265:
		fp = set_paging_efer_nxe;
		ap_init_value_process(fp);
		break;
	case 32270:
		fp = set_paging_eflags_ac;
		ap_init_value_process(fp);
		break;
	case 32272:
		fp = set_paging_cr4_smep;
		ap_init_value_process(fp);
		break;
	case 32274:
		fp = set_paging_cr4_smap;
		ap_init_value_process(fp);
		break;
	case 32276:
		fp = set_paging_cr4_pse;
		ap_init_value_process(fp);
		break;
	case 32280:
		fp = set_paging_cr4_pge;
		ap_init_value_process(fp);
		break;
	case 32297:
		fp = set_paging_cr4_pae;
		ap_init_value_process(fp);
		break;
	case 32304:
		fp = set_paging_cr0_wp;
		ap_init_value_process(fp);
		break;
	case 32306:
		fp = set_paging_cr0_pg;
		ap_init_value_process(fp);
		break;
	case 32302:
		fp = set_paging_cr2;
		ap_init_value_process(fp);
		break;
	case 32300:
	/*
	 * we need not to change cr3's value,when system boot, it has changed its value
	 */
		wait_ap = 1;
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif

static inline u32 get_startup_cr0()
{
	return *((volatile u32 *)STARTUP_CR0_ADDR);
}

static inline u32 get_startup_cr4()
{
	return *((volatile u32 *)STARTUP_CR4_ADDR);
}

static inline u32 get_init_cr0()
{
	return *((volatile u32 *)INIT_CR0_ADDR);
}

static inline u32 get_init_cr4()
{
	return *((volatile u32 *)INIT_CR4_ADDR);
}

static inline int test_instruction_fetch(void *p)
{
	asm volatile(ASM_TRY("1f")
		"call *%[addr]\n\t"
		"1:"
		: : [addr]"r"(p));

	return exception_vector();
}

static inline void free_gva(void *gva)
{
	set_page_control_bit(gva, PAGE_PTE, PAGE_P_FLAG, 1, true);
	free(gva);
}

static inline bool check_value_is_exist(u32 reg, u8 value)
{
	u32 i;

	for (i = 0; i < 4; i++) {
		if (((u8)(reg >> (i * 8)) & 0xff) == value) {
			return true;
		}
	}

	return false;
}

#ifdef __x86_64__
#ifdef IN_NATIVE
#include "64/paging_native_fn.c"
#else
/* test case which should run under 64bit */
#include "64/paging_fn.c"
#endif
#elif __i386__
/* test case which should run under 32bit */
#include "32/paging_fn.c"
#endif

struct test_case {
	int         case_id;
	int         suit_id;
	const char *name;
	void (*fn)(void);
};

static struct test_case cases[] = {
#ifdef __x86_64__
#ifdef IN_NATIVE
#include "64/paging_native_fn.h"
#else
#include "64/paging_fn.h"
#endif
#elif __i386__
#include "32/paging_fn.h"
#endif
};

static void test_paging()
{
	int i;
	struct test_case *c;

	for (i = 0; i < ARRAY_SIZE(cases); i++) {
		c = &cases[i];
		c->fn();
	}
}

static void print_case_list()
{
	int i, j, cnt;
	struct test_case *c;
	const int suits[] = {365, 356, 613, 370, 367, 376, 378, 377};

	printf("There are %zu cases\n", ARRAY_SIZE(cases));
	for (i = 0; i < ARRAY_SIZE(suits); i++) {
		for (j = 0, cnt = 0; j < ARRAY_SIZE(cases); j++) {
			if (cases[j].suit_id == suits[i]) {
				cnt++;
			}
		}
		printf("suite %d contains %d cases\n", suits[i], cnt);
	}

	for (i = 0; i < ARRAY_SIZE(cases); i++) {
		c = &cases[i];
		printf("\t\t Suite ID:%d Case ID:%d case name:%s\n\r", c->suit_id, c->case_id, c->name);
	}
}

int main(int ac, char *av[])
{
	printf("feature: paging\r\n");

	setup_idt();
	setup_vm();
	setup_ring_env();

	print_case_list();
	test_paging();

	return report_summary();
}

