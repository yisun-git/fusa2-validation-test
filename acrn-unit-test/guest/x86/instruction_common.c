#include "libcflat.h"
#include "desc.h"
#include "apic.h"
#include "processor.h"
#include "atomic.h"
#include "asm/barrier.h"
#include "asm/spinlock.h"
#include "instruction_common.h"

/* ---------------------- CPUID ---------------------- */

/**
 *@Sub-Conditions:
 *      CPUID.AVX: 1
 *@test purpose:
 *      Confirm the processor support AVX feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H,ECX=0):ECX[bit 28]
 *@Expected Result:
 *      CPUID.(EAX=01H,ECX=0):ECX[bit 28] should be 1
 */
bool cpuid_avx_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H,ECX=0):ECX[bit 28] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_28);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_28)) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX: 0
 *@test purpose:
 *      Confirm processor not support AVX Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H,ECX=0):ECX[bit 28]
 *@Expected Result:
 *      CPUID.(EAX=01H,ECX=0):ECX[bit 28] should be 0
 */
bool cpuid_avx_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("****** Check CPUID.(EAX=01H,ECX=0):ECX[bit 28] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_28);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX2: 1
 *@test purpose:
 *      Confirm processor support AVX2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 5]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 5] should be 1
 */
bool cpuid_avx2_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("****** Check CPUID.(EAX=07H,ECX=0):EBX[bit 5] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05)) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX2: 0
 *@test purpose:
 *      Confirm processor not support AVX2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 5]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 5] should be 0
 */
bool cpuid_avx2_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("****** Check CPUID.(EAX=07H,ECX=0):EBX[bit 5] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512F: 0
 *@test purpose:
 *      Confirm processor not support AVX512F Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 16]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 16] should be 0
 */
bool cpuid_avx512f_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("****** Check CPUID.(EAX=07H,ECX=0):EBX[bit 16] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_16);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512DQ: 0
 *@test purpose:
 *      Confirm processor not support AVX512DQ Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 17]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 17]  should be 0
 */
bool cpuid_avx512dq_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("****** Confirm processor not support AVX512DQ Feature **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL: 0
 *@test purpose:
 *      Confirm processor not support AVX512VL Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 31]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 31]  should be 0
 */
bool cpuid_avx512vl_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("****** Check CPUID.(EAX=07H,ECX=0):EBX[bit 31] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512BW: 0
 *@test purpose:
 *      Confirm processor not support AVX512BW Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 30]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 30]  should be 0
 */
bool cpuid_avx512bw_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("****** Check CPUID.(EAX=07H,ECX=0):EBX[bit 30] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_30);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512ER: 0
 *@test purpose:
 *      Confirm processor not support AVX512ER Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 27]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 27]  should be 0
 */
bool cpuid_avx512er_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("****** Check CPUID.(EAX=07H,ECX=0):EBX[bit 27] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_27);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512PF: 0
 *@test purpose:
 *      Confirm processor not support AVX512PF Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 26]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 26]  should be 0
 */
bool cpuid_avx512pf_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("****** Check CPUID.(EAX=07H,ECX=0):EBX[bit 26] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_26);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512_4VNNIW: 0
 *@test purpose:
 *      No step
 *@Design Steps:
 *      No step
 *@Expected Result:
 *      No step
 */
bool cpuid_avx512_4vnniw_(void){ return 0; }

/**
 *@Sub-Conditions:
 *      CPUID.AVX512_4FMAPS: 0
 *@test purpose:
 *      No step
 *@Design Steps:
 *      No step
 *@Expected Result:
 *      No step
 */
bool cpuid_avx512_4fmaps_(void){ return 0; }

/**
 *@Sub-Conditions:
 *      CPUID.AVX512CD: 0
 *@test purpose:
 *      Confirm processor not support AVX512CD Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 28]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 28] should be 0
 */
bool cpuid_avx512cd_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("****** Check CPUID.(EAX=07H,ECX=0):EBX[bit 28] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_28);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512_VBMI: 0
 *@test purpose:
 *      Confirm processor not support AVX512_VBMI Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):ECX[bit 1]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):ECX[bit 1] should be 0
 */
bool cpuid_avx512_vbmi_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("****** Check CPUID.(EAX=07H,ECX=0):ECX[bit 1] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_01);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512_IFMA: 0
 *@test purpose:
 *      Confirm processor not support AVX512_IFMA Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 21]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 21] should be 0
 */
bool cpuid_avx512_ifma_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=07H,ECX=0):EBX[bit 21] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_21);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL & CPUID.AVX512F: 0
 *@test purpose:
 *      Confirm either AVX512VL Feature or AVX512F Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 16]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 16]} should be 0
 */
bool cpuid_avx512vl_avx512f_to_0(void)
{
	unsigned long check_bit_vl = 0;
	unsigned long check_bit_f = 0;
	bool result = false;

	printf("***** Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 16]} *****\n");
	/* AVX512VL */
	check_bit_vl = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_vl &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31);
	/* AVX512F */
	check_bit_f = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_f &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_16);

	if ((check_bit_vl == 0) || (check_bit_f == 0)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL & CPUID.AVX512DQ: 0
 *@test purpose:
 *      Confirm either AVX512VL Feature or AVX512DQ Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 17]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 17]} should be 0
 */
bool cpuid_avx512vl_avx512dq_to_0(void)
{
	unsigned long check_bit_vl = 0;
	unsigned long check_bit_dq = 0;
	bool result = false;

	printf("***** Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 17]} *****\n");
	/* AVX512VL */
	check_bit_vl = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_vl &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31);
	/* AVX512DQ */
	check_bit_dq = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_dq &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17);

	if ((check_bit_vl == 0) || (check_bit_dq == 0)) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL & CPUID.AVX512BW: 0
 *@test purpose:
 *      Confirm either AVX512VL Feature or AVX512BW Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 30]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 30]} should be 0
 */
bool cpuid_avx512vl_avx512bw_to_0(void)
{
	unsigned long check_bit_vl = 0;
	unsigned long check_bit_bw = 0;
	bool result = false;

	printf("***** Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 30]} *****\n");
	/* AVX512VL */
	check_bit_vl = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_vl &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31);
	/* AVX512BW */
	check_bit_bw = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_bw &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_30);

	if ((check_bit_vl == 0) || (check_bit_bw == 0)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512B & CPUID.W: 0
 *@test purpose:
 *      Confirm processor not support AVX512BW Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 30]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 30] should be 0
 */
