#include "libcflat.h"
#include <atomic.h>
#include <desc.h>
#include <smp.h>
#include <processor.h>
#include <asm/barrier.h>
#include <apic.h>
#include <string.h>
#include <delay.h>
#include <alloc.h>
#include <vm.h>
#include <vmalloc.h>
#include <alloc_page.h>
#include <memory_type.h>

#define HYPERVISOR

#define TESTER_NAME_MAX_SIZE		16

#ifdef __i386__
	#define	PROTECTED_MODE
	#define TEST_CASE_NUM		1
#else
	#define TEST_CASE_NUM		4
#endif

#ifdef	HYPERVISOR
	#define CPU_NUM			2
#else
	#define CPU_NUM			4
#endif

/* Double the number of hyperthreaded CPUs */
#define APIC_ID_MAX			(CPU_NUM * 2)

#define SMP_EXE_LOOPS		10000000

#define READ_OK				0x01
#define WRITE_OK			0x02

#define OPT_OK				0x01
#define SMP_OK				0x02

#define CR0_BIT_NW			29
#define CR0_BIT_CD			30
#define CR0_BIT_PG			31
#define MSR_IA32_CR_PAT_TEST		0x00000277
#define IA32_MTRR_DEF_TYPE		    0x000002FF
#define IA32_PAT_MSR				0x00000277


#define MEM_TYPE_UC			0x0000000001040500
#define MEM_TYPE_WC			0x0000000001040501
#define MEM_TYPE_WT			0x0000000001040504
#define MEM_TYPE_WP			0x0000000001040505
#define MEM_TYPE_WB			0x0000000001040506



#define ATOMIC_INIT(i)		{ (i) }

/*#define USE_DEBUG*/
#ifdef USE_DEBUG
#define debug_print(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)
#else
#define debug_print(fmt, args...)
#endif
#define debug_error(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)

#define MSR_IA32_EXT_XAPICID			0x00000802U
#define nop()		do { asm volatile ("nop\n\t" :::"memory"); } while (0)
#define TEST_TIME 10000
#define END_NUMBER 200000U
#define MEM_LOOPS  100000U

static volatile int start_sem1;
static volatile int start_sem2;
static volatile int stop_flag1 = 0;
static volatile int stop_flag2 = 0;
static volatile int success_flag1 = 1;
static volatile int success_flag2 = 1;

static volatile int mem_finished_flag1;
static volatile int mem_finished_flag2;

static int time;
static int pass_time;
static volatile u32 ptr;

static int time1;
static int time2;

static volatile int stop = 0;
static volatile int start_case_id = 0;

atomic_t xchg_val1;
atomic_t xchg_val2;
atomic_t xchg_val0;
atomic_t mem_ptr;

u16 tr;
struct descriptor_table_ptr gdt_desc;
struct segment_desc64 *tr_entry;
pteval_t *ptep;
pteval_t *pdep;
u64 phy_add1;
u64 phy_add2;
u8 *gva;
u64 virt_addr;

enum{
	CASE_ID_1 = 32739,
	CASE_ID_2 = 32742,
	CASE_ID_3 = 28280,
	CASE_ID_4 = 28281,
	CASE_ID_5 = 28283,
	CASE_ID_6 = 28284,
	CASE_ID_7 = 28285,
	CASE_ID_8 = 37869,
	CASE_ID_9 = 28264,
	CASE_ID_10 = 28267,
	CASE_ID_11 = 37872,
	CASE_ID_12 = 37873,
	CASE_ID_13 = 37874,
	CASE_ID_14 = 28270,
	CASE_ID_15 = 28256,
	CASE_ID_16 = 28258,
	CASE_ID_17 = 28261,
	CASE_ID_18 = 39502,
	CASE_ID_19 = 39543,
};

union cache_line {
	u8 at[10];
	u16 data[5];
} __attribute__((aligned(2)));

union uncached_memory {
	volatile u8  at[4];
	volatile u32 data;
} __attribute__((aligned(4)));


union cache_line cache_line_v;
union uncached_memory uncached_mem;

__attribute__((aligned(1))) static volatile u8   a_byte_mem;
__attribute__((aligned(2)))  static volatile u16  a_word_mem;
__attribute__((aligned(4))) static volatile u32  double_word;
__attribute__((aligned(8))) static volatile u64  quadword;


u16 *unalign_16bit_ptr = (u16 *)&cache_line_v.at[1];
u32 *unalign_32bit_ptr = (u32 *)&cache_line_v.at[1];
u64 *unalign_64bit_ptr = (u64 *)&cache_line_v.at[1];
u16 *uncached_16bit_ptr = (u16 *)&uncached_mem.at[0];

static inline u16 atomic_word_noalign_read(void)
{
	u16 value;
	asm volatile ("movw %1, %0\n" : "=r"(value) : "r"(*unalign_16bit_ptr) :);
	return value;
}

static inline void atomic_word_noalign_write_0()
{
	asm volatile ("movw $0, %0\n" : "=m"(*unalign_16bit_ptr) : : "memory");
}


static inline void atomic_word_noalign_write_ff(void)
{
	asm volatile ("movw $0xFFFF, %0\n" : "=m"(*unalign_16bit_ptr) : : "memory");
}

static inline u16 atomic_word_uncached_read(void)
{
	u16 value;
	asm volatile ("movw %1, %0\n" : "=r"(value) : "r"(*uncached_16bit_ptr) :);
	return value;
}

static inline void atomic_word_uncached_write_0()
{
	asm volatile ("movw $0, %0\n" : "=m"(*uncached_16bit_ptr) : : "memory");
}


static inline void atomic_word_uncached_write_ff(void)
{
	asm volatile ("movw $0xFFFF, %0\n" : "=m"(*uncached_16bit_ptr) : : "memory");
}

