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
#include "pci_util.h"
#include "delay.h"
#include "mem_cache.h"

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;

__unused void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}

__unused static void notify_ap_modify_and_read_init_value(int case_id)
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

#ifdef __i386__
void ap_main(void)
{
	asm volatile ("pause");
}

#elif __x86_64__

typedef void (*ap_init_value_modify)(void);
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

__unused static void modify_cr0_cd_init_value()
{
	/*this function will change cd to 0*/
	mem_cache_reflush_cache();
}

__unused static void modify_cr0_nw_init_value()
{
	/*this function will change nw to 0*/
	mem_cache_reflush_cache();
}


__unused static void modify_cr3_pcd_init_value()
{
	write_cr3(read_cr3() | (1ul << CR3_BIT_PCD));
}

__unused static void modify_cr3_pwt_init_value()
{
	write_cr3(read_cr3() | (1ul << CR3_BIT_PWT));
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 23241:
		fp = modify_cr0_cd_init_value;
		ap_init_value_process(fp);
		break;
	case 36850:
		fp = modify_cr0_nw_init_value;
		ap_init_value_process(fp);
		break;
	case 36869:
		fp = modify_cr3_pcd_init_value;
		ap_init_value_process(fp);
	case 36853:
		fp = modify_cr3_pwt_init_value;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif


#ifdef IN_NATIVE
#define CACHE_IN_NATIVE
#define DUMP_CACHE_NATIVE_DATA
#endif

//#define NO_PAGING_TESTS

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

	CACHE_L1_READ_UC_NOFILL,
	CACHE_L2_READ_UC_NOFILL,
	CACHE_L3_READ_UC_NOFILL,
	CACHE_OVER_L3_READ_UC_NOFILL,

	CACHE_L1_WRITE_UC_NOFILL,
	CACHE_L2_WRITE_UC_NOFILL,
	CACHE_L3_WRITE_UC_NOFILL,
	CACHE_OVER_L3_WRITE_UC_NOFILL,

	CACHE_SIZE_TYPE_MAX
};

struct cache_data {
	u64 ave;
	u64 std;
};

