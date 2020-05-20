#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "pt.h"
#include "vm.h"
#include "vmalloc.h"
#include "misc.h"

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
	uint32_t a = ia32_rtit_status;
	uint32_t d = ia32_rtit_status >> 32;
	uint32_t c = IA32_RTIT_STATUS;
	asm volatile (ASM_TRY("1f")
				  "wrmsr\n\t"
				  "1:"
				  :
				  : "a"(a), "d"(d), "c"(c)
				  : "memory");
	vec = exception_vector();
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
	uint32_t c = IA32_RTIT_STATUS;
	asm volatile(ASM_TRY("1f")
			 "rdmsr\n\t"
			 "1:"
			 :
			 : "c"(c)
			 : "memory");
	vec = exception_vector();
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
	cur_vec = 0U;
	do_at_ring3(guest_ptwrite_64bit, "");
	is_pass = (cur_vec == UD_VECTOR) ? true : false;
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
	cur_vec = 0U;
	do_at_ring3(guest_ptwrite_32bit, "");
	is_pass = (cur_vec == UD_VECTOR) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

#endif

static __unused void print_case_list(void)
{
	printf("PT feature case list:\n\r");
#ifdef __x86_64__
	printf("\t\t Case ID:%d case name:%s\n\r", 36849u,
	"Guest PTWRITE_002");
#else
	printf("\t\t Case ID:%d case name:%s\n\r", 27270u,
	"Guest PTWRITE_001");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 27267u,
	"guest cpuid leaft 14h_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27268u,
	"guest cpuid pt bit_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27261u,
	"Guest IA32 RTIT STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27246u,
	"Guest IA32 RTIT STATUS_001");
}

int main(void)
{
	setup_vm();
	setup_idt();
	extern unsigned char kernel_entry;
	set_idt_entry(0x23, &kernel_entry, 3);
	print_case_list();
#ifdef __x86_64__
	pt_rqmid_36849_Guest_PTWRITE_002();
#else
	pt_rqmid_27270_Guest_PTWRITE_001();
#endif
	pt_rqmid_27267_guest_cpuid_leaft_14h_001();
	pt_rqmid_27268_guest_cpuid_pt_bit_001();
	pt_rqmid_27261_Guest_IA32_RTIT_STATUS_002();
	pt_rqmid_27246_Guest_IA32_RTIT_STATUS_001();
	return report_summary();
}
