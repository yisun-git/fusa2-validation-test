/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * Author:              yanting.jiang@intel.com
 * Date :                       2020/3/16
 * Description:         Test case/code for FuSa HSI (Hardware Software Interface Test)
 *
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

#define IOAPIC_TEST_VEC             0x024U
#define PIC_TEST_VEC                0x020U
#define TSC_TIMER_VEC               0x0E0U

#define TICKS_PER_SEC               (1000000000 / 100)

static atomic_t ioapic_pit_isr_count;
static void ioapic_pit_isr(isr_regs_t *regs)
{
	(void) regs;
	atomic_inc(&ioapic_pit_isr_count);

	eoi();
}

static atomic_t pic_pit_isr_count;
static void pic_pit_isr(isr_regs_t *regs)
{
	(void) regs;
	atomic_inc(&pic_pit_isr_count);

	outb(0x20, 0x20);  /* send eoi to PIC */
}

/*
 * Description: Initialize Master PIC and Slave PIC
 */
static void init_8259a_pic(void)
{
	/* icw1 fixed */
	outb(0x11, 0x20); /* master port A */
	outb(0x11, 0xA0); /* slave port A */
	/* icw2 */
	outb(PIC_TEST_VEC, 0x21); /* master offset of 0x20 in the IDT */
	outb(PIC_TEST_VEC + 8, 0xA1); /* slave offset of 0x28 in the IDT */
	/* icw3 fixed */
	outb(0x04, 0x21); /* slaves attached to IR line 2 */
	outb(0x02, 0xA1); /* this slave in IR line 2 of master */
	/* icw4 fixed*/
	outb(0x01, 0x21); /* set as master */
	outb(0x01, 0xA1); /* set as slave */
}

/*
 * Description: Arm/Disarm PIT timer interrupt with 5ms
 */
static void arm_5ms_PIT(bool flag)
{
	if (flag) {
		outb(0x30, 0x43);
		outb(0x17, 0x40); /* set MSB */
		outb(0x4e, 0x40); /* set LSB */
	} else {
		outb(0x30, 0x43);
		outb(0, 0x40); /* set MSB */
		outb(0, 0x40); /* set LSB */
	}
}

/*
 * @brief case name: HSI_Interrupt_Controllers_IOAPIC_masking_001
 *
 * Summary: Under 64 bit mode on native board, arm PIT timer,
 * when clear the mask bit in RTE of IOAPIC, PIT timer interrupt is delivered;
 * when set the mask bit in RTE of IOAPIC, PIT timer interrupt is not delivered.
 */
static void hsi_rqmid_35947_interrupt_controllers_ioapic_masking_001(void)
{
	u32 chk = 0;
	int cnt;
	const unsigned int vec = IOAPIC_TEST_VEC;

	irq_disable();
	mask_pic_interrupts();
	atomic_set(&ioapic_pit_isr_count, 0);
	handle_irq(vec, ioapic_pit_isr);

	/* PIT interrupt with mask bit of RTE in IOAPIC clear */
	irq_enable();
	ioapic_set_redir(0x02, vec, TRIGGER_EDGE);
	set_mask(0x02, false);

	cnt = 50 * TICKS_PER_SEC;
	arm_5ms_PIT(true);
	test_delay(5);
	while (atomic_read(&ioapic_pit_isr_count) < 1) {
		cnt--;
		if (cnt == 0) {
			break;
		}
	}
	arm_5ms_PIT(false);

	if (atomic_read(&ioapic_pit_isr_count) >= 1) {
		chk++;
		debug_print("Test clear mask bit of RTE in IOAPIC pass. PIT interrupt (IRQ 0) is delivered.\n");
	} else {
		debug_print("Test clear mask bit of RTE in IOAPIC fail.\n");
	}

	/* PIT interrupt with mask bit of RTE in IOAPIC set */
	set_mask(0x02, true);
	atomic_set(&ioapic_pit_isr_count, 0);
	cnt = 50 * TICKS_PER_SEC;

	arm_5ms_PIT(true);
	test_delay(5);
	while (atomic_read(&ioapic_pit_isr_count) < 1) {
		cnt--;
		if (cnt == 0) {
			break;
		}
	}
	arm_5ms_PIT(false);

	if (atomic_read(&ioapic_pit_isr_count) == 0) {
		chk++;
		debug_print("Test set mask bit of RTE in IOAPIC pass. PIT interrupt (IRQ 0) is not delivered.\n");
	} else {
		debug_print("Test set mask bit of RTE in IOAPIC fail.\n");
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Interrupt_Controllers_PIC_masking_001
 *
 * Summary: Under 64 bit mode on native board, arm PIT timer,
 * when enable PIC interrupts, PIT timer interrupt is delivered;
 * when disable PIC interrupts, PIT timer interrupt is not delivered.
 */
static void hsi_rqmid_35948_interrupt_controllers_pic_masking_001(void)
{
	u32 chk = 0;
	u32 cnt;
	static struct spinlock pic_lock;
	const unsigned int vec = PIC_TEST_VEC;

	irq_disable();
	atomic_set(&pic_pit_isr_count, 0);
	handle_irq(vec, pic_pit_isr);

	irq_enable();
	spin_lock(&pic_lock);
	init_8259a_pic();

	/* PIT timer with PIC interrupts enabled */
	/* ocw1: unmask IRQ 0-7*/
	outb(0x0, 0x21);
	cnt = 50 * TICKS_PER_SEC;
	arm_5ms_PIT(true);
	test_delay(5);

	while (atomic_read(&pic_pit_isr_count) < 1) {
		cnt--;
		if (cnt == 0) {
			break;
		}
	}
	arm_5ms_PIT(false);

	if (atomic_read(&pic_pit_isr_count) >= 1) {
		chk++;
		debug_print("Test enable PIC interrupts pass. PIT timer (IRQ 0) is delivered.\n");
	} else {
		debug_print("Test enable PIC interrupts fail.\n");
	}

	atomic_set(&pic_pit_isr_count, 0);
	cnt = 50 * TICKS_PER_SEC;

	/* PIT timer with PIC interrupts disabled */
	/* ocw1: mask IRQ 0-7*/
	outb(0xff, 0x21);
	arm_5ms_PIT(true);
	test_delay(5);
	while (atomic_read(&pic_pit_isr_count) < 1) {
		cnt--;
		if (cnt == 0) {
			break;
		}
	}
	arm_5ms_PIT(false);

	if (atomic_read(&pic_pit_isr_count) == 0) {
		chk++;
		debug_print("Test disable PIC interrupts pass. PIT timer (IRQ 0) is not delivered.\n");
	} else {
		debug_print("Test disable PIC interrupts fail.\n");
	}

	spin_unlock(&pic_lock);
	report("%s", (chk == 2), __FUNCTION__);
}

static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t HSI interrupt controllers features test case list: \n\r");
#ifdef IN_NATIVE
	printf("\t Case ID: %d Case name: %s\n\r", 35947u, "HSI interrupt controllers ioapic masking 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35948u, "HSI interrupt controllers pic masking 001");
#else
#endif
	printf("\t \n\r \n\r");
}

int main(void)
{
	setup_idt();
	asm volatile("fninit");

	print_case_list();

#ifdef IN_NATIVE
	hsi_rqmid_35947_interrupt_controllers_ioapic_masking_001();
	hsi_rqmid_35948_interrupt_controllers_pic_masking_001();

#else
#endif
	return report_summary();
}

