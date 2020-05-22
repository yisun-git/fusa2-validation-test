#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"


#define BIT_SHIFT(n)				((1U) << (n))
#define VMX_ECX_SUPPORT_MASK(b)		BIT_SHIFT(b)
#define CR4_VMXE_BIT_MASK			BIT_SHIFT(13)

#define VMX_DEBUG
#ifdef VMX_DEBUG
 #define vmx_debug(fmt, args...)	printf(fmt, ##args)
#else
 #define vmx_debug(fmt, args...)
#endif

#ifdef __x86_64__
/*
 * @brief case name: cpuid.01h:ECX[5]_001
 *
 * Summary: When a vCPU attempts to read CPUID.01H,
 * ACRN Hypervisor shall guarantee guest ECX[5] shall be 0.
 */
static __unused void vmx_rqmid_26828_cpuid_01h_ECX_bit5_001(void)
{
	struct cpuid cpuid_1 = {0};
	cpuid_1 = cpuid(1);
	u32 eax = cpuid_1.a;
	u32 ebx = cpuid_1.b;
	u32 ecx = cpuid_1.c;
	u32 edx = cpuid_1.d;

	/*Dump CPUID.01H*/
	printf("\ndump cpuid(1).eax=%x, ebx=%x, ecx=%x, edx=%x \n", eax, ebx, ecx, edx);

	/*Check guest ECX[5]*/
	ecx  &= VMX_ECX_SUPPORT_MASK(5);
	report("%s", (ecx == 0), __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_CR0_FIXED0_001
 *
 * Summary: When a vCPU attempts to read guest IA32_VMX_CR0_FIXED0,
 * ACRN hypervisor shall guarantee that the vCPU will receive #GP(0).
 */
static __unused void vmx_rqmid_26856_IA32_VMX_CR0_FIXED0_001(void)
{
	u32 a = 0;
	u32 d = 0;
	u32 index = MSR_IA32_VMX_CR0_FIXED0;
	asm volatile (ASM_TRY("1f")
				"rdmsr\n\t"
				"1:"
				: "=a"(a), "=d"(d)
				: "c"(index)
				: "memory");
	vmx_debug("\nexception_error_code = %d \n", exception_error_code());
	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == 0), __FUNCTION__);
}

/*
 * @brief case name: VMCALL_001
 *
 * Summary: When a vCPU attempts to execute VMCALL,
 * ACRN hypervisor shall guarantee that the vCPU will receive UD.
 */
static __unused void vmx_rqmid_29008_VMCALL_001(void)
{
	u32 a = 0;
	u32 b = 0;
	u32 c = 0;
	u32 d = 0;
	int ret = 0;
	u64 phy_addr = 0UL;
	void *mem = alloc_page();
	/*Enter VMX operation*/
	phy_addr = virt_to_phys(mem);
	if (0UL == phy_addr) {
		printf("\n %s phy_addr failed! \n", __FUNCTION__);
	}
	asm volatile(ASM_TRY("1f")
				 "vmxon (%%rax)\n\t"
				 "1:"
				:
				: "a"(phy_addr)
				: "cc", "memory");
	ret += (exception_vector() == UD_VECTOR);

	/*execute VMCALL*/
	asm volatile (ASM_TRY("1f")
				"vmcall\n\t"
				"1:"
				: "+a"(a), "=b"(b), "=c"(c), "=d"(d));

	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	report("%s", (ret == 2), __FUNCTION__);
}

/*
 * @brief case name: CR4.VMXE[13] initial INIT_001
 *
 * Summary: ACRN hypervisor shall set the initial cr4.VMXE[13] of guest VM to 0 following INIT.
 */
static __unused void vmx_rqmid_28988_CR4_VMXE_initial_INIT_001(void)
{

	volatile u16 ap_cr4 = 0;
	volatile u32 *ptr = NULL;
	/*Start up AP, refer to cstart64.S*/

	/*Get CR4 register value after AP start INIT, refer to ASM/vmx_init.S*/

	/*Check CR4.VMXE value*/
	ptr = (volatile u32 *)0x7004;
	ap_cr4 = *ptr;
	vmx_debug("AP:\n\r");
	vmx_debug("ap_greg_cr4:0x%x\n\r", ap_cr4);
	ap_cr4 &= CR4_VMXE_BIT_MASK;

	report("%s", (ap_cr4 == 0), __FUNCTION__);
}
#endif

static void print_case_list(void)
{
	printf("\t\t VMX feature case list:\n\r");
#ifdef __x86_64__
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 28988u, "CR4.VMXE[13] initial INIT_001");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 26828u, "cpuid.01h:ECX[5]_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26856u, "IA32_VMX_CR0_FIXED0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29008u, "VMCALL_001");
#endif
}

int main(void)
{
	setup_idt();
	print_case_list();
#ifdef __x86_64__
#ifdef IN_NON_SAFETY_VM
	vmx_rqmid_28988_CR4_VMXE_initial_INIT_001();
#endif
	setup_vm();
	vmx_rqmid_26828_cpuid_01h_ECX_bit5_001();
	vmx_rqmid_26856_IA32_VMX_CR0_FIXED0_001();
	vmx_rqmid_29008_VMCALL_001();
#endif
	return report_summary();
}
