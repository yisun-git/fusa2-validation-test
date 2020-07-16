/**
 * Test for x86 cache and memory cache control
 *
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "alloc.h"
#include "alloc_phys.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "asm/spinlock.h"
#include "vm.h"
#include "misc.h"
#include "types.h"
#include "apic.h"
#include "isr.h"
#include "fwcfg.h"
#include "memory_type.h"
#include "debug_print.h"

/*#define CACHE_IN_NATIVE*/

#define CACHE_TEST_TIME_MAX			40

static volatile int ud;
static volatile int isize;

/* default PAT entry value 0007040600070406 */
u64 cache_type_UC = CACHE_TYPE_UC;
u64 cache_type_WB = CACHE_TYPE_WB;
u64 cache_type_WC = CACHE_TYPE_WC;
u64 cache_type_WT = CACHE_TYPE_WT;
u64 cache_type_WP = CACHE_TYPE_WP;

u64 cache_line_size = CACHE_LINE_SIZE;

u64 cache_4k_size = CACHE_4k_SIZE;			/* 4k/8 */
u64 cache_l1_size = CACHE_L1_SIZE;			/* 16K/8 */
u64 cache_l2_size = CACHE_L2_SIZE;			/* 128K/8 */
u64 cache_l3_size = CACHE_L3_SIZE;			/* 4M/8 */
u64 cache_over_l3_size = CACHE_OVE_L3_SIZE;	/* 16M/8 */
u64 cache_malloc_size = CACHE_MALLOC_SIZE;			/* 16M/8 */

u64 *cache_test_array = NULL;
u64 tsc_delay[CACHE_TEST_TIME_MAX] = {0,};
u64 tsc_delay_before[CACHE_TEST_TIME_MAX] = {0,};
u64 tsc_delay_after[CACHE_TEST_TIME_MAX] = {0,};
u64 tsc_delay_delta[CACHE_TEST_TIME_MAX] = {0,};
u64 tsc_delay_delta_total = 0;
u64 tsc_delay_delta_stdev = 0;

#define ERROR_RANG		5

enum cache_size_type {
	CACHE_L1_READ_UC = 0,
	CACHE_L1_READ_WB,
	CACHE_L1_READ_WT,
	CACHE_L1_READ_WC,
	CACHE_L1_READ_WP,

	CACHE_L2_READ_UC = 5,
	CACHE_L2_READ_WB,
	CACHE_L2_READ_WT,
	CACHE_L2_READ_WC,
	CACHE_L2_READ_WP,

	CACHE_L3_READ_UC = 10,
	CACHE_L3_READ_WB,
	CACHE_L3_READ_WT,
	CACHE_L3_READ_WC,
	CACHE_L3_READ_WP,

	CACHE_OVER_L3_READ_UC = 15,
	CACHE_OVER_L3_READ_WB,
	CACHE_OVER_L3_READ_WT,
	CACHE_OVER_L3_READ_WC,
	CACHE_OVER_L3_READ_WP,

	CACHE_L1_WRITE_UC = 20,
	CACHE_L1_WRITE_WB,
	CACHE_L1_WRITE_WT,
	CACHE_L1_WRITE_WC,
	CACHE_L1_WRITE_WP,

	CACHE_L2_WRITE_UC = 25,
	CACHE_L2_WRITE_WB,
	CACHE_L2_WRITE_WT,
	CACHE_L2_WRITE_WC,
	CACHE_L2_WRITE_WP,

	CACHE_L3_WRITE_UC = 30,
	CACHE_L3_WRITE_WB,
	CACHE_L3_WRITE_WT,
	CACHE_L3_WRITE_WC,
	CACHE_L3_WRITE_WP,

	CACHE_OVER_L3_WRITE_UC = 35,
	CACHE_OVER_L3_WRITE_WB,
	CACHE_OVER_L3_WRITE_WT,
	CACHE_OVER_L3_WRITE_WC,
	CACHE_OVER_L3_WRITE_WP,

	CACHE_DEVICE_4K_READ = 40,
	CACHE_4K_READ,
	CACHE_DEVICE_4K_WRITE,
	CACHE_4K_WRITE,

	CACHE_CLFLUSH_DIS_READ = 44,
	CACHE_CLFLUSH_READ,
	CACHE_CLFLUSHOPT_DIS_READ,
	CACHE_CLFLUSHOPT_READ,
	CACHE_WBINVD_DIS_READ,
	CACHE_WBINVD_READ,

	CACHE_SIZE_TYPE_MAX
};

struct cache_data {
	u64 ave;
	u64 std;
};