/* The cache_bench is auto-generated, don't modify it. */
struct cache_data cache_bench[CACHE_SIZE_TYPE_MAX] = {
	[CACHE_L1_READ_WB] = {2566UL, 14UL},
	[CACHE_L2_READ_WB] = {19428UL, 28UL},
	[CACHE_L3_READ_WB] = {674996UL, 1086UL},
	[CACHE_OVER_L3_READ_WB] = {3665751UL, 26498UL},
	[CACHE_L1_WRITE_WB] = {2567UL, 18UL},
	[CACHE_L2_WRITE_WB] = {21832UL, 29UL},
	[CACHE_L3_WRITE_WB] = {771740UL, 33UL},
	[CACHE_OVER_L3_WRITE_WB] = {5741758UL, 107777UL},
	[CACHE_L1_READ_WC] = {433598UL, 23379UL},
	[CACHE_L2_READ_WC] = {3481055UL, 31877UL},
	[CACHE_L3_READ_WC] = {111432029UL, 41220UL},
	[CACHE_OVER_L3_READ_WC] = {445671900UL, 40189UL},
	[CACHE_L1_WRITE_WC] = {2847UL, 129UL},
	[CACHE_L2_WRITE_WC] = {28741UL, 8271UL},
	[CACHE_L3_WRITE_WC] = {933109UL, 19788UL},
	[CACHE_OVER_L3_WRITE_WC] = {3730164UL, 29432UL},
	[CACHE_L1_READ_WT] = {2564UL, 6UL},
	[CACHE_L2_READ_WT] = {19426UL, 23UL},
	[CACHE_L3_READ_WT] = {675270UL, 665UL},
	[CACHE_OVER_L3_READ_WT] = {3669097UL, 29404UL},
	[CACHE_L1_WRITE_WT] = {337452UL, 40UL},
	[CACHE_L2_WRITE_WT] = {2698911UL, 55UL},
	[CACHE_L3_WRITE_WT] = {86362423UL, 80UL},
	[CACHE_OVER_L3_WRITE_WT] = {345448347UL, 335UL},
	[CACHE_L1_READ_WP] = {2565UL, 6UL},
	[CACHE_L2_READ_WP] = {19429UL, 22UL},
	[CACHE_L3_READ_WP] = {675156UL, 708UL},
	[CACHE_OVER_L3_READ_WP] = {3665243UL, 24908UL},
	[CACHE_L1_WRITE_WP] = {337441UL, 41UL},
	[CACHE_L2_WRITE_WP] = {2698914UL, 73UL},
	[CACHE_L3_WRITE_WP] = {86362435UL, 88UL},
	[CACHE_OVER_L3_WRITE_WP] = {345448403UL, 370UL},
	[CACHE_L1_READ_UC] = {433552UL, 23122UL},
	[CACHE_L2_READ_UC] = {3481242UL, 28727UL},
	[CACHE_L3_READ_UC] = {111420110UL, 26730UL},
	[CACHE_OVER_L3_READ_UC] = {445679384UL, 44220UL},
	[CACHE_L1_WRITE_UC] = {337466UL, 49UL},
	[CACHE_L2_WRITE_UC] = {2698914UL, 60UL},
	[CACHE_L3_WRITE_UC] = {86362426UL, 98UL},
	[CACHE_OVER_L3_WRITE_UC] = {345448405UL, 312UL},
	[CACHE_DEVICE_4K_READ] = {2137022UL, 1358UL},
	[CACHE_DEVICE_4K_WRITE] = {1676842UL, 75UL},
	[CACHE_L1_READ_UC_NOFILL] = {4335223UL, 27896UL},
	[CACHE_L2_READ_UC_NOFILL] = {34610219UL, 32317UL},
	[CACHE_L3_READ_UC_NOFILL] = {1108764319UL, 785335UL},
	[CACHE_OVER_L3_READ_UC_NOFILL] = {4436098764UL, 3215272UL},
	[CACHE_L1_WRITE_UC_NOFILL] = {4715080UL, 26200UL},
	[CACHE_L2_WRITE_UC_NOFILL] = {37681405UL, 25600UL},
	[CACHE_L3_WRITE_UC_NOFILL] = {1208788775UL, 141990UL},
	[CACHE_OVER_L3_WRITE_UC_NOFILL] = {4835623807UL, 674855UL},
	[CACHE_CLFLUSH_READ] = {300314UL, 1219UL},
	[CACHE_CLFLUSH_DIS_READ] = {6202172UL, 17674UL},
	[CACHE_CLFLUSHOPT_READ] = {300002UL, 1204UL},
	[CACHE_CLFLUSHOPT_DIS_READ] = {6207999UL, 11483UL},
	[CACHE_WBINVD_READ] = {304443UL, 1647UL},
	[CACHE_WBINVD_DIS_READ] = {6199389UL, 14449UL},
};

struct case_fun_index {
	int rqmid;
	void (*func)(void);
};
typedef void (*trigger_func)(void *data);

struct msr_data {
	u64 value;
	u32 index;
};

void clear_cache();

unsigned long long asm_read_tsc(void)
{
	long long r;
#ifdef __x86_64__
	unsigned a, d;

	asm volatile("mfence" ::: "memory");
	asm volatile ("rdtsc" : "=a"(a), "=d"(d));
	r = a | ((long long)d << 32);
#else
	asm volatile ("rdtsc" : "=A"(r));
#endif
	asm volatile("mfence" ::: "memory");
	return r;
}

void asm_mfence()
{
	asm volatile("mfence" ::: "memory");
}

void asm_wbinvd()
{
	asm volatile ("wbinvd\n" : : : "memory");
}

void asm_mfence_wbinvd()
{
	asm_mfence();
	asm_wbinvd();
	asm_mfence();
}

void asm_invd()
{
	asm volatile ("invd\n" : : : "memory");
}

u64 disorder_access(u64 index, u64 size)
{
	int i = 0;
	u64 *p;
	u64 t[2] = {0};
	u64 disorder_index = 0;

	t[0] = asm_read_tsc();
	disorder_index = (index*(t[0]&0xffff))%size;
	p = &cache_test_array[disorder_index];

	t[0] = asm_read_tsc();
	asm_read_access_memory(p);
	t[1] = asm_read_tsc();

	i = t[1]-t[0];
	return i;
}

u64 disorder_access_size(u64 size)
{
	int i;
	u64 ts_delta = 0;
	u64 ts_delta_all = 0;

	i = size;
	while (i) {
		ts_delta = disorder_access(i, size);
		ts_delta_all = ts_delta_all + ts_delta;
		i--;
	}
	return ts_delta_all;
}