bool cpuid_avx512b_w_to_0(void)
{
	unsigned long check_bit;
	bool result = false;

	printf("***** Check CPUID.(EAX=07H,ECX=0):EBX[bit 30] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_30);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL & CPUID.AVX512CD: 0
 *@test purpose:
 *      Confirm either AVX512VL Feature or AVX512CD Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 28]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 28]} should be 0
 */
bool cpuid_avx512vl_avx512cd_to_0(void)
{
	unsigned long check_bit_vl = 0;
	unsigned long check_bit_cd = 0;
	bool result = false;

	printf("***** Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 28]} *****\n");
	/* AVX512VL */
	check_bit_vl = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_vl &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31);
	/* AVX512CD */
	check_bit_cd = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_cd &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_28);

	if ((check_bit_vl == 0) || (check_bit_cd == 0)) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL & CPUID.AVX512_VBMI: 0
 *@test purpose:
 *      Confirm either AVX512VL Feature or AVX512_VBMI Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):ECX[bit 1]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):ECX[bit 1]} should be 0
 */
bool cpuid_avx512vl_avx512vbmi_to_0(void)
{
	unsigned long check_bit_vl = 0;
	unsigned long check_bit_vbmi = 0;
	bool result = false;

	printf("***** Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 1]} *****\n");
	/* AVX512VL */
	check_bit_vl = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_vl &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31);
	/* AVX512VBMI */
	check_bit_vbmi = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit_vbmi &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_01);

	if ((check_bit_vl == 0) || (check_bit_vbmi == 0)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512_IFMA & CPUID.AVX512VL: 0
 *@test purpose:
 *      Confirm either AVX512_IFMA Feature or AVX512VL Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 21]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 31]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 21]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} should be 0
 */
bool cpuid_avx512ifma_avx512vl_to_0(void)
{
	unsigned long check_bit_vl = 0;
	unsigned long check_bit_ifma = 0;
	bool result = false;

	printf("***** Check {CPUID.(EAX=07H,ECX=0):EBX[bit 21]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} *****\n");
	/* AVX512VL */
	check_bit_vl = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_vl &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31);
	/* AVX512IFMA */
	check_bit_ifma = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_ifma &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_21);

	if ((check_bit_ifma == 0) || (check_bit_vl == 0)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512F & CPUID.AVX512BW: 0
 *@test purpose:
 *      Confirm either AVX512F Feature or AVX512BW Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 16]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 30]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 16]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 30]} should be 0
 */
bool cpuid_avx512f_avx512bw_to_0(void)
{
	unsigned long check_bit_f = 0;
	unsigned long check_bit_bw = 0;
	bool result = false;

	printf("***** Check {CPUID.(EAX=07H,ECX=0):EBX[bit 16]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 30]} *****\n");
	/* AVX512F */
	check_bit_f = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_f &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_16);
	/* AVX512BW */
	check_bit_bw = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit_bw &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_30);

	if ((check_bit_f == 0) || (check_bit_bw == 0)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.AVX512D & CPUID.Q: 0
 *@test purpose:
 *      Confirm processor not support AVX512DQ Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 17]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 17] should be 0
 */
bool cpuid_avx512d_avx512q_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=07H,ECX=0):EBX[bit 17] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.W
 *@test purpose:
 *      Confirm processor not support W(Ways of associativity)
 *@Design Steps:
 *      Check CPUID.(EAX=04H):EBX[bit 31:22]
 *@Expected Result:
 *      CPUID.(EAX=04H):EBX[bit 31:22] should be 0
 */
bool cpuid_w_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=04H):EBX[bit 31:22] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_04, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT_RANGE(CPU_NOT_SUPPORT_W, FEATURE_INFORMATION_22);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.P
 *@test purpose:
 *      Confirm processor not support P(Physical Line partitions)
 *@Design Steps:
 *      Check CPUID.(EAX=04H):EBX[bit 21:12]
 *@Expected Result:
 *      CPUID.(EAX=04H):EBX[bit 21:12] should be 0
 */
bool cpuid_p_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=04H):EBX[bit 21:12] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_04, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT_RANGE(CPU_NOT_SUPPORT_P, FEATURE_INFORMATION_12);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.L
 *@test purpose:
 *      Confirm processor not support L(System Coherency Line Size)
 *@Design Steps:
 *      Check CPUID.(EAX=04H):EBX[bit 11:0]
 *@Expected Result:
 *      CPUID.(EAX=04H):EBX[bit 11:0] should be 0
 */
bool cpuid_l_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=04H):EBX[bit 11:0] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_04, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT_RANGE(CPU_NOT_SUPPORT_L, FEATURE_INFORMATION_00);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSSE3: 0
 *@test purpose:
 *      Insure processor not support SSSE3 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 9]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 9] should be 0
 */
bool cpuid_ssse3_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 9] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_09);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSSE3: 1
 *@test purpose:
 *      Insure processor support SSSE3 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 9]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 9] should be 1
 */
bool cpuid_ssse3_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 9] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_09);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_09)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSE: 1
 *@test purpose:
 *      Confirm processor support SSE Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):EDX[bit 25]
 *@Expected Result:
 *      CPUID.(EAX=01H):EDX[bit 25] should be 1
 */
bool cpuid_sse_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):EDX[bit 25] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_25);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_25)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSE2: 1
 *@test purpose:
 *      Confirm processor support SSE2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):EDX[bit 26]
 *@Expected Result:
 *      CPUID.(EAX=01H):EDX[bit 26] should be 1
 */
bool cpuid_sse2_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):EDX[bit 26] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_26);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_26)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSE3: 1
 *@test purpose:
 *      Confirm processor support SSE3 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 0]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 0] should be 1
 */
bool cpuid_sse3_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 0] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSE4_1: 1
 *@test purpose:
 *      Confirm processor support SSE4.1 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 19]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 19] should be 1
 */
bool cpuid_sse4_1_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 19] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_19);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_19)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSE4_2: 1
 *@test purpose:
 *      Confirm processor support SSE4.2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 20]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 20] should be 1
 */
bool cpuid_sse4_2_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 20] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_20);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_20)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSE: 0
 *@test purpose:
 *      Confirm processor not support SSE Feature
 *@Design Steps:
 *      Check CPUID.(EAX=0DH,ECX=0):EAX[bit 1]
 *@Expected Result:
 *      CPUID.(EAX=0DH,ECX=0):EAX[bit 1] should be 0
 */
