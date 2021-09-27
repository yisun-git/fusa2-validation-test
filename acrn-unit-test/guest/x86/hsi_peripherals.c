/*
 * Copyright (C) 2020 Intel Corporation. All rights reserved.
 *
 * Author:              yanting.jiang@intel.com
 * Date :                       2020/3/16
 * Description:         Test case/code for FuSa HSI (Hardware Software Interface Test)
 *
 * Modify Author:              hexiangx.li@intel.com
 * Date :                       2020/11/03
 */
#define USE_DEBUG
#include "processor.h"
#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "delay.h"
#include "apic.h"
#include "isr.h"
#include "atomic.h"
#include "asm/spinlock.h"
#include "asm/io.h"
#include "fwcfg.h"
#include "xsave.h"
#include "debug_print.h"
#include <linux/pci_regs.h>

#define RTC_INDEX_REG                           0x70
#define RTC_TARGET_REG		                0x71
#define SECOND_INDEX                            0x0
#define MINUTE_INDEX                            0x2
#define HOUR_INDEX                              0x4
#define DAY_OF_WEEK_INDEX	                0x6
#define DAY_OF_MONTH_INDEX	                0x7
#define MONTH_INDEX		                0x8
#define YEAR_INDEX		                0x9
#define CENTURY_INDEX                           0x32
#define A_INDEX		                        0xA
#define B_INDEX		0xB
#define RTC_REG_A_CHECK_BIT (1UL << 7)
#define TSC_TIMER_VEC               0x0E0U
#define LVT_TIMER_ONESHOT                       (0 << 17)
#define PCI_CONFIG_ADDR(dev, reg)	((0x1 << 31) | (dev << 8) | (reg & 0xfc))

typedef struct {
	int case_id;
	const char *case_name;
	void (*case_fun)(void);
} st_case_suit_peri;

static u16 pci_cfg_readw(u16 dev, u8 offset)
{
	outl(PCI_CONFIG_ADDR(dev, offset), 0xCF8);
	return inw(0xCFC + (offset & 0x3));
}

static u8 pci_cfg_readb(u16 dev, u8 offset)
{
	outl(PCI_CONFIG_ADDR(dev, offset), 0xCF8);
	return inb(0xCFC + (offset & 0x3));
}

static void pci_cfg_writew(u16 dev, u8 offset, u16 val)
{
	outl(PCI_CONFIG_ADDR(dev, offset), 0xCF8);
	outw(val, 0xCFC + (offset & 0x3));
}

static void pci_cfg_writeb(u16 dev, u8 offset, u8 val)
{
	outl(PCI_CONFIG_ADDR(dev, offset), 0xCF8);
	outb(val, 0xCFC + (offset & 0x3));
}

/**/
/********************************************/
/*          timer calibration  */
/********************************************/
uint64_t tsc_hz;
uint64_t apic_timer_hz;
uint64_t cpu_hz;
uint64_t bus_hz;
static void tsc_calibrate(void)
{
	u32 eax_denominator;
	u32 ebx_numerator;
	u32 ecx_crystal_hz;
	u32 reserved;
	u32 eax_base_mhz = 0;
	u32 ebx_max_mhz = 0;
	u32 ecx_bus_mhz = 0;
	u32 edx;

	__asm volatile("cpuid"
		: "=a"(eax_denominator), "=b"(ebx_numerator), "=c"(ecx_crystal_hz), "=d"(reserved)
		: "0" (0x15), "2" (0)
		: "memory");

	debug_print("crystal_hz:%u\n\r", ecx_crystal_hz);
	if (ecx_crystal_hz != 0) {
		tsc_hz = ((uint64_t) ecx_crystal_hz *
			ebx_numerator) / eax_denominator;
		apic_timer_hz = ecx_crystal_hz;
	} else {
		__asm volatile("cpuid"
			: "=a"(eax_base_mhz), "=b"(ebx_max_mhz), "=c"(ecx_bus_mhz), "=d"(edx)
			: "0" (0x16), "2" (0)
			: "memory");

		tsc_hz = (uint64_t) eax_base_mhz * 1000000U;
		apic_timer_hz = tsc_hz * eax_denominator / ebx_numerator;
	}

	cpu_hz = eax_base_mhz * 1000000U;
	bus_hz = ecx_bus_mhz * 1000000U;
	debug_print("apic_timer_hz: %lu\n", apic_timer_hz);
	debug_print("tsc_hz: %lu\n", tsc_hz);
	debug_print("cpu_hz: %lu\n", cpu_hz);
	debug_print("bus_hz: %lu\n", bus_hz);

	/* apic_timer_hz: 23863636; 16C 2154H
	 * tsc_hz: 2100000000
	 * cpu_hz: 2100000000
	 * bus_hz: 100000000
	 */
}