struct cache_data cache_bench[CACHE_SIZE_TYPE_MAX] = {
	{433796UL, 23647UL},		/*CACHE_L1_READ_UC, 0*/
	{2559UL, 11UL},				/*CACHE_L1_READ_WB,*/
	{2557UL, 11UL},				/*CACHE_L1_READ_WT,*/
	{435176UL, 23877UL},		/*CACHE_L1_READ_WC,*/
	{2557UL, 11UL},				/*CACHE_L1_READ_WP,*/

	{3479968UL, 29924UL},		/*CACHE_L2_READ_UC, 5*/
	{19447UL, 39UL},			/*CACHE_L2_READ_WB,*/
	{19443UL, 35UL},			/*CACHE_L2_READ_WT,*/
	{3480217UL, 29759UL},		/*CACHE_L2_READ_WC,*/
	{19438UL, 34UL},			/*CACHE_L2_READ_WP,*/

	{111318576UL, 4757UL},		/*CACHE_L3_READ_UC, 10*/
	{675272UL, 651UL},			/*CACHE_L3_READ_WB,*/
	{675276UL, 807UL},			/*CACHE_L3_READ_WT,*/
	{111319218UL, 35080UL},		/*CACHE_L3_READ_WC,*/
	{675368UL, 690UL},			/*CACHE_L3_READ_WP,*/

	{445280643UL, 37021UL},		/*CACHE_OVER_L3_READ_UC, 15*/
	{3656013UL, 21757UL},		/*CACHE_OVER_L3_READ_WB,*/
	{3642278UL, 26704UL},		/*CACHE_OVER_L3_READ_WT,*/
	{445276301UL, 42465UL},		/*CACHE_OVER_L3_READ_WC,*/
	{3642478UL, 26895UL},		/*CACHE_OVER_L3_READ_WP,*/

	{337330UL, 50UL},			/*CACHE_L1_WRITE_UC, 20*/
	{3169UL, 16UL},				/*CACHE_L1_WRITE_WB,*/
	{337365UL, 83UL},			/*CACHE_L1_WRITE_WT,*/
	{3385UL, 118UL},			/*CACHE_L1_WRITE_WC,*/
	{337377UL, 81UL},			/*CACHE_L1_WRITE_WP,*/

	{2698424UL, 174UL},			/*CACHE_L2_WRITE_UC, 25*/
	{25060UL, 28UL},			/*CACHE_L2_WRITE_WB,*/
	{2698413UL, 168UL},			/*CACHE_L2_WRITE_WT,*/
	{28373UL, 5996UL},			/*CACHE_L2_WRITE_WC,*/
	{2698451UL, 162UL},			/*CACHE_L2_WRITE_WP,*/

	{86342539UL, 645UL},		/*CACHE_L3_WRITE_UC, 30*/
	{804229UL, 211UL},			/*CACHE_L3_WRITE_WB,*/
	{86342445UL, 693UL},		/*CACHE_L3_WRITE_WT,*/
	{932708UL, 12316UL},		/*CACHE_L3_WRITE_WC,*/
	{86342359UL, 589UL},		/*CACHE_L3_WRITE_WP,*/

	{345369076UL, 1194UL},		/*CACHE_OVER_L3_WRITE_UC, 35*/
	/* Abnormal large test on CI*/
	{5804371UL, 118930UL},		/*CACHE_OVER_L3_WRITE_WB,*/
	{345369505UL, 1590UL},		/*CACHE_OVER_L3_WRITE_WT,*/
	{3733753UL, 26365UL},		/*CACHE_OVER_L3_WRITE_WC,*/
	{345369240UL, 1273UL},		/*CACHE_OVER_L3_WRITE_WP,*/
#if 0
	{2071703UL, 1091UL},/*CACHE_DEVICE_4K_READ, 40*/
	{104626UL, 13887UL},/*CACHE_4K_READ,*/
	{1500492UL, 605UL},/*CACHE_DEVICE_4K_WRITE,*/
	{84378UL, 55UL},/*CACHE_4K_WRITE,*/

	{14750957UL, 127314UL},/*CACHE_CLFLUSH_DIS_READ, 44*/
	{664331UL, 36943UL},/*CACHE_CLFLUSH_READ,*/
	{14743159UL, 147640UL},/*CACHE_CLFLUSHOPT_DIS_READ,*/
	{657227UL, 32265UL},/*CACHE_CLFLUSHOPT_READ,*/
	{15223198UL, 125174UL},/*CACHE_WBINVD_DIS_READ,*/
	{226598UL, 27398UL},/*CACHE_WBINVD_READ,*/
#endif
};