bool cpuid_sse_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=0DH,ECX=0):EAX[bit 1] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_0D, EXTENDED_STATE_SUBLEAF_0).a;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_01);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSE2: 0
 *@test purpose:
 *      Confirm processor not support SSE2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):EDX[bit 26]
 *@Expected Result:
 *      CPUID.(EAX=01H):EDX[bit 26] should be 0
 */
bool cpuid_sse2_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):EDX[bit 26] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_26);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSE3: 0
 *@test purpose:
 *      Confirm processor not support SSE3 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 0]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 0] should be 0
 */
bool cpuid_sse3_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 0] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSE4_1: 0
 *@test purpose:
 *      Confirm processor not support SSE4.1 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 19]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 19] should be 0
 */
bool cpuid_sse4_1_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 19] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_19);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SSE4_2: 0
 *@test purpose:
 *      Confirm processor not support SSE4.2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 20]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 20] should be 0
 */
bool cpuid_sse4_2_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 20] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_20);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.SMAP: 1
 *@test purpose:
 *      Confirm processor support SMAP Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 20]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 20] should be 1
 */
bool cpuid_smap_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=07H,ECX=0):EBX[bit 20] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_20);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_20)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.MMX: 1
 *@test purpose:
 *      Confirm processor support MMX Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):EDX[bit 23]
 *@Expected Result:
 *      CPUID.(EAX=01H):EDX[bit 23] should be 1
 */
bool cpuid_mmx_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):EDX[bit 23] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_23);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_23)) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.MMX: 0
 *@test purpose:
 *      Confirm processor not support MMX Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):EDX[bit 23]
 *@Expected Result:
 *      CPUID.(EAX=01H):EDX[bit 23] should be 0
 */
bool cpuid_mmx_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):EDX[bit 23] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_23);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.RDTSCP: 0
 *@test purpose:
 *      Confirm processor NOT support RDTSCP
 *@Design Steps:
 *      Check CPUID.(EAX=80000001H):EDX[bit 27]
 *@Expected Result:
 *      CPUID.(EAX=80000001H):EDX[bit 27] should be 0
 */
bool cpuid_rdtscp_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=80000001H):EDX[bit 27] *****\n");

	check_bit = cpuid_indexed(CPUID_EXTERN_INFORMATION_801, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_27);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.RDTSCP: 1
 *@test purpose:
 *      Confirm processor support RDTSCP
 *@Design Steps:
 *      Check CPUID.(EAX=80000001H):EDX[bit 27]
 *@Expected Result:
 *      CPUID.(EAX=80000001H):EDX[bit 27] should be 1
 */
bool cpuid_rdtscp_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=80000001H):EDX[bit 27] *****\n");

	check_bit = cpuid_indexed(CPUID_EXTERN_INFORMATION_801, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_27);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_27)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.XSAVEOPT: 1
 *@test purpose:
 *      Confirm processor support XSAVEOPT
 *@Design Steps:
 *      Check CPUID.(EAX=0DH,ECX=1):EAX[bit 0]
 *@Expected Result:
 *      CPUID.(EAX=0DH,ECX=1):EAX[bit 0] should be 1
 */
bool cpuid_xsaveopt_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=0DH,ECX=1):EAX[bit 0] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_0D, EXTENDED_STATE_SUBLEAF_1).a;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.ADX:0
 *@test purpose:
 *      Confirm processor NOT support ADX
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 19]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 19] should be 0
 */
bool cpuid_adx_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=07H,ECX=0):EBX[bit 19] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_19);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.ADX:1
 *@test purpose:
 *      Confirm processor support ADX
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 19]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 19] should be 1
 */
bool cpuid_adx_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=07H,ECX=0):EBX[bit 19] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_19);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_19)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.LAHF-SAHFLOCK:1
 *@test purpose:
 *      Confirm processor support LAHF/SAHF
 *@Design Steps:
 *      Check CPUID.(EAX=80000001H):ECX[bit 0]
 *@Expected Result:
 *      CPUID.(EAX=80000001H):ECX[bit 0] should be 1
 */
bool cpuid_lahf_sahflock_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=80000001H):ECX[bit 0] *****\n");

	check_bit = cpuid_indexed(CPUID_EXTERN_INFORMATION_801, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.FSGSBASELOCK:1
 *@test purpose:
 *      Confirm processor support FSBASE/GSBASE
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 0]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 0] should be 1
 */
bool cpuid_fsgsbaselock_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=07H,ECX=0):EBX[bit 0] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.XSAVE:1
 *@test purpose:
 *      Confirm processor support XSAVE
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 26]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 26] should be 1
 */
bool cpuid_xsave_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 26] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_26);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_26)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.OSXSAVE:1
 *@test purpose:
 *      Confirm OS has set CR4.OSXSAVE[bit 18] to enable XSETBV/XGETBV instructions to access XCR0
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 27]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 27] should be 1
 */
bool cpuid_osxsave_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 27] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_27);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_27)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.F16C: 0
 *@test purpose:
 *      Confirm processor not support F16C Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 29]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 29] should be 0
 */
bool cpuid_f16c_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 29] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.F16C: 1
 *@test purpose:
 *      Confirm processor support F16C Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 29]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 29] should be 1
 */
bool cpuid_f16c_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 29] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.FMA: 0
 *@test purpose:
 *      Confirm processor not support FMA Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 12]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 12] should be 0
 */
bool cpuid_fma_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 12] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.FMA: 1
 *@test purpose:
 *      Confirm processor support FMA Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 12]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 12] should be 1
 */
bool cpuid_fma_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=01H):ECX[bit 12] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.CMPXCHG16B: 1
 *@test purpose:
 *      Confirm processor support CMPXCHG16B
 *@Design Steps:
 *      Check CPUID.01H:ECX.CMPXCHG16B[bit 13]
 *@Expected Result:
 *      CPUID.01H:ECX.CMPXCHG16B[bit 13] should be 1
 */
bool cpuid_cmpxchg16b_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.01H:ECX.CMPXCHG16B[bit 13] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.CMPXCHG16B: 0
 *@test purpose:
 *      Confirm processor NOT support CMPXCHG16B
 *@Design Steps:
 *      Check CPUID.01H:ECX.CMPXCHG16B[bit 13]
 *@Expected Result:
 *      CPUID.01H:ECX.CMPXCHG16B[bit 13] should be 0
 */
