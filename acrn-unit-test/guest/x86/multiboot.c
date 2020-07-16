/*
 * Test for multiboot
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */
#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "alloc.h"
#include "alloc_phys.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "asm/spinlock.h"
#include "debug_print.h"

#define MULTIBOOT_COMMAND_MAX       1024
#define RSDP_ADDRESS                0xF2400
#define ZERO_PAGE_BASE_ADDRESS      0xFFF000

#define __packed        __attribute__((packed))

/** Defines a single entry in an E820 memory map. */
struct e820_entry {
	/** The base address of the memory range. */
	uint64_t baseaddr;
	/** The length of the memory range. */
	uint64_t length;
	/** The type of memory region. */
	uint32_t type;
}  __attribute__((packed));

/* The real mode kernel header, refer to Documentation/x86/boot.txt */
struct _zeropage {
	uint8_t pad1[0x1e8];                    /* 0x000 */
	uint8_t e820_nentries;                  /* 0x1e8 */
	uint8_t pad2[0x8];                      /* 0x1e9 */

	struct	{
		uint8_t hdr_pad1[0x1f];         /* 0x1f1 */
		uint8_t loader_type;            /* 0x210 */
		uint8_t load_flags;             /* 0x211 */
		uint8_t hdr_pad2[0x2];          /* 0x212 */
		uint32_t code32_start;          /* 0x214 */
		uint32_t ramdisk_addr;          /* 0x218 */
		uint32_t ramdisk_size;          /* 0x21c */
		uint8_t hdr_pad3[0x8];          /* 0x220 */
		uint32_t bootargs_addr;         /* 0x228 */
		uint8_t hdr_pad4[0x3c];         /* 0x22c */
	} __attribute__((packed)) hdr;

	uint8_t pad3[0x68];                     /* 0x268 */
	struct e820_entry e820[0x80];           /* 0x2d0 */
	uint8_t pad4[0x330];                    /* 0xcd0 */
} __attribute__((packed));

#define ACPI_SIG_RSDP "RSD PTR " /* Root System Description Ptr */
#define ACPI_SIG_XSDT "XSDT" /* Extended  System Description Table */
#define ACPI_SIG_MADT "APIC" /* Multiple APIC Description Table */
#define RSDP_SIGNATURE_LEN	8
#define ACPI_OEM_ID_SIZE 6U
#define CONFIG_MAX_PCPU_NUM 4U
struct acpi_table_rsdp {
	/* ACPI signature, contains "RSD PTR " */
	char signature[RSDP_SIGNATURE_LEN];
	/* ACPI 1.0 checksum */
	uint8_t checksum;
	/* OEM identification */
	char oem_id[ACPI_OEM_ID_SIZE];
	/* Must be (0) for ACPI 1.0 or (2) for ACPI 2.0+ */
	uint8_t revision;
	/* 32-bit physical address of the RSDT */
	uint32_t rsdt_physical_address;
	/* Table length in bytes, including header (ACPI 2.0+) */
	uint32_t length;
	/* 64-bit physical address of the XSDT (ACPI 2.0+) */
	uint64_t xsdt_physical_address;
	/* Checksum of entire table (ACPI 2.0+) */
	uint8_t extended_checksum;
	/* Reserved, must be zero */
	uint8_t reserved[3];
} __attribute__((packed));

struct acpi_table_header {
	/* ASCII table signature */
	char signature[4];
	/* Length of table in bytes, including this header */
	uint32_t length;
	/* ACPI Specification minor version number */
	uint8_t revision;
	/* To make sum of entire table == 0 */
	uint8_t checksum;
	/* ASCII OEM identification */
	char oem_id[6];
	/* ASCII OEM table identification */
	char oem_table_id[8];
	/* OEM revision number */
	uint32_t oem_revision;
#if 1
	/* ASCII ASL compiler vendor ID */
	char asl_compiler_id[4];
	/* ASL compiler version */
	uint32_t asl_compiler_revision;
#else
	/* Vendor ID of utility that created the table */
	char creator_id[4];
	/* Revision of utility that created the table */
	uint32_t creator_revision;
#endif
} __attribute__((packed));

struct acpi_table_xsdt {
	/* Common ACPI table header */
	struct acpi_table_header header;
	/* Array of pointers to ACPI tables */
	uint64_t table_offset_entry[1];
} __attribute__((packed));

struct acpi_table_madt {
	/* Common ACPI table header */
	struct acpi_table_header header;
	/* Physical address of local APIC */
	uint32_t address;
	uint32_t flags;
} __attribute__((packed));

struct acpi_subtable_header {
	uint8_t type;
	uint8_t length;
} __attribute__((packed));

struct acpi_madt_local_apic_nmi {
	struct acpi_subtable_header header;
	uint8_t processor_id;
	uint16_t flags;
	uint8_t lint;
} __attribute__((packed));

