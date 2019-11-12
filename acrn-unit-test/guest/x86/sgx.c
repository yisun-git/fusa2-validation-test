#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "sgx.h"
#include "vm.h"
#include "vmalloc.h"
#include "misc.h"
#include "apic.h"
#include "isr.h"

extern void send_sipi();
bool g_is_init_ap = false;

void save_unchanged_reg()
{
    asm volatile (
        "mov $0x0000003a, %ecx\n"
        "rdmsr\n"
        "mov %eax, (0x7000)\n"
        "mov %edx, (0x7004)"
        );
}

/**
 * @brief case name: Guestee that MSR UNCORE PRMRR PHYS BASE_002
 *
 * Summary: Execute write MSR_UNCORE_PRMRR_PHYS_BASE register shall generate #GP
 */
static void sgx_rqmid_27375_write_msr_uncore_prmrr_phys_base()
{
	u64 msr_uncore_prmrr_phys_base = VALUE_TO_WRITE_MSR;
	report("\t\t sgx_rqmid_27375_write_msr_uncore_prmrr_phys_base",
		wrmsr_checking(MSR_UNCORE_PRMRR_PHYS_BASE,
		msr_uncore_prmrr_phys_base) == GP_VECTOR);

}

/**
 * @brief case name: Guesthat MSR PRMRR VALID CONFIG_001
 *
 * Summary: Read from guest MSR register MSR_PRMRR_VALID_CONFIG shall generate #GP
 */
static void sgx_rqmid_27376_read_msr_prmrr_valid_config()
{
	u64 msr_prmrr_valid_config;
	report("\t\t sgx_rqmid_27376_read_msr_prmrr_valid_config",
		rdmsr_checking(MSR_PRMRR_VALID_CONFIG, &msr_prmrr_valid_config)
		== GP_VECTOR);
}

/**
 * @brief case name: Guesthat MSR PRMRR VALID CONFIG_002
 *
 * Summary: Write a value to MSR register MSR_PRMRR_VALID_CONFIG shall generate #GP
 */
static void sgx_rqmid_27377_write_msr_prmrr_valid_config()
{
	u64 msr_prmrr_valid_config = VALUE_TO_WRITE_MSR;
	report("\t\t sgx_rqmid_27377_write_msr_prmrr_valid_config",
		wrmsr_checking(MSR_PRMRR_VALID_CONFIG, msr_prmrr_valid_config)
		== GP_VECTOR);
}

/**
 * @brief case name:Guest CPUID.SGX LC_001
 *
 * Summary: Read CPUID.(EAX=7H, ECX=0H):ECX[bit 30] from guest shall be 0
 */
static void sgx_rqmid_27401_check_sgx_support()
{
	report("\t\t sgx_rqmid_27401_check_sgx_support",
		((cpuid(7).c) & CPUID_07_SGX) == 0);
}

/**
 * @brief case name: Guest CPUID leaf 12H_001
 *
 * Summary: Execute CPUID.12H get EAX,EBX,ECX,EDX shall be 0
 */
static void sgx_rqmid_27400_guest_cpuid_leaf_12h()
{
	bool flag = false;
	if (cpuid(SGX_CPUID_ID).a == 0 && cpuid(SGX_CPUID_ID).b == 0
		&& cpuid(SGX_CPUID_ID).c == 0 && cpuid(SGX_CPUID_ID).d == 0) {
		flag = true;
	}
	report("\t\t sgx_rqmid_27400_guest_cpuid_leaf_12h", flag);
}

/**
 * @brief case name: Guesthat MSR PRMRR VALID CONFIG_001
 *
 * Summary: Read from guest MSR register msr_uncore_prmrr_phys_mask shall generate #GP
 */