static inline u32 atomic_dword_noalign_read(void)
{
	u32 value;
	asm volatile ("movl %1, %0\n" : "=r"(value) : "r"(*unalign_32bit_ptr) : );
	return value;
}

static inline void atomic_dword_noalign_write_0()
{
	asm volatile ("movl $0, %0\n" : "=m"(*unalign_32bit_ptr) : : "memory");
}

static inline void atomic_dword_noalign_write_ff(void)
{
	asm volatile ("movl $0xFFFFFFFF, %0\n" : "=m"(*unalign_32bit_ptr) : : "memory");
}

static u64 atomic_quadword_noalign_read(void)
{
	u64 value;
	asm volatile ("movq %1, %0\n" : "=r"(value) : "r"(*unalign_64bit_ptr) : "memory");
	return value;
}

static void atomic_quadword_noalign_write_0()
{
	asm volatile ("movq $0, %0\n" : "=m"(*unalign_64bit_ptr) : : "memory");
}


static void atomic_quadword_noalign_write_ff(void)
{
	asm volatile ("movq $0xFFFFFFFFFFFFFFFF, %0\n" : "=m"(*unalign_64bit_ptr) : : "memory");
}

#if 0
static inline void asm_wbinvd(void)
{
	asm volatile ("wbinvd\n" : : : "memory");
}
#endif

void asm_mfence()
{
	asm volatile("mfence" ::: "memory");
}

void asm_mfence_wbinvd()
{
	asm_mfence();
	asm_wbinvd();
	asm_mfence();
}

/*porting from mem_cache.c*/
void flush_tlb()
{
	u32 cr3;
	cr3 = read_cr3();
	write_cr3(cr3);
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

/*********************************************/
static inline u8 atomic_byte_read(void)
{
	u8 value;
	asm volatile ("mov %1, %0\n" : "=r"(value) : "m"(a_byte_mem) : );
	return value;
}

static inline void atomic_byte_write_0(void)
{
	asm volatile ("movb $0, %0\n" : "=m"(a_byte_mem) : : "memory");
}

static inline void atomic_byte_write_ff(void)
{
	asm volatile ("movb $0xFF, %0\n" : "=m"(a_byte_mem) : : "memory");
}

static inline u16 atomic_word_read(void)
{
	u16 value;
	asm volatile ("movw %1, %0\n" : "=r"(value) : "m"(a_word_mem) : );
	return value;
}

static inline void atomic_word_write_0(void)
{
	asm volatile ("movw $0, %0\n" : "=m"(a_word_mem) : : "memory");
}

static inline void atomic_word_write_ff(void)
{
	asm volatile ("movw $0xFFFF, %0\n" : "=m"(a_word_mem) : : "memory");
}

static inline u32 atomic_doubleword_read(void)
{
	u32 value;
	asm volatile ("movl %1, %0\n" : "=r"(value) : "m"(double_word) : );
	return value;
}

static inline void atomic_doubleword_write_0(void)
{
	asm volatile ("movl $0, %0\n" : "=m"(double_word) : : "memory");
}

static inline void atomic_doubleword_write_ff(void)
{
	asm volatile ("movl $0xFFFFFFFF, %0\n" : "=m"(double_word) : : "memory");
}

static inline u64 atomic_quadword_read(void)
{
	u64 value;
	asm volatile ("movq %1, %0\n" : "=r"(value) : "m"(quadword) : );
	return value;
}

static inline void atomic_quadword_write_0(void)
{
	asm volatile ("movq $0, %0\n" : "=m"(quadword) : : "memory");
}

static inline void atomic_quadword_write_ff(void)
{
	asm volatile ("movq $0xFFFFFFFFFFFFFFFF, %0\n" : "=m"(quadword) : : "memory");
}

void asm_bts(u32 volatile *addr, int bit, int is_lock)
{
	if (is_lock) {
		__asm__ __volatile__("lock bts %1, %0"
			     : "+m" (*addr) : "Ir" (bit) : "cc", "memory");
	}
	else {
		__asm__ __volatile__("bts %1, %0"
			     : "+m" (*addr) : "Ir" (bit) : "cc", "memory");
	}
}

static inline void asm_atomic_xchg(atomic_t *v, int volatile *op2)
{
	asm volatile("xchgl %k0,%1\n\t"
		     : "=r" (v->counter)
		     : "m" (*op2), "0" (v->counter));
}


int bp_test(int cpu_id, int is_lock)
{
	start_sem1 = 1;
	start_sem2 = 1;
	stop_flag1 = 0;
	stop_flag2 = 0;
	time = 0;
	pass_time = 0;
	ptr = 0;
	stop = 0;

	debug_print("bp start cpu_id =%d %s\n", cpu_id, is_lock ? "lock test" : "no lock test");

	while (1) {
		if ((stop_flag1 == 1) && (stop_flag2 == 1)) {
			stop_flag1 = 0;
			stop_flag2 = 0;
			time++;
		}
		else {
			continue;
		}

		if (ptr == 0xFFFFFFFF) {
			pass_time++;
		}

		if (time == TEST_TIME) {
			stop = 1;
			if (pass_time == TEST_TIME) {
				debug_print("%s %d pass %d %d \n", __FUNCTION__, __LINE__, time, pass_time);
				break;
			}
			else {
				debug_print("%s %d fail %d %d \n", __FUNCTION__, __LINE__, time, pass_time);
				break;
			}
		}
		else {
			ptr = 0;
			start_sem1 = 1;
			start_sem2 = 1;
			continue;
		}
	}

	debug_print("bp1 stop\n");
	return pass_time;
}

void ap1_test(int cpu_id, int is_lock)
{
	time1 = 0;
	debug_print("ap1 start cpu_id =%d %s\n", cpu_id, is_lock ? "lock test" : "no lock test");

	while (1) {
		if (start_sem1 != 1) {
			if (stop == 1) {
				break;
			}
			else {
				continue;
			}
		}

		while (time1 < 32) {
			asm_bts(&ptr, time1, is_lock);
			time1 = time1 + 2;
		}

		start_sem1 = 0;
		stop_flag1 = 1;
		time1 = 0;
		continue;
	}

	debug_print("ap1 stop\n");
}

void ap2_test(int cpu_id, int is_lock)
{
	time2 = 1;
	debug_print("ap2 start cpu_id =%d %s\n", cpu_id, is_lock ? "lock test" : "no lock test");

	while (1) {
		if (start_sem2 != 1) {
			if (stop == 1) {
				break;
			}
			else {
				continue;
			}
		}

		while (time2 < 32) {
			asm_bts(&ptr, time2, is_lock);
			time2 = time2 + 2;
		}

		start_sem2 = 0;
		stop_flag2 = 1;
		time2 = 1;
		continue;
	}

	debug_print("ap2 stop\n");
}

void atomic_memory_type_ap1_test(int cpu_id)
{
	int i;
	debug_print("ap1 start cpu_id =%d\n", cpu_id);
	for (i = 0; i < MEM_LOOPS; i++) {
		atomic_inc(&mem_ptr);
	}
	mem_finished_flag1 = 1;
}

void atomic_memory_type_ap2_test(int cpu_id)
{
	int i;
	debug_print("ap1 start cpu_id =%d\n", cpu_id);
	for (i = 0; i < MEM_LOOPS; i++) {
		atomic_inc(&mem_ptr);
	}
	mem_finished_flag2 = 1;
}

int atomic_memory_type_bp_test(int cpu_id)
{
	debug_print("bp start cpu_id =%d\n", cpu_id);

	while (1) {
		if (mem_finished_flag1 && mem_finished_flag2) {
			mem_finished_flag1 = 0;
			mem_finished_flag2 = 0;
			break;
		}
	}
	return mem_ptr.counter;
}

void xchg_instruction_ap1_test(int cpu_id)
{
	int i;
	debug_print("ap1 start cpu_id =%d\n", cpu_id);
	for (i = 0; i < TEST_TIME; i++) {
		asm_atomic_xchg(&xchg_val1, &(xchg_val0.counter));
	}
	stop_flag1 = 1;
}

void xchg_instruction_ap2_test(int cpu_id)
{
	int i;
	debug_print ("ap2 start cpu_id =%d\n", cpu_id);
	for (i = 0; i < TEST_TIME; i++) {
		asm_atomic_xchg(&xchg_val2, &(xchg_val0.counter));
	}
	stop_flag2 = 1;
}

int xchg_instruction_bp_test(int cpu_id)
{
	debug_print("bp start cpu_id =%d\n", cpu_id);
	stop_flag1 = 0;
	stop_flag2 = 0;

	while (1) {
		if (stop_flag1 && stop_flag2) {
			stop_flag1 = 0;
			stop_flag2 = 0;
			break;
		}
	}

	return (((atomic_read(&xchg_val1) == 0x0F) || (atomic_read(&xchg_val1) == 0xA3) ||
			(atomic_read(&xchg_val1) == 0xF0)) &&
			((atomic_read(&xchg_val2) == 0xF0) || (atomic_read(&xchg_val2) == 0xA3) ||
			(atomic_read(&xchg_val2) == 0x0F)) &&
			((atomic_read(&xchg_val0) == 0xA3) || (atomic_read(&xchg_val0) == 0x0F)
			|| (atomic_read(&xchg_val0) == 0xF0))) &&
			(atomic_read(&xchg_val0) != atomic_read(&xchg_val1)) &&
			(atomic_read(&xchg_val1) != atomic_read(&xchg_val2)) &&
			(atomic_read(&xchg_val0) != atomic_read(&xchg_val2));

}

void read_write_a_quadword_aligned_on_ap1()
{
	int i;
	u64 value;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_quadword_write_0();
		value = atomic_quadword_read();
		if (!((value == (long long)0xFFFFFFFFFFFFFFFF)
			|| (value == 0)))
		{
			success_flag1 = 0;
			break;
		}
	}
	stop_flag1 = 1;
}

