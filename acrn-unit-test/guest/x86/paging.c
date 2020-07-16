#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "paging.h"
#include "vm.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "alloc_phys.h"
#include "alloc.h"
#include "misc.h"
#include "register_op.h"

u32 get_startup_cr0()
{
	return *((volatile u32 *)STARTUP_CR0_ADDR);
}

u32 get_startup_cr4()
{
	return *((volatile u32 *)STARTUP_CR4_ADDR);
}

u32 get_init_cr0()
{
	return *((volatile u32 *)INIT_CR0_ADDR);
}

u32 get_init_cr4()
{
	return *((volatile u32 *)INIT_CR4_ADDR);
}

int test_instruction_fetch(void *p)
{
	asm volatile(ASM_TRY("1f")
		"call *%[addr]\n\t"
		"1:"
		: : [addr]"r"(p));

	return exception_vector();
}

void free_gva(void *gva)
{
	set_page_control_bit(gva, PAGE_PTE, PAGE_P_FLAG, 1, true);
	free(gva);
}

bool check_value_is_exist(u32 reg, u8 value)
{
	u32 i;

	for (i = 0; i < 4; i++) {
		if (((u8)(reg >> (i * 8)) & 0xff) == value) {
			return true;
		}
	}

	return false;
}

#ifdef __x86_64__
/**
 * @brief case name:Encoding of CPUID Leaf 2 Descriptors_001
 *
 * Summary: check TLB information in CPUID.02H. Because the order of descriptors
 *          in the EAX, EBX, ECX, and EDX registers is not defined,
 *          the descriptors may appear in any order. While checking CPUID:02 registers,
 *          the return value may be disordered while comparing with default value.
 *          So we will check each different "8bit value" if exists
 *          in EAX/EBX/ECX/EDX, and not care it's order.
 */
static void paging_rqmid_23896_check_tlb_info()
{
	struct cpuid r = cpuid(2);
	u8 tlb_info[8] = {TYPE_TLB_01, TYPE_TLB_03, TYPE_TLB_63, TYPE_TLB_76,
			TYPE_TLB_B6, TYPE_STLB_C3,
			TYPE_PREFECTH_F0, TYPE_GENEAL_FF
		};
	u32 cpuid_value[4] = {r.a, r.b, r.c, r.d};
	u32 exist_num = 0;
	u32 i;
	u32 j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 4; i++) {
			if (check_value_is_exist(cpuid_value[i], tlb_info[j])) {
				exist_num++;
				break;
			}
		}
	}

	report("paging_rqmid_23896_check_tlb_info", (exist_num == 8));
}

/**
 * @brief case name:Hide Processor Context Identifiers_001
 *
 * Summary: When process-context identifiers are hidden, CPUID.01H:ECX.
 *		PCID [bit 17] shall be 0, and changing CR4.PCIDE from 0 to 1,shall generate #GP.
 */
static void paging_rqmid_23897_hide_processor_context_identifiers()
{
	unsigned long cr4 = read_cr4();
	bool is_pass = false;

	if ((cpuid(1).c & (1 << 17)) == 0) {
		if (write_cr4_checking(cr4 | X86_CR4_PCIDE) == GP_VECTOR) {
			is_pass = true;
		}
	}

	report("paging_rqmid_23897_hide_processor_context_identifiers", is_pass);
}

/**
 * @brief case name: Global Pages Support_001
 *
 * Summary: Execute CPUID.01H:EDX.PGE [bit 13] shall be 1, set CR4.PGE to enable
 *		global-page feature shall have no exception
 */
static void paging_rqmid_23901_global_pages_support()
{
	unsigned long cr4 = read_cr4();
	bool is_pass = false;

	if ((cpuid(1).c & (1 << 13)) != 0) {
		if (write_cr4_checking(cr4 | X86_CR4_PGE) == PASS) {
			if ((read_cr4() & X86_CR4_PGE) != 0) {
				is_pass = true;
			}
		}
	}

	report("paging_rqmid_23901_global_pages_support", is_pass);
}

/**
 * @brief case name:CPUID.80000008H:EAX[7:0]_001
 *
 * Summary: Execute CPUID.80000008H:EAX[7:0] to get the physical-address width
 *		supported by the processor, it shall be 39.
 */
static void paging_rqmid_23895_check_physical_address_width()
{
	bool is_pass = false;

	if ((cpuid(0x80000008).a & 0xff) == PHYSICAL_ADDRESS_WIDTH) {
		is_pass = true;
	}

	report("paging_rqmid_23895_check_physical_address_width", is_pass);
}

/**
 * @brief case name:CPUID.80000008H:EAX[15:8]_001
 *
 * Summary: execute CPUID.80000008H:EAX[15:8] to get the linear-address width shall be 48
 */
