/*
 * Test for multiboot
 *
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
#include "multiboot.h"
#include "misc.h"
#include "fwcfg.h"
#include "delay.h"


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
	union {
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

		struct {
			uint8_t setup_sects; /* 0x1f1 */
			uint8_t hdr_pad1[0x14]; /* 0x1f2 */
			uint16_t version;	/* 0x206 */
			uint8_t hdr_pad8[0x8]; /* 0x208 */
			uint8_t loader_type; /* 0x210 */
			uint8_t load_flags; /* 0x211 */
			uint8_t hdr_pad2[0x6]; /* 0x212 */
			uint32_t ramdisk_addr; /* 0x218 */
			uint32_t ramdisk_size; /* 0x21c */
			uint8_t hdr_pad3[0x8]; /* 0x220 */
			uint32_t bootargs_addr; /* 0x228 */
			uint8_t hdr_pad4[0x8]; /* 0x22c */
			uint8_t relocatable_kernel; /* 0x234 */
			uint8_t hdr_pad5[0x13]; /* 0x235 */
			uint32_t payload_offset; /* 0x248 */
			uint32_t payload_length; /* 0x24c */
			uint8_t hdr_pad6[0x8]; /* 0x250 */
			uint64_t pref_addr; /* 0x258 */
			uint8_t hdr_pad7[8]; /* 0x260 */
		} __packed hdr1;
	};

	uint8_t pad3[0x68];                     /* 0x268 */
	struct e820_entry e820[0x80];           /* 0x2d0 */
	uint8_t pad4[0x330];                    /* 0xcd0 */
} __attribute__((packed));

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

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;
static volatile int esp = 0;

__unused void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}

__unused static void notify_modify_and_read_init_value(int case_id)
{
	cur_case_id = case_id;
	need_modify_init_value = 1;
	/* will change INIT value after AP reboot */
	send_sipi();
	wait_ap_ready();
	/* Will check INIT value after AP reboot again */
	send_sipi();
	wait_ap_ready();
}

#ifdef __i386__
void ap_main(void)
{
	asm volatile ("pause");
}

#elif __x86_64__

typedef void (*ap_init_value_modify)(void);
__unused static void ap_init_value_process(ap_init_value_modify modify_init_func)
{
	if (need_modify_init_value) {
		need_modify_init_value = 0;
		modify_init_func();
		wait_ap = 1;
	} else {
		wait_ap = 1;
	}
}