void read_write_a_quadword_aligned_on_ap2()
{
	int i;
	u64 value;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_quadword_write_0();
		value = atomic_quadword_read();
		if (!((value == (long long)0xFFFFFFFFFFFFFFFF)
			|| (value == 0)))
		{
			success_flag2 = 0;
			break;
		}
	}
	stop_flag2 = 1;
}

void read_write_a_word_unaligned_on_ap1()
{
	int i;
	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_word_noalign_write_0();
		if (!((atomic_word_noalign_read() == (u16)0xFFFF)
			|| (atomic_word_noalign_read() == 0)))
		{
			success_flag1 = 0;
			break;
		}
	}
	stop_flag1 = 1;
}

void read_write_a_word_unaligned_on_ap2()
{
	int i;
	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_word_noalign_write_ff();
		if (!((atomic_word_noalign_read() == (u16)0xFFFF)
			|| (atomic_word_noalign_read() == 0)))
		{
			success_flag2 = 0;
			break;
		}
	}
	stop_flag2 = 1;
}

void read_write_a_dword_unaligned_on_ap1()
{
	int i;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_dword_noalign_write_0();
		if (!((atomic_dword_noalign_read() == (u32)0xFFFFFFFF)
			|| (atomic_dword_noalign_read() == 0)))
		{
			success_flag1 = 0;
			break;
		}
	}
	stop_flag1 = 1;
}

void read_write_a_dword_unaligned_on_ap2()
{
	int i;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_dword_noalign_write_ff();
		if (!((atomic_dword_noalign_read() == (u32)0xFFFFFFFF)
			|| (atomic_dword_noalign_read() == 0)))
		{
			success_flag2 = 0;
			break;
		}
	}
	stop_flag2 = 1;
}