void write_cr0_bybit(u32 bit, u32 bitvalue)
{
	u32 cr0 = read_cr0();
	if (bitvalue) {
		write_cr0(cr0 | (1 << bit));
	} else {
		write_cr0(cr0 & ~(1 << bit));
	}
}

u64 PT_MEMORY_TYPE   = PT_MEMORY_TYPE_MASK0;

/*Modify the PTE/PCD/PWT bit in paging table entry*/
void set_memory_type_pt(void *address, u64 type, u64 size)
{
	unsigned long *ptep;
	u64 *next_addr;
	int i;
	int j = 0;
	int pat = PT_PAT;

	PT_MEMORY_TYPE = type;

	for (i = 0; i < size; i += PAGE_SIZE) {
		j++;
		next_addr = (u64 *)((u8 *)address + i);

		ptep = get_pte_level(current_page_table(), next_addr, 1);
		if (ptep == NULL) {
			pat = PT_PAT_LARGE_PAGE;
		}
		switch (type) {
		case PT_MEMORY_TYPE_MASK0:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 0, 0);
			break;
		case PT_MEMORY_TYPE_MASK1:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 0, 0);
			break;
		case PT_MEMORY_TYPE_MASK2:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 0, 0);
			break;
		case PT_MEMORY_TYPE_MASK3:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 0, 0);
			break;
		case PT_MEMORY_TYPE_MASK4:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 1, 0);
			break;
		case PT_MEMORY_TYPE_MASK5:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 1, 0);
			break;
		case PT_MEMORY_TYPE_MASK6:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 1, 0);
			break;
		case PT_MEMORY_TYPE_MASK7:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 1, 0);
			break;
		default:
			debug_error("error type\n");
			break;
		}
	}
}

void flush_tlb()
{
	u32 cr3;
	cr3 = read_cr3();
	write_cr3(cr3);
}

void mem_cache_reflush_cache()
{
	u32 cr4;

	/*Disable interrupts;*/
	irq_disable();

	/*Save current value of CR4;*/
	cr4 = read_cr4();

	/*Disable and flush caches;*/
	write_cr0_bybit(CR0_BIT_CD, 1);
	write_cr0_bybit(CR0_BIT_NW, 0);
	asm_wbinvd();

	/*Flush TLBs;*/
	flush_tlb();

	/*Disable MTRRs;*/
	//disable_MTRR();

	/*Flush caches and TLBs;*/
	asm_wbinvd();
	flush_tlb();

	/*Enable MTRRs;*/
	//enable_MTRR();

	/*enable caches;*/
	write_cr0_bybit(CR0_BIT_CD, 0);
	write_cr0_bybit(CR0_BIT_NW, 0);

	/*Restore value of CR4;*/
	write_cr4(cr4);

	/*Enable interrupts;*/
	irq_enable();
}

void set_mem_cache_type(u64 cache_type)
{
	set_mem_cache_type_addr(cache_test_array, cache_type, CACHE_OVE_L3_SIZE*8);
}

__attribute__((aligned(64))) u64 read_mem_cache_test(u64 size)
{
	return read_mem_cache_test_addr(cache_test_array, size);
}

__attribute__((aligned(64))) u64 write_mem_cache_test(u64 size)
{
	return write_mem_cache_test_addr(cache_test_array, size);
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

	clear_cache();

	tsc_delay_delta_total = cache_order_read(type, size);

	ret = cache_check_memory_type(tsc_delay_delta_total, ave, std, size);

	clear_cache();

	return ret;
}


u64 cache_order_write(enum cache_size_type type, u64 size)
{
	int i;

	tsc_delay_delta_total = 0;
	/*Remove the first test data*/
	write_mem_cache_test(size);
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	return tsc_delay_delta_total;
}

