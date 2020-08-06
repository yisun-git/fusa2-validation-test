#include "safety_analysis.h"
#define IA32_L3_MASK_1	(0xC91)
#define IA32_L2_MASK_1	(0xD11)
#define IA32_PQR_ASSOC	(0xC8F)

enum cat_cache_lev_e {
	CAT_CACHE_L3 = 1,
	CAT_CACHE_L2,
};

static __unused
bool check_cat_cache_level(enum cat_cache_lev_e lev)
{
	uint32_t index = 0;
	uint64_t value = 0U;
	int count = 0;
	uint32_t eax = 0U;
	index = (lev == CAT_CACHE_L3) ? IA32_L3_MASK_1 : IA32_L2_MASK_1;
	eax = cpuid_indexed(0x10, lev).a;
	eax &= SHIFT_MASK(4, 0);
	if (eax > 0) {
		count++;
	}

	wrmsr(index, 0xFF00);
	value = rdmsr(index);
	if (value == 0xFF00) {
		count++;
	}

	wrmsr(index, 0xFF);
	value = rdmsr(index);
	if (value == 0xFF) {
		count++;
	}

	wrmsr(IA32_PQR_ASSOC, 0x01);
	value = rdmsr(IA32_PQR_ASSOC);
	if (value == 0x01) {
		count++;
	}

	wrmsr(IA32_PQR_ASSOC, 0x02);
	value = rdmsr(IA32_PQR_ASSOC);
	if (value == 0x02) {
		count++;
	}
	return (count == 5) ? true : false;
}

/*
 * @brief case name:mitigation-Temporal interface-Physical CAT support_001
 *
 * Summary:The physical platform shall support cache allocation technology.
 */
static __unused
void safety_analysis_rqmid_38351_mitigation_Temporal_interface_Physical_CAT_support_001(void)
{
	bool is_pass = false;
	uint32_t ebx = 0U;
	int count = 0;

	/*check CPUID.(EAX=07H, ECX=0):EBX.PQE[bit 15] should be 1b*/
	ebx = cpuid(0x7).b;
	ebx = ebx & SHIFT_LEFT(1U, 15);
	if (ebx) {
		count++;
	}
	//SAFETY_ANALYSIS_DBG("\n ebx=%x", ebx);
	/*get CPUID.(EAX=10H, ECX=0H) Available Resource Type Identification,
	 *the EBX[bit2:1] > 0
	 */
	ebx = cpuid(0x10).b;
	if (ebx & SHIFT_LEFT(1U, 1)) {
		is_pass = check_cat_cache_level(CAT_CACHE_L3);
	} else if (ebx & SHIFT_LEFT(1U, 2)) {
		is_pass = check_cat_cache_level(CAT_CACHE_L2);
	}
	is_pass = ((is_pass == true) && (count == 1)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

static __unused void print_case_list(void)
{
	printf("\t\t safety_analysis feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 38351u,
	"mitigation-Temporal interface-Physical CAT support_001");
}

int main(void)
{
	setup_idt();
	print_case_list();

#ifdef IN_NATIVE
	safety_analysis_rqmid_38351_mitigation_Temporal_interface_Physical_CAT_support_001();
#endif

	return report_summary();
}