void read_write_a_quadword_unaligned_on_ap1()
{
	int i;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_quadword_noalign_write_0();
		if (!((atomic_quadword_noalign_read() == (u64)0xFFFFFFFFFFFFFFFF)
			|| (atomic_quadword_noalign_read() == 0)))
		{
			success_flag1 = 0;
			break;
		}
	}
	stop_flag1 = 1;
}

void read_write_a_quadword_unaligned_on_ap2()
{
	int i;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_quadword_noalign_write_ff();
		if (!((atomic_quadword_noalign_read() == (u64)0xFFFFFFFFFFFFFFFF)
			|| (atomic_quadword_noalign_read() == 0)))
		{
			success_flag2 = 0;
			break;
		}
	}
	stop_flag2 = 1;
}

void read_write_a_word_uncached_memory_ap1()
{
	int i;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_word_uncached_write_0();
		if (!((atomic_word_uncached_read() == (u16)0xFFFF)
			|| (atomic_word_uncached_read() == 0)))
		{
			success_flag1 = 0;
			break;
		}
	}
	stop_flag1 = 1;
}

void read_write_a_word_uncached_memory_ap2()
{
	int i;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_word_uncached_write_ff();
		if (!((atomic_word_uncached_read() == (u16)0xFFFF)
			|| (atomic_word_uncached_read() == 0)))
		{
			success_flag2 = 0;
			break;
		}
	}
	stop_flag2 = 1;
}

static int is_all_same(void *buf, size_t size, uint8_t value)
{
    return ((uint8_t *)buf)[0] == value && !memcmp(buf, buf + 1, size - 1);
}

void update_segment_descriptor_ap1()
{
	int i;
	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		memset(tr_entry, 0, sizeof(tr_entry));
		if (!(is_all_same(tr_entry, sizeof(tr_entry), 0x0) ||
		      is_all_same(tr_entry, sizeof(tr_entry), 0xF))) {
			success_flag1 = 0;
			break;
		}
	}
	stop_flag1 = 1;
}

void update_segment_descriptor_ap2()
{
	int i;
	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		memset(tr_entry, 0xF, sizeof(tr_entry));
		if (!(is_all_same(tr_entry, sizeof(tr_entry), 0x0) ||
		      is_all_same(tr_entry, sizeof(tr_entry), 0xF))) {
			success_flag2 = 0;
			break;
		}
	}
	stop_flag2 = 1;
}

void update_pte_ap1()
{
	int i;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		install_page((pgd_t *)read_cr3(), phy_add1, gva);
		if (!(((*ptep & (~0xFFF)) == phy_add1) || ((*ptep & (~0xFFF)) == phy_add2)))
		{
			success_flag1 = 0;
			break;
		}
	}
	stop_flag1 = 1;
}

void update_pte_ap2()
{
	int i;
	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		install_page((pgd_t *)read_cr3(), phy_add2, gva);
		if (!(((*ptep & (~0xFFF)) == phy_add1) || ((*ptep & (~0xFFF)) == phy_add2)))
		{
			success_flag2 = 0;
			break;
		}
	}
	stop_flag2 = 1;
}

void update_pde_ap1()
{
	int i;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		install_large_page((pgd_t *)read_cr3(), phy_add1, (void *)(ulong)virt_addr);
		if (!(((*pdep & (~0x1FFFFF)) == phy_add1) || ((*pdep & (~0x1FFFFF)) == phy_add2)))
		{
			success_flag1 = 0;
			break;
		}
	}
	stop_flag1 = 1;
}

void update_pde_ap2()
{
	int i;
	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		install_large_page((pgd_t *)read_cr3(), phy_add2, (void *)(ulong)virt_addr);
		if (!(((*pdep & (~0x1FFFFF)) == phy_add1) || ((*pdep & (~0x1FFFFF)) == phy_add2)))
		{
			success_flag2 = 0;
			break;
		}
	}
	stop_flag2 = 1;
}


void read_write_a_byte_aligned_ap1()
{
	int i;
	u8 value;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_byte_write_0();
		value = atomic_byte_read();
		if (!((value == 0xFF)
			|| (value == 0)))	{
			//printf("value:%d\n",value);
			success_flag1 = 0;
			break;
		}
	}
	stop_flag1 = 1;
}

void read_write_a_byte_aligned_ap2()
{
	int i;
	u8 value;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_byte_write_ff();
		value = atomic_byte_read();
		if (!((value == 0xFF)
			|| (value == 0)))	{
			printf("value:%d\n", value);
			success_flag1 = 0;
			break;
		}
	}
	stop_flag2 = 1;
}

void read_write_a_word_aligned_ap1()
{
	int i;
	u16 value;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_word_write_0();
		value = atomic_word_read();
		if (!((value == 0xFFFF)
			|| (value == 0)))	{
			printf("value:%d\n", value);
			success_flag1 = 0;
			break;
		}
	}
	stop_flag1 = 1;
}

void read_write_a_word_aligned_ap2()
{
	int i;
	u16 value;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_word_write_ff();
		value = atomic_word_read();
		if (!((value == 0xFFFF)
			|| (value == 0)))	{
			printf("value:%d\n", value);
			success_flag1 = 0;
			break;
		}
	}
	stop_flag2 = 1;
}

void read_write_double_word_aligned_ap1()
{
	int i;
	u32 value;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_doubleword_write_0();
		value = atomic_doubleword_read();
		if (!((value == 0xFFFFFFFF)
			|| (value == 0)))	{
			printf("value:%d\n", value);
			success_flag1 = 0;
			break;
		}
	}
	stop_flag1 = 1;
}

void read_write_double_word_aligned_ap2()
{
	int i;
	u32 value;

	for (i = 0; i < SMP_EXE_LOOPS; i++) {
		atomic_doubleword_write_ff();
		value = atomic_doubleword_read();
		if (!((value == 0xFFFFFFFF)
			|| (value == 0)))	{
			printf("value:%d\n", value);
			success_flag1 = 0;
			break;
		}
	}
	stop_flag2 = 1;
}

