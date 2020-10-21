
asm(".code16gcc");
#include "rmode_lib.h"
#define NO_EXCEPTION	0x0

typedef unsigned long long u64;

struct gdt_table_descr {
	u16 limit;
	void *base;
} __attribute__((packed));

static u8 lgdt_real_checking(const struct gdt_table_descr *ptr)
{
	asm volatile(ASM_TRY("1f"));
	asm volatile(
		"lgdt %0\n"
		"1:"
		: : "m" (*ptr) :
	);
	return exception_vector();
}

static u8 sgdt_real_checking(struct gdt_table_descr *ptr)
{
	asm volatile(ASM_TRY("1f"));
	asm volatile(
		"sgdt %0\n"
		"1:"
		: "=m" (*ptr) : : "memory"
	);
	return exception_vector();
}

static u64 gdt32[] = {
	0,
	0x00cf9b000000ffffull, // flat 32-bit code segment
	0x00cf93000000ffffull, // flat 32-bit data segment
};

static struct gdt_table_descr gdt_descr_test = {
	sizeof(gdt32) - 1,
	gdt32,
};

/**
 * @brief case name:HSI_Memory_Management_Features_Segmentation_Real_Mode_002
 *
 * Summary: Under real mode on native board, create a new gdt table,
 * execute LGDT instruction to install new GDT and execute SGDT instruction to save GDT to memory.
 * Verify that instructions execution with no exception and install/save operation is successful.
 */
static void hsi_rqmid_40053_memory_management_features_segmentation_real_mode_002()
{
	struct gdt_table_descr gdt_desc_save;
	u8 ret1 = 1;
	u8 ret2 = 1;
	u32 chk = 0;

	ret1 = lgdt_real_checking(&gdt_descr_test);
	if (ret1 == NO_EXCEPTION) {
		chk++;
	}

	ret2 = sgdt_real_checking(&gdt_desc_save);
	if (ret2 == NO_EXCEPTION) {
		chk++;
	}

	if ((gdt_desc_save.base == gdt_descr_test.base) &&
		(gdt_desc_save.limit == gdt_descr_test.limit)) {
		chk++;
	}

	print_serial("sgdt ret:");
	print_serial_u32(ret1);
	print_serial("\r\n");

	print_serial("lgdt ret:");
	print_serial_u32(ret2);
	print_serial("\r\n");

	report("hsi_rqmid_40053_memory_management_features_segmentation_real_mode_002", (chk == 3));
	print_serial("\r\n");

}

void main()
{
	hsi_rqmid_40053_memory_management_features_segmentation_real_mode_002();
	report_summary();
	print_serial("\r\n");
}

