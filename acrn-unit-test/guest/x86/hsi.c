/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * Author:              yanting.jiang@intel.com
 * Date :                       2020/3/16
 * Description:         Test case/code for FuSa HSI (Hardware Software Interface Test)
 *
 */

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
#include "hsi.h"

#define IOAPIC_TEST_VEC             0x024U
#define PIC_TEST_VEC                0x020U
#define TSC_TIMER_VEC               0x0E0U

#define TICKS_PER_SEC               (1000000000 / 100)
#define TEST_WAIT_MULTIPLIER        1000

#define LVT_TIMER_PERIODIC          (1 << 17)
#define LVT_TIMER_TSCDEADLINE       (2 << 17)

#define TARGET_AP                   1

static volatile u8 ap_nums;

/* start_run_id is used to identify which test case is running. */
static volatile int start_run_id = 0;

/* wait_bp/wait_ap are used to sync between bp and aps */
static atomic_t wait_bp;
static atomic_t wait_ap;

static volatile int target_ap_startup = 0;
static atomic_t ap_run_nums;
static atomic_t ap_chk;

static __unused void ap_sync(void)
{
	while (atomic_read(&wait_bp) == 0) {
		debug_print("%d %d\n", atomic_read(&wait_ap), atomic_read(&wait_bp));
		test_delay(1);
	}
	atomic_dec(&wait_bp);
}

static __unused void bp_sync(void)
{
	while (atomic_read(&wait_ap) != ap_nums) {
		debug_print("%d %d\n", atomic_read(&wait_ap), atomic_read(&wait_bp));
		test_delay(1);
	}
	atomic_set(&wait_ap, 0);
}

/* Following are interrupt handlers */
static atomic_t lapic_timer_isr_count;
static void lapic_timer_isr(isr_regs_t *regs)
{
	(void) regs;
	atomic_inc(&lapic_timer_isr_count);

	eoi();
}

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
 * @brief case name: HSI_Generic_Processor_Features_EFLAGS_Interrupt_Flag_001
 *
 * Summary: Under 64 bit mode on native board, arm apic timer,
 * when set EFLAGS.IF, apic timer interrupt is delivered;
 * when clear EFLAGS.IF, apic timer interrupt is not delivered.
 */