int synchronization_on_bp_test(int case_id)
{
	stop_flag1 = 0;
	stop_flag2 = 0;
	start_case_id = case_id;
	while (1) {
		if (stop_flag1 && stop_flag2) {
			break;
		}
	}
	stop_flag1 = 0;
	stop_flag2 = 0;

	return (success_flag1 && success_flag2);
}

__attribute__((unused)) static void locked_atomic_rqmid_32739_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		ap1_test(cpu_id, 1);
	}
	else if (cpu_id == 2) {
		ap2_test(cpu_id, 1);
	}
	else {
		debug_error("other ap no test cpu_id=%d\n", cpu_id);
	}
}
/**
 * @brief case name:Atomic - expose LOCK prefix to VM_002
 *
 * Summary:Under 64 bit mode, Test the atomicity of the lock instruction(BTS)
 * in the case of multicore concurrency(with lock prifex).
 */
__attribute__((unused)) static void locked_atomic_rqmid_32739_expose_lock_prefix_02()
{
	u32 cpu_id = 0;
	int ret;

	start_case_id = CASE_ID_1;
	debug_print("start_case_id=%d\n", start_case_id);

	ret = bp_test(cpu_id, 1);
	report("%s pass=%d", (ret == TEST_TIME), __FUNCTION__, ret);
}

__attribute__((unused)) static void locked_atomic_rqmid_32742_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		ap1_test(cpu_id, 0);
	}
	else if (cpu_id == 2) {
		ap2_test(cpu_id, 0);
	}
}

__attribute__((unused)) static void locked_atomic_rqmid_mem_type_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		atomic_memory_type_ap1_test(cpu_id);
	}
	else if (cpu_id == 2) {
		atomic_memory_type_ap2_test(cpu_id);
	}
}

__attribute__((unused)) static void locked_atomic_xchg_instruction_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		xchg_instruction_ap1_test(cpu_id);
	}
	else if (cpu_id == 2) {
		xchg_instruction_ap2_test(cpu_id);
	}
}

__attribute__((unused)) static void read_write_a_quadword_aligned_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		read_write_a_quadword_aligned_on_ap1(cpu_id);
	}
	else if (cpu_id == 2) {
		read_write_a_quadword_aligned_on_ap2(cpu_id);
	}
}

__attribute__((unused)) static void read_write_a_word_unaligned_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		read_write_a_word_unaligned_on_ap1(cpu_id);
	}
	else if (cpu_id == 2) {
		read_write_a_word_unaligned_on_ap2(cpu_id);
	}
}

__attribute__((unused)) static void read_write_a_dword_unaligned_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		read_write_a_dword_unaligned_on_ap1(cpu_id);
	}
	else if (cpu_id == 2) {
		read_write_a_dword_unaligned_on_ap2(cpu_id);
	}
}

__attribute__((unused)) static void read_write_a_quadword_unaligned_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		read_write_a_quadword_unaligned_on_ap1(cpu_id);
	}
	else if (cpu_id == 2) {
		read_write_a_quadword_unaligned_on_ap2(cpu_id);
	}
}

__attribute__((unused)) static void read_write_a_word_uncached_memory_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		read_write_a_word_uncached_memory_ap1(cpu_id);
	}
	else if (cpu_id == 2) {
		read_write_a_word_uncached_memory_ap2(cpu_id);
	}
}

__attribute__((unused)) static void update_segment_descriptor_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		update_segment_descriptor_ap1(cpu_id);
	}
	else if (cpu_id == 2) {
		update_segment_descriptor_ap2(cpu_id);
	}
}

__attribute__((unused)) static void update_pte_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		update_pte_ap1();
	}
	else if (cpu_id == 2) {
		update_pte_ap2();
	}
}

__attribute__((unused)) static void update_pde_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		update_pde_ap1();
	}
	else if (cpu_id == 2) {
		update_pde_ap2();
	}
}


__attribute__((unused)) static void read_write_a_byte_aligned_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		read_write_a_byte_aligned_ap1();
	}
	else if (cpu_id == 2) {
		read_write_a_byte_aligned_ap2();
	}
}

__attribute__((unused)) static void read_write_a_word_aligned_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		read_write_a_word_aligned_ap1();
	}
	else if (cpu_id == 2) {
		read_write_a_word_aligned_ap2();
	}
}

__attribute__((unused)) static void read_write_double_word_aligned_ap()
{
	u32 cpu_id = get_cpu_id();

	if (cpu_id == 1) {
		read_write_double_word_aligned_ap1();
	}
	else if (cpu_id == 2) {
		read_write_double_word_aligned_ap2();
	}
}

/**
 * @brief case name:Atomic - expose LOCK prefix to VM_003
 *
 * Summary:Under 64 bit mode, Test the atomicity of the lock instruction(BTS)
 * in the case of multicore concurrency(without lock prifex).
 */
__attribute__((unused)) static void locked_atomic_rqmid_32742_expose_unlock_prefix_03()
{
	u32 cpu_id = 0;
	int ret;

	start_case_id = CASE_ID_2;
	debug_print("start_case_id=%d\n", start_case_id);

	ret = bp_test(cpu_id, 0);
	report("%s pass=%d", (ret < TEST_TIME), __FUNCTION__, ret);
}


/**
 * @brief case name:Atomic - memory type_uc_01
 *
 * Summary:Under 64 bit mode,Testing the atomicity of atomic variables in uc memory type.
 */
__attribute__((unused)) static void locked_atomic_rqmid_28280_memory_type_uc_01()
{
	u32 cpu_id = 0;
	int ret;

	set_mem_cache_type_all(MEM_TYPE_UC);
	atomic_set(&mem_ptr, 0);
	start_case_id = CASE_ID_3;
	debug_print("start_case_id=%d\n", start_case_id);

	ret = atomic_memory_type_bp_test(cpu_id);
	report("%s pass=%d", (ret == END_NUMBER), __FUNCTION__, ret);
}


