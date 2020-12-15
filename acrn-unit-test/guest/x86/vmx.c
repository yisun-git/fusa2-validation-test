#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "misc.h"
#include "pci_util.h"
#include "vmx_check.h"

#define BIT_SHIFT(n)    ((1U) << (n))
#define VMX_ECX_SUPPORT_MASK(b)    BIT_SHIFT(b)
#define CR4_VMXE_BIT_MASK    BIT_SHIFT(13)
#define MSR_IA32_VMX_VMFUNC    0x00000491
#define VPID    (0x00U)

/*VMX_DEBUG for case log info*/
#define VMX_DEBUG
#ifdef VMX_DEBUG
 #define vmx_debug(fmt, args...)	printf(fmt, ##args)
#else
 #define vmx_debug(fmt, args...)
#endif

struct vmcs_hdr {
	u32 revision_id:31;
	u32 shadow_vmcs:1;
};

struct vmcs {
	struct vmcs_hdr hdr;
	u32 abort; /* VMX-abort indicator */
	/* VMCS data */
	char data[0];
};

static __unused
bool test_msr_write_exception(uint32_t index, uint64_t value, uint32_t vec, uint32_t err_code)
{
	int _vec = 0;
	unsigned _err_code = 0;
	bool is_pass = false;
	uint32_t edx = value >> 32;
	uint32_t eax = value;
	asm volatile(ASM_TRY("1f")
		"wrmsr \n\t"
		"1:"
		: : "c"(index), "a"(eax), "d"(edx));
	_vec = exception_vector();
	_err_code = exception_error_code();
	is_pass = ((_vec == vec) && (_err_code == err_code));
	return is_pass;
}