struct case_fun_index {
	int rqmid;
	void (*func)(void);
};
typedef void (*trigger_func)(void *data);

void set_mem_cache_type(u64 cache_type)
{
	set_mem_cache_type_addr(cache_test_array, cache_type, CACHE_OVE_L3_SIZE*8);
}

__attribute__((aligned(64))) u64 read_mem_cache_test(u64 size)
{
	return read_mem_cache_test_addr(cache_test_array, size);
}

void read_mem_cache_test_time_invd(u64 size, int time)
{
	int t_time = time;
	/*debug_print("read cache cache_test_size 0x%lx %ld\n",size, size*8);*/
	while (t_time--) {
		read_mem_cache_test(size);
	}

	asm_mfence_wbinvd();
}

u64 cache_order_read(enum cache_size_type type, u64 size)
{
	int i;

	tsc_delay_delta_total = 0;
	/*Remove the first test data*/
	read_mem_cache_test(size);
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	return tsc_delay_delta_total;
}

bool cache_check_memory_type(u64 average, u64 native_aver, u64 native_std, u64 size)
{
	bool ret = true;

	if ((average < ((native_aver*(100-ERROR_RANG))/100)) \
		|| (average > ((native_aver*(100+ERROR_RANG))/100))) {
		ret = false;
	}

#ifdef __x86_64__
	if (ret != true) {
		printf("read delta =%ld size=0x%lx [%ld, %ld]\n", tsc_delay_delta_total, size,
			(native_aver*(100-ERROR_RANG))/100, (native_aver*(100+ERROR_RANG))/100);
	}
#elif __i386__
	if (ret != true) {
		printf("read delta =%lld size=0x%llx [%lld, %lld]\n", tsc_delay_delta_total, size,
			(native_aver*(100-ERROR_RANG))/100, (native_aver*(100+ERROR_RANG))/100);
	}
#endif
	return ret;
}

bool cache_order_read_test(enum cache_size_type type, u64 size)
{
	bool ret = true;
	u64 ave;
	u64 std;

	ave = cache_bench[type].ave;
	std = cache_bench[type].std;

	asm_mfence_wbinvd();
	tsc_delay_delta_total = cache_order_read(type, size);

	ret = cache_check_memory_type(tsc_delay_delta_total, ave, std, size);

	asm_mfence_wbinvd();
	return ret;
}

int get_bit_range(u32 r, int start, int end)
{
	int mask = 0;
	int i = end-start+1;
	u32 t_r = r>>start;
	while (i--) {
		mask = mask<<1;
		mask += 1;
	}
	return t_r&mask;
}

void cache_fun_exec(struct case_fun_index *case_fun, int size, long rqmid)
{
	int i;

	debug_print("***************start test case number = %d rqmid=%ld***************\n", size, rqmid);
	for (i = 0; i < size; i++) {
		if (rqmid == case_fun[i].rqmid) {
			case_fun[i].func();
			break;
		}

		if (rqmid == 0) {
			case_fun[i].func();
		}
	}
}

#ifdef __x86_64__
/*test case which should run under 64bit  */
#include "64/mem_cache_fn.c"
#elif __i386__
/*test case which should run  under 32bit  */
#include "32/mem_cache_fn.c"
#endif

#ifdef __x86_64__
int ap_start_count = 0;
volatile u64 init_pat = 0;
volatile u64 set_pat = 0;
void save_unchanged_reg()
{
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	if (ap_start_count == 0) {
		/* save init value (defalut is 0007040600070406)*/
		init_pat = rdmsr(IA32_PAT_MSR);

		/* set new value to pat */
		wrmsr(IA32_PAT_MSR, 0x6);
		ap_start_count++;
	}

	set_pat = rdmsr(IA32_PAT_MSR);

	if (ap_start_count == 2) {
		/* resume environment */
		wrmsr(IA32_PAT_MSR, init_pat);
	}
}

void print_case_list_init_startup()
{
	printf("cache init statup feature case list:\n\r");
#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 23239u, "IA32_PAT INIT value_unchange_001");
	printf("\t Case ID:%d case name:%s\n\r", 23241u, "CR0.CD INIT value_001");
#endif
	printf("\t Case ID:%d case name:%s\n\r", 23242u, "CR0.NW start-up value_001");
}

/**
 * @brief case name:IA32_PAT INIT value_unchange_001
 *
 * Summary: After AP receives first INIT, set the PAT value to 6H;
 * dump IA32_PAT register value, AP INIT again, then dump IA32_PAT
 * register value again, two dumps value should be equal.
 */