bool cpuid_cmpxchg16b_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.01H:ECX.CMPXCHG16B[bit 13] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.LAHF-SAHF: 1
 *@test purpose:
 *      Confirm processor support LAHF-SAH
 *@Design Steps:
 *      Check CPUID.80000001H:ECX.LAHF-SAHF[bit 0]
 *@Expected Result:
 *      CPUID.80000001H:ECX.LAHF-SAHF[bit 0] should be 1
 */
bool cpuid_lahf_sahf_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.80000001H:ECX.LAHF-SAHF[bit 0] *****\n");

	check_bit = cpuid_indexed(CPUID_EXTERN_INFORMATION_801, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}


/**
 *@Sub-Conditions:
 *      CPUID.LAHF-SAHF: 0
 *@test purpose:
 *      Confirm processor NOT support LAHF-SAHF
 *@Design Steps:
 *      Check CPUID.80000001H:ECX.LAHF-SAHF[bit 0]
 *@Expected Result:
 *      CPUID.80000001H:ECX.LAHF-SAHF[bit 0] should be 0
 */
bool cpuid_lahf_sahf_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.80000001H:ECX.LAHF-SAHF[bit 0] *****\n");

	check_bit = cpuid_indexed(CPUID_EXTERN_INFORMATION_801, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.RDSEED: 1
 *@test purpose:
 *      Confirm processor support RDSEED
 *@Design Steps:
 *      Check CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18]
 *@Expected Result:
 *      CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18] should be 1
 */
bool cpuid_rdseed_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.RDSEED: 0
 *@test purpose:
 *      Confirm processor NOT support RDSEED
 *@Design Steps:
 *      Check CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18]
 *@Expected Result:
 *      CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18] should be 0
 */
bool cpuid_rdseed_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.RDRAND: 1
 *@test purpose:
 *      Confirm processor support RDRAND
 *@Design Steps:
 *      Check CPUID.01H:ECX.RDRAND[bit 30]
 *@Expected Result:
 *      CPUID.01H:ECX.RDRAND[bit 30] should be 1
 */
bool cpuid_rdrand_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.01H:ECX.RDRAND[bit 30] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_30);

	if (check_bit == FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_30)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CPUID.RDRAND: 0
 *@test purpose:
 *      Confirm processor NOT support RDRAND
 *@Design Steps:
 *      Check CPUID.01H:ECX.RDRAND[bit 30]
 *@Expected Result:
 *      CPUID.01H:ECX.RDRAND[bit 30] should be 0
 */
bool cpuid_rdrand_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Check CPUID.01H:ECX.RDRAND[bit 30] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_30);

	if (check_bit == 0) {
		result = true;
	} else {
		result = false;
	}

	return result;
}
/* ---------------------- CPUID END ---------------------- */

/* ---------------------- CR4 ---------------------- */
/**
 *@Sub-Conditions:
 *      CR4.OSXSAVE: 0
 *@test purpose:
 *      Disable XSAVE feature set
 *@Design Steps:
 *      Clear CR4.OSXSAVE[bit 18] to 0
 *@Expected Result:
 *      N/A
 */
bool cr4_osxsave_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;
	printf("***** Clear CR4.OSXSAVE[bit 18] to 0 *****\n");

	check_bit = read_cr4();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18)));

	write_cr4(check_bit);

	if (check_bit == read_cr4()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR4.OSXSAVE: 1
 *@test purpose:
 *      Enable XSAVE feature set
 *@Design Steps:
 *      Clear CR4.OSXSAVE[bit 18] to 1
 *@Expected Result:
 *      N/A
 */
bool cr4_osxsave_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;
	printf("***** Set CR4.OSXSAVE[bit 18] to 1 *****\n");

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18));

	write_cr4(check_bit);

	if (check_bit == read_cr4()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR4.OSFXSR: 0
 *@test purpose:
 *      Disable FXSAVE and FXRSTOR feature set
 *@Design Steps:
 *      Clear CR4.OSXSAVE[bit 19] to 0
 *@Expected Result:
 *      N/A
 */
bool cr4_osfxsr_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR4.OSXSAVE[bit 19] to 0 *****\n");

	check_bit = read_cr4();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_19)));

	write_cr4(check_bit);

	if (check_bit == read_cr4()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR4.OSFXSR: 1
 *@test purpose:
 *      Enable FXSAVE and FXRSTOR feature set
 *@Design Steps:
 *      Set CR4.OSXSAVE[bit 19] to 1
 *@Expected Result:
 *      N/A
 */
bool cr4_osfxsr_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR4.OSXSAVE[bit 19] to 1 *****\n");

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_19));

	write_cr4(check_bit);

	if (check_bit == read_cr4()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR4.OSXMMEXCPT: 0
 *@test purpose:
 *      Clear CR4.OSXMMEXCPT[bit 10]
 *@Design Steps:
 *      Clear CR4.OSXMMEXCPT[bit 10] to 0
 *@Expected Result:
 *      N/A
 */
bool cr4_osxmmexcpt_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR4.OSXMMEXCPT[bit 10] to 0 *****\n");

	check_bit = read_cr4();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_10)));

	write_cr4(check_bit);

	if (check_bit == read_cr4()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR4.OSXMMEXCPT: 1
 *@test purpose:
 *      To use CR4.OSXMMEXCPT[bit 10] to provide additional control
 *      over generation of SIMD floating-point exceptions
 *@Design Steps:
 *      Set CR4.OSXMMEXCPT[bit 10] to 1
 *@Expected Result:
 *      N/A
 */
bool cr4_osxmmexcpt_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR4.OSXMMEXCPT[bit 10] to 1 *****\n");

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_10));

	write_cr4(check_bit);

	if (check_bit == read_cr4()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}
/* ---------------------- CR4 END---------------------- */

