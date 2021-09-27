/**
 * Test for x86 cache and memory cache control
 *
 * 32-bit mode
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */
#include "segmentation.h"

/**
 * @brief Construct non canonical address
 *
 * Set the 63rd bit of a variable for the SS segment address to 1
 * Execute CLFLUSH will generate #SS(0).
 */
#define FIRST_SPARE_SEL 0x50
void setup_invalid_fs(void)
{
	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);
	set_gdt_entry(FIRST_SPARE_SEL, 0, SEGMENT_LIMIT2,
		SEGMENT_PRESENT_SET | DESCRIPTOR_TYPE_SYS | DESCRIPTOR_PRIVILEGE_LEVEL_0 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET);
	lgdt(&old_gdt_desc);
	write_fs(FIRST_SPARE_SEL);
}

static char stack_base[4096] __attribute__((aligned(4096)));
void setup_invalid_ss(void)
{
	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);
	//set stack limit, as 1 byte unit, not 4KB unit
	set_gdt_entry(FIRST_SPARE_SEL, (u32)stack_base, 4096,
		SEGMENT_PRESENT_SET | DESCRIPTOR_TYPE_SYS | DESCRIPTOR_PRIVILEGE_LEVEL_0 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);
	write_ss(FIRST_SPARE_SEL);
}

void clflush_invalid_fs(void)
{
	setup_invalid_fs();
	asm volatile("clflush %fs:0x100000");
}

void clflushopt_invalid_fs(void)
{
	setup_invalid_fs();
	asm volatile("clflushopt %fs:0x100000");
}

void clflush_invalid_ss(void)
{
	setup_invalid_ss();
	asm volatile("clflush %ss:0x100000");
}

void clflushopt_invalid_ss(void)
{
	setup_invalid_ss();
	asm volatile("clflushopt %ss:0x100000");
}

/**
 * @brief case name:Cache control CLFLUSH instruction exception_004
 *
 * Summary: In protected mode or 64bit mode, for an illegal memory operand effective address
 * in the CS, DS, ES, FS, or GS segments (select one segmentation register as the target test register),
 * execute clflush will generate GP(0).
 */
void cache_rqmid_27077_clflush_exception_004(void)
{
	bool ret = test_for_exception(GP_VECTOR, (trigger_func)clflush_invalid_fs, NULL);
	write_fs(read_ds());
	report("%s()", ret, __func__);
}

/**
 * @brief case name:Cache control CLFLUSH instruction exception_004
 *
 * Summary: In 64bit mode, execute clflush with the lock prefix generates #UD.
 */
void cache_rqmid_27079_clflush_exception_006(void)
{
	report("%s", false, __func__); // crash
	return;
	bool ret = test_for_exception(GP_VECTOR, (trigger_func)clflush_invalid_ss, NULL);
	write_ss(read_ds());
	report("%s()", ret, __func__);
}


/**
 * @brief case name:Cache control CLFLUSH instruction exception_007
 *
 * Summary: In protect mode, execute clflush for an not present
 * address generate PF.
 */
void cache_rqmid_27080_clflush_exception_007(void)
{
	u8 *gva = malloc(sizeof(u8));
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, true); /*not present*/
	bool ret = test_for_exception(PF_VECTOR, (trigger_func)asm_clflush, (void *)gva);
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 1, true); /*reset present*/
	free(gva);
	report("%s()", ret, __func__);
}

/**
 * @brief case name:Cache control CLFLUSH instruction exception_008
 *
 * Summary: In protect mode, execute clflush with the lock prefix generates #UD.
 */
void cache_rqmid_27081_clflush_exception_008(void)
{
	bool ret = test_for_exception(UD_VECTOR, (trigger_func)asm_clflush_lock, (void *)cache_test_array);
	report("%s()", ret, __func__);
}

/**
 * @brief case name:Cache control CLFLUSHOPT instruction exception_003
 *
 * Summary: In 64bit mode, execute clflushopt for an not present address generate PF.
 */
void cache_rqmid_27087_clflushopt_exception_007(void)
{
	report("%s", false, __func__); // crash
	return;
	bool ret = test_for_exception(GP_VECTOR, (trigger_func)clflushopt_invalid_ss, NULL);
	write_ss(read_ds());
	report("%s()", ret, __func__);
}

/**
 * @brief case name:Cache control CLFLUSHOPT instruction exception_008
 *
 * Summary: In protect mode, execute clflushopt for an not present
 * address generate PF.
 */
void cache_rqmid_27088_clflushopt_exception_008(void)
{
	u8 *gva = malloc(sizeof(u8));
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, true); /*not present*/
	bool ret = test_for_exception(PF_VECTOR, (trigger_func)asm_clflush, (void *)gva);
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 1, true); /*reset present*/
	free(gva);
	report("%s()", ret, __func__);
}

/**
 * @brief case name:Cache control CLFLUSHOPT instruction exception_009
 *
 * Summary: In protect mode, execute clflushopt with the lock F2 prefix generates #UD.
 */
void cache_rqmid_27089_clflushopt_exception_009(void)
{
	bool ret = test_for_exception(UD_VECTOR, (trigger_func)asm_clflushopt_lock_f2, (void *)&cache_test_array);
	report("%s()", ret, __func__);
}

/**
 * @brief case name: Cache control CLFLUSHOPT instruction exception_006
 *
 * Summary: In protected mode or 64bit mode, for an illegal memory operand effective address
 * in the CS, DS, ES, FS, or GS segments (select one segmentation register as the target test register),
 * execute clflushopt will generate GP(0).
 */
void cache_rqmid_27096_clflushopt_exception_006(void)
{
	bool ret = test_for_exception(GP_VECTOR, (trigger_func)clflushopt_invalid_fs, NULL);
	write_fs(read_ds());
	report("%s()", ret, __func__);
}

/**
 * @brief case name:Cache control CLFLUSHOPT instruction exception_015
 *
 * Summary: In protect mode, execute clflushopt with the lock F0 prefix generates #UD.
 */
void cache_rqmid_27467_clflushopt_exception_015(void)
{
	bool ret = test_for_exception(UD_VECTOR, (trigger_func)asm_clflushopt_lock, (void *)&cache_test_array);
	report("%s()", ret, __func__);
}

static struct case_fun_index cache_control_cases[] = {
	{27077, cache_rqmid_27077_clflush_exception_004},
	{27079, cache_rqmid_27079_clflush_exception_006}, //todo
	{27080, cache_rqmid_27080_clflush_exception_007},
	{27081, cache_rqmid_27081_clflush_exception_008},
	{27087, cache_rqmid_27087_clflushopt_exception_007}, //todo
	{27088, cache_rqmid_27088_clflushopt_exception_008},
	{27089, cache_rqmid_27089_clflushopt_exception_009},
	{27096, cache_rqmid_27096_clflushopt_exception_006},
	{27467, cache_rqmid_27467_clflushopt_exception_015}
};

void cache_test_32(long rqmid)
{
	cache_fun_exec(cache_control_cases, sizeof(cache_control_cases)/sizeof(cache_control_cases[0]), rqmid);
}