__unused static void modify_esi_init_value()
{
	asm volatile ("movl $0xFFFF, %esi\n");
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_cpu_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 34260:
		fp = modify_esi_init_value;
		ap_init_value_process(fp);
		break;

	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif

static int __unused zero_field_zero_page(unsigned char *point)
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

static struct _zeropage *get_zero_page(u32 address)
{
	struct _zeropage *zeropage = NULL;
	volatile u32 *ptr;
	volatile u32 bp_esi = 0;

	if (address == 0) {
		ptr = (volatile u32 *)MULTIBOOT_ESI_ADDR;
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

static int test_fun(int x)
{
	return x+1;
}

typedef int (*test_fun_p)(int);

static int read_write_exec_test(unsigned long start, unsigned long end, int is_exec)
{
	unsigned long len;
	unsigned char *point;
	int i;
	int ret;
	test_fun_p fun_p;

	debug_print("test start:%lx, end:%lx\n", start, end);
	len = end-start;
	point = (unsigned char *) start;
	fun_p = test_fun;

	for (i = 0; i < len; i += 4096) {
		/* test read and write*/
		point[i] = 0xFE;
		if (point[i] != 0xFE) {
			printf("write test error\n");
			return -1;
		}

		if (is_exec) {
			memcpy(&point[i], fun_p, 9);
			fun_p = (test_fun_p)&point[i];
			ret = fun_p(i);
			if (ret != (i+1)) {
				printf("exec error ret=%d, i=%d\n", ret, i);
				return -2;
			}
		}
	}
	return 0;
}

/**
 * check address area [start, end),
 * this address area value is equal to 'value',
 * the valid values of size are 1, 2, 4, 8.
 */
static bool read_test_value(unsigned long start, unsigned long end, u64 value, int size)
{
	unsigned long len;
	unsigned char *point;
	bool ret = true;
	int i;
	int flag = 1;
	debug_print("test start:%lx, end:%lx\n", start, end-1);

	len = end-start;
	point = (unsigned char *) start;

	for (i = 0; i < len; i += size) {
		switch (size) {
		case 1:
			if (value != (u64)point[i]) {
				ret = false;
				if (flag == 1) {
					flag = 0;
					printf("value=0x%lx, 0x%lx\n", value, (u64)point[i]);
				}
			}
			break;
		case 2:
			if (value != (u64)*(u16 *)&point[i]) {
				ret = false;
				if (flag == 1) {
					flag = 0;
					printf("value=0x%lx, 0x%lx\n", value, (u64)*(u16 *)&point[i]);
				}
			}
			break;
		case 4:
			if (value != (u64)*(u32 *)&point[i]) {
				ret = false;
				if (flag == 1) {
					flag = 0;
					printf("value=0x%lx, 0x%lx\n", value, (u64)*(u32 *)&point[i]);
				}
			}
			break;
		case 8:
			if (value != *(u64 *)&point[i]) {
				ret = false;
				if (flag == 1) {
					flag = 0;
					printf("value=0x%lx, 0x%lx\n", value, *(u64 *)&point[i]);
				}
			}
			break;
		default:
			printf("len 0x%lx is error\n", len);
			ret = false;
			break;
		}
	}
	debug_print("start:0x%lx, end:0x%lx, len:0x%lx test %s\n",
		start, end-1, len, (ret == true) ? "ok":"error");
	return ret;
}

/**
 * @brief case name:the guest ESI of vBSP of non-safety VM_001
 *
 * Summary: On 64 bit Mode, get the physical address of ZEROPAGE by the value of dump ESI,
 * if ZEROPAGE contains 55AAH magic number at 01FFH offset, and include at 0202H offset
 * Magic signature "HdrS" means ESI points to ZEROPAGE structure.
 */
static void __unused multiboot_rqmid_27210_guest_esi_of_non_safety_001(void)
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

/**
 * @brief case name:Non-safety VM command line_001
 *
 * Summary: On 64 bit Mode, get the physical address of the ZEROPAGE structure
 * via BP's ESI, get the value of the ZEROPAGE structure at 228H offset.
 */
static void __unused multiboot_rqmid_27236_non_safety_vm_command_line_001(void)
{
	bool ret = false;
	unsigned char *ptr;
	struct _zeropage *zeropage = NULL;

	zeropage = get_zero_page(0);
	ptr = (unsigned char *)zeropage;

	if ((u64)&(zeropage->hdr.bootargs_addr) == (u64)&(ptr[0x228])) {
		if (zeropage->hdr.bootargs_addr == 0xffe000) {
			ret = true;
		}
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:expose RSDP_001
 *
 * Summary: On 64 bit Mode, taking the physical address 000F2400H
 * as the base address, get the RSDP structure.
 */
static void __unused multiboot_rqmid_34091_expose_rsdp(void)
{
	bool ret = true;
	unsigned char sum = 0;
	int index = 0;
	u32 *plen = NULL;
	unsigned char *point = (unsigned char *) RSDP_ADDRESS;

	/* Verify the signature of RSDP */
	if (memcmp(&point[index], RSDP_SIG_RSDP, RSDP_SIGNATURE_LEN)) {
		printf("\n");
		ret = false;
	}

	/* Verify the Checksum of RSDP */
	for (index = 0; index < 20; index++) {
		sum += point[index];
	}
	if (sum != 0) {
		printf("%d\n", sum);
		ret = false;
	}

	/* verify the length of RSDP  address is 000F2414H */
	plen = (u32 *)&point[0x14];
	if (*plen != 0x24) {
		printf("\n");
		ret = false;
	}

	/* Verify the Extended checksum of RSDP */
	for (index = 0; index < 33; index++) {
		sum += point[index];
	}
	if (sum != 0) {
		printf("sum=%d\n", sum);
		ret = false;
	}

	/* Verify the Reserved of RSDP  address is 000F2421H*/
	for (index = 0; index < 3; index++) {
		if (point[index+0x21] != 0) {
			printf("\n");
			ret = false;
		}
	}

	report("%s", ret == true, __FUNCTION__);
}

static void low_memory_check(const char *fun_name)
{
	int ret = 0;
	unsigned long start;
	unsigned long end;

	start = 0x0;
	end = 0xEFFFF;
	ret = read_write_exec_test(start, end, 1);

	report("%s", ret == 0, fun_name);
}

/**
 * @brief case name:low memory region rights_001
 *
 * Summary: On 64 bit Mode, when testing memory region rights&map, select
 * a memory unit in each 4K page to test the read-write and executable.
 */
static void multiboot_rqmid_34118_low_memory_region_rights_001()
{
	low_memory_check(__FUNCTION__);
}

/**
 * @brief case name:base address of zero page_001
 *
 * Summary: On 64 bit Mode, get the physical address of ZEROPAGE by the value of dump ESI,
 * if ZEROPAGE contains 55AAH magic number at 01FFH offset, and include at 0202H
 * offset Magic signature "HdrS" means ESI points to ZEROPAGE structure.
 */
static void __unused multiboot_rqmid_34130_base_address_of_zero_page_001()
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

/**
 * @brief case name:type_of_loader field of setup header_001
 *
 * Summary: On 64 bit Mode, get the physical address of the ZEROPAGE structure via BP's ESI,
 * get the value of the ZEROPAGE structure at 210H offset, it shoule be FFH.
 */
static void __unused multiboot_rqmid_27249_type_of_loader_field_001(void)
{
	bool ret = false;
	unsigned char *ptr;
	struct _zeropage *zeropage = NULL;

	zeropage = get_zero_page(0);
	ptr = (unsigned char *)zeropage;

	if ((u64)&(zeropage->hdr.loader_type) == (u64)&(ptr[0x210])) {
		if (zeropage->hdr.loader_type == 0xFF) {
			ret = true;
		}
	}

	report("%s", ret == true, __FUNCTION__);
}


/**
 * @brief case name:bootprotocol version_001
 *
 * Summary: On 64 bit Mode, get the physical address of the ZEROPAGE structure via BP's ESI,
 * get 2 bytes of the ZEROPAGE structure at 206H offset, it shoule be 020CH.
 */
static void __unused multiboot_rqmid_39842_bootprotocol_version_field_001(void)
{
	bool ret = false;
	unsigned char *ptr;
	struct _zeropage *zeropage = NULL;

	zeropage = get_zero_page(0);
	ptr = (unsigned char *)zeropage;

	if ((u64)&(zeropage->hdr1.version) == (u64)&(ptr[0x206])) {
		if (zeropage->hdr1.version == 0x20C) {
			ret = true;
		}
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:Hypervisor shall provide e820 to non-safety vm_001
 *
 * Summary: On 64 bit Mode, get the physical address of the ZEROPAGE structure
 * via BP's ESI, the offset of E820 entry in zeropage is 2d0H.
 */
static void __unused multiboot_rqmid_27265_provide_e820_to_non_safety_001(void)
{
	bool ret = false;
	unsigned char *ptr;
	struct _zeropage *zeropage = NULL;

	zeropage = get_zero_page(0);
	ptr = (unsigned char *)zeropage;

	if ((u64)&(zeropage->e820) == (u64)&(ptr[0x2d0])) {
		ret = true;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:zero fields of zero page_001
 *
 * Summary: On 64 bit Mode, get all the values for other fields of the ZEROPAGE,
 * all values shall be 0H.
 */
static void __unused multiboot_rqmid_34243_zero_fields_of_zero_page_001(void)
{
	bool ret1 = true;
	bool ret2 = true;
	unsigned long start;
	int size;

	start = 0xFFF1E9;
	size = 8;
	ret1 = read_test_value(start, start + size, 0x0, size);

	start = 0xFFF268;
	size = 8;
	ret2 = read_test_value(start, start + size, 0x0, size);

	report("%s", ((ret1 == true) && (ret2 == true)), __FUNCTION__);
}

/**
 * @brief case name:e820_entries field_001
 *
 * Summary: On 64 bit Mode, gets the value of 1 byte at the
 * physical address FFF1E8H, the value is 3H.
 */
static void __unused multiboot_rqmid_34244_e820_entries_field_001(void)
{
	bool ret1 = true;
	unsigned long start;
	int size;

	start = 0xFFF1E8;
	size = 1;
	ret1 = read_test_value(start, start + size, 0x3, size);

	report("%s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:other fields of setup header_001
 *
 * Summary: On 64 bit Mode, get the corresponding values for non-safety
 * VM setup headers and kernel image, which should be equal.
 */
static void __unused multiboot_rqmid_34245_other_fields_of_setup_header_001(void)
{
	bool ret = true;
	struct _zeropage *zeropage = NULL;
	unsigned long start;
	int size;

	zeropage = get_zero_page(0);
	if (zeropage->hdr1.setup_sects != 0x2) {
		printf("setup_sects error 0x%x\n", zeropage->hdr1.setup_sects);
		ret = false;
	}
	if (zeropage->hdr1.ramdisk_addr != 0x0) {
		printf("ramdisk_addr error 0x%x\n", zeropage->hdr1.ramdisk_addr);
		ret = false;
	}
	if (zeropage->hdr1.ramdisk_size != 0x0) {
		printf("ramdisk_size error, 0x%x\n", zeropage->hdr1.ramdisk_size);
		ret = false;
	}
	if (zeropage->hdr1.relocatable_kernel != 0x1) {
		printf("relocatable_kernel error 0x%x\n", zeropage->hdr1.relocatable_kernel);
		ret = false;
	}
	if (zeropage->hdr1.payload_offset != 0x0) {
		printf("payload_offset error 0x%x\n", zeropage->hdr1.payload_offset);
		ret = false;
	}
	if (zeropage->hdr1.payload_length != 0x0) {
		printf("payload_length error 0x%x\n", zeropage->hdr1.payload_length);
		ret = false;
	}

	/*
	 *SSR-187079 The pref_address field of zero-page of non-safety VM kernel image shall be
	 *1000000H other than 0xFFF000
	 */
	if (zeropage->hdr1.pref_addr != 0x1000000) {
		printf("!pref_addr error 0x%lx\n", zeropage->hdr1.pref_addr);
		ret = false;
	}

	start = 0xFFF1F1;
	size = 1;
	if (read_test_value(start, start + size, 0x2, size) != true) {
		ret = false;
	}

	start = 0xFFF218;
	size = 4;
	if (read_test_value(start, start + size, 0x0, size) != true) {
		ret = false;
	}

	start = 0xFFF21C;
	size = 4;
	if (read_test_value(start, start + size, 0x0, size) != true) {
		ret = false;
	}

	start = 0xFFF234;
	size = 1;
	if (read_test_value(start, start + size, 0x1, size) != true) {
		ret = false;
	}

	start = 0xFFF248;
	size = 4;
	if (read_test_value(start, start + size, 0x0, size) != true) {
		ret = false;
	}

	start = 0xFFF24C;
	size = 4;
	if (read_test_value(start, start + size, 0x0, size) != true) {
		ret = false;
	}

	/*
	 *Keep the same value as the kernel image of the VM in the pref_addr field at address FFF258H
	 */
	start = 0xFFF258;
	size = 8;
	if (read_test_value(start, start + size, 0x1000000, size) != true) {
		ret = false;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:ve820 entry 0 base_001
 *
 * Summary: On 64 bit Mode, gets the value of 8 consecutive bytes at the
 * physical address FFF2D0H, the value is 0H.
 */
static void __unused multiboot_rqmid_34247_e820_entry_0_base_001(void)
{
	bool ret1 = true;
	unsigned long start;
	int size;

	start = 0xFFF2D0;
	size = 8;
	ret1 = read_test_value(start, start + size, 0x0, size);

	report("%s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:ve820 entry 0 length_001
 *
 * Summary: On 64 bit Mode, gets the value of 8 consecutive bytes at the
 * physical address FFF2D8H, the value is F0000H.
 */
static void __unused multiboot_rqmid_34249_e820_entry_0_length_001(void)
{
	bool ret1 = true;
	unsigned long start;
	int size;

	start = 0xFFF2D8;
	size = 8;
	ret1 = read_test_value(start, start + size, 0xF0000, size);

	report("%s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:ve820 entry 0 type_001
 *
 * Summary: On 64 bit Mode, gets the value of 4 consecutive bytes at the
 * physical address FFF2E0H, the value is 1H.
 */
static void __unused multiboot_rqmid_34250_e820_entry_0_type_001(void)
{
	bool ret1 = true;
	unsigned long start;
	int size;

	start = 0xFFF2E0;
	size = 4;
	ret1 = read_test_value(start, start + size, 0x1, size);

	report("%s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:ve820 entry 1 base_001
 *
 * Summary: On 64 bit Mode, gets the value of 8 consecutive bytes at the
 * physical address FFF2E4H, the value is F0000H.
 */
static void __unused multiboot_rqmid_34251_e820_entry_1_base_001(void)
{
	bool ret1 = true;
	unsigned long start;
	int size;

	start = 0xFFF2E4;
	size = 8;
	ret1 = read_test_value(start, start + size, 0xF0000, size);

	report("%s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:ve820 entry 1 length_001
 *
 * Summary: On 64 bit Mode, gets the value of 8 consecutive bytes at the
 * physical address FFF2ECH, the value is 10000H.
 */
static void __unused multiboot_rqmid_34252_e820_entry_1_length_001(void)
{
	bool ret1 = true;
	unsigned long start;
	int size;

	start = 0xFFF2EC;
	size = 8;
	ret1 = read_test_value(start, start + size, 0x10000, size);

	report("%s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:ve820 entry 1 type_001
 *
 * Summary: On 64 bit Mode, gets the value of 4 consecutive bytes at the
 * physical address FFF2F4H, the value is 2H.
 */
static void __unused multiboot_rqmid_34253_e820_entry_1_type_001(void)
{
	bool ret1 = true;
	unsigned long start;
	int size;

	start = 0xFFF2F4;
	size = 4;
	ret1 = read_test_value(start, start + size, 0x2, size);

	report("%s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:ve820 entry 2 base_001
 *
 * Summary: On 64 bit Mode, gets the value of 8 consecutive bytes at the
 * physical address FFF2F8H, the value is 100000H.
 */
static void __unused multiboot_rqmid_34254_e820_entry_2_base_001(void)
{
	bool ret1 = true;
	unsigned long start;
	int size;

	start = 0xFFF2F8;
	size = 8;
	ret1 = read_test_value(start, start + size, 0x100000, size);

	report("%s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:ve820 entry 2 length_001
 *
 * Summary: On 64 bit Mode, gets the value of 8 consecutive bytes at the
 * physical address FFF300H, the value is 3FF00000H.
 */
static void __unused multiboot_rqmid_34255_e820_entry_2_length_001(void)
{
	bool ret1 = true;
	unsigned long start;
	int size;

	start = 0xFFF300;
	size = 8;
	ret1 = read_test_value(start, start + size, 0x3FF00000, size);

	report("%s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:ve820 entry 2 type_001
 *
 * Summary: On 64 bit Mode, gets the value of 4 consecutive bytes at the
 * physical address FFF308H, the value is 1H.
 */
static void __unused multiboot_rqmid_34256_e820_entry_2_type_001(void)
{
	bool ret1 = true;
	unsigned long start;
	int size;

	start = 0xFFF308;
	size = 4;
	ret1 = read_test_value(start, start + size, 0x1, size);

	report("%s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:additional zero in zero page_001
 *
 * Summary: On 64 bit Mode, read all contents in the range of physical
 * address FFF30CH to FFFFFFH, all values are 0.
 */
static void __unused multiboot_rqmid_34257_additional_zero_in_zero_page_001(void)
{
	bool ret1 = true;
	unsigned long start;
	int size;

	start = 0xFFF30C;
	size = 4;
	ret1 = read_test_value(start, 0xFFFFFF + 1, 0x0, size);

	report("%s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:low memory region map_001
 *
 * Summary: On 64 bit Mode, when testing memory region rights&map,
 * select a memory unit in each 4K page to test the read-write and executable.
 */
static void multiboot_rqmid_34119_low_memory_region_map_001(void)
{
	low_memory_check(__FUNCTION__);
}

static void memory_64k_check(const char *fun_name)
{
	int ret = 0;
	unsigned long start;
	unsigned long end;

	start = 0xF0000;
	end = 0xFFFFF;
	ret = read_write_exec_test(start, end, 1);

	report("%s", ret == 0, fun_name);
}

/**
 * @brief case name:64K memory region rights_001
 *
 * Summary: On 64 bit Mode, when testing memory region rights&map,
 * select a memory unit in each 4K page to test the read-write and executable.
 */
static void multiboot_rqmid_34123_64k_memory_region_rights_001()
{
	memory_64k_check(__FUNCTION__);
}

/**
 * @brief case name:64K memory region map_001
 *
 * Summary: On 64 bit Mode, when testing memory region rights&map,
 * select a memory unit in each 4K page to test the read-write and executable.
 */
static void multiboot_rqmid_34124_64k_memory_region_map_001(void)
{
	memory_64k_check(__FUNCTION__);
}

static void memory_512M_check(const char *fun_name)
{
	int ret = 0;
	unsigned long start;
	unsigned long end;

	start = 0x10000000;	//256M
	end = 0x1FEFFFFF;
	ret = read_write_exec_test(start, end, 1);

	report("%s", ret == 0, fun_name);
}

/**
 * @brief case name:512M memory region rights_001
 *
 * Summary: On 64 bit Mode, when testing memory region rights&map,
 * select a memory unit in each 4K page to test the read-write and executable.
 */
static void __unused multiboot_rqmid_34125_512M_memory_region_rights_001()
{
	memory_512M_check(__FUNCTION__);
}

/**
 * @brief case name:512M memory region map_001
 *
 * Summary: On 64 bit Mode, when testing memory region rights&map,
 * select a memory unit in each 4K page to test the read-write and executable.
 */
static void __unused multiboot_rqmid_34126_512M_memory_region_map_001(void)
{
	memory_512M_check(__FUNCTION__);
}

static void memory_256M_to_1G_check(const char *fun_name)
{
	int ret = 0;
	unsigned long start;
	unsigned long end;

	start = 0x10000000;	//256M
	end = 0x3FFFFFFF;
	ret = read_write_exec_test(start, end, 1);

	//acrn-unit-test only has 512M memory, unable to test 1G memory access
	report("%s %d", ret == 0, fun_name, ret);
}

/**
 * @brief case name:1G memory region rights_001
 *
 * Summary: On 64 bit Mode, when testing memory region rights&map,
 * select a memory unit in each 4K page to test the read-write and executable.
 *This case and case 42697 construct 1G's check.
 */
static void __unused multiboot_rqmid_34127_1G_memory_region_rights_001()
{
	memory_256M_to_1G_check(__FUNCTION__);
}

int read_write_test(unsigned long start, unsigned long end)
{
	unsigned long len;
	unsigned char *point;
	int i;

	debug_print("test start:%lx, end:%lx\n", start, end);
	len = end-start;
	point = (unsigned char *) start;

	for (i = 0; i < len; i += 4096) {
		asm volatile (ASM_TRY("1f")
				"mov %0, %%eax\n\t"
				"1:"
				: : "m"(point[i]) : "memory");
		if (exception_vector() != PF_VECTOR) {
			return -1;
		}
	}

	for (i = 0; i < len; i += 4096) {
		asm volatile ("mov $0xFF, %%eax\n\t"
				ASM_TRY("1f")
				"mov %%eax, %0\n\t"
				"1:"
				: "=m"(point[i]) : : "memory");
		if (exception_vector() != PF_VECTOR) {
			return -1;
		}
	}
	return 0;
}

static int memory_1G_above_map_check()
{
	int ret = 0;
	unsigned long start;
	unsigned long end;

	start = 0x40000000;
	end = 0x40100000;
	ret = read_write_test(start, end);
	return ret;
}

/**
 * @brief case name:1G memory region map_001
 *
 * Summary: On 64 bit Mode, when testing memory region rights&map,
 * select a memory unit in each 4K page to test the read-write.
 * Only check 1G above area,1M below have special purpose.
 */
static void __unused multiboot_rqmid_34128_1G_memory_region_map_001(void)
{
	int ret = 0;
	struct _zeropage *pZeropage;

	pZeropage = get_zero_page(0);
	memory_1G_above_map_check();
	report("%s", (ret == 0) && (pZeropage->e820[2].baseaddr == 0x100000) && (pZeropage->e820[2].length ==
	 0x3FF00000), __FUNCTION__);
}

/**
 * @brief case name:Command line value to non-safety VM_001
 *
 * Summary: On 64 bit Mode, obtain the command line value through ACRN's unit test API,
 * which should contain "root=/dev/sda3 rw rootwait nohpet no_timer_check x2apic_phys noapic intel_iommu=off".
 */
static void __unused multiboot_rqmid_27266_command_line_value_to_non_safety_001(void)
{
	bool ret = false;
	const char *boot = "root=/dev/sda3 rw rootwait nohpet no_timer_check x2apic_phys noapic intel_iommu=off";

	struct _zeropage *zeropage = NULL;

	zeropage = get_zero_page(0);

	if (strcmp(boot, (const char *)((u64)zeropage->hdr.bootargs_addr)) == 0) {
		ret = true;
	}

	debug_print("command=%s\n", (const char *)((u64)zeropage->hdr.bootargs_addr));
	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:loadflags field of setup header_001
 *
 * Summary: On 64 bit Mode, get the physical address of the ZEROPAGE structure
 * via BP's ESI, get the value of the ZEROPAGE structure at 211H offset.
 */
static void __unused multiboot_rqmid_28104_loadflags_field_of_setup_header_001(void)
{
	bool ret = false;
	unsigned char *ptr;
	struct _zeropage *zeropage = NULL;

	zeropage = get_zero_page(0);
	ptr = (unsigned char *)zeropage;

	if ((u64)&(zeropage->hdr.load_flags) == (u64)&(ptr[0x211])) {
		if (zeropage->hdr.load_flags == 0x20) {
			ret = true;
		}
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name: Hypervisor shall set guest OS register CR0_001
 *
 * Summary: On 64 bit Mode, reading guest BP register CR0's PG
 * bit following start-up, the CR0.PG should be 0H.
 */
static void multiboot_rqmid_28133_cr0_register_001()
{
	volatile u32 cr0 = *((volatile u32 *)MULTIBOOT_STARTUP_CR0_ADDR);

	report("%s 0x%x", ((cr0 & X86_CR0_PG) == 0), __FUNCTION__, cr0);
}

/**
 * @brief case name: Hypervisor shall set guest OS register CR0_002
 *
 * Summary: On 64 bit Mode, reading guest AP register CR0's PG
 * bit following INIT, the CR0.PG should be 0H.
 */
static void __unused multiboot_rqmid_39085_cr0_register_002()
{
	volatile u32 cr0 = *((volatile u32 *)MULTIBOOT_INIT_CR0_ADDR);

	report("%s 0x%x", ((cr0 & X86_CR0_PG) == 0), __FUNCTION__, cr0);
}

/**
 * @brief case name: guest ESI of safety VM_001
 *
 * Summary: On 64 bit Mode, get the value of dump ESI following start-up.
 */
static void __unused multiboot_rqmid_27234_esi_of_safety_001()
{
	volatile u32 esi = *((volatile u32 *)MULTIBOOT_STARTUP_ESI_ADDR);

	report("%s 0x%x", (esi == 0), __FUNCTION__, esi);
}

/**
 * @brief case name: linux ESI_001
 *
 * Summary: On 64 bit Mode, save ESI register value at AP executing 1st instruction,
 * the value is 0.
 */
static void __unused multiboot_rqmid_34260_linux_esi_001()
{
	volatile u32 esi = *((volatile u32 *)MULTIBOOT_INIT_ESI_ADDR);
	bool is_pass = true;

	if (esi != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(34260);
	esi = *((volatile u32 *)MULTIBOOT_INIT_ESI_ADDR);
	if (esi != 0) {
		is_pass = false;
	}

	report("%s 0x%x", is_pass, __FUNCTION__, esi);
}

/**
 * @brief case name: linux BSP EIP_001
 *
 * Summary: To build a minimal stack environment, use the call instruction to push
 * the current EIP onto the top of the stack, use the pop instruction to pop into
 * an EDX register, and read it into a memory area, then read it,the value is
 * (setup_sects of setup header of zero page + 1H) * 200H + 1000000H + Instruction
 * code length that has been executed.
 */
static void __unused multiboot_rqmid_34261_linux_bsp_eip_001()
{
	bool ret = false;
	struct _zeropage *zeropage = NULL;
	u32 multi_eip;
	/* Instruction code length is 0xA
	 *	mov     $0xa0000, %esp
	 *	0:   bc 00 00 0a 00          mov    $0xa0000,%esp
	 *  call 11f
	 *	5:   e8 00 00 00 00          callq  a <start+0xa>
	 */
	u32 ins_len = 0xA;

	volatile u32 eip = *((volatile u32 *)MULTIBOOT_STARTUP_EIP_ADDR);

	zeropage = get_zero_page(0);
	multi_eip = (zeropage->hdr1.setup_sects + 1) * 0x200 + 0x1000000 + ins_len;
	if (multi_eip  == eip) {
		ret = true;
	}

	report("%s 0x%x 0x%x", (ret == true), __FUNCTION__, eip, multi_eip);
}

/**
 * @brief case name: zephry EIP_001
 *
 * Summary: To build a minimal stack environment, use the call instruction to push
 * the current EIP onto the top of the stack, use the pop instruction to pop into
 * an EDX register, and write it into a memory area (6000H), then read it,
 * the value is 100000H +Instruction code length that has been executed.
 */
static void __unused multiboot_rqmid_34263_zephry_eip_001()
{
	bool ret = false;
	u32 multi_eip;
	/* Instruction code length
	 *
	 *	#ifdef BP_STARTUP_CHECK
	 *		#include BP_STARTUP_CHECK
	 *	#endif
	 *		 mov %ebx, mb_boot_info
	 *		 mov $stacktop, %esp
	 *  call  11f
	 *	100000:       e8 00 00 00 00          callq  100005 <start+0x5>
	 */
	u32 ins_len = 0x1e;

	volatile u32 eip = *((volatile u32 *)MULTIBOOT_STARTUP_EIP_ADDR);

	multi_eip = 0x100000 + ins_len;
	if (multi_eip  == eip) {
		ret = true;
	}

	report("%s 0x%x 0x%x", (ret == true), __FUNCTION__, eip, multi_eip);
}

/**
 * @brief case name:RSDP OEM ID_001
 *
 * Summary: On 64 bit Mode, gets the ACPI RSDP OEM ID, the value is "ACRN" in ASCII encoding.
 */
static void __unused multiboot_rqmid_34093_rsdp_oem_id_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;

	/* Verify OME ID */
	if (((unsigned long)&(rsdp_p->oem_id) == RSDP_OEM_ID_ADDRESS) &&
		!memcmp(rsdp_p->oem_id, ACPI_SIG_OEM_ID, ACPI_OEM_ID_SIZE)) {
		ret = true;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:Revision field of RSDP_001
 *
 * Summary: On 64 bit Mode, get revision field of RSDP, the value is 2H.
 */
static void __unused multiboot_rqmid_34094_rvision_field_of_rsdp_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;

	/* Verify revision */
	if (((unsigned long)&(rsdp_p->revision) == RSDP_REVISION_ADDRESS)
		&& (rsdp_p->revision == 0x2)) {
		ret = true;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:Length field of RSDP_001
 *
 * Summary: On 64 bit Mode, get Length field of RSDP, the value is 24H.
 */
static void __unused multiboot_rqmid_34095_length_field_of_rsdp_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;

	/* Verify length */
	if (((unsigned long)&(rsdp_p->length) == RSDP_LENGTH_ADDRESS)
		&& (rsdp_p->length == RSDP_SIZE)) {
		ret = true;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:XsdtAddress field of RSDP_001
 *
 * Summary: On 64 bit Mode, get XsdtAddress field of RSDP, the value is F2480H.
 */
static void __unused multiboot_rqmid_34096_xsdtaddress_field_of_rsdp_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;

	/* Verify XSDT address */
	if (((unsigned long)&(rsdp_p->xsdt_physical_address) == RSDP_XSDT_ADDRESS)
		&& (rsdp_p->xsdt_physical_address == XSDT_ADDRESS)) {
		ret = true;
	}

	report("%s", ret == true, __FUNCTION__);
}

unsigned char checksum(void *point, unsigned long size)
{
	unsigned char sum = 0;
	unsigned char *p = (unsigned char *)point;
	unsigned long i;

	for (i = 0; i < size; i++) {
		sum += p[i];
	}

	return sum;
}

/**
 * @brief case name:expose Extended System Description Table_001
 *
 * Summary: On 64 bit Mode, taking the physical address 000F2480H as the base address,
 * get the XSDT structure.
 */
static void __unused multiboot_rqmid_34097_xsdt_table_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;

	/* Verify XSDT */
	if (((unsigned long)xsdt == XSDT_ADDRESS)
		&& ((unsigned long)&(xsdt->header.length) == XSDT_LENGTH_ADDRESS)
		&& (xsdt->header.length == XSDT_SIZE)
		&& ((unsigned long)&(xsdt->header.revision) == XSDT_REVISION_ADDRESS)
		&& (xsdt->header.revision == 0x1)
		&& (checksum(xsdt, XSDT_SIZE) == 0)) {
		ret = true;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:XSDT OEM ID_001
 *
 * Summary: On 64 bit Mode, gets the XSDT OEM ID, the value is "ACRN  " in ASCII encoding.
 */
static void __unused multiboot_rqmid_34098_xsdt_ome_id_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;

	/* Verify oem id */
	if (((unsigned long)&(xsdt->header.oem_id) == XSDT_OEM_ID_ADDRESS)
		&& !memcmp(xsdt->header.oem_id, ACPI_SIG_OEM_ID, ACPI_OEM_ID_SIZE)) {
		ret = true;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:XSDT OEM TABLE ID_001
 *
 * Summary: On 64 bit Mode, gets the XSDT OEM TABLE ID , the value is "VIRTNUC7" in ASCII encoding.
 */
static void __unused multiboot_rqmid_34099_xsdt_ome_table_id_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;

	/* Verify oem table id */
	if (((unsigned long)&(xsdt->header.oem_table_id) == XSDT_OEM_TABLE_ID_ADDRESS)
		&& !memcmp(xsdt->header.oem_table_id, XSDT_SIG_OEM_TABLE_ID, XSDT_OEM_TABLE_ID_SIZE)) {
		ret = true;
	}

	report("%s ome_table_id=%s", ret == true, __FUNCTION__, xsdt->header.oem_table_id);
}

/**
 * @brief case name:XSDT OEM revision_001
 *
 * Summary: On 64 bit Mode, gets the XSDT OEM revision , the value is 1H.
 */
static void __unused multiboot_rqmid_34100_xsdt_ome_revision_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;

	/* Verify ome revision */
	if (((unsigned long)&(xsdt->header.oem_revision) == XSDT_OEM_REVISION_ADDRESS)
		&& (xsdt->header.oem_revision == 0x1)) {
		ret = true;
	}

	report("%s oem_revision=%x", ret == true, __FUNCTION__, xsdt->header.oem_revision);
}

/**
 * @brief case name:creator ID field of EXSDT_001
 *
 * Summary: On 64 bit Mode, gets creator ID field of EXSDT , the value is "INTI" in ASCII encoding.
 */
static void __unused multiboot_rqmid_34101_xsdt_creator_id_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;

	/* Verify creator id */
	if (((unsigned long)&(xsdt->header.asl_compiler_id) == XSDT_CREATOR_ID_ADDRESS)
		&& !memcmp(xsdt->header.asl_compiler_id, XSDT_CREATOR_ID, XSDT_CREATOR_ID_SIZE)) {
		ret = true;
	}

	report("%s %c%c%c%c", ret == true, __FUNCTION__,
		xsdt->header.asl_compiler_id[0], xsdt->header.asl_compiler_id[1],
		xsdt->header.asl_compiler_id[2], xsdt->header.asl_compiler_id[3]);
}

/**
 * @brief case name:XSDT aml version_001
 *
 * Summary: On 64 bit Mode, gets XSDT aml version , the value is 0H.
 */
static void __unused multiboot_rqmid_34102_xsdt_aml_version_id_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;

	/* Verify revision */
	if (((unsigned long)&(xsdt->header.asl_compiler_revision) == XSDT_AML_VERSION_ADDRESS)
		&& (xsdt->header.asl_compiler_revision == 0x0)) {
		ret = true;
	}

	report("%s 0x%x", ret == true, __FUNCTION__, xsdt->header.asl_compiler_revision);
}

/**
 * @brief case name:expose Multiple APIC Description Table_001
 *
 * Summary: On 64 bit Mode, taking the physical address 000F2500H as the base address, get the MADT structure.
 */
static void __unused multiboot_rqmid_34103_expose_multiple_apic_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;
	struct acpi_table_madt *madt = (struct acpi_table_madt *)xsdt->table_offset_entry[0];

	/* Verify MADT */
	//length = 0x4a, revision = 3,
	if (((unsigned long)&(xsdt->table_offset_entry) == XSDT_TABLE_ENTRY_ADDRESS)
		&& (xsdt->table_offset_entry[0] == MADT_ADDRESS)
		&& !memcmp(madt->header.signature, ACPI_SIG_MADT, ACPI_SIG_MADT_SIZE)
		&& ((unsigned long)&(madt->header.length) == MADT_LENGTH_ADDRESS)
		&& (madt->header.length == MADT_SIZE)
		&& ((unsigned long)&(madt->header.revision) == MADT_REVISION_ADDRESS)
		&& (madt->header.revision == 0x4)
		&& (checksum(madt, MADT_SIZE) == 0)) {
		ret = true;
	}

	report("%s ", ret == true, __FUNCTION__);
}

/**
 * @brief case name:Local Interrupt Controller Address field_001
 *
 * Summary: On 64 bit Mode, gets Local Interrupt Controller Address field of MADT, the value is 0FEE00000H.
 */
static void __unused multiboot_rqmid_34104_local_interrupt_controller_address_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;
	struct acpi_table_madt *madt = (struct acpi_table_madt *)xsdt->table_offset_entry[0];

	/* Verify MADT address */
	if (((unsigned long)&(madt->address) == MADT_ENTRY_ADDRESS)
		&& (madt->address == 0x0FEE00000)) {
		ret = true;
	}

	report("%s ", ret == true, __FUNCTION__);
}

/**
 * @brief case name:flags field of MADT_001
 *
 * Summary: On 64 bit Mode, gets flags field of MADT, the value is 0H.
 */
static void __unused multiboot_rqmid_34105_flags_field_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;
	struct acpi_table_madt *madt = (struct acpi_table_madt *)xsdt->table_offset_entry[0];

	/* Verify MADT flags */
	if (((unsigned long)&(madt->flags) == MADT_FLAGS_ADDRESS)
		&& (madt->flags == 0x0)) {
		ret = true;
	}

	report("%s flags=%x", ret == true, __FUNCTION__, madt->flags);
}

/**
 * @brief case name:MADT OEM ID_001
 *
 * Summary: On 64 bit Mode, gets the MADT OEM ID, the value is "ACRN  " in ASCII encoding.
 */
static void __unused multiboot_rqmid_34106_madt_oem_id_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;
	struct acpi_table_madt *madt = (struct acpi_table_madt *)xsdt->table_offset_entry[0];

	/* Verify MADT oem_id */
	if (((unsigned long)&(madt->header.oem_id) == MADT_OEM_ID_ADDRESS)
		&& !memcmp(madt->header.oem_id, ACPI_SIG_OEM_ID, ACPI_OEM_ID_SIZE)) {
		ret = true;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:MADT OEM TABLE ID_001
 *
 * Summary: On 64 bit Mode, gets the MADT OEM TABLE ID, the value is "VIRTNUC7" in ASCII encoding.
 */
static void __unused multiboot_rqmid_34107_madt_oem_table_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;
	struct acpi_table_madt *madt = (struct acpi_table_madt *)xsdt->table_offset_entry[0];

	/* Verify MADT oem_table_id */
	if (((unsigned long)&(madt->header.oem_table_id) == MADT_OEM_TABLE_ID_ADDRESS)
		&& !memcmp(madt->header.oem_table_id, ACPI_SIG_OEM_TABLE_ID, ACPI_OEM_TABLE_ID_SIZE)) {
		ret = true;
	}

	report("%s oem_table_id=|%s|", ret == true, __FUNCTION__, madt->header.oem_table_id);
}

/**
 * @brief case name:MADT OEM revision_001
 *
 * Summary: On 64 bit Mode, gets the MADT OEM revision , the value is 1H.
 */
static void __unused multiboot_rqmid_34108_madt_oem_revision_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;
	struct acpi_table_madt *madt = (struct acpi_table_madt *)xsdt->table_offset_entry[0];

	/* Verify MADT oem_revision */
	if (((unsigned long)&(madt->header.oem_revision) == MADT_OEM_REVISION_ADDRESS)
		&& (madt->header.oem_revision == 0x1)) {
		ret = true;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:Creator ID of MADT_001
 *
 * Summary: On 64 bit Mode, gets Creator ID of MADT, the value is "INTI" in ASCII encoding.
 */
static void __unused multiboot_rqmid_34109_madt_creator_id_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;
	struct acpi_table_madt *madt = (struct acpi_table_madt *)xsdt->table_offset_entry[0];

	/* Verify MADT creator id */
	if (((unsigned long)&(madt->header.asl_compiler_id) == MADT_CREATOR_ID_ADDRESS)
		&& !memcmp(madt->header.asl_compiler_id, ACPI_CREATOR_ID, ACPI_CREATOR_ID_SIZE)) {
		ret = true;
	}

	report("%s %c%c%c%c", ret == true, __FUNCTION__,
		madt->header.asl_compiler_id[0], madt->header.asl_compiler_id[1],
		madt->header.asl_compiler_id[2], madt->header.asl_compiler_id[3]);
}

/**
 * @brief case name:MADT aml version_001
 *
 * Summary: On 64 bit Mode, gets MADT aml version, the value is 0H.
 */
static void __unused multiboot_rqmid_34110_madt_aml_version_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;
	struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)rsdp_p->xsdt_physical_address;
	struct acpi_table_madt *madt = (struct acpi_table_madt *)xsdt->table_offset_entry[0];

	/* Verify MADT aml version */
	if (((unsigned long)&(madt->header.asl_compiler_revision) == MADT_AML_VERSION_ADDRESS)
		&& (madt->header.asl_compiler_revision == 0x0)) {
		ret = true;
	}

	report("%s 0x%x", ret == true, __FUNCTION__, madt->header.asl_compiler_revision);
}

bool apic_type_len_check(unsigned char *apic_type, unsigned char *apic_length)
{
	bool ret = true;

	if ((*apic_type != 0x0) || (*apic_length != 0x8)) {
		printf("type=0x%x len=0x%x\n", *apic_type, *apic_length);
		ret = false;
	}

	return ret;
}
void apic_entry_check(const char *fun_name)
{
	bool ret = true;
	unsigned char *apic_type;
	unsigned char *apic_length;

	apic_type = (unsigned char *)APIC_ENTRY1_TYPE_ADDRESS;
	apic_length = (unsigned char *)APIC_ENTRY1_LENGTH_ADDRESS;
	ret &= apic_type_len_check(apic_type, apic_length);

	apic_type = (unsigned char *)APIC_ENTRY2_TYPE_ADDRESS;
	apic_length = (unsigned char *)APIC_ENTRY2_LENGTH_ADDRESS;
	ret &= apic_type_len_check(apic_type, apic_length);

	apic_type = (unsigned char *)APIC_ENTRY3_TYPE_ADDRESS;
	apic_length = (unsigned char *)APIC_ENTRY3_LENGTH_ADDRESS;
	ret &= apic_type_len_check(apic_type, apic_length);

	report("%s", ret == true, fun_name);
}

/**
 * @brief case name:processor local apic_001
 *
 * Summary: On 64 bit Mode, taking the physical address 000F252CH as the base address,
 * get the processor local apic structure.
 */
static void __unused multiboot_rqmid_34111_processor_local_apic_001(void)
{
	apic_entry_check(__FUNCTION__);
}

/**
 * @brief case name:non-safety vacpi shall provided 3 lapic entries_001
 *
 * Summary: On 64 bit Mode, taking the physical address 000F252CH as the base address,
 * get the processor local apic structure.
 */
static void __unused multiboot_rqmid_34112_vacpi_3_apic_001(void)
{
	apic_entry_check(__FUNCTION__);
}

/**
 * @brief case name:flags field of local apic_001
 *
 * Summary: On 64 bit Mode, gets flags field of all local apic, the value is 1H.
 */
static void __unused multiboot_rqmid_34113_apic_flags_001(void)
{
	bool ret = true;
	u32 *apic_flags;

	apic_flags = (u32 *)APIC_ENTRY1_FLAGS_ADDRESS;
	if (*apic_flags != 0x1) {
		printf("flags=0x%x\n", *apic_flags);
		ret = false;
	}

	apic_flags = (u32 *)APIC_ENTRY2_FLAGS_ADDRESS;
	if (*apic_flags != 0x1) {
		printf("flags=0x%x\n", *apic_flags);
		ret = false;
	}

	apic_flags = (u32 *)APIC_ENTRY3_FLAGS_ADDRESS;
	if (*apic_flags != 0x1) {
		printf("flags=0x%x\n", *apic_flags);
		ret = false;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:ACPI processor UID shall be unique_001
 *
 * Summary: On 64 bit Mode, gets ACPI processor UID field of all local apic,
 * the ACPI processor UID value shall be unique.
 */
static void __unused multiboot_rqmid_34114_acpi_uid_001(void)
{
	bool ret = true;
	unsigned char *acpi_uid;

	acpi_uid = (unsigned char *)APIC_ENTRY1_UID_ADDRESS;
	if (*acpi_uid != 0x0) {
		printf("uid=0x%x\n", *acpi_uid);
		ret = false;
	}

	acpi_uid = (unsigned char *)APIC_ENTRY2_UID_ADDRESS;
	if (*acpi_uid != 0x1) {
		printf("uid=0x%x\n", *acpi_uid);
		ret = false;
	}

	acpi_uid = (unsigned char *)APIC_ENTRY3_UID_ADDRESS;
	if (*acpi_uid != 0x2) {
		printf("uid=0x%x\n", *acpi_uid);
		ret = false;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:vacpi id and lapic id shall be consistent_001
 *
 * Summary: On 64 bit Mode, gets lapic id field of all local apic,
 * the lapic id and ACPI processor UID shall be consistent
 */
static void __unused multiboot_rqmid_34115_vapic_id_001(void)
{
	bool ret = true;
	unsigned char *vapic_id;

	vapic_id = (unsigned char *)APIC_ENTRY1_LAPIC_ID_ADDRESS;
	if (*vapic_id != 0x0) {
		printf("uid=0x%x\n", *vapic_id);
		ret = false;
	}

	vapic_id = (unsigned char *)APIC_ENTRY2_LAPIC_ID_ADDRESS;
	if (*vapic_id != 0x1) {
		printf("uid=0x%x\n", *vapic_id);
		ret = false;
	}

	vapic_id = (unsigned char *)APIC_ENTRY3_LAPIC_ID_ADDRESS;
	if (*vapic_id != 0x2) {
		printf("uid=0x%x\n", *vapic_id);
		ret = false;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:RSDP Rsdtaddress_001
 *
 * Summary: On 64 bit Mode, gets the RSDP Rsdtaddress, the value is 0H.
 */
static void __unused multiboot_rqmid_34116_rsdp_address_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;

	/* Verify rsdp address */
	if (((unsigned long)&(rsdp_p->rsdt_physical_address) == RSDP_RSDT_PHYSICAL_ADDRESS)
		&& (rsdp_p->rsdt_physical_address == 0x0)) {
		ret = true;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:baseaddress of RSDP_001
 *
 * Summary: On 64 bit Mode, an RSDP structure is stored at the address starting at 000F2400H.
 */
static void __unused multiboot_rqmid_34117_rsdp_baseaddress_001(void)
{
	bool ret = false;
	struct acpi_table_rsdp *rsdp_p = (struct acpi_table_rsdp *)RSDP_ADDRESS;

	/* Verify rsdp baseaddress */
	if (((unsigned long)&(rsdp_p->signature) == RSDP_SIG_ADDRESS)
		&& !memcmp(rsdp_p->signature, RSDP_SIG_RSDP, RSDP_SIGNATURE_LEN)
		&& (checksum(rsdp_p, 20) == 0)
		&& (checksum(rsdp_p, 33) == 0)) {
		ret = true;
	}

	report("%s", ret == true, __FUNCTION__);
}

/**
 * @brief case name:expose zero page_001
 *
 * Summary: On 64 bit Mode, get the corresponding values for non-safety VM setup
 * headers and kernel image, the value should be consistent with the expected value.
 */
static void __unused multiboot_rqmid_34129_expose_zero_page_001(void)
{
	bool ret = true;
	volatile u32 ebx;
	volatile u32 ebp;
	volatile u32 edi;
	volatile u32 cr0;
	volatile u32 eflags;
	unsigned char *ptr;
	struct _zeropage *zeropage = NULL;
	u8 *p_char;
	u16 *p_short;
	u32 *p_int;
	u64 *p_long;

	/* step 6 verify ebx register */
	ebx = *((volatile u32 *)MULTIBOOT_STARTUP_EBX_ADDR);
	if (ebx != 0x0) {
		ret = false;
	}

	/* step 7 verify ebp register */
	ebp = *((volatile u32 *)MULTIBOOT_STARTUP_EBP_ADDR);
	if (ebp != 0x0) {
		ret = false;
	}

	/* step 8 verify edi register */
	edi = *((volatile u32 *)MULTIBOOT_STARTUP_EDI_ADDR);
	if (edi != 0x0) {
		ret = false;
	}

	/* step 9 verify CR0.PE[bit 0] */
	cr0 = *((volatile u32 *)MULTIBOOT_STARTUP_CR0_ADDR);
	if ((cr0&X86_CR0_PE) != X86_CR0_PE) {
		ret = false;
	}

	/* step 10 verify eflags register */
	eflags = *((volatile u32 *)MULTIBOOT_STARTUP_EFLAGS_ADDR);
	if ((eflags) != 0x2) {
		ret = false;
		printf("%s %d eflags=%d\n", __FUNCTION__, __LINE__, eflags);
	}

	zeropage = get_zero_page(0);
	ptr = (unsigned char *)zeropage;

	/* step 11 verify setup_sects field */
	p_char = (u8 *)(u64)&(ptr[0x1f1]);
	if (*p_char != 0x2) {
		ret = false;
	}

	/* step 12 verify root_flags field */
	p_short = (u16 *)(u64)&(ptr[0x1f2]);
	if (*p_short != 0x0) {
		ret = false;
	}

	/* step 13 verify syssize field */
	p_int = (u32 *)(u64)&(ptr[0x1f4]);
	if (*p_int != 0x0) {
		ret = false;
	}

	/* step 14 verify ram_size field */
	p_short = (u16 *)(u64)&(ptr[0x1f8]);
	if (*p_short != 0x0) {
		ret = false;
	}

	/* step 15 verify vid_mode field */
	p_short = (u16 *)(u64)&(ptr[0x1fa]);
	if (*p_short != 0x0) {
		ret = false;
	}

	/* step 16 verify root_dev field */
	p_short = (u16 *)(u64)&(ptr[0x1fc]);
	if (*p_short != 0x0) {
		ret = false;
	}

	/* step 17 verify boot_flag field */
	p_short = (u16 *)(u64)&(ptr[0x1fe]);
	if (*p_short != 0xaa55) {
		ret = false;
	}

	/* step 18 verify jump field */
	p_short = (u16 *)(u64)&(ptr[0x200]);
	if (*p_short != 0x66eb) {
		ret = false;
	}

	/* step 19 verify header field */
	p_int = (u32 *)(u64)&(ptr[0x202]);
	if (*p_int != 0x53726448) {
		ret = false;
	}

	/* step 20 verify version field */
	p_short = (u16 *)(u64)&(ptr[0x206]);
	if (*p_short != 0x20c) {
		ret = false;
	}

	/* step 21 verify realmode_switch field */
	p_int = (u32 *)(u64)&(ptr[0x208]);
	if (*p_int != 0x0) {
		ret = false;
	}

	/* step 22 verify start_sys_seg field */
	p_short = (u16 *)(u64)&(ptr[0x20C]);
	if (*p_short != 0x0) {
		ret = false;
	}

	/* step 23 verify kernel_version field */
	p_short = (u16 *)(u64)&(ptr[0x20E]);
	if (*p_short != 0x200) {
		ret = false;
	}

	/* step 24 verify type_of_loader field */
	p_char = (u8 *)(u64)&(ptr[0x210]);
	if (*p_char != 0xff) {
		ret = false;
	}

	/* step 25 verify loadflags field */
	p_char = (u8 *)(u64)&(ptr[0x211]);
	if (*p_char != 0x20) {
		ret = false;
	}

	/* step 26 verify setup_move_size field */
	p_short = (u16 *)(u64)&(ptr[0x212]);
	if (*p_short != 0x0) {
		ret = false;
	}

	/* step 27 verify code32_start field */
	p_int = (u32 *)(u64)&(ptr[0x214]);
	if (*p_int != 0x100000) {
		ret = false;
	}

	/* step 28 verify ramdisk_image field */
	p_int = (u32 *)(u64)&(ptr[0x218]);
	if (*p_int != 0x0) {
		ret = false;
	}

	/* step 29 verify ramdisk_size field */
	p_int = (u32 *)(u64)&(ptr[0x21c]);
	if (*p_int != 0x0) {
		ret = false;
	}

	/* step 30 verify bootsect_kludge field */
	p_int = (u32 *)(u64)&(ptr[0x220]);
	if (*p_int != 0x0) {
		ret = false;
	}

	/* step 31 verify heap_end_ptr field */
	p_short = (u16 *)(u64)&(ptr[0x224]);
	if (*p_short != 0x0) {
		ret = false;
	}

	/* step 32 verify ext_loader_ver field */
	p_char = (u8 *)(u64)&(ptr[0x226]);
	if (*p_char != 0x0) {
		ret = false;
	}

	/* step 33 verify ext_loader_type field */
	p_char = (u8 *)(u64)&(ptr[0x227]);
	if (*p_char != 0x0) {
		ret = false;
	}

	/* step 34 verify cmd_line_ptr field */
	p_int = (u32 *)(u64)&(ptr[0x228]);
	if (*p_int != 0xffe000) {
		ret = false;
	}

	/* step 35 verify initrd_addr_max field */
	p_int = (u32 *)(u64)&(ptr[0x22c]);
	if (*p_int != 0x7fffffff) {
		ret = false;
	}

	/* step 36 verify kernel_alignment field */
	p_int = (u32 *)(u64)&(ptr[0x230]);
	if (*p_int != 0x0) {
		ret = false;
	}

	/* step 37 verify relocatable_kernel field */
	p_char = (u8 *)(u64)&(ptr[0x234]);
	if (*p_char != 0x1) {
		ret = false;
	}

	/* step 38 verify min_alignment field */
	p_char = (u8 *)(u64)&(ptr[0x235]);
	if (*p_char != 0x0) {
		ret = false;
	}

	/* step 39 verify xloadflags field */
	p_short = (u16 *)(u64)&(ptr[0x236]);
	if (*p_short != 0x0) {
		ret = false;
	}

	/* step 40 verify cmdline_size field */
	p_int = (u32 *)(u64)&(ptr[0x238]);
	if (*p_int != 0x7ff) {
		ret = false;
	}

	/* step 41 verify hardware_subarch field */
	p_int = (u32 *)(u64)&(ptr[0x23c]);
	if (*p_int != 0x0) {
		ret = false;
	}

	/* step 42 verify hardware_subarch_data field */
	p_long = (u64 *)(u64)&(ptr[0x240]);
	if (*p_long != 0x0) {
		ret = false;
	}

	/* step 43 verify payload_offset field */
	p_int = (u32 *)(u64)&(ptr[0x248]);
	if (*p_int != 0x0) {
		ret = false;
	}

	/* step 44 verify payload_length field */
	p_int = (u32 *)(u64)&(ptr[0x24c]);
	if (*p_int != 0x0) {
		ret = false;
	}

	/* step 45 verify setup_data field */
	p_long = (u64 *)(u64)&(ptr[0x250]);
	if (*p_long != 0x0) {
		ret = false;
	}

	/* step 46 verify pref_address field */
	p_long = (u64 *)(u64)&(ptr[0x258]);
	if (*p_long != 0x1000000) {
		ret = false;
	}

	/* step 47 verify init_size field */
	p_int = (u32 *)(u64)&(ptr[0x260]);
	if (*p_int != 0x0) {
		ret = false;
	}

	/* step 48 verify handover_offset field */
	p_int = (u32 *)(u64)&(ptr[0x264]);
	if (*p_int != 0x0) {
		ret = false;
	}
	report("%s", ret == true, __FUNCTION__);
}

static void print_case_list(void)
{
	printf("\t\t multiboot feature case list:\n\r");
	printf("\t\t multiboot 64-Bits Mode:\n\r");

#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 27210u, "the guest ESI of vBSP of non-safety VM_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27236u, "Non-safety VM command line_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34091u, "expose RSDP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34130u, "base address of zero page_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27249u, "type_of_loader field of setup header_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 39842u, "bootprotocol version_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27265u, "Hypervisor shall provide e820 to non-safety vm_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34243u, "zero fields of zero page_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34244u, "e820_entries field_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34245u, "other fields of setup header_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 34247u, "ve820 entry 0 base_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34249u, "ve820 entry 0 length_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34250u, "ve820 entry 0 type_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34251u, "ve820 entry 1 base_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34252u, "ve820 entry 1 length_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 34253u, "ve820 entry 1 type_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34254u, "ve820 entry 2 base_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34255u, "ve820 entry 2 length_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34256u, "ve820 entry 2 type_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34257u, "additional zero in zero page_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 34127u, "1G memory region rights_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34128u, "1G memory region map_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27266u, "Command line value to non-safety VM_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28104u, "loadflags field of setup header_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39085u, "Hypervisor shall set guest OS register CR0_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 34260u, "linux ESI_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34261u, "linux BSP EIP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34093u, "RSDP OEM ID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34094u, "Revision field of RSDP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34095u, "Length field of RSDP_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 34096u, "XsdtAddress field of RSDP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34097u, "expose Extended System Description Table_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34098u, "XSDT OEM ID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34099u, "XSDT OEM TABLE ID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34100u, "XSDT OEM revision_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 34101u, "creator ID field of EXSDT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34102u, "XSDT aml version_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34103u, "expose Multiple APIC Description Table_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34104u, "Local Interrupt Controller Address field_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34105u, "flags field of MADT_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 34106u, "MADT OEM ID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34107u, "MADT OEM TABLE ID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34108u, "MADT OEM revision_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34109u, "Creator ID of MADT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34110u, "MADT aml version_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 34111u, "processor local apic_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34112u, "non-safety vacpi shall provided 3 lapic entries_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34113u, "flags field of local apic_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34114u, "ACPI processor UID shall be unique_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34115u, "vacpi id and lapic id shall be consistent_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 34116u, "RSDP Rsdtaddress_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34117u, "baseaddress of RSDP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34129u, "expose zero page_001");
#endif

#ifdef IN_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 34125u, "512M memory region rights_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34126u, "512M memory region map_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27234u, "guest ESI of safety VM_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34263u, "zephry EIP_001");
#endif

	printf("\t\t Case ID:%d case name:%s\n\r", 34118u, "low memory region rights_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34119u, "low memory region map_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34123u, "64K memory region rights_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 34124u, "64K memory region map_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28133u, "Hypervisor shall set guest OS register CR0_001");
}

static void test_multiboot_list(void)
{
#ifdef IN_NON_SAFETY_VM
	multiboot_rqmid_27210_guest_esi_of_non_safety_001();
	multiboot_rqmid_27236_non_safety_vm_command_line_001();
	multiboot_rqmid_34091_expose_rsdp();
	multiboot_rqmid_34130_base_address_of_zero_page_001();
	multiboot_rqmid_27249_type_of_loader_field_001();

	multiboot_rqmid_39842_bootprotocol_version_field_001();
	multiboot_rqmid_27265_provide_e820_to_non_safety_001();
	multiboot_rqmid_34243_zero_fields_of_zero_page_001();
	multiboot_rqmid_34244_e820_entries_field_001();
	multiboot_rqmid_34245_other_fields_of_setup_header_001();

	multiboot_rqmid_34247_e820_entry_0_base_001();
	multiboot_rqmid_34249_e820_entry_0_length_001();
	multiboot_rqmid_34250_e820_entry_0_type_001();
	multiboot_rqmid_34251_e820_entry_1_base_001();
	multiboot_rqmid_34252_e820_entry_1_length_001();

	multiboot_rqmid_34253_e820_entry_1_type_001();
	multiboot_rqmid_34254_e820_entry_2_base_001();
	multiboot_rqmid_34255_e820_entry_2_length_001();
	multiboot_rqmid_34256_e820_entry_2_type_001();
	multiboot_rqmid_34257_additional_zero_in_zero_page_001();

	multiboot_rqmid_34127_1G_memory_region_rights_001();
	multiboot_rqmid_34128_1G_memory_region_map_001();
	multiboot_rqmid_27266_command_line_value_to_non_safety_001();
	multiboot_rqmid_28104_loadflags_field_of_setup_header_001();
	multiboot_rqmid_39085_cr0_register_002();

	multiboot_rqmid_34260_linux_esi_001();
	multiboot_rqmid_34261_linux_bsp_eip_001();
	multiboot_rqmid_34093_rsdp_oem_id_001();
	multiboot_rqmid_34094_rvision_field_of_rsdp_001();
	multiboot_rqmid_34095_length_field_of_rsdp_001();

	multiboot_rqmid_34096_xsdtaddress_field_of_rsdp_001();
	multiboot_rqmid_34097_xsdt_table_001();
	multiboot_rqmid_34098_xsdt_ome_id_001();
	multiboot_rqmid_34099_xsdt_ome_table_id_001();
	multiboot_rqmid_34100_xsdt_ome_revision_001();

	multiboot_rqmid_34101_xsdt_creator_id_001();
	multiboot_rqmid_34102_xsdt_aml_version_id_001();
	multiboot_rqmid_34103_expose_multiple_apic_001();
	multiboot_rqmid_34104_local_interrupt_controller_address_001();
	multiboot_rqmid_34105_flags_field_001();

	multiboot_rqmid_34106_madt_oem_id_001();
	multiboot_rqmid_34107_madt_oem_table_001();
	multiboot_rqmid_34108_madt_oem_revision_001();
	multiboot_rqmid_34109_madt_creator_id_001();
	multiboot_rqmid_34110_madt_aml_version_001();

	multiboot_rqmid_34111_processor_local_apic_001();
	multiboot_rqmid_34112_vacpi_3_apic_001();
	multiboot_rqmid_34113_apic_flags_001();
	multiboot_rqmid_34114_acpi_uid_001();
	multiboot_rqmid_34115_vapic_id_001();

	multiboot_rqmid_34116_rsdp_address_001();
	multiboot_rqmid_34117_rsdp_baseaddress_001();
	multiboot_rqmid_34129_expose_zero_page_001();
#endif

#ifdef IN_SAFETY_VM
	multiboot_rqmid_34125_512M_memory_region_rights_001();
	multiboot_rqmid_34126_512M_memory_region_map_001();
	multiboot_rqmid_27234_esi_of_safety_001();
	multiboot_rqmid_34263_zephry_eip_001();
#endif

	multiboot_rqmid_34118_low_memory_region_rights_001();
	multiboot_rqmid_34119_low_memory_region_map_001();
	multiboot_rqmid_34123_64k_memory_region_rights_001();
	multiboot_rqmid_34124_64k_memory_region_map_001();
	multiboot_rqmid_28133_cr0_register_001();
}

int main(int ac, char **av)
{
	setup_idt();
	setup_vm();
	print_case_list();
	test_multiboot_list();
	return report_summary();
}