/* ---------------------- CR0 ---------------------- */
/**
 *@Sub-Conditions:
 *      CR0.PE: 0
 *@test purpose:
 *      Clear Page Enable flag, to enables real-address
 *      mode(only enables segment-level protection)
 *@Design Steps:
 *      Clear CR0.PE[bit 0] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_pe_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.PE[bit 0] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00)));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.PE: 1
 *@test purpose:
 *      Set Page Enable flag, to enables protected
 *      mode(only enables segment-level protection)
 *@Design Steps:
 *      Set CR0.PE[bit 0] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_pe_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR0.PE[bit 0] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.MP: 0
 *@test purpose:
 *      Clear Monitor Coprocessor flag
 *@Design Steps:
 *      Clear CR0.MP[bit 1] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_mp_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.MP[bit 1] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_01)));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.MP: 1
 *@test purpose:
 *      Set Monitor Coprocessor flag
 *@Design Steps:
 *      Set CR0.MP[bit 1] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_mp_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR0.MP[bit 1] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_01));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.EM: 0
 *@test purpose:
 *      Clear Emulation flag
 *@Design Steps:
 *      Clear CR0.EM[bit 2] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_em_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.EM[bit 2] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_02)));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.EM: 1
 *@test purpose:
 *      Set Emulation flag
 *@Design Steps:
 *      Set CR0.EM[bit 2] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_em_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR0.EM[bit 2] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_02));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.TS: 0
 *@test purpose:
 *      Clear Task Switched flag
 *@Design Steps:
 *      Clear CR0.TS[bit 3] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_ts_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.TS[bit 3] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_03)));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.TS: 1
 *@test purpose:
 *      Sets Task Switched flag on every task switch and tests it
 *      when executing x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions
 *@Design Steps:
 *      Set CR0.TS[bit 3] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_ts_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR0.TS[bit 3] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_03));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.ET: 1
 *@test purpose:
 *      Extension Type,In the Pentium 4, Intel Xeon,
 *      and P6 family processors, this flag is hardcoded to 1
 *@Design Steps:
 *      Set CR0.ET[bit 4] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_et_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR0.ET[bit 4] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_04));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.NE: 0
 *@test purpose:
 *      Clear Numeric Error flag,enables the PC-style x87 FPU error reporting mechanism
 *@Design Steps:
 *      Clear CR0.NE[bit 5] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_ne_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.NE[bit 5] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05)));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.NE: 1
 *@test purpose:
 *      Set Numeric Error flag,enables the native (internal) mechanism for reporting x87 FPU errors
 *@Design Steps:
 *      Set CR0.NE[bit 5] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_ne_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR0.NE[bit 5] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.WP: 0
 *@test purpose:
 *      Clear Write Protect flag to allow supervisor-level procedures to write into read-only pages
 *@Design Steps:
 *      Clear CR0.WP[bit 16] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_wp_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.WP[bit 16] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_16)));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.WP: 1
 *@test purpose:
 *      Set Write Protect flag, inhibits supervisor-level procedures from writing into readonly pages
 *@Design Steps:
 *      Set CR0.WP[bit 16] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_wp_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR0.WP[bit 16] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_16));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.AM: 0
 *@test purpose:
 *      Disables automatic alignment checking
 *@Design Steps:
 *      Clear CR0.AM[bit 18] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_am_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.AM[bit 18] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18)));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.AM: 1
 *@test purpose:
 *      Enables automatic alignment checking
 *@Design Steps:
 *      Set CR0.AM[bit 18] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_am_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR0.AM[bit 18] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.AC: 0
 *@test purpose:
 *      Disables automatic alignment checking
 *@Design Steps:
 *      Clear CR0.AM[bit 18] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_ac_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.AM[bit 18] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18)));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.AC: 1
 *@test purpose:
 *      Enables automatic alignment checking
 *@Design Steps:
 *      Set CR0.AM[bit 18] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_ac_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR0.AM[bit 18] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.NW: 0
 *@test purpose:
 *      Enable write-back/write-throug
 *@Design Steps:
 *      Clear CR0.NW[bit 29] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_nw_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.NW[bit 29] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29)));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.NW: 1
 *@test purpose:
 *      Disable write-back/write-through
 *@Design Steps:
 *      Set CR0.NW[bit 29] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_nw_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR0.NW[bit 29] to 0 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.CD: 0
 *@test purpose:
 *      Enable Cache
 *@Design Steps:
 *      Clear CR0.CD[bit 30] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_cd_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.CD[bit 30] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_30)));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.CD: 1
 *@test purpose:
 *      Disable Cache
 *@Design Steps:
 *      Set CR0.CD[bit 30] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_cd_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set CR0.CD[bit 30] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_30));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.PG: 0
 *@test purpose:
 *      Disables paging, try to clear the bit of CR0.PG
 *@Design Steps:
 *      Clear CR0.PG[bit 31] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_pg_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.PG[bit 31] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31)));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR0.PG: 1
 *@test purpose:
 *      Enables paging
 *@Design Steps:
 *      Instruction execute Normally:
 *      after set PE(CR0[bit 0]) to 0, set CR0.PG[bit 31] to 1;
 *      #GP: clear PE(CR0[bit 0]), then set CR0.PG[bit 31] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_pg_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Clear CR0.PG[bit 31] to 0 *****\n");

	/* Clear PE(CR0[bit 0]) to 0 */
	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00)));
	write_cr0(check_bit);

	/* Set PG(CR0[bit 31]) to 1 */
	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31));

	write_cr0(check_bit);

	if (check_bit == read_cr0()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}
/* ---------------------- CR0 END---------------------- */

/* ---------------------- EFLAGS ---------------------- */
/**
 *@Sub-Conditions:
 *      EFLAGS.CF: 1
 *@Design Steps:
 *      Set EFLAGS.CF[bit 0] to 1
 *@Expected Result:
 *      N/A
 */
bool eflags_cf_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set EFLAGS.CF[bit 0] to 1 *****\n");

	check_bit = read_rflags();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00));

	write_rflags(check_bit);

	if (check_bit == read_rflags()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}


/**
 *@Sub-Conditions:
 *      EFLAGS.AC: 1
 *@test purpose:
 *      Enable Alignment check/access control flag, when CR0.AM is set
 *@Design Steps:
 *      Set EFLAGS.AC[bit 18] to 1
 *@Expected Result:
 *      N/A
 */
