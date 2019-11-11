#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "pt.h"
#include "vm.h"
#include "vmalloc.h"
#include "misc.h"

static int test_at_ring3(void (*fn)(void), const char *arg)
{
	static unsigned char user_stack[4096];
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
		  "mov %%dx, %%ds\n\t"
		  "mov %%dx, %%es\n\t"
		  "mov %%dx, %%fs\n\t"
		  "mov %%dx, %%gs\n\t"
		  "mov %%" R "sp, %%" R "cx\n\t"
		  "push" W " %%" R "dx \n\t"
		  "lea %[user_stack_top], %%" R "dx \n\t"
		  "push" W " %%" R "dx \n\t"
		  "pushf" W "\n\t"
		  "push" W " %[user_cs] \n\t"
		  "push" W " $1f \n\t"
		  "iret" W "\n"
		  "1: \n\t"
		  "push %%" R "cx\n\t"   /* save kernel SP */

#ifndef __x86_64__
		  "push %[arg]\n\t"
#endif
		  "call *%[fn]\n\t"
#ifndef __x86_64__
		  "pop %%ecx\n\t"
#endif

		  "pop %%" R "cx\n\t"
		  "mov $1f, %%" R "dx\n\t"
		  "int %[kernel_entry_vector]\n\t"
		  ".section .text.entry \n\t"
		  "kernel_entry: \n\t"
		  "mov %%" R "cx, %%" R "sp \n\t"
		  "mov %[kernel_ds], %%cx\n\t"
		  "mov %%cx, %%ds\n\t"
		  "mov %%cx, %%es\n\t"
		  "mov %%cx, %%fs\n\t"
		  "mov %%cx, %%gs\n\t"
		  "jmp *%%" R "dx \n\t"
		  ".section .text\n\t"
		  "1:\n\t"
		  : [ret] "=&a" (ret)
		  : [user_ds] "i" (USER_DS),
		    [user_cs] "i" (USER_CS),
		    [user_stack_top]"m"(user_stack[sizeof user_stack]),
		    [fn]"r"(fn),
		    [arg]"D"(arg),
		    [kernel_ds]"i"(KERNEL_DS),
		    [kernel_entry_vector]"i"(0x20)
		  : "rcx", "rdx");
	return ret;
}


static int ptwrite_checking()
{
	u64 ptw_packet = 0x123;

	asm volatile(ASM_TRY("1f")
		     "ptwrite  %0\n\t"
		     "1:"
		     : : "m"(ptw_packet));
	return exception_vector();
}

static void guest_ptwrite()
{
	report("\t\t pt_rqmid_27270_check_ptwrite_support", ptwrite_checking() == UD_VECTOR);
}

/**
 * @brief case name: Guest PTWRITE_001
 *
 * Summary: Under ring3 environment, execute PTWRITE instruction shall generate #UD
 */
 static void pt_rqmid_27270_check_ptwrite_support()
{
	test_at_ring3(guest_ptwrite, "test ring3");
}

/**
 * @brief case name: Guest IA32 RTIT STATUS_002
 *
 * Summary: Execute write MSR IA32_RTIT_STATUS shall generate #GP
 */
static void pt_rqmid_27261_write_msr_ia32_rtit_status()
{
	u64 ia32_rtit_status = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_STATUS, ia32_rtit_status)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT STATUS_001
 *
 * Summary: Execute read MSR IA32_RTIT_STATUS shall generate #GP
 */
static void pt_rqmid_27246_read_msr_ia32_rtit_status()
{
	u64 ia32_rtit_status;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_STATUS, &ia32_rtit_status)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: guest cpuid pt bit_001
 *
 * Summary:Read CPUID.(EAX=7H,ECX=0H):EBX[bit 25] shall be 0
 */
static void pt_rqmid_27268_check_pt_supported()
{
	report("\t\t %s", (cpuid(7).b & CPUID_07_PT_BIT) == 0, __FUNCTION__);
}

/**
 * @brief case name: guest cpuid leaft 14h_001
 *
 * Summary: Execute CPUID.14H get EAX,EBX,ECX,EDX shall be 0
 */