/**
 * @brief case name:Atomic - memory type_wc_02
 *
 * Summary:Under 64 bit mode,Testing the atomicity of atomic variables in wc memory type.
 */
__attribute__((unused)) static void locked_atomic_rqmid_28281_memory_type_wc_02()
{
	u32 cpu_id = 0;
	int ret;

	set_mem_cache_type_all(MEM_TYPE_WC);
	atomic_set(&mem_ptr, 0);
	start_case_id = CASE_ID_4;
	debug_print("start_case_id=%d\n", start_case_id);

	ret = atomic_memory_type_bp_test(cpu_id);
	report("%s pass=%d", (ret == END_NUMBER), __FUNCTION__, ret);
}

/**
 * @brief case name:Atomic - memory type_wt_03
 *
 * Summary:Under 64 bit mode,Testing the atomicity of atomic variables in wt memory type.
 */
__attribute__((unused)) static void locked_atomic_rqmid_28283_memory_type_wt_03()
{
	u32 cpu_id = 0;
	int ret;

	set_mem_cache_type_all(MEM_TYPE_WT);
	atomic_set(&mem_ptr, 0);
	start_case_id = CASE_ID_5;
	debug_print("start_case_id=%d\n", start_case_id);

	ret = atomic_memory_type_bp_test(cpu_id);
	report("%s pass=%d", (ret == END_NUMBER), __FUNCTION__, ret);
}

/**
 * @brief case name:Atomic - memory type_wp_04
 *
 * Summary:Under 64 bit mode,Testing the atomicity of atomic variables in wp memory type.
 */
__attribute__((unused)) static void locked_atomic_rqmid_28284_memory_type_wp_04()
{
	u32 cpu_id = 0;
	int ret;

	set_mem_cache_type_all(MEM_TYPE_WP);
	atomic_set(&mem_ptr, 0);
	start_case_id = CASE_ID_6;
	debug_print("start_case_id=%d\n", start_case_id);

	ret = atomic_memory_type_bp_test(cpu_id);
	report("%s pass=%d", (ret == END_NUMBER), __FUNCTION__, ret);
}

/**
 * @brief case name:Atomic - memory type_wb_05
 *
 * Summary:Under 64 bit mode,Testing the atomicity of atomic variables in wb memory.
 */
__attribute__((unused)) static void locked_atomic_rqmid_28285_memory_type_wb_05()
{
	u32 cpu_id = 0;
	int ret;

	atomic_set(&mem_ptr, 0);
	start_case_id = CASE_ID_7;
	debug_print("start_case_id=%d\n", start_case_id);

	set_mem_cache_type_all(MEM_TYPE_WB);

	ret = atomic_memory_type_bp_test(cpu_id);
	report("%s pass=%d", (ret == END_NUMBER), __FUNCTION__, ret);
}

/**
 * @brief case name:Atomic - XCHG instruction_01
 *
 * Summary:Under 64 bit mode,Testing the atomicity of xchg instruction.
 */
__attribute__((unused)) static void locked_atomic_rqmid_37869_XCHG_instruction_01()
{
	u32 cpu_id = 0;
	int ret;

	start_case_id = CASE_ID_8;
	debug_print("start_case_id=%d\n", start_case_id);
	atomic_set(&xchg_val1, 0x0F);
	atomic_set(&xchg_val2, 0xF0);
	atomic_set(&xchg_val0, 0xA3);

	ret = xchg_instruction_bp_test(cpu_id);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);
}

/**
 * @brief case name: Atomic - unaligned accesses to cached memory that fit within a cache line 16 bit_001
 *
 * Summary: Testing atomicity of unaligned accesses to 16 bit cached memory  in multi-core concurrent environment
 *
 * @retval None
 *
 */

__attribute__((unused)) static void locked_atomic_rqmid_28267_unaligned_accesses_to_cached_memory_16_bit_01()
{
	int ret;
	int i;

	success_flag1 = 1;
	success_flag2 = 1;
	set_mem_cache_type_all(MEM_TYPE_WB);
	for (i = 0; i < 10; i++) {
		cache_line_v.at[i] = 0;
	}

	ret = synchronization_on_bp_test(CASE_ID_10);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);

}

/**
 * @brief case name: Atomic - unaligned accesses to cached memory that fit within a cache line 32 bit_001
 *
 * Summary: Testing atomicity of unaligned accesses to 32 bit cached memory  in multi-core concurrent environment
 *
 * @retval None
 *
 */
__attribute__((unused)) static void locked_atomic_rqmid_37872_unaligned_accesses_to_cached_memory_32_bit_02()
{
	int ret;
	int i;

	success_flag1 = 1;
	success_flag2 = 1;

	for (i = 0; i < 10; i++) {
		cache_line_v.at[i] = 0;
	}
	ret = synchronization_on_bp_test(CASE_ID_11);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);
}

/**
 * @brief case name: Atomic - unaligned accesses to cached memory that fit within a cache line 64 bit_001
 *
 * Summary: Testing atomicity of unaligned accesses to 64 bit cached memory  in multi-core concurrent environment
 * @retval None
 *
 */

__attribute__((unused)) static void locked_atomic_rqmid_37873_unaligned_accesses_to_cached_memory_64_bit_03()
{
	int ret;
	int i;
	success_flag1 = 1;
	success_flag2 = 1;

	for (i = 0; i < 10; i++) {
		cache_line_v.at[i] = 0;
	}

	ret = synchronization_on_bp_test(CASE_ID_12);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);
}

/**
 * @brief case name: Atomic_16-bit_accesses_to_uncached_memory_locations_that_fit within_a_32-bit_data_bus_01
 *
 * Summary: Testing atomicity of 16-bit_accesses_to_uncached_memory_locations_that_fit within_a_32-bit_data_bus
 * in multi-core concurrent environment
 * @retval None
 *
 */

