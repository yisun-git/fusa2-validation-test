#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#define MULTIBOOT_ESI_ADDR				(0x6000)
#define MULTIBOOT_STARTUP_EIP_ADDR		(0x6004)
#define MULTIBOOT_STARTUP_CR0_ADDR		(0x6008)
#define MULTIBOOT_STARTUP_ESI_ADDR		(0x600C)
#define MULTIBOOT_STARTUP_EDI_ADDR		(0x6010)
#define MULTIBOOT_STARTUP_EBX_ADDR		(0x6014)
#define MULTIBOOT_STARTUP_EBP_ADDR		(0x6018)
#define MULTIBOOT_STARTUP_EFLAGS_ADDR	(0x601C)

#define MULTIBOOT_INIT_CR0_ADDR			(0x7000)
#define MULTIBOOT_INIT_ESI_ADDR			(0x7004)

#define MULTIBOOT_COMMAND_MAX       1024
#define ZERO_PAGE_BASE_ADDRESS      0xFFF000

#define RSDP_ADDRESS                0xF2400
#define RSDP_SIG_ADDRESS			RSDP_ADDRESS	//+0
#define RSDP_CHECKSUM_ADDRESS		0xF2408			//+0x8
#define RSDP_OEM_ID_ADDRESS			0xF2409			//+0x9
#define RSDP_REVISION_ADDRESS		0xF240F			//+0xF
#define RSDP_RSDT_PHYSICAL_ADDRESS	0xF2410			//+0x10
#define RSDP_LENGTH_ADDRESS			0xF2414			//+0x14
#define RSDP_XSDT_ADDRESS			0xF2418			//+0x18
#define RSDP_EXT_CHECKSUM_ADDRESS	0xF2420			//+0x20
#define RSDP_RESERVED_ADDRESS		0xF2421			//+0x21
#define RSDP_SIZE					0x24

#define XSDT_ADDRESS				0xF2480
#define XSDT_SIG_ADDRESS			XSDT_ADDRESS	//+0
#define XSDT_LENGTH_ADDRESS			0xF2484			//+0x4
#define XSDT_REVISION_ADDRESS		0xF2488			//+0x8
#define XSDT_CHECKSUM_ADDRESS		0xF2489			//+0x9
#define XSDT_OEM_ID_ADDRESS			0xF248A			//+0xA
#define XSDT_OEM_TABLE_ID_ADDRESS	0xF2490			//+0x10
#define XSDT_OEM_REVISION_ADDRESS	0xF2498			//+0x18
#define XSDT_CREATOR_ID_ADDRESS		0xF249C			//+0x1C
#define XSDT_AML_VERSION_ADDRESS	0xF24A0			//+0x20
#define XSDT_TABLE_ENTRY_ADDRESS	0xF24A4			//+0x24

#define MADT_ADDRESS	0xF2500
#define MADT_SIG_ADDRESS			MADT_ADDRESS	//+0
#define MADT_LENGTH_ADDRESS			0xF2504			//+0x4
#define MADT_REVISION_ADDRESS		0xF2508			//+0x8
#define MADT_CHECKSUM_ADDRESS		0xF2509			//+0x9
#define MADT_OEM_ID_ADDRESS			0xF250A			//+0xA
#define MADT_OEM_TABLE_ID_ADDRESS	0xF2510			//+0x10
#define MADT_OEM_REVISION_ADDRESS	0xF2518			//+0x18
#define MADT_CREATOR_ID_ADDRESS		0xF251C			//+0x1C
#define MADT_AML_VERSION_ADDRESS	0xF2520			//+0x20
#define MADT_ENTRY_ADDRESS			0xF2524			//+0x24
#define MADT_FLAGS_ADDRESS			0xF2528			//+0x28

#define APIC_ENTRY1_TYPE_ADDRESS		0xF252C			//+0	1byte
#define APIC_ENTRY1_LENGTH_ADDRESS		0xF252D			//+1	1byte
#define APIC_ENTRY1_UID_ADDRESS			0xF252E			//+2	1bytes
#define APIC_ENTRY1_LAPIC_ID_ADDRESS	0xF252F			//+3	1bytes
#define APIC_ENTRY1_FLAGS_ADDRESS		0xF2530			//+4	4bytes

#define APIC_ENTRY2_TYPE_ADDRESS		0xF2534			//+0	1byte
#define APIC_ENTRY2_LENGTH_ADDRESS		0xF2535			//+1	1byte
#define APIC_ENTRY2_UID_ADDRESS			0xF2536			//+2	1bytes
#define APIC_ENTRY2_LAPIC_ID_ADDRESS	0xF2537			//+3	1bytes
#define APIC_ENTRY2_FLAGS_ADDRESS		0xF2538			//+4	4bytes

#define APIC_ENTRY3_TYPE_ADDRESS		0xF253C			//+0	1byte
#define APIC_ENTRY3_LENGTH_ADDRESS		0xF253D			//+1	1byte
#define APIC_ENTRY3_UID_ADDRESS			0xF253E			//+2	1bytes
#define APIC_ENTRY3_LAPIC_ID_ADDRESS	0xF253F			//+3	1bytes
#define APIC_ENTRY3_FLAGS_ADDRESS		0xF2540			//+4	4bytes

#define XSDT_SIG_OEM_TABLE_ID	"VIRTNUC7"
#define XSDT_OEM_TABLE_ID_SIZE	8U

#define XSDT_CREATOR_ID			"INTL"
#define XSDT_CREATOR_ID_SIZE	4U

#define RSDP_SIG_RSDP			"RSD PTR " /* Root System Description Ptr */
#define RSDP_SIGNATURE_LEN	8

#define ACPI_SIG_XSDT			"XSDT" /* Extended  System Description Table */
#define ACPI_SIG_XSDT_SIZE		4U

#define ACPI_SIG_MADT			"APIC" /* Multiple APIC Description Table */
#define ACPI_SIG_MADT_SIZE		4U

#define ACPI_SIG_OEM_ID			"ACRN  "
#define ACPI_OEM_ID_SIZE 6U

#define ACPI_SIG_OEM_TABLE_ID	XSDT_SIG_OEM_TABLE_ID
#define ACPI_OEM_TABLE_ID_SIZE	XSDT_OEM_TABLE_ID_SIZE

#define ACPI_CREATOR_ID			XSDT_CREATOR_ID
#define	ACPI_CREATOR_ID_SIZE	XSDT_CREATOR_ID_SIZE


#define CONFIG_MAX_PCPU_NUM 4U

#define XSDT_SIZE					0x2C
#define MADT_SIZE					0x44

#define __packed        __attribute__((packed))

#endif