static void pt_rqmid_27267_guest_cpuid_leaf_14h()
{
	bool flag = false;
	if (cpuid(CPUID_14H_PT).a == 0 && cpuid(CPUID_14H_PT).b == 0
		&& cpuid(CPUID_14H_PT).c == 0 && cpuid(CPUID_14H_PT).d == 0) {
		flag = true;
	}
	report("\t\t %s", flag, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT OUTPUT MASK PTRS_002
 *
 * Summary: Execute write MSR IA32_RTIT_OUTPUT_MASK_PTRS shall generate #GP
 */
static void pt_rqmid_27264_write_msr_ia32_rtit_output_mask_ptrs()
{
	u64 ia32_rtit_output_mask_ptrs = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_OUTPUT_MASK_PTRS,
		ia32_rtit_output_mask_ptrs) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT OUTPUT MASK PTRS_001
 *
 * Summary: Execute read MSR IA32_RTIT_OUTPUT_MASK_PTRS shall generate #GP
 */
static void pt_rqmid_27250_read_msr_ia32_rtit_output_mask_ptrs()
{
	u64 ia32_rtit_output_mask_ptrs;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_OUTPUT_MASK_PTRS,
		&ia32_rtit_output_mask_ptrs) == GP_VECTOR, __FUNCTION__);
}


/**
 * @brief case name: Guest IA32 RTIT OUTPUT BASE_002
 *
 * Summary: Execute write MSR IA32_RTIT_OUTPUT_BASE shall generate #GP
 */
static void pt_rqmid_27263_write_msr_ia32_rtit_output_base()
{
	u64 ia32_rtit_output_base = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_OUTPUT_BASE,
		ia32_rtit_output_base) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT OUTPUT BASE_001
 *
 * Summary: Execute read MSR IA32_RTIT_OUTPUT_BASE shall generate #GP
 */
static void pt_rqmid_27248_read_msr_ia32_rtit_output_base()
{
	u64 ia32_rtit_output_base;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_OUTPUT_BASE,
		&ia32_rtit_output_base) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT CTL_002
 *
 * Summary: Execute write MSR IA32_RTIT_CTL shall generate #GP
 */
static void pt_rqmid_27262_write_msr_ia32_rtit_ctl()
{
	u64 ia32_rtit_ctl = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_CTL,
		ia32_rtit_ctl) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT CTL_001
 *
 * Summary: Execute read MSR IA32_RTIT_CTL shall generate #GP
 */
static void pt_rqmid_27247_read_msr_ia32_rtit_ctl()
{
	u64 ia32_rtit_ctl;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_CTL,
		&ia32_rtit_ctl) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT CR3 MATCH_002
 *
 * Summary: Execute write MSR IA32_RTIT_CR3_MATCH shall generate #GP
 */
static void pt_rqmid_27259_write_msr_ia32_rtit_cr3_match()
{
	u64 ia32_rtit_cr3_match = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_CR3_MATCH,
		ia32_rtit_cr3_match) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT CR3 MATCH_001
 *
 * Summary: Execute read MSR IA32_RTIT_CR3_MATCH shall generate #GP
 */
static void pt_rqmid_27245_read_msr_ia32_rtit_cr3_match()
{
	u64 ia32_rtit_cr3_match;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_CR3_MATCH,
		&ia32_rtit_cr3_match) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR3 B_002
 *
 * Summary: Execute write MSR IA32_RTIT_ADDR3_B shall generate #GP
 */
static void pt_rqmid_27251_write_msr_ia32_rtit_addr3_b()
{
	u64 ia32_rtit_addr3_b = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_ADDR3_B,
		ia32_rtit_addr3_b) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR3 B_001
 *
 * Summary: Execute read MSR IA32_RTIT_ADDR3_B shall generate #GP
 */
static void pt_rqmid_27233_read_msr_ia32_rtit_addr3_b()
{
	u64 ia32_rtit_addr3_b;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_ADDR3_B,
		&ia32_rtit_addr3_b) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR2 B_002
 *
 * Summary: Execute write MSR IA32_RTIT_ADDR2_B shall generate #GP
 */
static void pt_rqmid_27253_write_msr_ia32_rtit_addr2_b()
{
	u64 ia32_rtit_addr2_b = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_ADDR2_B,
		ia32_rtit_addr2_b) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR2 B_001
 *
 * Summary: Execute read MSR IA32_RTIT_ADDR2_B shall generate #GP
 */
