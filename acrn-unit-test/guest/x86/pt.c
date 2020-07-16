#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "pt.h"
#include "vm.h"
#include "vmalloc.h"
#include "misc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "register_op.h"

#define PT_DEBUG
#ifdef PT_DEBUG
#define pt_debug(fmt, arg...)	printf(fmt, ##arg)
#else
#define pt_debug(fmt, arg...)
#endif

#define swap_force(x, y) do { \
	asm volatile("xchg %1, %%" R "ax \n\t" \
				 "xchg %0, %%" R "ax \n\t" \
				 "xchg %1, %%" R "ax \n\t" \
				 : \
				 : "m"(x), "m"(y) \
				 : "memory" \
				); \
} while (0)

static uint32_t cur_vec = 0U;
/*
 * @brief case name: Guest IA32 RTIT STATUS_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_STATUS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_27261_Guest_IA32_RTIT_STATUS_002(void)
{
	bool is_pass = false;
	uint64_t ia32_rtit_status = MSR_VALUE;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	vec = wrmsr_checking(IA32_RTIT_STATUS, ia32_rtit_status);
	err_code = exception_error_code();
	is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT STATUS_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_STATUS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_27246_Guest_IA32_RTIT_STATUS_001(void)
{
	bool is_pass = false;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	uint64_t result = 0UL;
	vec = rdmsr_checking(IA32_RTIT_STATUS, &result);
	err_code = exception_error_code();
	is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: guest cpuid pt bit_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=7H, ECX=0H),
 * ACRN hypervisor shall write 0H to guest EBX [bit 25].
 */
