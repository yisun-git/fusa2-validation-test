#define LAR_LOW_8_BIT_VALUE 0

static int lsl_checking(volatile u32 *limit, u16 sel)
{
	asm volatile(ASM_TRY("1f")
		"lsl %1, %0\n"
		"1:"
		: "+r"(*limit) : "m"(sel) : "memory"
	);
	return exception_vector();
}

static int lar_checking(volatile u32 *access, u16 sel)
{
	asm volatile(ASM_TRY("1f")
		"lar %1, %0\n"
		"1:"
		: "+r"(*access) : "m"(sel) : "memory"
	);
	return exception_vector();
}

/**
 * @brief case name:HSI_Memory_Management_Features_Segmentation_32bit_Mode_002
 *
 * Summary: Under 32 bit mode on native board, create a new gdt table,
 * execute LGDT instruction to install new GDT and execute SGDT instruction to save GDT to memory.
 * Verify that instructions execution with no exception and install/save operation is successful.
 */
__unused static void hsi_rqmid_40035_memory_management_features_segmentation_32bit_mode_002()
{
	segment_lgdt_sgdt_test();
}

/**
 * @brief case name:HSI_Memory_Management_Features_Segmentation_32bit_mode_003
 *
 * Summary: Under 32 bit mode on native board, create a new gdt table,
 * set IDT entry in GDT table, Execute LLDT instruction to load new LDT and
 * execute SLDT intructions to save LDT to memory.
 * Verify that instructions execution with no exception and install/save operation is successful.
 */
static __unused void hsi_rqmid_40036_memory_management_features_segmentation_32bit_mode_003()
{
	segment_lldt_sldt_test();
}

/**
 * @brief case name:HSI_Memory_Management_Features_Segmentation_32bit_Mode_004
 *
 * Summary: Under 32 bit mode on native board, execute LAR/LSL instructions to load
 * the access rights and segment limit from a data segment descriptor.
 * Verify the load operation is successful.
 */
static __unused void hsi_rqmid_40037_memory_management_features_segmentation_32bit_mode_004()
{
	int ret1 = 1;
	int ret2 = 1;
	u32 chk = 0;
	struct descriptor_table_ptr gdt32;

	u32 limit = 0;
	u32 access = 0;
	ret1 = lar_checking(&access, KERNEL_DS);
	if (ret1 == PASS) {
		chk++;
	}

	ret2 = lsl_checking(&limit, KERNEL_DS);
	if (ret2 == PASS) {
		chk++;
	}

	/* get ds segmentation descriptor */
	sgdt(&gdt32);
	gdt_entry_t *gdt_entry = (gdt_entry_t *)gdt32.base;
	gdt_entry_t kernel_ds_entry = gdt_entry[KERNEL_DS >> 3];
	/* as SDM said:the following fields are loaded by the LAR instruction:
	 * • Bits 7:0 are returned as 0
	 * • Bits 11:8 return the segment type.
	 * • Bit 12 returns the S flag.
	 * • Bits 14:13 return the DPL.
	 * • Bit 15 returns the P flag.
	 */
	if (((access & 0xff) == LAR_LOW_8_BIT_VALUE) &&
		(((access >> 8) & 0xff) == kernel_ds_entry.access)) {
		chk++;
	}

	/* as SMD said: The segment limit is a 20-bit value
	 * contained in bytes 0 and 1 and in the first 4 bits of byte 6
	 *of the segment descriptor.
	 */
	if (((limit & 0xffff) == kernel_ds_entry.limit_low) &&
		(((limit >> 16) & 0xf) == (kernel_ds_entry.granularity & 0xf))) {
		chk++;
	}
	debug_print("ds_entry:%llx\n", *((u64 *)&kernel_ds_entry));
	debug_print("lar ret1:%d lsl ret2:%d limit:0x%x access:0x%x\n", ret1, ret2, limit, access);
	report("\t\t %s", (chk == 4), __FUNCTION__);
}

st_case_suit case_suit[] = {
	{
		.case_fun = hsi_rqmid_40035_memory_management_features_segmentation_32bit_mode_002,
		.case_id = 40035,
		.case_name = GET_CASE_NAME(hsi_rqmid_40035_memory_management_features_segmentation_32bit_mode_002),
	},

	{
		.case_fun = hsi_rqmid_40036_memory_management_features_segmentation_32bit_mode_003,
		.case_id = 40036,
		.case_name = GET_CASE_NAME(hsi_rqmid_40036_memory_management_features_segmentation_32bit_mode_003),
	},

	{
		.case_fun = hsi_rqmid_40037_memory_management_features_segmentation_32bit_mode_004,
		.case_id = 40037,
		.case_name = GET_CASE_NAME(hsi_rqmid_40037_memory_management_features_segmentation_32bit_mode_004),
	},
};