static void sgx_rqmid_27372_read_msr_uncore_prmrr_phys_mask()
{
	u64 msr_uncore_prmrr_phys_mask;
	report("\t\t %s",
		rdmsr_checking(MSR_UNCORE_PRMRR_PHYS_MASK,
		&msr_uncore_prmrr_phys_mask) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guestee that MSR UNCORE PRMRR PHYS MASK_002
 *
 * Summary: write from guest MSR register msr_uncore_prmrr_phys_mask shall generate #GP
 */
static void sgx_rqmid_27373_write_msr_uncore_prmrr_phys_mask()
{
	u64 msr_uncore_prmrr_phys_mask = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(MSR_UNCORE_PRMRR_PHYS_MASK,
		msr_uncore_prmrr_phys_mask) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guestee that MSR UNCORE PRMRR PHYS BASE_001
 *
 * Summary: Read from guest MSR register msr_uncore_prmrr_phys_base shall generate #GP
 */
static void sgx_rqmid_27374_read_msr_uncore_prmrr_phys_base()
{
	u64 msr_uncore_prmrr_phys_base;
	report("\t\t %s",
		rdmsr_checking(MSR_UNCORE_PRMRR_PHYS_BASE,
		&msr_uncore_prmrr_phys_base) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest MSR SGXOWNEREPOCH1_002
 *
 * Summary: write from guest MSR register msr_sgxownerepoch1 shall generate #GP
 */
static void sgx_rqmid_27384_write_msr_sgxownerepoch1()
{
	u64 msr_sgxownerepoch1 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(MSR_SGXOWNEREPOCH1, msr_sgxownerepoch1)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest MSR SGXOWNEREPOCH1_001
 *
 * Summary: read from guest MSR register msr_sgxownerepoch1 shall generate #GP
 */
static void sgx_rqmid_27382_read_msr_sgxownerepoch1()
{
	u64 msr_sgxownerepoch1;
	report("\t\t %s",
		rdmsr_checking(MSR_SGXOWNEREPOCH1, &msr_sgxownerepoch1)
		== GP_VECTOR, __FUNCTION__);
}


/**
 * @brief case name: Guest MSR SGXOWNEREPOCH0_002
 *
 * Summary: write from guest MSR register msr_sgxownerepoch0 shall generate #GP
 */
static void sgx_rqmid_27387_write_msr_sgxownerepoch0()
{
	u64 msr_sgxownerepoch0 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(MSR_SGXOWNEREPOCH0, msr_sgxownerepoch0)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest MSR SGXOWNEREPOCH0_001
 *
 * Summary: read from guest MSR register msr_sgxownerepoch0 shall generate #GP
 */
static void sgx_rqmid_27386_read_msr_sgxownerepoch0()
{
	u64 msr_sgxownerepoch0;
	report("\t\t %s",
		rdmsr_checking(MSR_SGXOWNEREPOCH0, &msr_sgxownerepoch0)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest MSR PRMRR PHYS MASK_002
 *
 * Summary: write from guest MSR register msr_prmrr_phys_mask shall generate #GP
 */
static void sgx_rqmid_27379_write_msr_prmrr_phys_mask()
{
	u64 msr_prmrr_phys_mask = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(MSR_PRMRR_PHYS_MASK, msr_prmrr_phys_mask)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest MSR PRMRR PHYS MASK_001
 *
 * Summary: read from guest MSR register msr_prmrr_phys_mask shall generate #GP
 */
static void sgx_rqmid_27378_read_msr_prmrr_phys_mask()
{
	u64 msr_prmrr_phys_mask;
	report("\t\t %s",
		rdmsr_checking(MSR_PRMRR_PHYS_MASK, &msr_prmrr_phys_mask)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest MSR PRMRR PHYS BASE_002
 *
 * Summary: write from guest MSR register msr_prmrr_phys_base shall generate #GP
 */
static void sgx_rqmid_27381_write_msr_prmrr_phys_base()
{
	u64 msr_prmrr_phys_base = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(MSR_PRMRR_PHYS_BASE, msr_prmrr_phys_base)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest MSR PRMRR PHYS BASE_001
 *
 * Summary: read from guest MSR register msr_prmrr_phys_base shall generate #GP
 */
static void sgx_rqmid_27380_read_msr_prmrr_phys_base()
{
	u64 msr_prmrr_phys_base;
	report("\t\t %s",
		rdmsr_checking(MSR_PRMRR_PHYS_BASE, &msr_prmrr_phys_base)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH3_002
 *
 * Summary: write from guest MSR register ia32_sgxlepubkeyhash3 shall generate #GP
 */
static void sgx_rqmid_27393_write_ia32_sgxlepubkeyhash3()
{
	u64 ia32_sgxlepubkeyhash3 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(IA32_SGXLEPUBKEYHASH3, ia32_sgxlepubkeyhash3)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH3_001
 *
 * Summary: read from guest MSR register ia32_sgxlepubkeyhash3 shall generate #GP
 */
static void sgx_rqmid_27392_read_ia32_sgxlepubkeyhash3()
{
	u64 ia32_sgxlepubkeyhash3;
	report("\t\t %s",
		rdmsr_checking(IA32_SGXLEPUBKEYHASH3, &ia32_sgxlepubkeyhash3)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH2_002
 *
 * Summary: write from guest MSR register ia32_sgxlepubkeyhash2 shall generate #GP
 */
static void sgx_rqmid_27395_write_ia32_sgxlepubkeyhash2()
{
	u64 ia32_sgxlepubkeyhash2 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(IA32_SGXLEPUBKEYHASH2, ia32_sgxlepubkeyhash2)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH2_001
 *
 * Summary: read from guest MSR register ia32_sgxlepubkeyhash2 shall generate #GP
 */
static void sgx_rqmid_27394_read_ia32_sgxlepubkeyhash2()
{
	u64 ia32_sgxlepubkeyhash2;
	report("\t\t %s",
		rdmsr_checking(IA32_SGXLEPUBKEYHASH2, &ia32_sgxlepubkeyhash2)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH1_002
 *
 * Summary: write from guest MSR register ia32_sgxlepubkeyhash1 shall generate #GP
 */
static void sgx_rqmid_27397_write_ia32_sgxlepubkeyhash1()
{
	u64 ia32_sgxlepubkeyhash1 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(IA32_SGXLEPUBKEYHASH1, ia32_sgxlepubkeyhash1)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH1_001
 *
 * Summary: read from guest MSR register ia32_sgxlepubkeyhash1 shall generate #GP
 */
static void sgx_rqmid_27396_read_ia32_sgxlepubkeyhash1()
{
	u64 ia32_sgxlepubkeyhash1;
	report("\t\t %s",
		rdmsr_checking(IA32_SGXLEPUBKEYHASH1, &ia32_sgxlepubkeyhash1)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH0_002
 *
 * Summary: write from guest MSR register ia32_sgxlepubkeyhash0 shall generate #GP
 */
static void sgx_rqmid_27399_write_ia32_sgxlepubkeyhash0()
{
	u64 ia32_sgxlepubkeyhash0 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(IA32_SGXLEPUBKEYHASH0, ia32_sgxlepubkeyhash0)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH0_001
 *
 * Summary: read from guest MSR register ia32_sgxlepubkeyhash0 shall generate #GP
 */
static void sgx_rqmid_27398_read_ia32_sgxlepubkeyhash0()
{
	u64 ia32_sgxlepubkeyhash0;
	report("\t\t %s",
		rdmsr_checking(IA32_SGXLEPUBKEYHASH0, &ia32_sgxlepubkeyhash0)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGX SVN STATUS_002
 *
 * Summary: write from guest MSR register ia32_sgx_svn_status shall generate #GP
 */
static void sgx_rqmid_27391_write_ia32_sgx_svn_status()
{
	u64 ia32_sgx_svn_status = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(IA32_SGX_SVN_STATUS, ia32_sgx_svn_status)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGX SVN STATUS_001
 *
 * Summary: read from guest MSR register ia32_sgx_svn_status shall generate #GP
 */
static void sgx_rqmid_27390_read_ia32_sgx_svn_status()
{
	u64 ia32_sgx_svn_status;
	report("\t\t %s",
		rdmsr_checking(IA32_SGX_SVN_STATUS, &ia32_sgx_svn_status)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest CPUID.SGX_001
 *
 * Summary: Execute CPUID.(EAX=7H, ECX=0H):EBX[bit 2] shall be 1H.
 */
static void sgx_rqmid_27402_check_supported_sgx()
{
	report("\t\t %s", ((cpuid(7).b) & (1ul << 2)) == (1ul << 2), __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 FEATURE CONTROL.SGX ENABLE following start-up_001
 *
 * Summary: Get IA32_FEATURE_CONTROL.SGX_ENABL at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void sgx_rqmid_27403_ia32_feature_control_startup()
{
	volatile u32 *ptr = (volatile u32 *)IA32_FEATURE_CONTROL_STARTUP_ADDR;
	u64 ia32_feature_control;

	ia32_feature_control = *ptr + ((u64)(*(ptr + 1)) << 32);
	report("\t\t %s", (ia32_feature_control & SGX_ENABLE_BIT) == 0, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 FEATURE CONTROL.SGX ENABLE following INIT_001
 *
 * Summary: After AP receives first INIT, set the value of IA32_FEATURE_CONTROL.SGX_ENABLE [bit 18];
 * 	    Dump IA32_FEATURE_CONTROL.SGX_ENABLE [bit 18] value shall get the same value after second INIT.
 */
static void sgx_rqmid_27404_ia32_feature_control_init()
{
	volatile u32 *ptr = (volatile u32 *)IA32_FEATURE_CONTROL_INIT1_ADDR;
	u64 ia32_init_second;
	u64 ia32_init_first;

	ia32_init_first = *ptr + ((u64)(*(ptr + 1)) << 32);

	/* send sipi to ap */
	send_sipi();
	/* set up init ap flag */
	g_is_init_ap = true;


	ptr = (volatile u32 *)IA32_FEATURE_CONTROL_INIT2_ADDR;
	ia32_init_second = *ptr + ((u64)(*(ptr + 1)) << 32);

	report("\t\t %s", (ia32_init_first & SGX_ENABLE_BIT) == (ia32_init_second & SGX_ENABLE_BIT),
		__FUNCTION__);
}

/**
 * @brief case name: Guest IA32_FEATURE_CONTROL.SGX_lauch Control following start-up_001
 *
 * Summary: Get IA32_FEATURE_CONTROL.SGX_lauch_Control_Enable at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void sgx_rqmid_29563_sgx_lauch_bit_startup()
{
	volatile u32 *ptr = (volatile u32 *)IA32_FEATURE_CONTROL_STARTUP_ADDR;
	u64 ia32_feature_control;

	ia32_feature_control = *ptr + ((u64)(*(ptr + 1)) << 32);
	report("\t\t %s", (ia32_feature_control & SGX_LAUCH_BIT) == 0, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32_FEATURE_CONTROL.Launch Control Enable following INIT_001
 *
 * Summary: After AP receives first INIT, set the value of IA32_FEATURE_CONTROL.SGX_LAUCH [bit 17];
 * 	    Dump IA32_FEATURE_CONTROL.SGX_LAUCH [bit 17] value shall get the same value after second INIT.
 */
static void sgx_rqmid_29562_sgx_lauch_bit_init()
{
	volatile u32 *ptr = (volatile u32 *)IA32_FEATURE_CONTROL_INIT1_ADDR;
	u64 ia32_init_second;
	u64 ia32_init_first;

	ia32_init_first = *ptr + ((u64)(*(ptr + 1)) << 32);

	ptr = (volatile u32 *)IA32_FEATURE_CONTROL_INIT2_ADDR;
	ia32_init_second = *ptr + ((u64)(*(ptr + 1)) << 32);

	if (g_is_init_ap) {
		report("\t\t %s", (ia32_init_first & SGX_LAUCH_BIT) == (ia32_init_second & SGX_LAUCH_BIT),
			__FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

static void print_case_list()
{
	printf("SGX feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 27375u, "Guestee that MSR UNCORE PRMRR PHYS BASE_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27376u, "Guesthat MSR PRMRR VALID CONFIG_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27377u, "Guesthat MSR PRMRR VALID CONFIG_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27401u, "Guest CPUID.SGX LC_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27400u, "Guest CPUID leaf 12H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27372u, "Guesthat MSR PRMRR VALID CONFIG_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27373u, "Guestee that MSR UNCORE PRMRR PHYS MASK_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27374u, "Guestee that MSR UNCORE PRMRR PHYS BASE_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27384u, "Guest MSR SGXOWNEREPOCH1_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27382u, "Guest MSR SGXOWNEREPOCH1_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27387u, "Guest MSR SGXOWNEREPOCH0_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27386u, "Guest MSR SGXOWNEREPOCH0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27379u, "Guest MSR PRMRR PHYS MASK_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27381u, "Guest MSR PRMRR PHYS BASE_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27380u, "Guest MSR PRMRR PHYS BASE_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27393u, "Guest IA32 SGXLEPUBKEYHASH3_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27392u, "Guest IA32 SGXLEPUBKEYHASH3_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27395u, "Guest IA32 SGXLEPUBKEYHASH2_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27394u, "Guest IA32 SGXLEPUBKEYHASH2_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27397u, "Guest IA32 SGXLEPUBKEYHASH1_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27396u, "Guest IA32 SGXLEPUBKEYHASH1_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27399u, "Guest IA32 SGXLEPUBKEYHASH0_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27398u, "Guest IA32 SGXLEPUBKEYHASH0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27391u, "Guest IA32 SGX SVN STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27390u, "Guest IA32 SGX SVN STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27402u, "Guest CPUID.SGX_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27403u, "Guest IA32 FEATURE CONTROL.SGX ENABLE following start-up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27404u, "Guest IA32 FEATURE CONTROL.SGX ENABLE following INIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29562u, "Guest IA32_FEATURE_CONTROL.Launch Control Enable following INIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29563u, "Guest IA32_FEATURE_CONTROL.SGX_lauch Control following start-up_001");
}

static void test_sgx()
{
	sgx_rqmid_27375_write_msr_uncore_prmrr_phys_base();
	sgx_rqmid_27376_read_msr_prmrr_valid_config();
	sgx_rqmid_27377_write_msr_prmrr_valid_config();
	sgx_rqmid_27401_check_sgx_support();
	sgx_rqmid_27400_guest_cpuid_leaf_12h();
	sgx_rqmid_27372_read_msr_uncore_prmrr_phys_mask();
	sgx_rqmid_27373_write_msr_uncore_prmrr_phys_mask();
	sgx_rqmid_27374_read_msr_uncore_prmrr_phys_base();
	sgx_rqmid_27384_write_msr_sgxownerepoch1();
	sgx_rqmid_27382_read_msr_sgxownerepoch1();
	sgx_rqmid_27387_write_msr_sgxownerepoch0();
	sgx_rqmid_27386_read_msr_sgxownerepoch0();
	sgx_rqmid_27379_write_msr_prmrr_phys_mask();
	sgx_rqmid_27378_read_msr_prmrr_phys_mask();
	sgx_rqmid_27381_write_msr_prmrr_phys_base();
	sgx_rqmid_27380_read_msr_prmrr_phys_base();
	sgx_rqmid_27393_write_ia32_sgxlepubkeyhash3();
	sgx_rqmid_27392_read_ia32_sgxlepubkeyhash3();
	sgx_rqmid_27395_write_ia32_sgxlepubkeyhash2();
	sgx_rqmid_27394_read_ia32_sgxlepubkeyhash2();
	sgx_rqmid_27397_write_ia32_sgxlepubkeyhash1();
	sgx_rqmid_27396_read_ia32_sgxlepubkeyhash1();
	sgx_rqmid_27399_write_ia32_sgxlepubkeyhash0();
	sgx_rqmid_27398_read_ia32_sgxlepubkeyhash0();
	sgx_rqmid_27391_write_ia32_sgx_svn_status();
	sgx_rqmid_27390_read_ia32_sgx_svn_status();
	sgx_rqmid_27402_check_supported_sgx();
	sgx_rqmid_27403_ia32_feature_control_startup();
	sgx_rqmid_29562_sgx_lauch_bit_init();
	sgx_rqmid_29563_sgx_lauch_bit_startup();
}

int main(void)
{
	print_case_list();
	sgx_rqmid_27404_ia32_feature_control_init();
	setup_idt();
	test_sgx();

	return report_summary();
}