bool eflags_ac_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set EFLAGS.AC[bit 18] to 1 *****\n");

	check_bit = read_rflags();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18));

	write_rflags(check_bit);

	if (check_bit == read_rflags()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      EFLAGS.AC: 0
 *@test purpose:
 *      Clear Alignment check/access control flag
 *@Design Steps:
 *      Set EFLAGS.AC[bit 18] to 0
 *@Expected Result:
 *      N/A
 */
bool eflags_ac_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set EFLAGS.AC[bit 18] to 0 *****\n");

	check_bit = read_rflags();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18)));

	write_rflags(check_bit);

	if (check_bit == read_rflags()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      EFLAGS.NT: 1
 *@test purpose:
 *      Set EFLAGS.NT to 1 in the current task's TSS
 *@Design Steps:
 *      Set current TSS's EFLAGS.NT[bit 14] to 1
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception.
 */
bool eflags_nt_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set current TSS's EFLAGS.NT[bit 14] to 1 *****\n");

	check_bit = read_rflags();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_14));

	write_rflags(check_bit);

	if (check_bit == read_rflags()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      EFLAGS.NT: 0
 *@test purpose:
 *      Set EFLAGS.NT to 0 in the current task's TSS
 *@Design Steps:
 *      Set current TSS's EFLAGS.NT[bit 14] to 0
 *@Expected Result:
 *      N/A
 */
bool eflags_nt_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set current TSS's EFLAGS.NT[bit 14] to 0 *****\n");

	check_bit = read_rflags();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_14)));

	write_rflags(check_bit);

	if (check_bit == read_rflags()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR4.R.W: 1
 *@test purpose:
 *      Try to Set the reserved bit combinations in CR4
 *@Design Steps:
 *      Set the reserved bit in CR4, such as CR4[12] or [23:31]
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception
 */
void write_cr4_checking(unsigned long val)
{
	asm volatile("mov %0, %%cr4\n\t"
		: : "r"(val));
}
void cr4_r_w_to_1(void)
{
	gp_trigger_func fun;
	bool ret;
	unsigned long check_bit = 0;

	check_bit = read_cr4();

	switch (get_random_value()%2) {
	case 0:
		printf("***** Set the reserved bit in CR4, such as CR4[12]*****\n");
		check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12));

		fun = (gp_trigger_func)write_cr4_checking;
		ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

		/* Expected write #GP exception */
		report("#GP exception", ret == true);
		break;
	case 1:
		printf("***** Set the reserved bit in CR4, such as CR4[23:31]*****\n");
		check_bit |= (FEATURE_INFORMATION_BIT_RANGE(CR4_RESEVED_BIT_23, FEATURE_INFORMATION_23));

		fun = (gp_trigger_func)write_cr4_checking;
		ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

		/* Expected write #GP exception */
		report("#GP exception", ret == true);
		break;
	}
}

/**
 *@Sub-Conditions:
 *      CR3.R.W: 1
 *@test purpose:
 *      Try to Set the reserved bit combinations in CR3
 *@Design Steps:
 *      Set the reserved bit in CR3, such as CR3[0:2] or [5:11]
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception
 */
void write_cr3_checking(unsigned long val)
{
	asm volatile(
		"mov %0, %%cr3\n\t"
		"1:"
		: : "r"(val));
}

void cr3_r_w_to_1(void)
{
	gp_trigger_func fun;
	bool ret;
	unsigned long check_bit = 0;

	check_bit = read_cr3();

	switch (get_random_value()%2) {
	case 0:
		printf("***** Set the reserved bit in CR3, such as CR3[0:2]*****\n");
		check_bit &= (FEATURE_INFORMATION_BIT_RANGE(CR3_RESEVED_BIT_2, FEATURE_INFORMATION_00));

		/* Expected write #GP exception */
		fun = (gp_trigger_func)write_cr3_checking;
		ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);
		report("#GP exception", ret == true);
		break;
	case 1:
		printf("***** Set the reserved bit in CR3, such as CR3[5:11] *****\n");
		check_bit &= (FEATURE_INFORMATION_BIT_RANGE(CR3_RESEVED_BIT_5, FEATURE_INFORMATION_05));

		/* Expected write #GP exception */
		fun = (gp_trigger_func)write_cr3_checking;
		ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);
		report("#GP exception", ret == true);
		break;
	}
}

/**
 *@Sub-Conditions:
 *      CR8.R.W: 1
 *@test purpose:
 *      Try to Set the reserved bit combinations in CR8
 *@Design Steps:
 *      Set the reserved bit in CR8[64:4]
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception
 */
void write_cr8_checking(unsigned long val)
{
	asm volatile("mov %0, %%cr8\n\t"
		: : "r"(val));
}
inline void cr8_r_w_to_1(void)
{
	gp_trigger_func fun;
	bool ret;
	unsigned long check_bit = 0;

	printf("***** Set the reserved bit in CR8[64:4] *****\n");

	check_bit = read_cr8();
	check_bit &= (FEATURE_INFORMATION_BIT_RANGE(CR8_RESEVED_BIT_4, FEATURE_INFORMATION_04));

	fun = (gp_trigger_func)write_cr8_checking;
	ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

	/* Expected write #GP exception */
	report("%s #GP exception", ret == true, __FUNCTION__);
}

/**
 *@Sub-Conditions:
 *      CR4.PCIDE: 1
 *@test purpose:
 *      Try to Set the bit of Enables process-context identifiers in CR4
 *@Design Steps:
 *      Using MOV instruction to set the PCID-Enable Bit CR4.PCIDE[bit 17] to 1
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception
 */
bool cr4_pcide_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Using MOV instruction to set the PCID-Enable Bit CR4.PCIDE[bit 17] to 1  *****\n");

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));

	write_cr4(check_bit);

	if (check_bit == read_cr4()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR4.PCIDE: 0
 *@test purpose:
 *      Try to clear the bit of Enables process-context identifiers in CR4
 *@Design Steps:
 *      Using MOV instruction to set the PCID-Enable Bit CR4.PCIDE[bit 17] to 0
 *@Expected Result:
 *      No exception
 */
bool cr4_pcide_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Using MOV instruction to set the PCID-Enable Bit "
	"CR4.PCIDE[bit 17] to 0 *****\n");

	check_bit = read_cr4();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17)));

	write_cr4(check_bit);

	if (check_bit == read_cr4()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      CR4.PCIDE_UP: true
 *@test purpose:
 *      Try to Set the bit of Enables process-context identifiers in CR4
 *      with CR3[11:0] isn't equal with 000H
 *@Design Steps:
 *      Using MOV instruction to set the PCID-Enable Bit CR4.PCIDE[bit 17] to 1,
 *      when CR3[11:0] isn't equal with 000H
 *      (if CR3[11:0] isn't equal 000H, set CR3[11:0] to 000H)
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception
 */
bool cr4_pcide_up_to_1(void)
{
	unsigned long check_bit = 0;
	unsigned long cr3_tmp = 0;
	bool result = false;

	printf("***** Using MOV instruction to set the PCID-Enable Bit CR4.PCIDE[bit 17] to 1"	\
		"when CR3[11:0] isn't equal with 000H *****\n");

	check_bit = read_cr3();
	cr3_tmp = read_cr3();
	cr3_tmp &= FEATURE_INFORMATION_BIT_RANGE(0xfff, FEATURE_INFORMATION_00);

	if (cr3_tmp != 0) {
		check_bit &= (~FEATURE_INFORMATION_BIT_RANGE(0xfff, FEATURE_INFORMATION_00));
		write_cr3(check_bit);
	} else {
		check_bit = read_cr4();
		check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));

		write_cr4(check_bit);

		if (check_bit == read_cr4()) {
			result = true;
		} else {
			result = false;
		}
	}
	return result;
}