struct acpi_madt_local_apic {
	struct acpi_subtable_header header;
	/* ACPI processor id */
	uint8_t processor_id;
	/* Processor's local APIC id */
	uint8_t id;
	uint32_t lapic_flags;
} __attribute__((packed));

struct acpi_table_info {
	struct acpi_table_rsdp rsdp;
	struct acpi_table_xsdt xsdt;

	struct {
		struct acpi_table_madt madt;
		struct acpi_madt_local_apic_nmi lapic_nmi;
		struct acpi_madt_local_apic lapic_array[CONFIG_MAX_PCPU_NUM];
	} __attribute__((packed));
};

int zero_field_zero_page(unsigned char *point)
{
	int i;
	printf("0x1E9:");
	for (i = 0x1E9; i <= 0x1F0; i++) {
		printf("%x ", point[i]);
	}
	printf("\n");

	for (i = 0x268; i < 0x2CF; i++) {
		if (((i - 0x268) % 16) == 0) {
			printf("\n 0x%x:", i);
		}
		printf("%x ", point[i]);
	}

	return 0;
}

struct _zeropage *get_zero_page(u32 address)
{
	struct _zeropage *zeropage = NULL;
	volatile u32 *ptr;
	volatile u32 bp_esi = 0;

	if (address == 0) {
		ptr = (volatile u32 *)0x6000;
		bp_esi = *ptr;
	} else {
		bp_esi = address;
	}

	debug_print("bp_esi=%x\n", bp_esi);
	if (bp_esi != 0) {
		zeropage = (struct _zeropage *)((intptr_t)bp_esi);
	}
	return zeropage;
}

int test_fun(int x)
{
	return x+1;
}

typedef int (*test_fun_p)(int);

int read_write_exec_test(unsigned long start, unsigned long end, int is_exec)
{
	unsigned long len;
	unsigned char *point;
	int i;
	int ret;
	unsigned char value = 0xFF;
	test_fun_p fun_p;

	debug_print("test start:%lx, end:%lx\n", start, end);
	len = end-start;
	point = (unsigned char *) start;
	fun_p = test_fun;

	for (i = 0; i < len; i += 4096) {
		/*test read*/
		value = point[i];
		if ((point[i] != 0xFF) && (value != point[i])) {
			printf("read error\n");
			return -1;
		}

		/*test write*/
		point[i] = 0xFE;
		if (point[i] != 0xFE) {
			printf("write test error\n");
			return -2;
		}

		if (is_exec) {
			memcpy(&point[i], fun_p, 9);
			fun_p = (test_fun_p)&point[i];
			ret = fun_p(i);
			if (ret != (i+1)) {
				debug_print("exec error ret=%d, i=%d\n", ret, i);
				return -3;
			}
		}
	}
	return 0;
}

int read_test_value(unsigned long start, unsigned long end, u64 value, int size)
{
	unsigned long len;
	unsigned char *point;
	int ret = 0;
	int i;
	int flag = 1;
	printf("test start:%lx, end:%lx\n", start, end);

	//start = start - 0xc00800;
	//end = end - 0xc00800;
	len = (end-start+1);
	point = (unsigned char *) start;

	for (i = 0; i < len; i += size) {
		switch (size) {
		case 1:
			if (value != (u64)point[i]) {
				ret = 1;
				if (flag == 1) {
					flag = 0;
					printf("value=0x%lx, 0x%lx\n", value, (u64)point[i]);
				}
			}
			break;
		case 2:
			if (value != (u64)*(u16 *)&point[i]) {
				ret = 1;
				if (flag == 1) {
					flag = 0;
					printf("value=0x%lx, 0x%lx\n", value, (u64)point[i]);
				}
			}
			break;
		case 4:
			if (value != (u64)*(u32 *)&point[i]) {
				ret = 1;
				if (flag == 1) {
					flag = 0;
					printf("value=0x%lx, 0x%lx\n", value, (u64)point[i]);
				}
			}
			break;
		case 8:
			if (value != *(u64 *)&point[i]) {
				ret = 1;
				if (flag == 1) {
					flag = 0;
					printf("value=0x%lx, 0x%lx\n", value, (u64)point[i]);
				}
			}
			break;
		default:
			printf("len 0x%lx is error\n", len);
			ret = 1;
			break;
		}
	}
	printf("start:0x%lx, end:0x%lx, len:0x%lx test %s\n",
		start, end, len, (ret == 0) ? "ok":"error");
	return 0;
}

unsigned long long asm_read_tsc(void)
{
	long long r;
#ifdef __x86_64__
	unsigned a, d;

	asm volatile("mfence" ::: "memory");
	asm volatile ("rdtsc" : "=a"(a), "=d"(d));
	r = a | ((long long)d << 32);
#else
	asm volatile ("rdtsc" : "=A"(r));
#endif
	asm volatile("mfence" ::: "memory");
	return r;
}