static void pt_rqmid_27239_read_msr_ia32_rtit_addr2_b()
{
	u64 ia32_rtit_addr2_b;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_ADDR2_B,
		&ia32_rtit_addr2_b) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR1 B_002
 *
 * Summary: Execute write MSR IA32_RTIT_ADDR1_B shall generate #GP
 */
static void pt_rqmid_27255_write_msr_ia32_rtit_addr1_b()
{
	u64 ia32_rtit_addr1_b = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_ADDR1_B,
		ia32_rtit_addr1_b) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR1 B_001
 *
 * Summary: Execute read MSR IA32_RTIT_ADDR1_B shall generate #GP
 */
static void pt_rqmid_27240_read_msr_ia32_rtit_addr1_b()
{
	u64 ia32_rtit_addr1_b;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_ADDR1_B,
		&ia32_rtit_addr1_b) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR0 B_002
 *
 * Summary: Execute write MSR IA32_RTIT_ADDR0_B shall generate #GP
 */
static void pt_rqmid_27257_write_msr_ia32_rtit_addr0_b()
{
	u64 ia32_rtit_addr0_b = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_ADDR0_B,
		ia32_rtit_addr0_b) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR0 B_001
 *
 * Summary: Execute read MSR IA32_RTIT_ADDR0_B shall generate #GP
 */
static void pt_rqmid_27243_read_msr_ia32_rtit_addr0_b()
{
	u64 ia32_rtit_addr0_b;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_ADDR0_B,
		&ia32_rtit_addr0_b) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR3 A_002
 *
 * Summary: Execute write MSR IA32_RTIT_ADDR3_A shall generate #GP
 */
static void pt_rqmid_27252_write_msr_ia32_rtit_addr3_a()
{
	u64 ia32_rtit_addr3_a = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_ADDR3_A,
		ia32_rtit_addr3_a) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR3 A_001
 *
 * Summary: Execute read MSR IA32_RTIT_ADDR3_A shall generate #GP
 */
static void pt_rqmid_27237_read_msr_ia32_rtit_addr3_a()
{
	u64 ia32_rtit_addr3_a;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_ADDR3_A,
		&ia32_rtit_addr3_a) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR2 A_002
 *
 * Summary: Execute write MSR IA32_RTIT_ADDR2_A shall generate #GP
 */
static void pt_rqmid_27254_write_msr_ia32_rtit_addr2_a()
{
	u64 ia32_rtit_addr2_a = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_ADDR2_A,
		ia32_rtit_addr2_a) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR2 A_001
 *
 * Summary: Execute read MSR IA32_RTIT_ADDR2_A shall generate #GP
 */
static void pt_rqmid_27235_read_msr_ia32_rtit_addr2_a()
{
	u64 ia32_rtit_addr2_a;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_ADDR2_A,
		&ia32_rtit_addr2_a) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR1 A_002
 *
 * Summary: Execute write MSR IA32_RTIT_ADDR1_A shall generate #GP
 */
static void pt_rqmid_27256_write_msr_ia32_rtit_addr1_a()
{
	u64 ia32_rtit_addr1_a = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_ADDR1_A,
		ia32_rtit_addr1_a) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR1 A_001
 *
 * Summary: Execute read MSR IA32_RTIT_ADDR1_A shall generate #GP
 */
static void pt_rqmid_27242_read_msr_ia32_rtit_addr1_a()
{
	u64 ia32_rtit_addr1_a;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_ADDR1_A,
		&ia32_rtit_addr1_a) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR0 A_002
 *
 * Summary: Execute write MSR IA32_RTIT_ADDR0_A shall generate #GP
 */