/* Following are interrupt handlers */
static atomic_t lapic_timer_isr_count;
static void lapic_timer_isr(isr_regs_t *regs)
{
	(void) regs;
	atomic_inc(&lapic_timer_isr_count);

	eoi();
}

/*
 * @brief case name: HSI_Peripherals_Features_RTC_001
 *
 * Summary: On native board, read RTC target configuration registers AH , verify that bit 7 is 0.
 */
static __unused void hsi_rqmid_36056_peripherals_features_rtc_001(void)
{
	u32 chk = 0;
	u8 reg_a_val = 0;

	/* test read RTC register AH */
	outb(A_INDEX, RTC_INDEX_REG);
	reg_a_val = inb(RTC_TARGET_REG);

	if ((reg_a_val & RTC_REG_A_CHECK_BIT) == 0) {
		chk++;
	}

	debug_print("RTC register A value:%x\n", reg_a_val);
	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Peripherals_Features_RTC_002
 *
 * Summary: On native board, read RTC target date/timer registers:
 * second, minute, hour, day of week, day of month, year, check the reads
 * are successful; arm a 5-second apic timer to verify the increment of RTC time.
 */
static __unused void hsi_rqmid_36057_peripherals_features_rtc_002(void)
{
	u32 chk = 0;

	u8 index[8] = {SECOND_INDEX, MINUTE_INDEX, HOUR_INDEX, DAY_OF_WEEK_INDEX, \
		DAY_OF_MONTH_INDEX, MONTH_INDEX, YEAR_INDEX, CENTURY_INDEX};
	u32 currtime[8] = {0};
	u32 newtime[8] = {0};
	u32 tm_year;
	u8 tm_bcd;
	u8 reg_val = 0;
	u8 during = 0;

	const unsigned int vec = TSC_TIMER_VEC;
	u64 initial_value = apic_timer_hz * 5;
	u32 lvtt;

	/* set RTC data mode: BCD, hour format: 24h */
	outb(B_INDEX, RTC_INDEX_REG);
	reg_val = inb(RTC_TARGET_REG);
	reg_val &= (~(1 << 2));
	reg_val |= (1 << 1);
	outb(reg_val, RTC_TARGET_REG);

	/* enable apic oneshot timer mode */
	irq_disable();
	atomic_set(&lapic_timer_isr_count, 0);
	handle_irq(vec, lapic_timer_isr);
	irq_enable();
	lvtt = LVT_TIMER_ONESHOT | vec;
	apic_write(APIC_LVTT, lvtt);
	apic_write(APIC_TDCR, APIC_TDR_DIV_1);

	/* read RTC date and time by writes and reads from I/O port 0x70 and 0x71 respectively */
	for (int i = 0; i < 8; i++) {
		outb(index[i], RTC_INDEX_REG);
		tm_bcd = inb(RTC_TARGET_REG);
		currtime[i] = ((tm_bcd & 0xf0) >> 4) * 10;
		currtime[i] += (tm_bcd & 0x0f);
		chk++;
	}

	/* arm apic oneshot timer to generate an interrupt */
	apic_write(APIC_TMICT, initial_value);

	while (atomic_read(&lapic_timer_isr_count) < 1) {}

	/* read RTC date and time again */
	for (int i = 0; i < 8; i++) {
		outb(index[i], RTC_INDEX_REG);
		tm_bcd = inb(RTC_TARGET_REG);
		newtime[i] = ((tm_bcd & 0xf0) >> 4) * 10;
		newtime[i] += (tm_bcd & 0x0f);
	}

	during = (newtime[0] < currtime[0]) ? (newtime[0] + 60) : newtime[0];
	during -= currtime[0];
	if ((during >= 4) && (during <= 6)) {
		chk++;
	}

	tm_year = ((u32)currtime[7] * 100) + currtime[6];
	debug_print("round 1 - Date: %d/%d/%d  Week: %d  Time: %d:%d:%d\n", \
		tm_year, currtime[5], currtime[4], currtime[3], currtime[2], currtime[1], currtime[0]);

	tm_year = ((u32)newtime[7] * 100) + newtime[6];
	debug_print("round 2 - Date: %d/%d/%d  Week: %d  Time: %d:%d:%d\n", \
		tm_year, newtime[5], newtime[4], newtime[3], newtime[2], newtime[1], newtime[0]);

	report("%s", (chk == 9), __FUNCTION__);
}

/*
 * @brief case name: HSI_Peripherals_Features_RTC_003
 *
 * Summary: On native board, Loop read RTC register A to get the time duration between
 * a “bit 7 value from 0 to 1” and (from that on) the most recent
 * “bit 7 value from 1 to 0”, which is the RTC update cycle.
 * verify that the time duration is less than (488 + 1984) us
 */
static __unused void hsi_rqmid_43752_peripherals_features_rtc_003(void)
{
	u32 chk = 0;
	u64 tsc_0to1 = 0;
	u64 tsc_1to0 = 0;

	/* test read RTC register AH */
	outb(A_INDEX, RTC_INDEX_REG);

	/* make sure rtc register A bit7 is 0 */
	while ((inb(RTC_TARGET_REG) & RTC_REG_A_CHECK_BIT) != 0) {
		nop();
	}

	/* wait for bit7 from 0 to 1*/
	while ((inb(RTC_TARGET_REG) & RTC_REG_A_CHECK_BIT) == 0) {
		nop();
	}

	/* record the current tsc */
	tsc_0to1 = rdtsc();

	/* wait for bit7 from 1 to 0 */
	while ((inb(RTC_TARGET_REG) & RTC_REG_A_CHECK_BIT) != 0) {
		nop();
	}
	/* record tsc */
	tsc_1to0 = rdtsc();

	u64 tsc_diff = tsc_1to0 - tsc_0to1;

	/* get duration time(us) */
	double duration = ((double)tsc_diff / tsc_hz) * 1000000;
	const double max_duration = 488.00 + 1984.00;

	/* verify that the time duration is less than (488 + 1984) us */
	if (duration < max_duration) {
		chk++;
	}
	debug_print("tsc_diff:%lx duration:%lx max_duration:%lx\n", tsc_diff, (u64)duration, (u64)max_duration);
	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Peripherals_Features_PCI_configuration_space_001
 *
 * Summary: On native board, Access PCI configuration space of USB
 * controller device, should get information about Device ID, Header Type,
 * Cache line Size, Status, and Capabilities Pointer.
 */
static __unused void hsi_rqmid_36112_peripherals_features_pci_configuration_space_001(void)
{
	u32 chk = 0;

	/* Test with PCI USB device 00:14.0 */
	u16 usb_bdf = (0x14 << 3);
	u16 reg_org_w;
	u8 reg_org_b;

	/* read Device ID Register */
	reg_org_w = pci_cfg_readw(usb_bdf, PCI_DEVICE_ID);
	if (reg_org_w == 0x9d2f) {
		chk++;
	}

	/* read Header Type Register */
	reg_org_b = pci_cfg_readb(usb_bdf, PCI_HEADER_TYPE);
	if (reg_org_b == 0x80) {
		chk++;
	}

	/* read Cache Line Size Register */
	reg_org_b = pci_cfg_readb(usb_bdf, PCI_CACHE_LINE_SIZE);
	if (reg_org_b == 0x0) {
		chk++;
	}

	/* read Status Register */
	reg_org_w = pci_cfg_readw(usb_bdf, PCI_STATUS);
	chk++;

	/* read Capabilities Pointer Register */
	reg_org_b = pci_cfg_readb(usb_bdf, PCI_CAPABILITY_LIST);
	chk++;

	report("%s", (chk == 5), __FUNCTION__);
}

/*
 * @brief case name: HSI_Peripherals_Features_PCI_configuration_space_002
 *
 * Summary: On native board, Access PCI configuration space of USB
 * controller device, write Command register and Interrupt Line register,
 * read these 2 registers to check the writes are successful.
 */
static __unused void hsi_rqmid_36124_peripherals_features_pci_configuration_space_002(void)
{
	u32 chk = 0;

	/* Test with PCI USB device 00:14.0 */
	u16 usb_bdf = (0x14 << 3);
	u16 reg_org_w;
	u16 reg_val_w;
	u16 tmp_val;
	u8 reg_org_b;
	u8 reg_val_b;

	/* read & write Command Register */
	reg_org_w = pci_cfg_readw(usb_bdf, PCI_COMMAND);
	tmp_val = reg_org_w | (1 << 2) | (1 << 6) | (1 << 8) | (1 << 10);
	pci_cfg_writew(usb_bdf, PCI_COMMAND, tmp_val);
	reg_val_w = pci_cfg_readw(usb_bdf, PCI_COMMAND);

	if (reg_val_w == tmp_val) {
		chk++;
	}
	/* recover register value */
	pci_cfg_writew(usb_bdf, PCI_COMMAND, reg_org_w);
	debug_print("command: org 0x%x, new 0x%x\n", reg_org_w, reg_val_w);

	/* read & write Interrupt Line Register */
	reg_org_b = pci_cfg_readb(usb_bdf, PCI_INTERRUPT_LINE);
	tmp_val = 0xa;
	pci_cfg_writeb(usb_bdf, PCI_INTERRUPT_LINE, tmp_val);
	reg_val_b = pci_cfg_readb(usb_bdf, PCI_INTERRUPT_LINE);
	if (reg_val_b == tmp_val) {
		chk++;
	}
	/* recover register value */
	pci_cfg_writeb(usb_bdf, PCI_INTERRUPT_LINE, reg_org_b);
	debug_print("interrupt line: org 0x%x, new 0x%x\n", reg_org_b, reg_val_b);

	report("%s", (chk == 2), __FUNCTION__);
}

static st_case_suit_peri case_suit_peri[] = {
	{
		.case_fun = hsi_rqmid_36056_peripherals_features_rtc_001,
		.case_id = 36056,
		.case_name = "HSI_Peripherals_Features_RTC_001",
	},

	{
		.case_fun = hsi_rqmid_36057_peripherals_features_rtc_002,
		.case_id = 36057,
		.case_name = "HSI_Peripherals_Features_RTC_002",
	},

	{
		.case_fun = hsi_rqmid_43752_peripherals_features_rtc_003,
		.case_id = 43752,
		.case_name = "HSI_Peripherals_Features_RTC_003",
	},

	{
		.case_fun = hsi_rqmid_36112_peripherals_features_pci_configuration_space_001,
		.case_id = 36112,
		.case_name = "HSI_Peripherals_Features_PCI_configuration_space_001",
	},

	{
		.case_fun = hsi_rqmid_36124_peripherals_features_pci_configuration_space_002,
		.case_id = 36124,
		.case_name = "HSI_Peripherals_Features_PCI_configuration_space_002",
	},

};

static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t HSI peripherals features case list: \n\r");
#ifdef IN_NATIVE
	int i;
	for (i = 0; i < ARRAY_SIZE(case_suit_peri); i++) {
		printf("\t Case ID: %d Case name: %s\n\r", case_suit_peri[i].case_id, case_suit_peri[i].case_name);
	}
#else
#endif
	printf("\t \n\r \n\r");
}

int main(void)
{
	setup_idt();
	asm volatile("fninit");

	print_case_list();
	tsc_calibrate();

#ifdef IN_NATIVE
	int i;
	for (i = 0; i < ARRAY_SIZE(case_suit_peri); i++) {
		case_suit_peri[i].case_fun();
	}
#else
#endif
	return report_summary();
}