void delay(u64 count)
{
	u64 t[2];
	u64 tsc_delay = count * 2100000000UL;

	t[0] = asm_read_tsc();
	while (1) {
		t[1] = asm_read_tsc();
		if ((t[1] - t[0]) > tsc_delay) {
			break;
		}
	}
}

void multiboot_rqmid_27210_guest_esi_of_non_safety_001(void)
{
	bool ret = false;
	unsigned char *ptr = NULL;

	ptr = (unsigned char *)get_zero_page(0);
	if (ptr != NULL) {
		if ((ptr[0x1FE] == 0x55) && (ptr[0x1FF] == 0xAA) && (ptr[0x202] == 'H')
			&& (ptr[0x203] == 'd') && (ptr[0x204] == 'r') && (ptr[0x205] == 'S')) {
			ret = true;
		}
	}

	report("%s", ret == true, __FUNCTION__);
}

void multiboot_rqmid_27236_non_safety_vm_command_line_001(void)
{
	bool ret = false;
	unsigned char *ptr;
	int len = 0;
	struct _zeropage *zeropage = NULL;

	zeropage = get_zero_page(0);
	ptr = (unsigned char *)zeropage;

	if ((u64)&(zeropage->hdr.bootargs_addr) == (u64)&(ptr[0x228])) {
		ptr = (unsigned char *)(u64)(zeropage->hdr.bootargs_addr);
		len = strlen((char *)ptr);
		if ((len > 0) && (len < MULTIBOOT_COMMAND_MAX)) {
			ret = true;
		}
	}

	report("%s", ret == true, __FUNCTION__);
}

void multiboot_rqmid_34091_expose_rsdp(void)
{
	bool ret = true;
	unsigned char sum = 0;
	int index = 0;
	u32 *plen = NULL;
	unsigned char *point = (unsigned char *) RSDP_ADDRESS;

	/* Verify the signature of RSDP */
	if (memcmp(&point[index], ACPI_SIG_RSDP, RSDP_SIGNATURE_LEN)) {
		debug_print("\n");
		ret = false;
	}

	/* Verify the Checksum of RSDP */
	for (index = 0; index < 20; index++) {
		sum += point[index];
	}
	if (sum != 0) {
		debug_print("%d\n", sum);
		ret = false;
	}

	/* verify the length of RSDP  address is 000F2414H */
	plen = (u32 *)&point[0x14];
	if (*plen != 0x24) {
		debug_print("\n");
		ret = false;
	}

	/* Verify the Extended checksum of RSDP */
	for (index = 0; index < 33; index++) {
		sum += point[index];
	}
	if (sum != 0) {
		debug_print("sum=%d\n", sum);
		ret = false;
	}

	/* Verify the Reserved of RSDP  address is 000F2421*/
	for (index = 0; index < 3; index++) {
		if (point[index+0x21] != 0) {
			debug_print("\n");
			ret = false;
		}
	}

	report("%s", ret == true, __FUNCTION__);
}

static void multiboot_rqmid_34118_low_memory_region_rights_001()
{
	int ret = 0;

	ret = read_write_exec_test(0x0, 0xEFFFF, 1);

	report("%s", ret == 0, __FUNCTION__);
}

void multiboot_rqmid_34130_base_address_of_zero_page_001()
{
	bool ret = false;
	unsigned char *ptr = NULL;

	ptr = (unsigned char *)get_zero_page(ZERO_PAGE_BASE_ADDRESS);
	if (ptr != NULL) {
		if ((ptr[0x1FE] == 0x55) && (ptr[0x1FF] == 0xAA) && (ptr[0x202] == 'H')
			&& (ptr[0x203] == 'd') && (ptr[0x204] == 'r') && (ptr[0x205] == 'S')) {
			ret = true;
		}
	}

	report("%s", ret == true, __FUNCTION__);
}


static void print_case_list(void)
{
	printf("\t\t multiboot feature case list:\n\r");
	printf("\t\t multiboot 64-Bits Mode:\n\r");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 27210u, "multiboot_rqmid_27210_guest_esi_of_non_safety_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27236u, "multiboot_rqmid_27236_non_safety_vm_command_line_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34091u, "multiboot_rqmid_34091_expose_rsdp");
	printf("\t\t Case ID:%d case name:%s\n\r", 34130u, "multiboot_rqmid_34130_base_address_of_zero_page_001");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 34118u, "multiboot_rqmid_low_memory_region_rights_001");
}

static void test_multiboot_list(void)
{
#ifdef IN_NON_SAFETY_VM
	multiboot_rqmid_27210_guest_esi_of_non_safety_001();
	multiboot_rqmid_27236_non_safety_vm_command_line_001();
	multiboot_rqmid_34091_expose_rsdp();
	multiboot_rqmid_34130_base_address_of_zero_page_001();
#endif
	multiboot_rqmid_34118_low_memory_region_rights_001();
}

int main(int ac, char **av)
{
	print_case_list();
	test_multiboot_list();
	return report_summary();
}

void ap_main(void)
{
	return;
}