/**
 *@Sub-Conditions:
 *      CR4.PAE: 0
 *@test purpose:
 *      Try to clear the bit of CR4.PAE[bit5] to leave IA-32e mode
 *@Design Steps:
 *      Using MOV instruction to clear CR4.PAE[bit 5] to 0
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception
 */
bool cr4_pae_to_0(void)
{
	gp_trigger_func fun;
	bool ret;

	unsigned long check_bit = 0;

	printf("***** Using MOV instruction to clear CR4.PAE[bit 5] to 0 *****\n");

	check_bit = read_cr4();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05)));

	fun = (gp_trigger_func)write_cr4;
	ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

	return ret;
}

/**
 *@Sub-Conditions:
 *      CR4.PAE: 1
 *@test purpose:
 *      Try to set the bit of CR4.PAE[bit5] to leave IA-32e mode
 *@Design Steps:
 *      Using MOV instruction to set CR4.PAE[bit 5] to 1
 *@Expected Result:
 *      No exception
 */
bool cr4_pae_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Using MOV instruction to set CR4.PAE[bit 5] to 1 *****\n");

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05));

	write_cr4(check_bit);

	if (check_bit == read_cr4()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}
/* ---------------------- EFLAGS END---------------------- */

/* ---------------------- GP ---------------------- */

/**
 *@Sub-Conditions:
 *      DR7.GD: 0
 *@test purpose:
 *      Clear GD flag in DR7(general detect enable flag)
 *@Design Steps:
 *      Set DR7.GD[bit 13]  to 0
 *@Expected Result:
 *      N/A
 */
bool dr7_gd_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set DR7.GD[bit 13]  to 0 *****\n");

	check_bit = read_dr7();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13)));

	write_dr7(check_bit);

	if (check_bit == read_dr7()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      DR7.GD: 1
 *@test purpose:
 *      Set GD flag in DR7(general detect enable flag)
 *@Design Steps:
 *      Set DR7.GD[bit 13] to 1
 *@Expected Result:
 *      N/A
 */
bool dr7_gd_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set DR7.GD[bit 13] to 1 *****\n");

	check_bit = read_dr7();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13));

	write_dr7(check_bit);

	if (check_bit == read_dr7()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

/**
 *@Sub-Conditions:
 *      DR6: 1
 *@test purpose:
 *      Using MOV instruction to write a 1 to any of bits 63:32 in DR6
 *@Design Steps:
 *      Using MOV instruction to set DR6[any bit 32:63]to 1
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception in real mode
 */
void write_dr6_checking(unsigned long val)
{
	asm volatile("mov %0, %%dr6\n\t"
		: : "r"(val)
		: "memory");
}

#ifdef __x86_64__
void dr6_to_1(void)
{
	gp_trigger_func fun;
	bool ret;

	unsigned long check_bit = 0;

	printf("***** Using MOV instruction to set DR6[any bit 32:63]to 1 *****\n");

	check_bit |= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_32);

	fun = (gp_trigger_func)write_dr6_checking;
	ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

	/* Expected write #GP exception */
	report("#GP exception", ret == true);
}
#endif

/**
 *@Sub-Conditions:
 *      DR7: 1
 *@test purpose:
 *      Using MOV instruction to write a 1 to any of bits 63:32 in DR7
 *@Design Steps:
 *      Using MOV instruction to set DR7[any bit 32:63] to 1
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception in real mode
 */
void write_dr7_checking(unsigned long val)
{
	asm volatile("mov %0, %%dr7\n\t"
		: : "r"(val)
		: "memory");
}
#ifdef __x86_64__
void dr7_to_1(void)
{
	gp_trigger_func fun;
	bool ret;
	unsigned long check_bit = 0;

	printf("***** Using MOV instruction to set DR7[any bit 32:63]to 1 *****\n");

	check_bit |= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_32);

	fun = (gp_trigger_func)write_dr7_checking;
	ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

	/* Expected write #GP exception */
	report("#GP exception", ret == true);
}
#endif

/**
 *@Sub-Conditions:
 *      OF_Flag: 1
 *@test purpose:
 *      Set OF flag to 1
 *@Design Steps:
 *      Set EFLAGE.OF to 1
 *@Expected Result:
 *      N/A
 */
bool of_flag_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	printf("***** Set EFLAGE.OF to 1 ******\n");

	check_bit = read_rflags();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_11));

	write_rflags(check_bit);

	if (check_bit == read_rflags()) {
		result = true;
	} else {
		result = false;
	}

	return result;
}
/* ---------------------- GP END---------------------- */

/*---------------- ring1 ring2 ring3 -------------------*/
int do_at_ring1(void (*fn)(void), const char *arg)
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
		"kernel_entry1: \n\t"
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
		: [user_ds] "i" (OSSERVISE1_DS),
		[user_cs] "i" (OSSERVISE1_CS32),
		[user_stack_top]"m"(user_stack[sizeof user_stack]),
		[fn]"r"(fn),
		[arg]"D"(arg),
		[kernel_ds]"i"(KERNEL_DS),
		[kernel_entry_vector]"i"(0x21)
		: "rcx", "rdx");
	return ret;
}

int do_at_ring2(void (*fn)(void), const char *arg)
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
		"kernel_entry2: \n\t"
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
		: [user_ds] "i" (OSSERVISE2_DS),
		[user_cs] "i" (OSSERVISE2_CS32),
		[user_stack_top]"m"(user_stack[sizeof user_stack]),
		[fn]"r"(fn),
		[arg]"D"(arg),
		[kernel_ds]"i"(KERNEL_DS),
		[kernel_entry_vector]"i"(0x22)
		: "rcx", "rdx");
	return ret;
}