static void paging_rqmid_32346_check_linear_address_width()
{
	bool is_pass = false;

	if (((cpuid(0x80000008).a >> 8) & 0xff) == LINEAR_ADDRESS_WIDTH) {
		is_pass = true;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: IA32 EFER.NXE state following INIT_001
 *
 * Summary: Get IA32_EFER.NXE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32265_ia32_efer_nxe_init()
{
	volatile u32 *ptr = (volatile u32 *)INIT_IA32_EFER_LOW_ADDR;
	u64 ia32_init = *ptr + ((u64)(*(ptr + 1)) << 32);

	report("%s", (ia32_init & X86_IA32_EFER_NXE) == 0, __FUNCTION__);
}

/**
 * @brief case name: EFLAGS.AC state following INIT_001
 *
 * Summary: Get EFLAGS.AC at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32270_eflags_ac_init()
{
	volatile u32 eflags_ac_init = *((volatile u32 *)INIT_EFLAGS_ADDR);

	report("%s", (eflags_ac_init & X86_EFLAGS_AC) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.SMEP state following INIT_001
 *
 * Summary: Get CR4.SMEP at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32272_cr4_smep_init()
{
	u32 cr4 = get_init_cr4();

	report("%s", (cr4 & X86_CR4_SMEP) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.SMAP state following INIT_001
 *
 * Summary: Get CR4.SMAP at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32274_cr4_smap_init()
{
	u32 cr4 = get_init_cr4();

	report("%s", (cr4 & X86_CR4_SMAP) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PSE state following INIT_001
 *
 * Summary:  Get CR4.PSE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32276_cr4_pse_init()
{
	u32 cr4 = get_init_cr4();

	report("%s", (cr4 & X86_CR4_PSE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PKE state following INIT_001
 *
 * Summary:  Get CR4.PKE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32278_cr4_pke_init()
{
	u32 cr4 = get_init_cr4();

	report("%s", (cr4 & X86_CR4_PKE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PGE state following INIT_001
 *
 * Summary:  Get CR4.PGE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32280_cr4_pge_init()
{
	u32 cr4 = get_init_cr4();

	report("%s", (cr4 & X86_CR4_PGE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PAE state following INIT_001
 *
 * Summary:  Get CR4.PAE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32297_cr4_pae_init()
{
	u32 cr4 = get_init_cr4();

	report("%s", (cr4 & X86_CR4_PAE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PCIDE state following INIT_001
 *
 * Summary:  Get CR4.PCIDE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32290_cr4_pcide_init()
{
	u32 cr4 = get_init_cr4();

	report("%s", (cr4 & X86_CR4_PCIDE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR0.WP state following INIT_001
 *
 * Summary:  Get CR0.WP at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32304_cr0_wp_init()
{
	u32 cr0 = get_init_cr0();

	report("%s", (cr0 & X86_CR0_WP) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR0.PG state following INIT_001
 *
 * Summary:  Get CR0.PG at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32306_cr0_pg_init()
{
	u32 cr0 = get_init_cr0();

	report("%s", (cr0 & X86_CR0_PG) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR2 state following INIT_001
 *
 * Summary: Get CR2 at AP init,shall be 00000000H and same with SDM definition.
 */
static void paging_rqmid_32302_cr2_init()
{
	volatile u32 cr2 = *((volatile u32 *)INIT_CR2_ADDR);

	report("%s", (cr2 == 0), __FUNCTION__);
}

/**
 * @brief case name: CR3[bit 63:12] state following INIT_001
 *
 * Summary: Get CR3.[bit 63:12] at AP init,shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32300_cr3_init()
{
	volatile u64 cr3 = *((volatile u64 *)INIT_CR3_ADDR);

	report("%s", ((cr3 & ~(0xfffull)) == 0), __FUNCTION__);
}

/**
 * @brief case name: IA32 EFER.NXE state following start-up_001
 *
 * Summary: Get IA32_EFER.NXE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32264_ia32_efer_nxe_start_up()
{
	volatile u32 *ptr = (volatile u32 *)STARTUP_IA32_EFER_LOW_ADDR;
	u64 ia32_startup = *ptr + ((u64)(*(ptr + 1)) << 32);

	report("%s", (ia32_startup & X86_IA32_EFER_NXE) == 0, __FUNCTION__);
}

/**
 * @brief case name: EFLAGS.AC state following start-up_001
 *
 * Summary: Get EFLAGS.AC at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32269_eflags_ac_start_up()
{
	volatile u32 eflags = *((volatile u32 *)STARTUP_EFLAGS_ADDR);

	report("%s", (eflags & X86_EFLAGS_AC) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.SMAP state following start-up_001
 *
 * Summary: Get CR4.SMAP at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32271_cr4_smap_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_SMAP) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.SMEP state following start-up_001
 *
 * Summary: Get CR4.SMEP at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32273_cr4_smep_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_SMEP) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PSE state following start-up_001
 *
 * Summary: Get CR4.PSE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32275_cr4_pse_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_PSE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PKE state following start-up_001
 *
 * Summary: Get CR4.PKE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32277_cr4_pke_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_PKE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PGE state following start-up_001
 *
 * Summary: Get CR4.PGE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32279_cr4_pge_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_PGE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PCIDE state following start-up_001
 *
 * Summary: Get CR4.PCIDE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32285_cr4_pcide_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_PCIDE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PAE state following start-up_001
 *
 * Summary: Get CR4.PAE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32295_cr4_pae_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_PAE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR3[bit 63:12] state following start-up_001
 *
 * Summary: Get CR3.[bit 63:12] value at BP start-up,  shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32298_cr3_start_up()
{
	volatile u64 cr3 = *((volatile u64 *)STARTUP_CR3_ADDR);

	report("%s", (cr3 & ~0xfffull) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR2 state following start-up_001
 *
 * Summary: Get CR2 value at BP start-up,  shall be 00000000H and same with SDM definition.
 */
static void paging_rqmid_32301_cr2_start_up()
{
	volatile u32 cr2 = *((volatile u32 *)STARTUP_CR2_ADDR);

	report("%s", (cr2 == 0), __FUNCTION__);
}

/**
 * @brief case name: CR0.WP state following start-up_001
 *
 * Summary: Get CR0.WP at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32303_cr0_wp_start_up()
{
	volatile u32 cr0 = get_startup_cr0();

	report("%s", ((cr0 & X86_CR0_WP) == 0), __FUNCTION__);
}

/**
 * @brief case name: CR0.PG state following start-up_001
 *
 * Summary: Get CR0.PG at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32305_cr0_pg_start_up()
{
	volatile u32 cr0 = get_startup_cr0();

	report("%s", ((cr0 & X86_CR0_PG) == 0), __FUNCTION__);
}
#endif

#ifdef __x86_64__
/* test case which should run under 64bit */
#include "64/paging_fn.c"
#elif __i386__
/* test case which should run  under 32bit */
#include "32/paging_fn.c"
#endif

#ifdef __x86_64__
void test_paging_p2()
{
	paging_rqmid_32346_check_linear_address_width();

	/* ----------- init and start-up --------------- */
	paging_rqmid_32265_ia32_efer_nxe_init();
	paging_rqmid_32270_eflags_ac_init();
	paging_rqmid_32272_cr4_smep_init();
	paging_rqmid_32274_cr4_smap_init();
	paging_rqmid_32276_cr4_pse_init();
	paging_rqmid_32278_cr4_pke_init();
	paging_rqmid_32280_cr4_pge_init();
	paging_rqmid_32297_cr4_pae_init();
	paging_rqmid_32290_cr4_pcide_init();
	paging_rqmid_32304_cr0_wp_init();
	paging_rqmid_32306_cr0_pg_init();
	paging_rqmid_32302_cr2_init();
	paging_rqmid_32300_cr3_init();
	paging_rqmid_32264_ia32_efer_nxe_start_up();
	paging_rqmid_32269_eflags_ac_start_up();
	paging_rqmid_32271_cr4_smap_start_up();
	paging_rqmid_32273_cr4_smep_start_up();
	paging_rqmid_32275_cr4_pse_start_up();
	paging_rqmid_32277_cr4_pke_start_up();
	paging_rqmid_32279_cr4_pge_start_up();
	paging_rqmid_32285_cr4_pcide_start_up();
	paging_rqmid_32295_cr4_pae_start_up();
	paging_rqmid_32298_cr3_start_up();
	paging_rqmid_32301_cr2_start_up();
	paging_rqmid_32303_cr0_wp_start_up();
	paging_rqmid_32305_cr0_pg_start_up();
	/* -----------init and start-up--------------- */
}
#endif

static void test_paging()
{
#ifdef __x86_64__
	paging_rqmid_23896_check_tlb_info();
	paging_rqmid_23897_hide_processor_context_identifiers();
	paging_rqmid_23895_check_physical_address_width();

	test_paging_64bit_mode();

	paging_rqmid_23901_global_pages_support();
#elif __i386__
	test_paging_32bit_mode();
#endif
}

static void print_case_list()
{
	printf("paging feature case list:\n\r");
#ifdef __x86_64__
	printf("\t\t Case ID:%d case name:%s\n\r", 23896u, "Encoding of CPUID Leaf 2 Descriptors_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 23897u, "Hide Processor Context Identifiers_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 23895u, "CPUID.80000008H:EAX[7:0]_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 23901u, "Global Pages Support_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 23912u, "Hide Invalidate Process-Context Identifier_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 23918u, "Write Protect Support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24522u, "TLB Support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24519u,
		"Invalidate TLB When vCPU writes CR3_disable global paging_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26017u, "Supervisor Mode Execution Prevention Support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 23917u, "Protection Keys Hide_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24460u, "Invalidate TLB When vCPU changes CR4.SMAP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26827u,
		"Invalidate TLB When vCPU writes CR3_enable global paging_002");
#elif __i386__
	printf("\t\t Case ID:%d case name:%s\n\r", 24415u, "32-Bit Paging Support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 25249u, "Execute Disable support_001");
#endif
}

int main(int ac, char *av[])
{
	printf("feature: paging\r\n");
	printf("ac:%d\r\n", ac);
	for (int i = 0; i < ac; i++) {
		printf("av  i:%d value:%s\r\n", i, av[i]);
	}
	setup_idt();
	setup_vm();
	setup_ring_env();

	print_case_list();
	test_paging();

	return report_summary();
}