static __unused void pt_rqmid_27268_guest_cpuid_pt_bit_001(void)
{
	bool is_pass = false;
	is_pass = ((cpuid(7).b & CPUID_07_PT_BIT) == 0);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: guest cpuid leaft 14h_001
 *
 * Summary: When a vCPU attempts to read CPUID.14H,
 * ACRN hypervisor shall write 0H to guest EAX, EBX,
 * ECX and EDX.
 */
static __unused void pt_rqmid_27267_guest_cpuid_leaft_14h_001(void)
{
	bool is_pass = false;
	struct cpuid cpuid14h = {0};
	cpuid14h = cpuid(CPUID_14H_PT);
	is_pass = (cpuid14h.a == 0) \
				& (cpuid14h.b == 0) \
				& (cpuid14h.c == 0) \
				& (cpuid14h.d == 0);
	report("%s \n", is_pass, __FUNCTION__);
}

#ifdef __x86_64__
/*
 * @brief case name: Guest PTWRITE_002
 *
 * Summary: When a vCPU attempts to execute PTWRITE,
 * ACRN hypervisor shall guarantee that the vCPU receives #UD.
 */
static __unused void guest_ptwrite_64bit(const char *arg)
{
	uint64_t ptw_packet = 0x123UL;
	asm volatile(ASM_TRY("1f")
			 "REX.W ptwrite  %0\n\t"
			 "1:"
			 : : "m"(ptw_packet));
	/*get the exception vector*/
	cur_vec = exception_vector();
}

static __unused void pt_rqmid_36849_Guest_PTWRITE_002(void)
{
	bool is_pass = false;
	struct cpuid cpuid14h = {0};
	int check_cnt = 0;
	cur_vec = 0U;
	cpuid14h = cpuid(CPUID_14H_PT);
	if ((cpuid14h.b & CPUID_PTWRITE_BIT) == 0U) {
		check_cnt++;
	}
	do_at_ring3(guest_ptwrite_64bit, "");
	if (cur_vec == UD_VECTOR) {
		check_cnt++;
	}
	is_pass = (check_cnt == 2) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

#else

/*
 * @brief case name: Guest PTWRITE_001
 *
 * Summary: When a vCPU attempts to execute PTWRITE,
 * ACRN hypervisor shall guarantee that the vCPU receives #UD.
 */
static __unused void guest_ptwrite_32bit(const char *arg)
{
	uint32_t ptw_packet = 0x123U;

	asm volatile(ASM_TRY("1f")
		     "ptwrite  %0\n\t"
		     "1:"
		     : : "m"(ptw_packet));
	/*get the exception vector*/
	cur_vec = exception_vector();
}

static __unused void pt_rqmid_27270_Guest_PTWRITE_001(void)
{
	bool is_pass = false;
	struct cpuid cpuid14h = {0};
	int check_cnt = 0;
	cur_vec = 0U;
	cpuid14h = cpuid(CPUID_14H_PT);
	if ((cpuid14h.b & CPUID_PTWRITE_BIT) == 0U) {
		check_cnt++;
	}
	do_at_ring3(guest_ptwrite_32bit, "");
	if (cur_vec == UD_VECTOR) {
		check_cnt++;
	}
	is_pass = (check_cnt == 2) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}
#endif

//<The scaling part>
static __unused bool test_Guest_IA32_RTIT_ADDR_x_GP_write(bool is_A, uint32_t IA32_RTIT_ADDR_x)
{
	bool is_pass = false;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	void *addr = NULL;
	uint64_t addr1 = 0UL;
	void *mem = alloc_page();
	if (!mem) {
		is_pass = false;
	} else {
		addr = (is_A) ? mem : (mem + (PAGE_SIZE - 1));
		swap_force(addr, addr1);
		vec = wrmsr_checking(IA32_RTIT_ADDR_x, addr1);
		err_code = exception_error_code();
		free_page((void *)mem);
		is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	}
	return is_pass;
}

static __unused bool test_Guest_IA32_RTIT_ADDR_x_GP_read(uint32_t IA32_RTIT_ADDR_x)
{
	bool is_pass = false;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	uint64_t result = 0UL;
	vec = rdmsr_checking(IA32_RTIT_ADDR_x, &result);
	err_code = exception_error_code();
	is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	return is_pass;
}

/*
 * @brief case name: Guest IA32 RTIT ADDR0 A_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR0_A,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32492_Guest_IA32_RTIT_ADDR0_A_001(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_read(IA32_RTIT_ADDR0_A);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR0 A_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR0_A,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32503_Guest_IA32_RTIT_ADDR0_A_002(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_write(true, IA32_RTIT_ADDR0_A);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR0 B_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR0_B,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32420_Guest_IA32_RTIT_ADDR0_B_001(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_read(IA32_RTIT_ADDR0_B);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR0 B_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR0_B,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32502_Guest_IA32_RTIT_ADDR0_B_002(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_write(false, IA32_RTIT_ADDR0_B);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR1 A_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR1_A,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32419_Guest_IA32_RTIT_ADDR1_A_001(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_read(IA32_RTIT_ADDR1_A);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR1 A_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR1_A,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32501_Guest_IA32_RTIT_ADDR1_A_002(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_write(true, IA32_RTIT_ADDR1_A);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR1 B_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR1_B,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32418_Guest_IA32_RTIT_ADDR1_B_001(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_read(IA32_RTIT_ADDR1_B);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR1 B_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR1_B,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32500_Guest_IA32_RTIT_ADDR1_B_002(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_write(false, IA32_RTIT_ADDR1_B);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR2 A_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR2_A,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32417_Guest_IA32_RTIT_ADDR2_A_001(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_read(IA32_RTIT_ADDR2_A);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR2 A_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR2_A,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32499_Guest_IA32_RTIT_ADDR2_A_002(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_write(true, IA32_RTIT_ADDR2_A);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR2 B_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR2_B,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32416_Guest_IA32_RTIT_ADDR2_B_001(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_read(IA32_RTIT_ADDR2_B);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR2 B_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR2_B,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32498_Guest_IA32_RTIT_ADDR2_B_002(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_write(false, IA32_RTIT_ADDR2_B);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR3 A_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR3_A,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32414_Guest_IA32_RTIT_ADDR3_A_001(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_read(IA32_RTIT_ADDR3_A);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR3 A_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR3_A,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32555_Guest_IA32_RTIT_ADDR3_A_002(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_write(true, IA32_RTIT_ADDR3_A);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR3 B_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR3_B,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32412_Guest_IA32_RTIT_ADDR3_B_001(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_read(IA32_RTIT_ADDR3_B);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT ADDR3 B_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_ADDR3_B,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32497_Guest_IA32_RTIT_ADDR3_B_002(void)
{
	bool is_pass = false;
	is_pass = test_Guest_IA32_RTIT_ADDR_x_GP_write(false, IA32_RTIT_ADDR3_B);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT CR3 MATCH_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_CR3_MATCH,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32493_Guest_IA32_RTIT_CR3_MATCH_001(void)
{
	bool is_pass = false;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	uint64_t result = 0UL;
	vec = rdmsr_checking(IA32_RTIT_CR3_MATCH, &result);
	err_code = exception_error_code();
	is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT CR3 MATCH_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_CR3_MATCH,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32525_Guest_IA32_RTIT_CR3_MATCH_002(void)
{
	bool is_pass = false;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	uint64_t addr = 0UL;
	void *mem = alloc_page();
	if (!mem) {
		is_pass = false;
	} else {
		addr = virt_to_phys(mem);
		vec = wrmsr_checking(IA32_RTIT_CR3_MATCH, addr);
		err_code = exception_error_code();
		free_page((void *)mem);
		is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	}
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT CTL_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_CTL,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32494_Guest_IA32_RTIT_CTL_001(void)
{
	bool is_pass = false;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	uint64_t result = 0UL;
	vec = rdmsr_checking(IA32_RTIT_CTL, &result);
	err_code = exception_error_code();
	is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT CTL_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_CTL,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32526_Guest_IA32_RTIT_CTL_002(void)
{
	bool is_pass = false;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	vec = wrmsr_checking(IA32_RTIT_CTL, 0UL);
	err_code = exception_error_code();
	is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT OUTPUT BASE_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_OUTPUT_BASE,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32495_Guest_IA32_RTIT_OUTPUT_BASE_001(void)
{
	bool is_pass = false;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	uint64_t result = 0UL;
	vec = rdmsr_checking(IA32_RTIT_OUTPUT_BASE, &result);
	err_code = exception_error_code();
	is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT OUTPUT BASE_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_OUTPUT_BASE,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32527_Guest_IA32_RTIT_OUTPUT_BASE_002(void)
{
	bool is_pass = false;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	void *addr = NULL;
	uint64_t addr1 = 0UL;
	void *mem = alloc_page();
	if (!mem) {
		is_pass = false;
	} else {
		addr = mem;
		swap_force(addr, addr1);
		vec = wrmsr_checking(IA32_RTIT_OUTPUT_BASE, addr1);
		err_code = exception_error_code();
		free_page((void *)mem);
		is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	}
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT OUTPUT MASK PTRS_001
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_OUTPUT_MASK_PTRS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32496_Guest_IA32_RTIT_OUTPUT_MASK_PTRS_001(void)
{
	bool is_pass = false;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	uint64_t result = 0UL;
	vec = rdmsr_checking(IA32_RTIT_OUTPUT_MASK_PTRS, &result);
	err_code = exception_error_code();
	is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Guest IA32 RTIT OUTPUT MASK PTRS_002
 *
 * Summary: When a vCPU attempts to access IA32_RTIT_OUTPUT_MASK_PTRS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused void pt_rqmid_32528_Guest_IA32_RTIT_OUTPUT_MASK_PTRS_002(void)
{
	bool is_pass = false;
	uint32_t vec = 0U;
	uint32_t err_code = 0U;
	vec = wrmsr_checking(IA32_RTIT_OUTPUT_MASK_PTRS, 0xFFUL);
	err_code = exception_error_code();
	is_pass = ((vec == GP_VECTOR) & (err_code == 0)) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

static __unused void print_case_list(void)
{
	printf("PT feature case list:\n\r");
#ifdef IN_NATIVE

	printf("\t\t Case ID:%d case name:%s\n\r", 27268u,
	"guest cpuid pt bit_001");
#ifdef __x86_64__
	printf("\t\t Case ID:%d case name:%s\n\r", 36849u,
	"Guest PTWRITE_002");
#else
	printf("\t\t Case ID:%d case name:%s\n\r", 27270u,
	"Guest PTWRITE_001");
#endif

#else

	printf("\t\t Case ID:%d case name:%s\n\r", 27268u,
	"guest cpuid pt bit_001");
#ifdef __x86_64__
	printf("\t\t Case ID:%d case name:%s\n\r", 36849u,
	"Guest PTWRITE_002");
#else
	printf("\t\t Case ID:%d case name:%s\n\r", 27270u,
	"Guest PTWRITE_001");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 27267u,
	"guest cpuid leaft 14h_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27261u,
	"Guest IA32 RTIT STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27246u,
	"Guest IA32 RTIT STATUS_001");
	//<The scaling part start>
	printf("\t\t Case ID:%d case name:%s\n\r", 32492u,
	"Guest IA32 RTIT ADDR0 A_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32503u,
	"Guest IA32 RTIT ADDR0 A_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32420u,
	"Guest IA32 RTIT ADDR0 B_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32502u,
	"Guest IA32 RTIT ADDR0 B_002");

	printf("\t\t Case ID:%d case name:%s\n\r", 32419u,
	"Guest IA32 RTIT ADDR1 A_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32501u,
	"Guest IA32 RTIT ADDR1 A_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32418u,
	"Guest IA32 RTIT ADDR1 B_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32500u,
	"Guest IA32 RTIT ADDR1 B_002");

	printf("\t\t Case ID:%d case name:%s\n\r", 32417u,
	"Guest IA32 RTIT ADDR2 A_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32499u,
	"Guest IA32 RTIT ADDR2 A_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32416u,
	"Guest IA32 RTIT ADDR2 B_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32498u,
	"Guest IA32 RTIT ADDR2 B_002");

	printf("\t\t Case ID:%d case name:%s\n\r", 32414u,
	"Guest IA32 RTIT ADDR3 A_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32555u,
	"Guest IA32 RTIT ADDR3 A_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32412u,
	"Guest IA32 RTIT ADDR3 B_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32497u,
	"Guest IA32 RTIT ADDR3 B_002");

	printf("\t\t Case ID:%d case name:%s\n\r", 32493u,
	"Guest IA32 RTIT CR3 MATCH_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32525u,
	"Guest IA32 RTIT CR3 MATCH_002");

	printf("\t\t Case ID:%d case name:%s\n\r", 32494u,
	"Guest IA32 RTIT CTL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32526u,
	"Guest IA32 RTIT CTL_002");

	printf("\t\t Case ID:%d case name:%s\n\r", 32495u,
	"Guest IA32 RTIT OUTPUT BASE_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32527u,
	"Guest IA32 RTIT OUTPUT BASE_002");

	printf("\t\t Case ID:%d case name:%s\n\r", 32496u,
	"Guest IA32 RTIT OUTPUT MASK PTRS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32528u,
	"Guest IA32 RTIT OUTPUT MASK PTRS_002");
	//<The scaling part end>
#endif
}

int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	print_case_list();
#ifdef IN_NATIVE

	pt_rqmid_27268_guest_cpuid_pt_bit_001();
#ifdef __x86_64__
	pt_rqmid_36849_Guest_PTWRITE_002();
#else
	pt_rqmid_27270_Guest_PTWRITE_001();
#endif

#else

	pt_rqmid_27268_guest_cpuid_pt_bit_001();
#ifdef __x86_64__
	pt_rqmid_36849_Guest_PTWRITE_002();
#else
	pt_rqmid_27270_Guest_PTWRITE_001();
#endif

	pt_rqmid_27267_guest_cpuid_leaft_14h_001();
	pt_rqmid_27261_Guest_IA32_RTIT_STATUS_002();
	pt_rqmid_27246_Guest_IA32_RTIT_STATUS_001();
	//<The scaling part start>
	pt_rqmid_32492_Guest_IA32_RTIT_ADDR0_A_001();
	pt_rqmid_32503_Guest_IA32_RTIT_ADDR0_A_002();
	pt_rqmid_32420_Guest_IA32_RTIT_ADDR0_B_001();
	pt_rqmid_32502_Guest_IA32_RTIT_ADDR0_B_002();

	pt_rqmid_32419_Guest_IA32_RTIT_ADDR1_A_001();
	pt_rqmid_32501_Guest_IA32_RTIT_ADDR1_A_002();
	pt_rqmid_32418_Guest_IA32_RTIT_ADDR1_B_001();
	pt_rqmid_32500_Guest_IA32_RTIT_ADDR1_B_002();

	pt_rqmid_32417_Guest_IA32_RTIT_ADDR2_A_001();
	pt_rqmid_32499_Guest_IA32_RTIT_ADDR2_A_002();
	pt_rqmid_32416_Guest_IA32_RTIT_ADDR2_B_001();
	pt_rqmid_32498_Guest_IA32_RTIT_ADDR2_B_002();

	pt_rqmid_32414_Guest_IA32_RTIT_ADDR3_A_001();
	pt_rqmid_32555_Guest_IA32_RTIT_ADDR3_A_002();
	pt_rqmid_32412_Guest_IA32_RTIT_ADDR3_B_001();
	pt_rqmid_32497_Guest_IA32_RTIT_ADDR3_B_002();

	pt_rqmid_32493_Guest_IA32_RTIT_CR3_MATCH_001();
	pt_rqmid_32525_Guest_IA32_RTIT_CR3_MATCH_002();

	pt_rqmid_32494_Guest_IA32_RTIT_CTL_001();
	pt_rqmid_32526_Guest_IA32_RTIT_CTL_002();

	pt_rqmid_32495_Guest_IA32_RTIT_OUTPUT_BASE_001();
	pt_rqmid_32527_Guest_IA32_RTIT_OUTPUT_BASE_002();

	pt_rqmid_32496_Guest_IA32_RTIT_OUTPUT_MASK_PTRS_001();
	pt_rqmid_32528_Guest_IA32_RTIT_OUTPUT_MASK_PTRS_002();
	//<The scaling part end>
#endif
	return report_summary();
}