__unused void config_gdt_description(u32 index, u8 dpl, u8 is_code)
{
	__unused u32 base;
	__unused u8 access, gran;
	__unused short selector;
	__unused u32 functtion;
	u8 flag = 0;

	gdt_entry_t   *oldpgdt, *pnewgdt;

	struct descriptor_table_ptr gdt_descriptor_table;

	if ((index == 0) || (index >= 15)) {
		return;
	}

	sgdt(&gdt_descriptor_table);

	oldpgdt = (gdt_entry_t *)gdt_descriptor_table.base;
	pnewgdt = &oldpgdt[index];

	pnewgdt->limit_low = SEGMENT_LIMIT;
	pnewgdt->base_low = 0x0000;

	if (is_code) {
		flag = SEGMENT_TYPE_CODE_EXE_RAED_ACCESSED;
	}
	else {
		flag = SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED;
	}
	pnewgdt->access =  SEGMENT_PRESENT_SET|(dpl&0x60)|DESCRIPTOR_TYPE_CODE_OR_DATA|(flag&0xF);
	pnewgdt->base_high = 0x00;
	pnewgdt->base_middle = 0x00;
	if (is_code) {
		pnewgdt->granularity = GRANULARITY_SET|L_64_BIT_CODE_SEGMENT|0xF;
	}
	else {
		pnewgdt->granularity = GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT|0xF;
	}
	//printf("*******index=%d, table=%lx\n", index, *(u64 *)pnewgdt);
	lgdt(&gdt_descriptor_table);
}

__unused void init_gdt_description(void)
{
	u32 index;
	u8 dpl;

	index = 11; // 32-bit code segment [OS services] ,
	dpl = DPLEVEL1;
	config_gdt_description(index, dpl, 1);

	index = 12; // 32-bit data segment [OS services] ,
	dpl = DPLEVEL1;
	config_gdt_description(index, dpl, 0);

	index = 13; // 32-bit code segment [OS services] ,
	dpl = DPLEVEL2;
	config_gdt_description(index, dpl, 1);

	index = 14; // 32-bit data segment [OS services] ,
	dpl = DPLEVEL2;
	config_gdt_description(index, dpl, 0);
}

int do_at_ring3(void (*fn)(void), const char *arg)
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

/*------------ring1 ring2 ring3 end----------------*/

/**CPUID Function:**/
uint64_t get_supported_xcr0(void)
{
	struct cpuid r;
	r = cpuid_indexed(0xd, 0);
	printf("eax 0x%x, ebx 0x%x, ecx 0x%x, edx 0x%x\n",
	r.a, r.b, r.c, r.d);
	return r.a + ((u64)r.d << 32);
}

int check_cpuid_1_ecx(unsigned int bit)
{
	return (cpuid(1).c & bit) != 0;
}

/**Function about Control register and EFLAG register**/
void set_cr0_AM(int am)
{
	unsigned long cr0 = read_cr0();
	unsigned long old_cr0 = cr0;

	cr0 &= ~CR0_AM_MASK;
	if (am) {
		cr0 |= CR0_AM_MASK;
	}
	if (old_cr0 != cr0) {
		write_cr0(cr0);
	}
}

void set_eflag_ac(int ac)
{
	/* set EFLAGS.AC */
	//asm volatile("orl $" xstr(X86_EFLAGS_AC) ", 2*"S"(%"R "sp)\n" );
	stac();
}

/**Some common function **/
#ifdef __x86_64__
uint64_t get_random_value(void)
{
	uint64_t random = 0UL;

	asm volatile ("1: rdrand %%rax\n"
		"jnc 1b\n"
		"mov %%rax, %0\n"
		: "=r"(random)
		:
		: "%rax");
	return random;
}
#else
uint64_t get_random_value(void)
{
	u32 random = 0UL;

	asm volatile ("1: rdrand %%eax\n"
		"jnc 1b\n"
		"mov %%eax, %0\n"
		: "=r"(random)
		:
		: "%eax");
	return random;
}
#endif

__unused static struct descriptor_table stor_idt(void)
{
	struct descriptor_table idtb = {0U, 0UL};
	asm volatile ("sidt %0":"=m"(idtb)::"memory");
	return idtb;
}

__unused static void set_idt(struct descriptor_table idtd)
{
	asm volatile ("lidt %[idtd]\n"
		: : [idtd] "m"(idtd));
}

static void de_exception_hander(struct ex_regs *regs)
{
	de_ocurred = true;
	printf("#DE exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void db_exception_hander(struct ex_regs *regs)
{
	db_ocurred = true;
	printf("#DB exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void nmi_exception_hander(struct ex_regs *regs)
{
	nmi_ocurred = true;
	printf("#NMI exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void bp_exception_hander(struct ex_regs *regs)
{
	bp_ocurred = true;
	printf("#BP exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void of_exception_hander(struct ex_regs *regs)
{
	of_ocurred = true;
	printf("#OF exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void br_exception_hander(struct ex_regs *regs)
{
	br_ocurred = true;
	printf("#BR exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void ud_exception_hander(struct ex_regs *regs)
{
	ud_ocurred = true;
	printf("#UD exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void nm_exception_hander(struct ex_regs *regs)
{
	nm_ocurred = true;
	printf("#NM exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void df_exception_hander(struct ex_regs *regs)
{
	df_ocurred = true;
	printf("#DF exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void ts_exception_hander(struct ex_regs *regs)
{
	ts_ocurred = true;
	printf("#TS exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void np_exception_hander(struct ex_regs *regs)
{
	np_ocurred = true;
	printf("#NP exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void ss_exception_hander(struct ex_regs *regs)
{
	ss_ocurred = true;
	printf("#SS exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void gp_exception_hander(struct ex_regs *regs)
{
	gp_ocurred = true;
	printf("#GP exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void pf_exception_hander(struct ex_regs *regs)
{
	pf_ocurred = true;
	printf("#PF exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void mf_exception_hander(struct ex_regs *regs)
{
	mf_ocurred = true;
	printf("#MF exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void mc_exception_hander(struct ex_regs *regs)
{
	mc_ocurred = true;
	printf("#MC exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}

static void xm_exception_hander(struct ex_regs *regs)
{
	xm_ocurred = true;
	printf("#XM exception:\n");
	printf("     rip:  0x%08lx\n", regs->rip);
	printf("err code:  0x%08lx\n", regs->error_code);
	regs->rip += instruction_len;
}