bool cache_order_write_test(enum cache_size_type type, u64 size)
{
	bool ret = true;
	u64 ave;
	u64 std;

	ave = cache_bench[type].ave;
	std = cache_bench[type].std;

	clear_cache();

	tsc_delay_delta_total = cache_order_write(type, size);

	ret = cache_check_memory_type(tsc_delay_delta_total, ave, std, size);

	clear_cache();

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

void asm_clflush(long unsigned int addr)
{
	asm volatile("clflush (%0)" : : "b" (addr));
}

void clflush_all_line(u64 size)
{
	int i;

	for (i = 0; i < size; i++) {
		asm volatile("clflush (%0)" : : "b" (&cache_test_array[i]));
	}
}

static inline void asm_clflushopt(void *p)
{
	asm volatile ("clflushopt (%0)" :: "r"(p));
}

void clflushopt_all_line(u64 size)
{
	int i;

	for (i = 0; i < size; i++) {
		/*asm volatile(".byte 0x66, 0x0f, 0xae, 0x3b" : : "b" (&cache_test_array[i]));*/
		asm_clflushopt(&cache_test_array[i]);
	}
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

#define BAR_REMAP_USB_BASE    0xDFFF0000
#define PCI_BAR_MASK	      0xFFFF0000
#define USB_HCIVERSION        0x02
#define PCI_PCIR_BAR(x)	      (0x10 + (x) * 4)

/**
 * @brief In 64-bit mode,Select USB device BAR0 as the test object.
 *       Write the value 0xDFFF_0000 to the BAR0 register of USB device.
 *       Read the HCIVERSION register of USB device
 *       If HCIVERSION register value is 0x100, pass,otherwize fail
 * none
 * Application Constraints: Only for USB.
 *
 * @param none
 *
 * @return OK. seccessed,pci_mem_address value is valid
 *
 * @retval (-1) fialed,pci_mem_address value is invalid
 */
int pci_mem_get(void *pci_mem_address)
{
	union pci_bdf bdf = {.bits = {.b = 0, .d = 0x14, .f = 0} };
	uint32_t bar_base;
	uint32_t reg_val;
	int ret = OK;

	pci_pdev_write_cfg(bdf, PCI_PCIR_BAR(0), 4, BAR_REMAP_USB_BASE);

	reg_val = pci_pdev_read_cfg(bdf, PCI_PCIR_BAR(0), 4);
	DBG_INFO("R reg[%xH] = [%xH]", PCI_PCIR_BAR(0), reg_val);

	bar_base = PCI_BAR_MASK & reg_val;

	reg_val = pci_pdev_read_mem(bdf, (bar_base + USB_HCIVERSION), 2);
	if (0x100 != reg_val) {
		DBG_ERRO("R mem[%xH] != [%xH]", bar_base + USB_HCIVERSION, 0x100);
		ret = ERROR;
	}

	if (OK == ret) {
		*((uint64_t *)pci_mem_address) = BAR_REMAP_USB_BASE;
	}

	return ret;
}

void clear_cache()
{
	clflush_all_line(cache_malloc_size);
	flush_tlb();
	asm_mfence_wbinvd();
}

#ifdef __x86_64__
#ifndef CACHE_IN_NATIVE
/* test case which should run under 64bit  */
#include "64/mem_cache_fn.c"
#else
# ifdef DUMP_CACHE_NATIVE_DATA
#include "64/mem_cache_fn.c"
# endif
/* native test cases */
#include "64/mem_cache_native.c"
#endif

//#elif __i386__
/*test case which should run  under 32bit  */
//#include "32/mem_cache_fn.c"
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
	bool is_pass = true;

	ptr = (volatile u32 *)INIT_CR0_ADDR;
	ap_cr0 = *ptr;
	if ((ap_cr0 & (1ul << CR0_BIT_CD)) == 0) {
		printf("(%s):%d\n", __FUNCTION__, __LINE__);
		is_pass = false;
	}
	notify_ap_modify_and_read_init_value(23241);

	ptr = (volatile u32 *)INIT_CR0_ADDR;
	ap_cr0 = *ptr;

	if ((ap_cr0 & (1ul << CR0_BIT_CD)) == 0) {
		printf("(%s):%d\n", __FUNCTION__, __LINE__);
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
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

void cache_rqmid_36851_ia32_pat_startup(void)
{
	volatile u32 ia32_pat_l = *(volatile u32 *)(0x6000 + 0x8);
	volatile u32 ia32_pat_h = *(volatile u32 *)(0x6000 + 0xC);

	/* The value should be 0007040600070406H */
	report("%s", (ia32_pat_l == 0x00070406) &&
		(ia32_pat_h == 0x00070406), __FUNCTION__);
}

void cache_rqmid_36852_cr0_cd_startup(void)
{
	volatile u32 cr0 = *(volatile u32 *)0x6000;

	report("%s", (cr0 & (1 << 30)), __FUNCTION__);
}

void cache_rqmid_36850_cr0_nw_init(void)
{
	volatile u32 cr0 = *(volatile u32 *)INIT_CR0_ADDR;
	bool is_pass = true;

	if ((cr0 & (1ul << CR0_BIT_NW)) == 0) {
		is_pass = false;
	}

	notify_ap_modify_and_read_init_value(36850);

	cr0 = *(volatile u32 *)INIT_CR0_ADDR;
	if ((cr0 & (1ul << CR0_BIT_NW)) == 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

void cache_rqmid_36869_cr3_pcd_init(void)
{
	volatile u32 cr3 = *(volatile u32 *)INIT_CR3_ADDR;
	bool is_pass = true;

	if ((cr3 & (1ul << CR3_BIT_PCD))) {
		is_pass = false;
	}

	notify_ap_modify_and_read_init_value(36869);
	cr3 = *(volatile u32 *)INIT_CR3_ADDR;

	if ((cr3 & (1ul << CR3_BIT_PCD))) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

void cache_rqmid_36853_cr3_pwt_init(void)
{
	volatile u32 cr3 = *(volatile u32 *)INIT_CR3_ADDR;
	bool is_pass = true;

	if ((cr3 & (1 << CR3_BIT_PWT))) {
		is_pass = false;
	}

	notify_ap_modify_and_read_init_value(36853);
	cr3 = *(volatile u32 *)INIT_CR3_ADDR;

	if ((cr3 & (1 << CR3_BIT_PWT))) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

void cache_rqmid_36854_cr3_pcd_startup(void)
{
	volatile u32 cr3 = *(volatile u32 *)(0x6000 + 0x4);

	report("%s", ~(cr3 & (1 << 4)), __FUNCTION__);
}

void cache_rqmid_36855_cr3_pwt_startup(void)
{
	volatile u32 cr3 = *(volatile u32 *)(0x6000 + 0x4);

	report("%s", ~(cr3 & (1 << 3)), __FUNCTION__);
}

/* 10 init startup case */
void cache_test_init_startup(long rqmid)
{
	print_case_list_init_startup();

	struct case_fun_index case_fun[] = {
	#ifdef IN_NON_SAFETY_VM
		/*Must be in front of 23239 */
		{37108, cache_rqmid_37108_ia32_pat_init_unchange_02},
		{23239, cache_rqmid_23239_ia32_pat_init_unchange_01},
		{23241, cache_rqmid_23241_cr0_cd_init},
	#endif
		{23242, cache_rqmid_23242_cr0_nw_startup},
		{36851, cache_rqmid_36851_ia32_pat_startup},
		{36852, cache_rqmid_36852_cr0_cd_startup},
		{36850, cache_rqmid_36850_cr0_nw_init},
		{36869, cache_rqmid_36869_cr3_pcd_init},
		{36853, cache_rqmid_36853_cr3_pwt_init},
		{36854, cache_rqmid_36854_cr3_pcd_startup},
		{36855, cache_rqmid_36855_cr3_pwt_startup},
	};

	cache_fun_exec(case_fun, sizeof(case_fun)/sizeof(case_fun[0]), rqmid);
}

#ifndef CACHE_IN_NATIVE

void cache_test_no_paging_tests(long rqmid)
{
	int i;

	struct case_fun_index case_fun[] = {
		{36873, cache_rqmid_36873_memory_mapped_guest_physical_normal},
		{36876, cache_rqmid_36876_device_mapped_guest_physical_normal},
		{36886, cache_rqmid_36886_empty_mapped_guest_physical_normal},
	};

	printf("cache 64bit No Paging tests list:\n\r");

	for (i = 0; i < (sizeof(case_fun)/sizeof(case_fun[0])); i++) {
		printf("Case ID: %d\n", case_fun[i].rqmid);
	}

	printf("\n");

	cache_fun_exec(case_fun, sizeof(case_fun)/sizeof(case_fun[0]), rqmid);
}

#endif // CACHE_IN_NATIVE

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

#ifndef CACHE_IN_NATIVE
#ifdef NO_PAGING_TESTS
	cache_test_no_paging_tests(rqmid);
#endif
#endif
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
# ifdef DUMP_CACHE_NATIVE_DATA
	dump_cache_bentch_mark_64(rqmid);
# else
	cache_test_native(rqmid);
# endif
#endif
#elif defined(__i386__)
	/*cache_test_32(rqmid);*/
#endif

	free(cache_test_array);
	cache_test_array = NULL;
	return report_summary();

}