__attribute__((unused)) static void locked_atomic_rqmid_28265_16_bit_accesses_to_uncached_memory_locations_01()

{
	int ret;
	success_flag1 = 1;
	success_flag2 = 1;

	set_mem_cache_type_all(MEM_TYPE_UC);
	ret = synchronization_on_bp_test(CASE_ID_13);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);
}


/**
 * @brief case name: Atomic - update segment descriptor_01
 *
 * Summary: Testing atomicity of update segment descriptor in multi-core concurrent environment.
 * @retval None
 *
 */

__attribute__((unused)) static void locked_atomic_rqmid_28270_update_segment_descriptor_01()

{
	int ret;
	struct segment_desc64 *gdt_table;

	tr = str();
	sgdt(&gdt_desc);
	gdt_table = (struct segment_desc64 *) gdt_desc.base;
	tr_entry = &gdt_table[tr / sizeof(struct segment_desc64)];
	success_flag1 = 1;
	success_flag2 = 1;

	ret = synchronization_on_bp_test(CASE_ID_14);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);
}

/**
 * @brief case name: Atomic - update page-directory and page-table entries_01
 *
 * Summary: The physical platform shall guarantee that updating page-directory and page-table entries
 * in VMX non-root operation is atomic.
 * @retval None
 *
 */

__attribute__((unused)) static void locked_atomic_rqmid_39502_update_page_directory_and_page_table_entries_01()

{
	int ret;

	pgd_t *pgd = (pgd_t *)read_cr3();
	gva = malloc(PAGE_SIZE);
	ptep = get_pte(pgd,	(void *)gva);
	phy_add1 = *ptep & (~0xFFF);
	phy_add2 = phy_add1 + PAGE_SIZE;
	success_flag1 = 1;
	success_flag2 = 1;

	ret = synchronization_on_bp_test(CASE_ID_18);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);
	free(gva);
}

/**
 * @brief case name: Atomic - update page-directory and page-table entries_02
 *
 * Summary: The physical platform shall guarantee that updating page-directory and page-table entries
 * in VMX non-root operation is atomic.
 * @retval None
 *
 */

__attribute__((unused)) static void locked_atomic_rqmid_39543_update_page_directory_and_page_table_entries_02()

{
	int ret;
	pgd_t *pgd = (pgd_t *)read_cr3();
	/* mapping to 128M/130M physical address */
	phy_add1 = 0x8000000;
	phy_add2 = phy_add1 + LARGE_PAGE_SIZE;
	virt_addr = phy_add1;
	install_large_page(pgd, phy_add1, (void *)(ulong)virt_addr);
	/* when pde is lage page entry, get_pte return the pde entry*/
	pdep = get_pte(pgd, (void *)virt_addr);
	if (!(*pdep & (1 << PAGE_PS_FLAG))) {
		printf("this PDE is not mapped to 2M page, test fail\n");
	}

	success_flag1 = 1;
	success_flag2 = 1;
	ret = synchronization_on_bp_test(CASE_ID_19);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);
}


/**
 * @brief case name: Atomic - read/write a byte_001
 *
 * Summary: Testing atomicity of read and write a byte in multi-core concurrent environment
 * Requirement ID: 145065
 *
 * @retval None
 *
 */
__attribute__((unused)) static void locked_atomic_rqmid_28256_read_write_a_byte_01(void)
{
	int ret;

	success_flag1 = 1;
	success_flag2 = 1;

	ret = synchronization_on_bp_test(CASE_ID_15);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);
}

/**
 * @brief case name: Atomic - read/write a word aligned on 16-bit_001
 *
 * Summary: Testing atomicity of read and write a word aligned on 16-bit in multi-core concurrent environment
 *
 * @retval None
 *
 */
__attribute__((unused)) static void locked_atomic_rqmid_28258_read_write_a_word_aligned_on_16_bit_01(void)
{
	int ret;

	success_flag1 = 1;
	success_flag2 = 1;

	ret = synchronization_on_bp_test(CASE_ID_16);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);
}

/**
 * @brief case name: Atomic - read/write a word aligned on 32-bit_001
 *
 * Summary: Testing atomicity of read and write a doubleword aligned on 32-bit in multi-core concurrent environment
 *
 * @retval None
 *
 */
__attribute__((unused)) static void locked_atomic_rqmid_28261_read_write_a_doubleword_aligned_on_32_bit_01()
{
	int ret;

	success_flag1 = 1;
	success_flag2 = 1;

	ret = synchronization_on_bp_test(CASE_ID_17);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);
}

/**
 * @brief case name: Atomic - read/write a quadword aligned on 64-bit_001
 *
 * Summary: Testing atomicity of read and write a quadword aligned on 64-bit in multi-core concurrent environment
 *
 * @retval None
 *
 */

__attribute__((unused)) static void locked_atomic_rqmid_28264_read_write_a_quadword_aligned_on_64_bit_01()
{
	int ret;

	success_flag1 = 1;
	success_flag2 = 1;

	ret = synchronization_on_bp_test(CASE_ID_9);
	report("%s pass=%d", (ret == 1), __FUNCTION__, ret);
}


