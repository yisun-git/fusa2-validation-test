#include "segmentation.h"

#define USE_HAND_EXECEPTION
//#define USE_DEBUG
#ifdef USE_DEBUG
#define debug_print(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)
#else
#define debug_print(fmt, args...)
#endif
#define debug_error(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)

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
	lss.selector = 0x0;

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
	lss.selector = 0x2;

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
	lss.selector = 0x180;

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
	lss.selector = 0x280;

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
	lss.selector = 0x281;

	asm volatile(
		"lss  %0, %%eax\t\n"
		::"m"(lss)
	);
}

void lss_index_1024(void *data)
{
	struct lseg_st lss;

	lss.offset = 0xffffffff;
	/*index =50 RPL =0 */
	lss.selector = 0x2000;

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
	lfs.selector = 0x180;

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
	lfs.selector = 0x280;

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
	lfs.selector = 0x183;

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
	lfs.selector = 0x283;

	asm volatile(
		"lfs  %0, %%eax\t\n"
		::"m"(lfs)
	);
}

void lfs_index_1024(void *data)
{
	struct lseg_st lfs;

	lfs.offset = 0xffffffff;
	/*index =1K RPL =0 */
	lfs.selector = 0x2000;

	asm volatile(
		"lfs  %0, %%eax\t\n"
		::"m"(lfs)
	);
}

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

int main(void)
{
	setup_vm();

	setup_ring_env();

	setup_idt();

	print_case_list();

	test_segmentation();

	return report_summary();
}

