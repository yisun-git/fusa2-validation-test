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