static __unused
bool test_msr_read_exception(uint32_t index, uint64_t *value, uint32_t vec, uint32_t err_code)
{
	int _vec = 0;
	unsigned _err_code = 0;
	bool is_pass = false;
	uint32_t eax = 0U;
	uint32_t edx = 0U;
	asm volatile(ASM_TRY("1f")
			"rdmsr \n\t"
			"1:"
			: "=a"(eax), "=d"(edx) : "c"(index));
	*value = eax + ((u64)edx << 32);
	_vec = exception_vector();
	_err_code = exception_error_code();
	is_pass = ((_vec == vec) && (_err_code == err_code));
	return is_pass;
}

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
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_CR0_FIXED0, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
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
	ptr = (volatile u32 *)INIT_CR4_SAVE_ADDR;
	ap_cr4 = *ptr;
	vmx_debug("AP:\n\r");
	vmx_debug("ap_greg_cr4:0x%x\n\r", ap_cr4);
	ap_cr4 &= CR4_VMXE_BIT_MASK;

	report("%s", (ap_cr4 == 0), __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_VMFUNC_002
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_VMFUNC,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26835_IA32_VMX_VMFUNC_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_VMFUNC, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_VMFUNC_001
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_VMFUNC,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26834_IA32_VMX_VMFUNC_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_VMFUNC, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_VMCS_ENUM_002
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_VMCS_ENUM,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26849_IA32_VMX_VMCS_ENUM_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_VMCS_ENUM, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_VMCS_ENUM_001
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_VMCS_ENUM,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26848_IA32_VMX_VMCS_ENUM_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_VMCS_ENUM, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_PROCBASED_CTLS2_002
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_PROCBASED_CTLS2,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26847_IA32_VMX_PROCBASED_CTLS2_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_PROCBASED_CTLS2, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_PROCBASED_CTLS2_001
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_PROCBASED_CTLS2,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26846_IA32_VMX_PROCBASED_CTLS2_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_PROCBASED_CTLS2, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_TRUE_PROCBASED_CTLS_002
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_TRUE_PROCBASED_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26841_IA32_VMX_TRUE_PROCBASED_CTLS_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_TRUE_PROC, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_TRUE_PROCBASED_CTLS_001
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_TRUE_PROCBASED_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26840_IA32_VMX_TRUE_PROCBASED_CTLS_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_TRUE_PROC, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_TRUE_PINBASED_CTLS_002
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_TRUE_PINBASED_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26843_IA32_VMX_TRUE_PINBASED_CTLS_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_TRUE_PIN, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_TRUE_PINBASED_CTLS_001
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_TRUE_PINBASED_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26842_IA32_VMX_TRUE_PINBASED_CTLS_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_TRUE_PIN, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_TRUE_EXIT_CTLS_002
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_TRUE_EXIT_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26839_IA32_VMX_TRUE_EXIT_CTLS_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_TRUE_EXIT, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_TRUE_EXIT_CTLS_001
 *
 * Summary: When a vCPU attempts to access guest IA32_VMX_TRUE_EXIT_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26838_IA32_VMX_TRUE_EXIT_CTLS_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_TRUE_EXIT, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_TRUE_ENTRY_CTLS_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_TRUE_ENTRY_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26837_IA32_VMX_TRUE_ENTRY_CTLS_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_TRUE_ENTRY, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_TRUE_ENTRY_CTLS_001
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_TRUE_ENTRY_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26836_IA32_VMX_TRUE_ENTRY_CTLS_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_TRUE_ENTRY, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_PROCBASED_CTLS_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_PROCBASED_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26865_IA32_VMX_PROCBASED_CTLS_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_PROCBASED_CTLS, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_PROCBASED_CTLS_001
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_PROCBASED_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26864_IA32_VMX_PROCBASED_CTLS_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_PROCBASED_CTLS, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_PINBASED_CTLS_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_PINBASED_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26867_IA32_VMX_PINBASED_CTLS_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_PINBASED_CTLS, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_PINBASED_CTLS_001
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_PINBASED_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26866_IA32_VMX_PINBASED_CTLS_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_PINBASED_CTLS, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_MISC_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_MISC,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26859_IA32_VMX_MISC_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_MISC, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_MISC_001
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_MISC,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26858_IA32_VMX_MISC_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_MISC, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_EXIT_CTLS_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_EXIT_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26863_IA32_VMX_EXIT_CTLS_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_EXIT_CTLS, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_EXIT_CTLS_001
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_EXIT_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26862_IA32_VMX_EXIT_CTLS_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_EXIT_CTLS, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_EPT_VPID_CAP_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_EPT_VPID_CAP,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26845_IA32_VMX_EPT_VPID_CAP_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_EPT_VPID_CAP, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_EPT_VPID_CAP_001
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_EPT_VPID_CAP,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26844_IA32_VMX_EPT_VPID_CAP_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_EPT_VPID_CAP, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_ENTRY_CTLS_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_ENTRY_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26861_IA32_VMX_ENTRY_CTLS_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_ENTRY_CTLS, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_ENTRY_CTLS_001
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_ENTRY_CTLS,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26860_IA32_VMX_ENTRY_CTLS_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_ENTRY_CTLS, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_CR4_FIXED1_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_CR4_FIXED1,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26851_IA32_VMX_CR4_FIXED1_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_CR4_FIXED1, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_CR4_FIXED1_001
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_CR4_FIXED1,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26850_IA32_VMX_CR4_FIXED1_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_CR4_FIXED1, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_CR4_FIXED0_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_CR4_FIXED0,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26853_IA32_VMX_CR4_FIXED0_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_CR4_FIXED0, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_CR4_FIXED0_001
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_CR4_FIXED0,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26852_IA32_VMX_CR4_FIXED0_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_CR4_FIXED0, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_CR0_FIXED1_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_CR0_FIXED1,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26855_IA32_VMX_CR0_FIXED1_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_CR0_FIXED1, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_CR0_FIXED1_001
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_CR0_FIXED1,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26854_IA32_VMX_CR0_FIXED1_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_CR0_FIXED1, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_CR0_FIXED0_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_CR0_FIXED0,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26857_IA32_VMX_CR0_FIXED0_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_CR0_FIXED0, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_BASIC_002
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_BASIC,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26830_IA32_VMX_BASIC_002(void)
{
	bool is_pass = false;
	is_pass = test_msr_write_exception(MSR_IA32_VMX_BASIC, 1, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_VMX_BASIC_001
 *
 * Summary:When a vCPU attempts to access guest IA32_VMX_BASIC,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26829_IA32_VMX_BASIC_001(void)
{
	u64 value = 0;
	bool is_pass = false;
	is_pass = test_msr_read_exception(MSR_IA32_VMX_BASIC, &value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: VMXON_001
 *
 * Summary:When a vCPU attempts to execute VMXON,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_28991_VMXON_001(void)
{
	bool is_pass = false;
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
	free_page(mem);
	is_pass = (exception_vector() == UD_VECTOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: VMXOFF_001
 *
 * Summary:When a vCPU attempts to execute VMXOFF,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_29005_VMXOFF_001(void)
{
	int ret = 0;
	bool is_pass = false;
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

	asm volatile(ASM_TRY("1f")
				 "vmxoff \n\t"
				 "1:"
				:
				:
				: "memory");
	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	is_pass = (ret == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: VMLAUNCH_001
 *
 * Summary:When a vCPU attempts to execute VMLAUNCH,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_29006_VMLAUNCH_001(void)
{
	int ret = 0;
	bool is_pass = false;
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

	asm volatile(ASM_TRY("1f")
				 "vmlaunch \n\t"
				 "1:"
				:
				:
				: "cc", "memory");
	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	is_pass = (ret == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: VMRESUME_001
 *
 * Summary:When a vCPU attempts to execute VMRESUME,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_29007_VMRESUME_001(void)
{
	int ret = 0;
	bool is_pass = false;
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

	asm volatile(ASM_TRY("1f")
				 "vmresume \n\t"
				 "1:"
				:
				:
				: "cc", "memory");
	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	is_pass = (ret == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: VMPTRST_001
 *
 * Summary:When a vCPU attempts to execute VMPTRST,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_29009_VMPTRST_001(void)
{
	unsigned long pa = 0ULL;
	int ret = 0;
	bool is_pass = false;
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

	asm volatile(ASM_TRY("1f")
				 "vmptrst %0 \n\t"
				 "1:"
				: "=m" (pa)
				:
				: "cc", "memory");
	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	is_pass = (ret == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: VMPTRLD_001
 *
 * Summary:When a vCPU attempts to execute VMPTRLD,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_29010_VMPTRLD_001(void)
{
	static struct vmcs vmcs0;
	void *pa = &vmcs0;
	int ret = 0;
	bool is_pass = false;
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

	asm volatile(ASM_TRY("1f")
				 "vmptrld %0 \n\t"
				 "1:"
				: "=m"(pa)
				:
				: "cc", "memory");
	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	is_pass = (ret == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: VMREAD_001
 *
 * Summary:When a vCPU attempts to execute VMREAD,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_29011_VMREAD_001(void)
{
	u64 enc = VPID;
	int ret = 0;
	bool is_pass = false;
	u64 pa = 0ULL;
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

	asm volatile(ASM_TRY("1f")
				 "vmread %1, %0 \n\t"
				 "1:"
				: "=rm"(pa)
				: "r"(enc)
				: "cc", "memory");
	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	is_pass = (ret == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: VMWRITE_001
 *
 * Summary:When a vCPU attempts to execute VMWRITE,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_29012_VMWRITE_001(void)
{
	u64 enc = VPID;
	int ret = 0;
	bool is_pass = false;
	u64 pa = 0ULL;
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

	asm volatile(ASM_TRY("1f")
				 "vmwrite %0, %1 \n\t"
				 "1:"
				:
				: "m"(pa), "r"(enc)
				: "cc", "memory");
	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	is_pass = (ret == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: VMCLEAR_001
 *
 * Summary:When a vCPU attempts to execute VMCLEAR,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_29013_VMCLEAR_001(void)
{
	static struct vmcs vmcs0;
	void *pa = &vmcs0;
	int ret = 0;
	bool is_pass = false;
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

	asm volatile(ASM_TRY("1f")
				 "vmclear %0 \n\t"
				 "1:"
				:
				: "m"(pa)
				: "cc", "memory");
	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	is_pass = (ret == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: INVEPT_001
 *
 * Summary:When a vCPU attempts to execute INVEPT,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_29015_INVEPT_001(void)
{
	u64 type = 0;
	struct {
		u64 eptp, gpa;
	} operand = {0, 0};
	int ret = 0;
	bool is_pass = false;
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

	asm volatile(ASM_TRY("1f")
				 "invept %0, %1 \n\t"
				 "1:"
				:
				: "m"(operand), "r"(type)
				: "cc", "memory");
	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	is_pass = (ret == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: VMFUNC_001
 *
 * Summary:When a vCPU attempts to execute VMFUNC,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_29014_VMFUNC_001(void)
{
	int ret = 0;
	bool is_pass = false;
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

	asm volatile(ASM_TRY("1f")
				 "vmfunc \n\t"
				 "1:"
				:
				:
				: );
	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	is_pass = (ret == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: INVVPID_001
 *
 * Summary:When a vCPU attempts to execute INVVPID,
 * ACRN hypervisor shall guarantee that the vCPU receives UD.
 */
static __unused
void vmx_rqmid_29016_INVVPID_001(void)
{
	struct invvpid_operand {
		u64 vpid;
		u64 gla;
	} operand = {0, 0};
	u64 type = 0;
	int ret = 0;
	bool is_pass = false;
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

	asm volatile(ASM_TRY("1f")
				 "invvpid %0, %1 \n\t"
				 "1:"
				:
				: "m"(operand), "r"(type)
				: "cc", "memory");
	ret += (exception_vector() == UD_VECTOR);
	free_page(mem);
	is_pass = (ret == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: CR4.VMXE[13]_read_001
 *
 * Summary:When a vCPU attempts to read CR4.VMXE,
 * ACRN hypervisor shall guarantee that the vCPU gets 0H.
 */
static __unused
void vmx_rqmid_26831_CR4_VMXE_bit13_read_001(void)
{
	bool is_pass = false;
	ulong cr4_val = 0;
	cr4_val = read_cr4();
	cr4_val &= BIT_SHIFT(13);
	is_pass = (cr4_val == 0) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: CR4.VMXE[13]_write_001
 *
 * Summary:When a vCPU attempts to write guest CR4.VMXE and
 * the new guest CR4.VMXE is different from the old guest CR4.VMXE,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static __unused
void vmx_rqmid_26832_CR4_VMXE_bit13_write_001(void)
{
	bool is_pass = false;
	u64 cr4_val = 0;
	cr4_val = read_cr4();
	cr4_val |= BIT_SHIFT(13);
	asm volatile(ASM_TRY("1f")
				 "mov %0, %%cr4 \n\t"
				 "1:"
				:
				: "r"(cr4_val)
				: "memory");
	int _vec = exception_vector();
	int _err_code = exception_error_code();
	is_pass = (_vec == GP_VECTOR) && (_err_code == 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_FEATURE_CONTROL_001
 *
 * Summary:When a vCPU attempts to read IA32_FEATURE_CONTROL,
 * ACRN Hypervisor shall guarantee that the guest IA32_FEATURE_CONTROL[bit 2:1] is 0H.
 */
static __unused
void vmx_rqmid_26876_IA32_FEATURE_CONTROL_001(void)
{
	bool is_pass = false;
	u64 value = 0;
	value = rdmsr(MSR_IA32_FEATURE_CONTROL);
	value &= SHIFT_MASK(2, 1);
	is_pass = (value == 0) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: CR4.VMXE[13] initial start-up_001
 *
 * Summary:ACRN hypervisor shall set the initial guest CR4.VMXE to 0 following start-up.
 */
static __unused
void vmx_rqmid_28990_CR4_VMXE_bit13_initial_start_up_001(void)
{
	bool is_pass = false;
	u32 bp_cr4 = 0U;
	bp_cr4 = *(u32 *)STARTUP_CR4_SAVE_ADDR;
	bp_cr4 &= BIT_SHIFT(13);
	is_pass = (bp_cr4 == 0) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_FEATURE_CONTROL[bit 2:1] following start-up_001
 *
 * Summary:ACRN hypervisor shall set initial guest IA32_FEATURE_CONTROL[bit 2:1]
 * to 0H following start-up.
 */
static __unused
void vmx_rqmid_38554_IA32_FEATURE_CONTROL_bit2_1_following_start_up_001(void)
{
	bool is_pass = false;
	u64 value = 0;
	u32 edx = 0;
	u32 eax = 0;
	edx = *(u32 *)(STARTUP_MSR_IA32_FEATURE_CONTROL_MSB_32bit_ADDR);
	eax = *(u32 *)(STARTUP_MSR_IA32_FEATURE_CONTROL_LSB_32bit_ADDR);
	value = edx;
	value <<= 32;
	value |= eax;
	value &= SHIFT_MASK(2, 1);
	is_pass = (value == 0) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: IA32_FEATURE_CONTROL[bit 2:1] following INIT_001
 *
 * Summary:ACRN hypervisor shall set initial guest IA32_FEATURE_CONTROL[bit 2:1]
 * to 0H following INIT.
 */
static __unused
void vmx_rqmid_38557_IA32_FEATURE_CONTROL_bit2_1_following_INIT_001(void)
{
	bool is_pass = false;
	u64 value = 0;
	u32 edx = 0;
	u32 eax = 0;
	edx = *(u32 *)(INIT_MSR_IA32_FEATURE_CONTROL_MSB_32bit_ADDR);
	eax = *(u32 *)(INIT_MSR_IA32_FEATURE_CONTROL_LSB_32bit_ADDR);
	value = edx;
	value <<= 32;
	value |= eax;
	value &= SHIFT_MASK(2, 1);
	is_pass = (value == 0) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: Write_IA32_FEATURE_CONTROL[bit 2:1]_001
 *
 * Summary:When a vCPU attempts to write guest IA32_FEATURE_CONTROL[bit 2:1],
 * ACRN hypervisor shall inject #GP(0) to the vCPU.
 */
static __unused
void vmx_rqmid_40132_Write_IA32_FEATURE_CONTROL_bit_2_1_001(void)
{
	bool is_pass = false;
	u64 value = 0;
	value = rdmsr(MSR_IA32_FEATURE_CONTROL);
	value ^= 0x06UL;
	is_pass = test_msr_write_exception(MSR_IA32_FEATURE_CONTROL, value, GP_VECTOR, 0);
	report("%s", is_pass, __FUNCTION__);
}

static void print_case_list(void)
{
	printf("\t\t VMX feature case list:\n\r");
#ifdef __x86_64__
/******************************sample case part************************************/
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 28988u, "CR4.VMXE[13] initial INIT_001");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 26828u, "cpuid.01h:ECX[5]_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26856u, "IA32_VMX_CR0_FIXED0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29008u, "VMCALL_001");

/******************************scaling case part************************************/
//396:Hide VMX related Registers start
	printf("\t\t Case ID:%d case name:%s\n\r", 26835u, "IA32_VMX_VMFUNC_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26834u, "IA32_VMX_VMFUNC_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26849u, "IA32_VMX_VMCS_ENUM_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26848u, "IA32_VMX_VMCS_ENUM_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26847u, "IA32_VMX_PROCBASED_CTLS2_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26846u, "IA32_VMX_PROCBASED_CTLS2_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26841u, "IA32_VMX_TRUE_PROCBASED_CTLS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26840u, "IA32_VMX_TRUE_PROCBASED_CTLS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26843u, "IA32_VMX_TRUE_PINBASED_CTLS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26842u, "IA32_VMX_TRUE_PINBASED_CTLS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26839u, "IA32_VMX_TRUE_EXIT_CTLS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26838u, "IA32_VMX_TRUE_EXIT_CTLS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26837u, "IA32_VMX_TRUE_ENTRY_CTLS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26836u, "IA32_VMX_TRUE_ENTRY_CTLS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26865u, "IA32_VMX_PROCBASED_CTLS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26864u, "IA32_VMX_PROCBASED_CTLS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26867u, "IA32_VMX_PINBASED_CTLS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26866u, "IA32_VMX_PINBASED_CTLS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26859u, "IA32_VMX_MISC_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26858u, "IA32_VMX_MISC_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26863u, "IA32_VMX_EXIT_CTLS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26862u, "IA32_VMX_EXIT_CTLS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26845u, "IA32_VMX_EPT_VPID_CAP_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26844u, "IA32_VMX_EPT_VPID_CAP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26861u, "IA32_VMX_ENTRY_CTLS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26860u, "IA32_VMX_ENTRY_CTLS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26851u, "IA32_VMX_CR4_FIXED1_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26850u, "IA32_VMX_CR4_FIXED1_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26853u, "IA32_VMX_CR4_FIXED0_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26852u, "IA32_VMX_CR4_FIXED0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26855u, "IA32_VMX_CR0_FIXED1_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26854u, "IA32_VMX_CR0_FIXED1_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26857u, "IA32_VMX_CR0_FIXED0_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26830u, "IA32_VMX_BASIC_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26829u, "IA32_VMX_BASIC_001");
//396:Hide VMX related Registers end

//441:Hide VMX instruction start
	printf("\t\t Case ID:%d case name:%s\n\r", 28991u, "VMXON_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29005u, "VMXOFF_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29006u, "VMLAUNCH_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29007u, "VMRESUME_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29009u, "VMPTRST_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29010u, "VMPTRLD_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29011u, "VMREAD_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29012u, "VMWRITE_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29013u, "VMCLEAR_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29015u, "INVEPT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29014u, "VMFUNC_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29016u, "INVVPID_001");
//441:Hide VMX instruction end

//398:Hide VMX operation start
	printf("\t\t Case ID:%d case name:%s\n\r", 26831u, "CR4.VMXE[13]_read_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26832u, "CR4.VMXE[13]_write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26876u, "IA32_FEATURE_CONTROL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 40132u, "Write_IA32_FEATURE_CONTROL[bit 2:1]_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28990u, "CR4.VMXE[13] initial start-up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38554u, "IA32_FEATURE_CONTROL[bit 2:1] following start-up_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 38557u, "IA32_FEATURE_CONTROL[bit 2:1] following INIT_001");
#endif
//398:Hide VMX operation end
#endif
}

int main(void)
{
	setup_vm();
	setup_idt();
	print_case_list();
#ifdef __x86_64__

/******************************sample case part************************************/
#ifdef IN_NON_SAFETY_VM
	vmx_rqmid_28988_CR4_VMXE_initial_INIT_001();
#endif
	vmx_rqmid_26828_cpuid_01h_ECX_bit5_001();
	vmx_rqmid_26856_IA32_VMX_CR0_FIXED0_001();
	vmx_rqmid_29008_VMCALL_001();

/******************************scaling case part************************************/
//396:Hide VMX related Registers start
	vmx_rqmid_26835_IA32_VMX_VMFUNC_002();
	vmx_rqmid_26834_IA32_VMX_VMFUNC_001();
	vmx_rqmid_26849_IA32_VMX_VMCS_ENUM_002();
	vmx_rqmid_26848_IA32_VMX_VMCS_ENUM_001();
	vmx_rqmid_26847_IA32_VMX_PROCBASED_CTLS2_002();
	vmx_rqmid_26846_IA32_VMX_PROCBASED_CTLS2_001();
	vmx_rqmid_26841_IA32_VMX_TRUE_PROCBASED_CTLS_002();
	vmx_rqmid_26840_IA32_VMX_TRUE_PROCBASED_CTLS_001();
	vmx_rqmid_26843_IA32_VMX_TRUE_PINBASED_CTLS_002();
	vmx_rqmid_26842_IA32_VMX_TRUE_PINBASED_CTLS_001();
	vmx_rqmid_26839_IA32_VMX_TRUE_EXIT_CTLS_002();
	vmx_rqmid_26838_IA32_VMX_TRUE_EXIT_CTLS_001();
	vmx_rqmid_26837_IA32_VMX_TRUE_ENTRY_CTLS_002();
	vmx_rqmid_26836_IA32_VMX_TRUE_ENTRY_CTLS_001();
	vmx_rqmid_26865_IA32_VMX_PROCBASED_CTLS_002();
	vmx_rqmid_26864_IA32_VMX_PROCBASED_CTLS_001();
	vmx_rqmid_26867_IA32_VMX_PINBASED_CTLS_002();
	vmx_rqmid_26866_IA32_VMX_PINBASED_CTLS_001();
	vmx_rqmid_26859_IA32_VMX_MISC_002();
	vmx_rqmid_26858_IA32_VMX_MISC_001();
	vmx_rqmid_26863_IA32_VMX_EXIT_CTLS_002();
	vmx_rqmid_26862_IA32_VMX_EXIT_CTLS_001();
	vmx_rqmid_26845_IA32_VMX_EPT_VPID_CAP_002();
	vmx_rqmid_26844_IA32_VMX_EPT_VPID_CAP_001();
	vmx_rqmid_26861_IA32_VMX_ENTRY_CTLS_002();
	vmx_rqmid_26860_IA32_VMX_ENTRY_CTLS_001();
	vmx_rqmid_26851_IA32_VMX_CR4_FIXED1_002();
	vmx_rqmid_26850_IA32_VMX_CR4_FIXED1_001();
	vmx_rqmid_26853_IA32_VMX_CR4_FIXED0_002();
	vmx_rqmid_26852_IA32_VMX_CR4_FIXED0_001();
	vmx_rqmid_26855_IA32_VMX_CR0_FIXED1_002();
	vmx_rqmid_26854_IA32_VMX_CR0_FIXED1_001();
	vmx_rqmid_26857_IA32_VMX_CR0_FIXED0_002();
	vmx_rqmid_26830_IA32_VMX_BASIC_002();
	vmx_rqmid_26829_IA32_VMX_BASIC_001();
//396:Hide VMX related Registers end

//441:Hide VMX instruction start
	vmx_rqmid_28991_VMXON_001();
	vmx_rqmid_29005_VMXOFF_001();
	vmx_rqmid_29006_VMLAUNCH_001();
	vmx_rqmid_29007_VMRESUME_001();
	vmx_rqmid_29009_VMPTRST_001();
	vmx_rqmid_29010_VMPTRLD_001();
	vmx_rqmid_29011_VMREAD_001();
	vmx_rqmid_29012_VMWRITE_001();
	vmx_rqmid_29013_VMCLEAR_001();
	vmx_rqmid_29015_INVEPT_001();
	vmx_rqmid_29014_VMFUNC_001();
	vmx_rqmid_29016_INVVPID_001();
//441:Hide VMX instruction end

//398:Hide VMX operation start
	vmx_rqmid_26831_CR4_VMXE_bit13_read_001();
	vmx_rqmid_26832_CR4_VMXE_bit13_write_001();
	vmx_rqmid_26876_IA32_FEATURE_CONTROL_001();
	vmx_rqmid_40132_Write_IA32_FEATURE_CONTROL_bit_2_1_001();
	vmx_rqmid_28990_CR4_VMXE_bit13_initial_start_up_001();
	vmx_rqmid_38554_IA32_FEATURE_CONTROL_bit2_1_following_start_up_001();
#ifdef IN_NON_SAFETY_VM
	vmx_rqmid_38557_IA32_FEATURE_CONTROL_bit2_1_following_INIT_001();
#endif
//398:Hide VMX operation end
#endif

	return report_summary();
}