void __unused cache_rqmid_23239_ia32_pat_init_unchange_01(void)
{
	volatile u64 ia32_pat1;
	volatile u64 ia32_pat2;

	/*get set_pat value */
	ia32_pat1 = set_pat;

	/*send sipi to ap*/
	send_sipi();

	/*get pat value again after ap reset*/
	ia32_pat2 = set_pat;

	/*compare init value with unchanged */
	report("%s ", ia32_pat1 == ia32_pat2, __FUNCTION__);
}

/*
 * @brief case name: IA32_PAT INIT value_unchange_002
 *
 * Summary: At BP executing 1st instruction and AP receives the first init,
 * save the PAT value, and the two PAT values should be equal.
 */
void __unused cache_rqmid_37108_ia32_pat_init_unchange_02(void)
{
	volatile u64 ia32_pat1;
	volatile u64 ia32_pat2;
	volatile u32 unchanged_ap_pat1 = 0;
	volatile u32 unchanged_ap_pat2 = 0;
	volatile u32 *ptr;

	/* startup pat value */
	ia32_pat1 = IA32_PAT_STARTUP_VALUE;

	/* init pat value */
	ptr = (volatile u32 *)(0x8000 + 0x8);
	unchanged_ap_pat1 = *ptr;
	unchanged_ap_pat2 = *(ptr + 1);
	ia32_pat2 = unchanged_ap_pat1 | ((u64)unchanged_ap_pat2 << 32);

	report("%s ", ia32_pat1 == ia32_pat2, __FUNCTION__);
}

/**
 * @brief case name:CR0.CD INIT value_001
 *
 * Summary: Get CR0.CD[bit 30] at AP init, the bit shall be 1 and same with SDM definition.
 */
void __unused cache_rqmid_23241_cr0_cd_init(void)
{
	volatile u32 ap_cr0 = 0;
	volatile u32 *ptr;

	ptr = (volatile u32 *)0x8000;
	ap_cr0 = *ptr;

	report("%s", (ap_cr0 & (1<<30)), __FUNCTION__);
}

/**
 * @brief case name:CR0.NW start-up value_001
 *
 * Summary: Get CR0.NW[bit 29] at BP start-up, the bit shall be 1 and same with SDM definition.
 */
void cache_rqmid_23242_cr0_nw_startup(void)
{
	volatile u32 *ptr;
	volatile u32 bp_cr0;

	ptr = (volatile u32 *)0x6000;
	bp_cr0 = *ptr;

	report("%s", (bp_cr0 & (1<<29)), __FUNCTION__);
}

/* 10 init startup case */
void cache_test_init_startup(long rqmid)
{
	print_case_list_init_startup();

	struct case_fun_index case_fun[] = {
	#ifdef IN_NON_SAFETY_VM
		/*Must be in front of 23239 */
		{33333, cache_rqmid_37108_ia32_pat_init_unchange_02},
		{23239, cache_rqmid_23239_ia32_pat_init_unchange_01},
		{23241, cache_rqmid_23241_cr0_cd_init},
	#endif
		{23242, cache_rqmid_23242_cr0_nw_startup},
	};

	cache_fun_exec(case_fun, sizeof(case_fun)/sizeof(case_fun[0]), rqmid);
}
#endif

int main(int ac, char **av)
{
	long rqmid = 0;

	if (ac >= 2) {
		rqmid = atol(av[1]);
	}

#ifdef __x86_64__
	cache_test_init_startup(rqmid);
#endif

#ifdef __x86_64__
	setup_idt();

	/*default PAT entry value 0007040600070406*/
	set_mem_cache_type_all(0x0000000001040506);
	setup_vm();

#elif defined(__i386__)
	setup_idt();
#endif

	cache_test_array = (u64 *)malloc(cache_malloc_size*8);
	if (cache_test_array == NULL) {
		debug_error("malloc error\n");
		return -1;
	}

	debug_print("cache_test_array=%p\n", cache_test_array);
	memset(cache_test_array, 0xFF, cache_malloc_size*8);
	debug_print("mem cache control memory malloc success addr=%lx\n", (u64)cache_test_array);
	/*test_cache_type();*/

	debug_print("rqmid = %ld\n", rqmid);

	//delay(10);
#ifdef __x86_64__
#ifndef CACHE_IN_NATIVE
	cache_test_64(rqmid);
#else
	/*cache_test_native(rqmid);*/
#endif
#elif defined(__i386__)
	/*cache_test_32(rqmid);*/
#endif

	free(cache_test_array);
	cache_test_array = NULL;
	return report_summary();
}