static void hsi_rqmid_35949_generic_processor_features_eflags_interrupt_flag_001(void)
{
	u32 chk = 0;
	u32 lvtt;
	const unsigned int vec = TSC_TIMER_VEC;
	u32 initial_value = 1000;

	irq_disable();
	atomic_set(&lapic_timer_isr_count, 0);
	handle_irq(vec, lapic_timer_isr);

	/* TSC periodic timer with IRQ enabled */
	irq_enable();
	/* Enable TSC periodic timer mode */
	lvtt = LVT_TIMER_PERIODIC | vec;
	apic_write(APIC_LVTT, lvtt);
	/* Arm the timer */
	apic_write(APIC_TMICT, initial_value);
	test_delay(5);

	//initial_value *= TEST_WAIT_MULTIPLIER;
	while (atomic_read(&lapic_timer_isr_count) < 1) {
		initial_value--;
		if (initial_value == 0) {
			break;
		}
	}
	apic_write(APIC_TMICT, 0);

	if (atomic_read(&lapic_timer_isr_count) >= 1) {
		chk++;
		debug_print("Test set EFLAGS.IF flag pass. LAPIC timer interrupt is delivered\n");
	} else {
		debug_print("Test set EFLAGS.IF flag fail.\n");
	}

	/* TSC periodic timer with IRQ disabled */
	irq_disable();
	initial_value = 1000;
	atomic_set(&lapic_timer_isr_count, 0);
	apic_write(APIC_TMICT, initial_value);
	test_delay(5);

	//initial_value *= TEST_WAIT_MULTIPLIER;
	while (atomic_read(&lapic_timer_isr_count) < 1) {
		initial_value--;
		if (initial_value == 0) {
			break;
		}
	}
	apic_write(APIC_TMICT, 0);

	if (atomic_read(&lapic_timer_isr_count) == 0) {
		chk++;
		debug_print("Test clear EFLAGS.IF flag pass. LAPIC timer interrupt is not delivered\n");
	} else {
		debug_print("Test clear EFLAGS.IF flag fail.\n");
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Inter_Processor_Interrupt_001
 *
 * Summary: Under 64 bit mode on native board, BP send INIT IPI
 * and startup IPI to target AP, target AP should restart successfully.
 */
static void hsi_rqmid_35951_generic_processor_features_inter_processor_interrupt_001(void)
{
	int apic_id = get_lapicid_map(TARGET_AP);
	start_run_id = 35951;

	bp_sync();
	test_delay(5);
	/*issue sipi to awake AP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT, apic_id);
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_STARTUP, apic_id);

	/* wait for 1s fot ap to finish startup*/
	test_delay(1000);
	start_run_id = 0;
	report("%s", (target_ap_startup == 2), __FUNCTION__);
}

static void hsi_rqmid_35951_ap(void)
{
	if (get_cpu_id() == TARGET_AP) {
		target_ap_startup++;
		if (target_ap_startup == 1) {
			atomic_set(&wait_ap, ap_nums);
		}
	}
}

/*
 * @brief case name: HSI_Generic_Processor_Features_x2APIC_mode_001
 *
 * Summary: Under 64 bit mode on native board, on each physical processor,
 * set bit[11:10] of IA32_APIC_BASE MSR to enable x2APIC mode should be successfully.
 */
static void hsi_rqmid_35950_generic_processor_features_x2apic_mode_001(void)
{
	u32 bp_chk = 0;
	struct cpuid id;
	u64 base_org;
	u8 ap_nums = fwcfg_get_nb_cpus() - 1;
	atomic_set(&ap_chk, 0);

	start_run_id = 35950;

	id = cpuid(LAPIC_CPUID_APIC_CAPABILITY);

	if (id.c & LAPIC_CPUID_APIC_CAP_X2APCI) {
		base_org = rdmsr(MSR_IA32_APIC_BASE);
		/* Enable LAPIC in xAPIC mode and x2APIC mode */
		wrmsr(MSR_IA32_APIC_BASE, (base_org | EN_xAPIC | EN_x2APIC));

		if ((rdmsr(MSR_IA32_APIC_BASE) & (EN_xAPIC | EN_x2APIC)) == (EN_xAPIC | EN_x2APIC)) {
			bp_chk++;
		}
		wrmsr(MSR_IA32_APIC_BASE, base_org);
	}
	test_delay(100);

	start_run_id = 0;
	report("%s", ((bp_chk == 1) && (atomic_read(&ap_chk) == ap_nums)), __FUNCTION__);
}

static void hsi_rqmid_35950_ap(void)
{
	struct cpuid id;
	u64 base_org;

	id = cpuid(LAPIC_CPUID_APIC_CAPABILITY);

	if (id.c & LAPIC_CPUID_APIC_CAP_X2APCI) {
		base_org = rdmsr(MSR_IA32_APIC_BASE);
		/* Enable LAPIC in xAPIC mode and x2APIC mode */
		wrmsr(MSR_IA32_APIC_BASE, (base_org | EN_xAPIC | EN_xAPIC));

		if ((rdmsr(MSR_IA32_APIC_BASE) & (EN_xAPIC | EN_xAPIC)) == (EN_xAPIC | EN_xAPIC)) {
			atomic_inc(&ap_chk);
		}
		wrmsr(MSR_IA32_APIC_BASE, base_org);
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
	printf("\t HSI test case list: \n\r");
#ifdef IN_NATIVE
	printf("\t Case ID: %d Case name: %s\n\r", 35951u, "HSI generic processor features " \
		"inter processor interrupt 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35950u, "HSI generic processor features x2apic mode 001");

	printf("\t Case ID: %d Case name: %s\n\r", 35949u, "HSI generic processor features eflags interrupt flag 001");

	printf("\t Case ID: %d Case name: %s\n\r", 35947u, "HSI interrupt controllers ioapic masking 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35948u, "HSI interrupt controllers pic masking 001");
#else
#endif
	printf("\t \n\r \n\r");
}

int ap_main(void)
{
#ifdef IN_NATIVE
	debug_print("ap %d starts running \n", get_cpu_id());
	while (start_run_id == 0) {
		test_delay(5);
	}

	while (start_run_id != 35951) {
		test_delay(5);
	}
	debug_print("ap start to run case %d\n", start_run_id);
	hsi_rqmid_35951_ap();

	while (start_run_id != 35950) {
		test_delay(5);
	}
	debug_print("ap start to run case %d\n", start_run_id);
	hsi_rqmid_35950_ap();

#endif
	return 0;
}

int main(void)
{
	setup_idt();
	asm volatile("fninit");

	print_case_list();
	atomic_set(&ap_run_nums, 0);
	ap_nums = fwcfg_get_nb_cpus();

#ifdef IN_NATIVE
	hsi_rqmid_35951_generic_processor_features_inter_processor_interrupt_001();
	hsi_rqmid_35950_generic_processor_features_x2apic_mode_001();

	hsi_rqmid_35949_generic_processor_features_eflags_interrupt_flag_001();

	hsi_rqmid_35947_interrupt_controllers_ioapic_masking_001();
	hsi_rqmid_35948_interrupt_controllers_pic_masking_001();
#else
#endif
	start_run_id = 0;
	return report_summary();
}