void ap_main(void)
{
	/* safety only 1 cpu, no ap run*/
	printf("ap %d main test start\n", get_cpu_id());

#if defined(IN_NON_SAFETY_VM) || defined(IN_NATIVE)
	while (start_case_id != CASE_ID_1) {
		nop();
	}
	locked_atomic_rqmid_32739_ap();

	while (start_case_id != CASE_ID_18) {
		nop();
	}
	update_pte_ap();
#endif

#ifdef IN_NON_SAFETY_VM
	while (start_case_id != CASE_ID_2) {
		nop();
	}
	locked_atomic_rqmid_32742_ap();
#endif

#ifdef IN_NATIVE
	while (start_case_id != CASE_ID_3) {
		nop();
	}
	locked_atomic_rqmid_mem_type_ap();
	while (start_case_id != CASE_ID_4) {
		nop();
	}
	locked_atomic_rqmid_mem_type_ap();
	while (start_case_id != CASE_ID_5) {
		nop();
	}
	locked_atomic_rqmid_mem_type_ap();
	while (start_case_id != CASE_ID_6) {
		nop();
	}
	locked_atomic_rqmid_mem_type_ap();
	while (start_case_id != CASE_ID_7) {
		nop();
	}
	locked_atomic_rqmid_mem_type_ap();

	while (start_case_id != CASE_ID_8) {
		nop();
	}
	locked_atomic_xchg_instruction_ap();

	while (start_case_id != CASE_ID_15) {
		nop();
	}
	read_write_a_byte_aligned_ap();

	while (start_case_id != CASE_ID_16) {
		nop();
	}
	read_write_a_word_aligned_ap();

	while (start_case_id != CASE_ID_17) {
		nop();
	}
	read_write_double_word_aligned_ap();

	while (start_case_id != CASE_ID_9) {
		nop();
	}
	read_write_a_quadword_aligned_ap();

	while (start_case_id != CASE_ID_10) {
		nop();
	}
	read_write_a_word_unaligned_ap();

	while (start_case_id != CASE_ID_11) {
		nop();
	}
	read_write_a_dword_unaligned_ap();

	while (start_case_id != CASE_ID_12) {
		nop();
	}
	read_write_a_quadword_unaligned_ap();

	while (start_case_id != CASE_ID_13) {
		nop();
	}
	read_write_a_word_uncached_memory_ap();

	while (start_case_id != CASE_ID_14) {
		nop();
	}
	update_segment_descriptor_ap();

	while (start_case_id != CASE_ID_19) {
		nop();
	}
	update_pde_ap();
#endif
	debug_print("ap main test stop\n");
}

static void print_case_list()
{
	printf("locked atomic feature case list:\n\r");

#if defined(IN_NON_SAFETY_VM) || defined(IN_NATIVE)
	printf("\t\t Case ID:%d case name:%s\n\r", 32739U, "Atomic - expose LOCK prefix to VM_002/Atomic - \
		physical LOCK prefix support");
	printf("\t\t Case ID:%d case name:%s\n\r", 39502U, "Atomic - update page-directory and page-table entries_01");
#endif

#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 32742u, "Atomic - expose LOCK prefix to VM_003");
#endif

#ifdef IN_NATIVE
	printf("\t\t Case ID:%d case name:%s\n\r", 28280U, "Atomic - memory type_uc_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 28281U, "Atomic - memory type_wc_02");
	printf("\t\t Case ID:%d case name:%s\n\r", 28283U, "Atomic - memory type_wt_03");
	printf("\t\t Case ID:%d case name:%s\n\r", 28284U, "Atomic - memory type_wp_04");
	printf("\t\t Case ID:%d case name:%s\n\r", 28285U, "Atomic - memory type_wb_05");
	printf("\t\t Case ID:%d case name:%s\n\r", 37869U, "Atomic - XCHG instruction_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 28264U, "Atomic - read/write a quadword aligned on 64-bit_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 28267U, "Atomic - unaligned accesses to cached memory that fit \
	within a cache line 16 bit_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37872U, "Atomic - unaligned accesses to cached memory that fit \
	within a cache line 32 bit_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 37873U, "Atomic - unaligned accesses to cached memory that fit \
	within a cache line 64 bit_003");

	printf("\t\t Case ID:%d case name:%s\n\r", 28256U, "Atomic - read/write a byte_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28258U, "Atomic - read/write a word aligned on 16-bit_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28261U, "Atomic_read/write_a_doubleword_aligned_on_32-bit_03");
	printf("\t\t Case ID:%d case name:%s\n\r", 28270U, "Atomic - update segment descriptor_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 39543U, "Atomic - update page-directory and page-table entries_02");
#endif
}

int main(int ac, char **av)
{
	print_case_list();

#if defined(IN_NON_SAFETY_VM) || defined(IN_NATIVE)
	locked_atomic_rqmid_32739_expose_lock_prefix_02();
	locked_atomic_rqmid_39502_update_page_directory_and_page_table_entries_01();
#endif

#ifdef IN_NON_SAFETY_VM
	locked_atomic_rqmid_32742_expose_unlock_prefix_03();
#endif
#ifdef IN_NATIVE
	locked_atomic_rqmid_28280_memory_type_uc_01();
	locked_atomic_rqmid_28281_memory_type_wc_02();
	locked_atomic_rqmid_28283_memory_type_wt_03();
	locked_atomic_rqmid_28284_memory_type_wp_04();
	locked_atomic_rqmid_28285_memory_type_wb_05();
	locked_atomic_rqmid_37869_XCHG_instruction_01();
	locked_atomic_rqmid_28256_read_write_a_byte_01();
	locked_atomic_rqmid_28258_read_write_a_word_aligned_on_16_bit_01();
	locked_atomic_rqmid_28261_read_write_a_doubleword_aligned_on_32_bit_01();
	locked_atomic_rqmid_28264_read_write_a_quadword_aligned_on_64_bit_01();
	locked_atomic_rqmid_28267_unaligned_accesses_to_cached_memory_16_bit_01();
	locked_atomic_rqmid_37872_unaligned_accesses_to_cached_memory_32_bit_02();
	locked_atomic_rqmid_37873_unaligned_accesses_to_cached_memory_64_bit_03();
	locked_atomic_rqmid_28265_16_bit_accesses_to_uncached_memory_locations_01();
	locked_atomic_rqmid_28270_update_segment_descriptor_01();
	locked_atomic_rqmid_39543_update_page_directory_and_page_table_entries_02();
#endif
	return report_summary();
}