static void pt_rqmid_27258_write_msr_ia32_rtit_addr0_a()
{
	u64 ia32_rtit_addr0_a = MSR_VALUE;
	report("\t\t %s",
		wrmsr_checking(IA32_RTIT_ADDR0_A,
		ia32_rtit_addr0_a) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 RTIT ADDR0 A_001
 *
 * Summary: Execute read MSR IA32_RTIT_ADDR0_A shall generate #GP
 */
static void pt_rqmid_27244_read_msr_ia32_rtit_addr0_a()
{
	u64 ia32_rtit_addr0_a;
	report("\t\t %s",
		rdmsr_checking(IA32_RTIT_ADDR0_A,
		&ia32_rtit_addr0_a) == GP_VECTOR, __FUNCTION__);
}

static void print_case_list()
{
	printf("PT feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 27270u, "Guest PTWRITE_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27261u, "Guest IA32 RTIT STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27246u, "Guest IA32 RTIT STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27268u, "guest cpuid pt bit_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27267u, "guest cpuid leaft 14h_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27251u, "Guest IA32 RTIT ADDR3 B_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27233u, "Guest IA32 RTIT ADDR3 B_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27253u, "Guest IA32 RTIT ADDR2 B_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27239u, "Guest IA32 RTIT ADDR2 B_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27255u, "Guest IA32 RTIT ADDR1 B_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27240u, "Guest IA32 RTIT ADDR1 B_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27257u, "Guest IA32 RTIT ADDR0 B_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27243u, "Guest IA32 RTIT ADDR0 B_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27252u, "Guest IA32 RTIT ADDR3 A_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27237u, "Guest IA32 RTIT ADDR3 A_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27254u, "Guest IA32 RTIT ADDR2 A_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27235u, "Guest IA32 RTIT ADDR2 A_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27256u, "Guest IA32 RTIT ADDR1 A_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27242u, "Guest IA32 RTIT ADDR1 A_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27258u, "Guest IA32 RTIT ADDR0 A_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27244u, "Guest IA32 RTIT ADDR0 A_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27264u, "Guest IA32 RTIT OUTPUT MASK PTRS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27250u, "Guest IA32 RTIT OUTPUT MASK PTRS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27263u, "Guest IA32 RTIT OUTPUT BASE_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27248u, "Guest IA32 RTIT OUTPUT BASE_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27262u, "Guest IA32 RTIT CTL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27247u, "Guest IA32 RTIT CTL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27259u, "Guest IA32 RTIT CR3 MATCH_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27245u, "Guest IA32 RTIT CR3 MATCH_001");
	printf("-------------------------\n\r");
}

static void test_pt()
{
	pt_rqmid_27270_check_ptwrite_support();
	pt_rqmid_27261_write_msr_ia32_rtit_status();
	pt_rqmid_27246_read_msr_ia32_rtit_status();
	pt_rqmid_27268_check_pt_supported();
	pt_rqmid_27267_guest_cpuid_leaf_14h();
	pt_rqmid_27251_write_msr_ia32_rtit_addr3_b();
	pt_rqmid_27233_read_msr_ia32_rtit_addr3_b();
	pt_rqmid_27253_write_msr_ia32_rtit_addr2_b();
	pt_rqmid_27239_read_msr_ia32_rtit_addr2_b();
	pt_rqmid_27255_write_msr_ia32_rtit_addr1_b();
	pt_rqmid_27240_read_msr_ia32_rtit_addr1_b();
	pt_rqmid_27257_write_msr_ia32_rtit_addr0_b();
	pt_rqmid_27243_read_msr_ia32_rtit_addr0_b();
	pt_rqmid_27252_write_msr_ia32_rtit_addr3_a();
	pt_rqmid_27237_read_msr_ia32_rtit_addr3_a();
	pt_rqmid_27254_write_msr_ia32_rtit_addr2_a();
	pt_rqmid_27235_read_msr_ia32_rtit_addr2_a();
	pt_rqmid_27256_write_msr_ia32_rtit_addr1_a();
	pt_rqmid_27242_read_msr_ia32_rtit_addr1_a();
	pt_rqmid_27258_write_msr_ia32_rtit_addr0_a();
	pt_rqmid_27244_read_msr_ia32_rtit_addr0_a();
	pt_rqmid_27264_write_msr_ia32_rtit_output_mask_ptrs();
	pt_rqmid_27250_read_msr_ia32_rtit_output_mask_ptrs();
	pt_rqmid_27263_write_msr_ia32_rtit_output_base();
	pt_rqmid_27248_read_msr_ia32_rtit_output_base();
	pt_rqmid_27262_write_msr_ia32_rtit_ctl();
	pt_rqmid_27247_read_msr_ia32_rtit_ctl();
	pt_rqmid_27259_write_msr_ia32_rtit_cr3_match();
	pt_rqmid_27245_read_msr_ia32_rtit_cr3_match();
}

int main(void)
{
	extern unsigned char kernel_entry;
	setup_vm();
	setup_idt();
	set_idt_entry(0x20, &kernel_entry, 3);

	print_case_list();
	test_pt();

	return report_summary();
}

