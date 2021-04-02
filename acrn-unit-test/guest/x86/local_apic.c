/*
 * Copyright (c) 2019 Intel Corporation. All rights reserved.
 * Test mode: 64-bit
 */
#include "libcflat.h"
#include "register_op.h"
#include "pci_util.h"
#include "pci_check.h"
#include "e1000_regs.h"
#include "e1000e.h"
#include "apic.h"
#include "vm.h"
#include "smp.h"
#include "desc.h"
#include "isr.h"
#include "msr.h"
#include "atomic.h"
#include "local_apic.h"
#include "fwcfg.h"
#include "local_apic.h"
#include "misc.h"
#include "delay.h"

/* common code */
#define LAPIC_REG_MASK  0x0FFFFFFFFUL
#define APIC_RESET_BASE 0xfee00000
#define LAPIC_APIC_STRUCT(reg)  (((reg) >> 2) + 0)
#ifndef APIC_MSR_CR8
#define APIC_MSR_CR8            0x400
#endif
#ifndef APIC_MSR_CR81
#define APIC_MSR_CR81           0x410
#endif
#ifndef APIC_MSR_IA32_APICBASE
#define APIC_MSR_IA32_APICBASE  0x420
#endif
#ifndef APIC_MSR_IA32_APICBASE1
#define APIC_MSR_IA32_APICBASE1 0x430
#endif
#ifndef APIC_MSR_IA32_TSCDEADLINE
#define APIC_MSR_IA32_TSCDEADLINE       0x440
#endif
#ifndef APIC_MSR_IA32_TSCDEADLINE1
#define APIC_MSR_IA32_TSCDEADLINE1      0x450
#endif
#ifndef APIC_LVTCMCI
#define APIC_LVTCMCI 0x2f0
#endif
#define APIC_RESET_BASE 0xfee00000

#ifndef LAPIC_INIT_BP_BASE_ADDR
#define LAPIC_INIT_BP_BASE_ADDR 0x8000
#endif
#ifndef LAPIC_INIT_AP_BASE_ADDR
#define LAPIC_INIT_AP_BASE_ADDR 0x8800
#endif
#ifndef LAPIC_INIT_APIC_SIZE
#define LAPIC_INIT_APIC_SIZE    0x800
#endif

#ifndef LAPIC_PRIVATE_MEM_ADDR
#define LAPIC_PRIVATE_MEM_ADDR  0x8000UL
#endif
#ifndef LAPIC_PRIVATE_SIZE
#define LAPIC_PRIVATE_SIZE      0x800UL
#endif

#ifndef LAPIC_IRR_INDEX
#define LAPIC_IRR_INDEX(idx)    ((idx) << 4)
#endif
#ifndef LAPIC_ISR_INDEX
#define LAPIC_ISR_INDEX(idx)    ((idx) << 4)
#endif
#ifndef LAPIC_TMR_INDEX
#define LAPIC_TMR_INDEX(idx)    ((idx) << 4)
#endif

#define LAPIC_CPUID_APIC_CAPABILITY 1
#define LAPIC_CPUID_APIC_CAP_OK     (0x1U << 9)
#define LAPIC_CPUID_APIC_CAP_X2APCI (0x1U << 21)
#define LAPIC_TIMER_CAPABILITY          6
#define LAPIC_TIMER_CAPABILITY_ARAT     (0x1U << 2)

#define LAPIC_FIRST_VEC     16
#define LAPIC_MAX_VEC       255
#define LAPIC_TIMER_WAIT_MULTIPLIER 1000U
#define LAPIC_TIMER_INITIAL_VAL 1000U
#define LAPIC_TEST_VEC      0x0E1U
#define LAPIC_SELF_VEC		0xE2U
#define LAPIC_ONESHOT_TIMER_VEC	0xE3U
#define LAPIC_PERIODIC_TIMER_VEC	0xE4U
#define TSC_DEADLINE_TIMER_VECTOR 0x40U
#define LAPIC_TEST_INVALID_VEC  0U
#define LAPIC_TEST_INVALID_VEC1 15UL
#define LAPIC_TEST_VEC_HIGH 0x0E3U
#define LAPIC_INTR_TARGET_SELF  0U
#define LAPIC_INTR_TARGET_ID1   1U
#define LAPIC_INTR_TARGET_ID2   2U
#define LAPIC_MSR(reg)      (0x800U + ((unsigned)(reg) >> 4))
#define LAPIC_TPR_MAX       0x0FFU
#define LAPIC_TPR_MIN       0U
#define LAPIC_TPR_MID       0x20U
#define LAPIC_APIC_ID_VAL   0U
#define LAPIC_APIC_PPR_VAL  0U
#define LAPIC_APIC_LDR_VAL  0U
#define LAPIC_APIC_LVR_VAL  0U
#define LAPIC_APIC_RRR_VAL  0U
#define LAPIC_APIC_ISR0_VAL 0U
#define LAPIC_APIC_ISR1_VAL 0U
#define LAPIC_APIC_ISR2_VAL 0U
#define LAPIC_APIC_ISR3_VAL 0U
#define LAPIC_APIC_ISR4_VAL 0U
#define LAPIC_APIC_ISR5_VAL 0U
#define LAPIC_APIC_ISR6_VAL 0U
#define LAPIC_APIC_ISR7_VAL 0U
#define LAPIC_APIC_TMR0_VAL 0U
#define LAPIC_APIC_TMR1_VAL 0U
#define LAPIC_APIC_TMR2_VAL 0U
#define LAPIC_APIC_TMR3_VAL 0U
#define LAPIC_APIC_TMR4_VAL 0U
#define LAPIC_APIC_TMR5_VAL 0U
#define LAPIC_APIC_TMR6_VAL 0U
#define LAPIC_APIC_TMR7_VAL 0U
#define LAPIC_APIC_IRR0_VAL 0U
#define LAPIC_APIC_IRR1_VAL 0U
#define LAPIC_APIC_IRR2_VAL 0U
#define LAPIC_APIC_IRR3_VAL 0U
#define LAPIC_APIC_IRR4_VAL 0U
#define LAPIC_APIC_IRR5_VAL 0U
#define LAPIC_APIC_IRR6_VAL 0U
#define LAPIC_APIC_IRR7_VAL 0U
#define LAPIC_APIC_ESR_VAL  0U
#define LAPIC_APIC_TMCCT_VAL    0U
#define LAPIC_APIC_EOI_VAL  0
#define LAPIC_APIC_SELF_IPI_VAL 0
#define LAPIC_RESERVED_BIT(val) val

#define LAPIC_TEST_INVALID_MAX_VEC 15UL
#define APIC_DM_RESERVED 0x600

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;

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
__unused static void modify_apic_cr8_init_value()
{
	write_cr8(0x1);
}

__unused static void modify_apic_cmci_init_value()
{
	apic_write(APIC_CMCI, 0);
}

__unused static void modify_apic_lint0_init_value()
{
	apic_write(APIC_LVT0, 0);
}

__unused static void modify_apic_lint1_init_value()
{
	apic_write(APIC_LVT1, 0);
}

__unused static void modify_apic_lvterr_init_value()
{
	apic_write(APIC_LVTERR, 0);
}

__unused static void modify_apic_lvtthmr_init_value()
{
	apic_write(APIC_LVTTHMR, 0);
}

__unused static void modify_apic_lvtpc_init_value()
{
	apic_write(APIC_LVTPC, 0);
}

__unused static void modify_apic_lvttimer_init_value()
{
	apic_write(APIC_LVTT, 0);
}

__unused static void modify_apic_spiv_init_value()
{
	apic_write(APIC_SPIV, 0);
}

__unused static void modify_apic_icr_init_value()
{
	apic_write(APIC_ICR, 0xF);
}

__unused static void modify_apic_tpr_init_value()
{
	apic_write(APIC_TASKPRI, LAPIC_TPR_MAX);
}

__unused static void modify_apic_tmict_init_value()
{
	apic_write(APIC_TMICT, 0xF);
}

__unused static void modify_apic_tdcr_init_value()
{
	/* Divider == 1 */
	apic_write(APIC_TDCR, 0x0000000b);
}

__unused static void modify_apic_tscdeadline_init_value()
{
	wrmsr(MSR_IA32_TSCDEADLINE, 0xF);
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 38686:
		fp = modify_apic_cmci_init_value;
		ap_init_value_process(fp);
		break;
	case 38687:
		fp = modify_apic_lint0_init_value;
		ap_init_value_process(fp);
		break;
	case 38688:
		fp = modify_apic_lint1_init_value;
		ap_init_value_process(fp);
		break;
	case 38691:
		fp = modify_apic_lvterr_init_value;
		ap_init_value_process(fp);
		break;
	case 38692:
		fp = modify_apic_lvtthmr_init_value;
		ap_init_value_process(fp);
		break;
	case 38693:
		fp = modify_apic_lvtpc_init_value;
		ap_init_value_process(fp);
		break;
	case 38694:
		fp = modify_apic_lvttimer_init_value;
		ap_init_value_process(fp);
		break;
	case 38698:
		fp = modify_apic_spiv_init_value;
		ap_init_value_process(fp);
		break;
	case 38699:
		fp = modify_apic_icr_init_value;
		ap_init_value_process(fp);
		break;
	case 38700:
		fp = modify_apic_tpr_init_value;
		ap_init_value_process(fp);
		break;
	case 38707:
		fp = modify_apic_cr8_init_value;
		ap_init_value_process(fp);
		break;
	case 38708:
		fp = modify_apic_tmict_init_value;
		ap_init_value_process(fp);
		break;
	case 38710:
		fp = modify_apic_tdcr_init_value;
		ap_init_value_process(fp);
		break;
	case 38711:
		fp = modify_apic_tscdeadline_init_value;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif

void sleep(u64 ticks)
{
	for (int i = 1; i < ticks; i++) {
		asm volatile ("pause");
	}
}
int ipi_count;
void lapic_self_ipi_isr(isr_regs_t *regs)
{
	++ipi_count;
	eoi();
}
void lapic_ipi_isr_no_eoi(isr_regs_t *regs)
{
	++ipi_count;
}

volatile int tdt_isr_cnt = 0;
void tsc_deadline_timer_isr(isr_regs_t *regs)
{
	tdt_isr_cnt++;
	eoi();
}
volatile int lapic_timer_cnt;
void lapic_timer_handler(isr_regs_t *regs)
{
	lapic_timer_cnt++;
	eoi();
}
void start_dtimer(u64 ticks_interval)
{
	u64 tsc;

	tsc = rdmsr(MSR_IA32_TSC);
	tsc += ticks_interval;
	wrmsr(MSR_IA32_TSCDEADLINE, tsc);
}
int enable_tsc_deadline_timer(void)
{
	u32 lvtt;

	if (cpuid(1).c & (1 << 24)) {
		lvtt = APIC_LVT_TIMER_TSCDEADLINE | TSC_DEADLINE_TIMER_VECTOR;
		apic_write(APIC_LVTT, lvtt);
		return 1;
	} else {
		return 0;
	}
}
bool init_rtsc_dtimer(void)
{
	if (enable_tsc_deadline_timer()) {
		handle_irq(TSC_DEADLINE_TIMER_VECTOR, tsc_deadline_timer_isr);
		irq_enable();
		return true;
	}
	return false;
}
static struct pci_dev pci_devs[MAX_PCI_DEV_NUM];
static uint32_t nr_pci_devs = MAX_PCI_DEV_NUM;

static __unused bool pci_probe_msi_capability(union pci_bdf bdf, uint32_t *msi_addr)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	int i = 0;
	int repeat = 0;
	reg_addr = PCI_CAPABILITY_LIST;
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 1);
	DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);

	reg_addr = reg_val;
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
	DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
	/*
	 * pci standard config space [00h,100h).So,MSI register must be in [00h,100h),
	 * if there is MSI register
	 */
	repeat = PCI_CONFIG_SPACE_SIZE / 4;
	for (i = 0; i < repeat; i++) {
		/*Find MSI register group*/
		if (PCI_CAP_ID_MSI == (reg_val & SHIFT_MASK(7, 0))) {
			break;
		}
		reg_addr = (reg_val & SHIFT_MASK(15, 8)) >> 8;
		reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
	}
	if (repeat == i) {
		DBG_ERRO("no msi cap found!!!");
		return false;
	}
	*msi_addr = reg_addr;
	return true;
}
static uint64_t g_get_msi_int = 0;
static __unused int msi_int_handler(void *p_arg)
{
	uint32_t lapic_id = apic_id();
	DBG_INFO("[ int ]Run %s(%p), lapic_id=%d", __func__, p_arg, lapic_id);
	g_get_msi_int |= SHIFT_LEFT(1UL, lapic_id);
	return 0;
}

static __unused void msi_trigger_interrupt(void)
{
	/*init e1000e net pci device, get ready for msi int*/
	uint32_t lapic_id = apic_id();
	g_get_msi_int &= ~SHIFT_LEFT(1UL, lapic_id);
	msi_int_connect(msi_int_handler, (void *)1);
	msi_int_simulate();
	delay_loop_ms(5000);
}
#ifdef IN_NON_SAFETY_VM
void save_unchanged_reg()
{
	ulong cr8 = 0;
	asm("mov %%cr8, %%rax;mov %%rax, %0":"=m"(cr8));
	*(ulong *) CR8_INIT_ADDR = cr8;
}
#else
void save_unchanged_reg()
{
	asm volatile ("pause");
}
#endif
#ifndef IN_NATIVE
/**
 *
 * @brief case name APIC capability
 *
 * Summary: When a vCPU reads CPUID.1H, ACRN hypervisor shall set guest EDX [bit 9] to 1H.
 *  The Advanced Programmable Interrupt Controller (APIC), referred to in the
 *  following sections as the local APIC, is used by VM to receive and handle
 *  interrupts. Thus CPUID shall report the presence of Local APIC/xAPIC.
 *
 *	Enumerate CPUID.1:EDX[bit 9]. Verify the result is 1.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27696_apic_capability(void)
{
	struct cpuid id;

	id = cpuid(0x1);
	report("%s", (id.d & (1 << 9)) != 0, __FUNCTION__);
}
/**
 * @brief case name x2APIC capability
 *
 * Summary: When a vCPU reads CPUID.1H, ACRN hypervisor shall set guest ECX[bit 21] to 1H.
 *  x2APIC mode is used to provide extended processor addressability to the VM.
 *
 * Enumerate CPUID.1:ECX[bit 21]. Verify the result is 1.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27694_x2apic_capability_001(void)
{
	struct cpuid id;

	id = cpuid(LAPIC_CPUID_APIC_CAPABILITY);
	report("%s", (id.c & LAPIC_CPUID_APIC_CAP_X2APCI), __FUNCTION__);
}

#define MAX_CPU 64
static struct spinlock lock;
static int cpu_nr;
static u32 x2apic_id[MAX_CPU];
void x2apic_id_ipi_hander(isr_regs_t *regs)
{
	spin_lock(&lock);
	cpu_nr++;
	x2apic_id[cpu_nr] = cpuid(0xb).d;
	spin_unlock(&lock);
	eoi();
}
/**
 * @brief case name Different x2APIC ID
 *
 *
 * Summary: ACRN hypervisor shall guarantee that different guest LAPICs of the same VM have
 *  different extended LAPIC IDs.
 *  Each LAPIC shall have a unique ID to distinguish from other LAPICs
 *
 * Enumerate CPUID.0BH:EDX on each vCPU to get its x2APIC ID.
 * Verify that the result is different from each other.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27685_different_x2apic_id_001(void)
{
	unsigned cpus;
	bool ret = true;

#define X2APIC_ID_IPI_VEC 0xF0
	handle_irq(X2APIC_ID_IPI_VEC, x2apic_id_ipi_hander);

	cpus = fwcfg_get_nb_cpus();
	for (unsigned i = 1; i < cpus; i++) {
		apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_FIXED | APIC_INT_ASSERT | X2APIC_ID_IPI_VEC, i);
		sleep(1000);
	}
	sleep(500);

	x2apic_id[0] = cpuid(0xb).d;
	for (u32 i = 0; i < cpus; i++) {
		for (u32 j = i + 1; j < cpus; j++) {
			if (x2apic_id[i] == x2apic_id[j]) {
				ret = false;
				break;
			}
		}
	}

	report("%s", ret, __FUNCTION__);
}

/**
 * @brief case name Software disable support
 *
 * Summary: ACRN hypervisor shall expose LAPIC software disable support to any VM, in
 *  compliance with Chapter 10.4.7.2, Vol. 3, SDM.
 *  LAPIC is in a software-disabled state following power-up or reset according to
 *  Chapter 10.4.7.1, Vol. 3, SDM. Thus software-disabled state shall be supported
 *  and available to VMs.
 *
 * Try to software disable LAPIC, then check the LVTT register state.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27677_software_disable_support_001(void)
{
	unsigned int val;

	val = apic_read(APIC_LVTT);
	val &= ~APIC_LVT_MASKED;
	apic_write(APIC_LVTT, val);

	/*disable LAPIC*/
	val = apic_read(APIC_SPIV);
	if (val & APIC_SPIV_APIC_ENABLED) {
		val &= ~APIC_SPIV_APIC_ENABLED;
	}
	apic_write(APIC_SPIV, val);

	/*check LVTT[bit16],it should keep 1H when LAPIC is disable*/
	val = apic_read(APIC_LVTT);
	apic_write(APIC_LVTT, val & ~APIC_LVT_MASKED);
	report("%s", (val & APIC_LVT_MASKED) && (apic_read(APIC_LVTT) & APIC_LVT_MASKED), __FUNCTION__);

	/*recover the environment  */
	apic_write(APIC_SPIV, 0x1FF);
}
/**
 * @brief case name Encoding of LAPIC Version Register
 *
 * Summary: When a vCPU attempts to read LAPIC version register, ACRN hypervisor shall
 *  guarantee that the vCPU gets 1060015H.
 *  Keep LAPIC Version Register of VM the same with the target physical platform, in
 *  compliance with Chapter 10.4.8, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27675_encoding_of_lapic_version_register_001(void)
{
	u32 val;

	val = apic_read(APIC_LVR);
	report("%s", val == 0x1060015U, __FUNCTION__);
}

/**
 * @brief case name Expose LVT support
 *
 *
 * Summary: ACRN hypervisor shall expose local vector table to any VM, in compliance with
 *  Chapter 10.5.1, Vol.3 and table 10-6, Vol. 3, SDM.
 *  The local vector table (LVT) allows software to specify the manner in which the
 *  local interrupts are delivered to the processor core. ACRN hypervisor shall
 *  provide LVT CMCI Register, LVT Timer Register, LVT Thermal Monitor Register, LVT
 *  Performance Counter Register, LVT LINT0 Register, LVT LINT1 Register, LVT Error
 *  Register for VM to use in compliance with Chapter 10.5 Vol.3 and table 10-6, Vol.
 *  3, SDM.
 *
 * Arm an APIC timer via LVT timer register, verify that corresponding handler is called.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27674_expose_lvt_support_001(void)
{
	const unsigned int vec = LAPIC_TEST_VEC;
	unsigned initial_value = LAPIC_TIMER_INITIAL_VAL;
	unsigned int lvtt;

	handle_irq(vec, lapic_timer_handler);

	lapic_timer_cnt = 0;
	wrmsr(MSR_IA32_TSCDEADLINE, 0);
	apic_write(APIC_TMICT, 0);
	irq_enable();

	/*arm oneshot timer*/
	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;

	lvtt |= APIC_LVT_TIMER_ONESHOT;
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	apic_write(APIC_TDCR, 0x0000000b);
	asm("nop;nop");
	apic_write(APIC_TMICT, initial_value);

	sleep(initial_value + 10);

	report("%s", lapic_timer_cnt == 1, __FUNCTION__);
}

/**
 * @brief case name Expose LAPIC error handling
 *
 * Summary: ACRN hypervisor shall expose error handling support to any VM in compliance with
 *  Chapter 10.5.3 and table 10-6, Vol 3, SDM.
 *  ESR will be used to record errors detected during interrupt handling, in
 *  compliance with Chapter 10.5.3 and table 10-6, Vol 3, SDM.
 *
 *
 *	When a vCPU trigger a SELF IPI with illegal vector (0~15)
 *	Then the error will be caught and ESR be set meanwhile the interrupt should not be triggered
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27659_expose_lapic_error_handling_001(void)
{
	u32 apic_esr;

	for (int i = 0; i < 16; i++) {
		ipi_count = 0;
		handle_irq(i, lapic_self_ipi_isr);
		apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_FIXED | i,
					   0);
		sleep(100);
		apic_write(APIC_ESR, 0);
		apic_esr = apic_read(APIC_ESR);
		/*ISR should not be triggered && ESR bit5: Send Illegal Vector should be set*/
		if (ipi_count || !(apic_esr & (1 << 5))) {
			printf("%d %x ", i, apic_esr);
			report("%s", 0, __FUNCTION__);
			return;
		}
	}
	report("%s", 1, __FUNCTION__);
	setup_idt(); /*recover idt for exception */
	apic_write(APIC_ESR, 0);
}

/**
 * @brief case name Physical LAPIC SVR support
 *
 * Summary: ACRN hypervisor shall expose spurious interrupt support to any VM,
 *  in compliance with Chapter 10.9, Vol.3, SDM.
 *  SVR will be used for Spurious interrupt and enable/disable APIC, Focus Processor
 *  Checking, EOI-Broadcast Suppression.
 *
 *  Write a value to LAPIC SVR register. Verify that it is stored and read back.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27731_expose_lapic_svr_support(void)
{
	unsigned int val1, val2, val3;

	val1 = apic_read(APIC_SPIV);

	val2 = val1 ^ APIC_SPIV_APIC_ENABLED;/*reverse 8bit*/
	val2 |= LAPIC_TEST_VEC;
	apic_write(APIC_SPIV, val2);

	val3 = apic_read(APIC_SPIV);

	report("%s", val3 == val2, __FUNCTION__);
	/*recover the environment*/
	apic_write(APIC_SPIV, val1);
}

/**
 * @brief case name Expose ARAT support
 * ACRN hypervisor shall expose APIC Timer always running feature to any VM, in
 *  compliance with Chapter 10.5.4, Vol.3, SDM. For precision, we need the
 *  processor APIC timer runs at a constant rate regardless of P-state
 *  transitions and it continues to run at the same rate in deep C-states.
 *
 * Enumerate CPUID.06H.EAX.ARAT[bit 2]. Verify the result is 1.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27654_expose_arat_support_001(void)
{
	report("%s", (cpuid(0x6).a & (1U << 2)), __FUNCTION__);
}

/**
 * @brief case name Expose Interrupt Priority Feature
 *
 * Summary: ACRN hypervisor shall expose task priority support to any VM, in compliance with
 *  Chapter 10.8.3.1, Vol.3, SDM.
 *  VM will use this feature to process Interrupt Priority.
 *
 * When set interrupt priority register(80H) to 0xFF meanwhile send a self IPI
 * Then the IPI should not be delivared due to the low priority.
 * When set interrupt priority register to 0
 * Then the IPI should be delivared at once.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27932_expose_interrupt_priority_feature_001(void)
{
	bool ret = false;

	ipi_count = 0;
	handle_irq(LAPIC_TEST_VEC, lapic_self_ipi_isr);

	apic_write(APIC_TASKPRI, 0xFFU);
	apic_write(APIC_SELF_IPI, LAPIC_TEST_VEC);
	sleep(100);
	ret = (ipi_count == 0);
	apic_write(APIC_TASKPRI, 0U);
	sleep(100);
	ret = ((ret == true) && (ipi_count == 1));

	report("%s", ret, __FUNCTION__);
}

/**
 * @brief case name The value of PPR
 *
 * Summary: ACRN hypervisor shall guarantee the value of PPR is calculated in compliance
 *  with Chapter 10.8.3.1, Vol.3, SDM.
 *  In compliance with Chapter 10.8.3.1, Vol.3, SDM.
 *
 * When set interrupt priority register(80H) to 0x2F meanwhile send a self IPI .
 * Then read processor priority register(A0H), it should return 0x2F
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27930_the_value_of_ppr_001(void)
{
	const unsigned int destination = LAPIC_INTR_TARGET_SELF;
	const unsigned int vec = LAPIC_TEST_VEC_HIGH;
	const unsigned int mode = APIC_DEST_PHYSICAL | APIC_DM_FIXED;
	unsigned int val;

	eoi();
	for (int i = 0; i < APIC_ISR_NR; i++) {
		if (apic_read(APIC_ISR + (i << 4))) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	val = apic_read(APIC_SPIV);
	val |= APIC_VECTOR_MASK;
	val |= APIC_SPIV_APIC_ENABLED;
	apic_write(APIC_SPIV, val);

	ipi_count = 0;
	handle_irq(vec, lapic_self_ipi_isr);
	apic_write(APIC_TASKPRI, 0x2F);
	apic_icr_write(mode | vec, destination);

	sleep(100);

	report("%s", (ipi_count == 1) && (apic_read(APIC_PROCPRI) == 0x2F), __FUNCTION__);
}

/**
 * @brief case name Expose Interrupt Acceptance for Fixed Interrupts Infrastructure
 *
 * Summary: ACRN hypervisor shall expose interrupt handling support to any VM, in compliance
 *  with Chapter 10.8.3 and Chapter 10.8.4, Vol.3, SDM.
 *
 * When a vCPU trigger a SELF IPI with deliver mode "Fixed" (000)
 * Then the irq handler function should be run
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27927_expose_interrupt_acceptance_for_fixed_interrupts_infrastructure_001(void)
{
	const unsigned int vec = LAPIC_TEST_VEC;
	const unsigned int mode = APIC_DEST_PHYSICAL | APIC_DM_FIXED;

	handle_irq(vec, lapic_self_ipi_isr);
	ipi_count = 0;
	apic_icr_write(mode | vec, 0);
	sleep(1000);
	report("%s", ipi_count == 1, __FUNCTION__);
	ipi_count = 0;
}

/**
 * @brief case name Expose EOI
 *
 * Summary: ACRN hypervisor shall expose EOI to any VM in compliance with Chapter 10.8.5,
 *  Vol.3, SDM.
 *  EOI will be used by system software to signaling interrupt servicing Completion.
 *
 * When send an IPI with SELF as target and without a EOI in handler,
 * Then we can detect that the IRQ handler can be completed only when a APIC_EOI sent
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27599_expose_eoi_001(void)
{
	bool result = false;
	const unsigned int destination = LAPIC_INTR_TARGET_SELF;
	const unsigned int vec = LAPIC_TEST_VEC;
	const unsigned int mode = APIC_DEST_PHYSICAL | APIC_DM_FIXED;

	handle_irq(vec, lapic_ipi_isr_no_eoi);
	ipi_count = 0;
	/*trigger fisrt ipi, the handler should be executed.*/
	apic_icr_write(mode | vec, destination);
	sleep(1000);
	result = (ipi_count == 1);

	/*trigger second ipi, bcz the EOI dose not send, so this IPI interrupt doesn't report to CPU*/
	apic_icr_write(mode | vec, destination);
	sleep(1000);
	result = (ipi_count == 1) && (result == true);
	/*send EOI, the second IPI should report to CPU, the handler should be run again*/
	eoi();
	sleep(1000);
	result = (ipi_count == 2) && (result == true);

	report("%s", result, __FUNCTION__);
	eoi();
	ipi_count = 0;
}


/**
 * @brief case name Expose SELF IPI Register in x2APIC mode
 *
 * Summary: When in x2APIC mode, ACRN hypervisor shall expose SELF IPI register, in
 *  compliance with Chapter 10.12.11, Vol.3, SDM.
 *  SELF IPIs are used extensively by some system software. The x2APIC architecture
 *  introduces a new register interface for easily configuring self IPI.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27595_expose_self_ipi_register_in_x2apic_mode_001(void)
{
	const unsigned int vec = LAPIC_TEST_VEC;

	ipi_count = 0;
	handle_irq(vec, lapic_self_ipi_isr);
	apic_write(APIC_SELF_IPI, vec);

	report("%s", ipi_count == 1, __FUNCTION__);
	ipi_count = 0;
}

/**
 * @brief case name illegal vector in LVT register
 *
 * Summary: When a vCPU writes LAPIC LVT register and the new guest LAPIC LVT register [bit
 *  7:0] is less than 10H, ACRN hypervisor shall guarantee that the guest LAPIC of
 *  the vCPU detects a received illegal vector error.
 *  Keep the behavior the same with the native.
 *
 * check APIC_LVTT register.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27571_illegal_vector_in_lvt_register_001(void)
{
	unsigned reg = APIC_LVTT;
	unsigned long vec = LAPIC_TEST_INVALID_VEC1;
	volatile unsigned long val;
	u32 esr;

	val = apic_read(reg);
	val &= ~APIC_VECTOR_MASK;
	val &= ~APIC_LVT_MASKED;
	val &= ~APIC_MODE_MASK;/*make bit[10-8] to 0*/
	val |= vec;
	apic_write(reg, val);

	apic_write(APIC_ESR, 0U);
	esr = apic_read(APIC_ESR);

	report("%s", (esr & (0x1UL << 6)), __FUNCTION__);
}

/**
 * @brief case name illegal vector in LVT register
 *
 * Summary: When a vCPU writes LAPIC LVT register and the new guest LAPIC LVT register [bit
 *  7:0] is less than 10H, ACRN hypervisor shall guarantee that the guest LAPIC of
 *  the vCPU detects a received illegal vector error.
 *  Keep the behavior the same with the native.
 *
 * check APIC_LVTCMCI register
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38519_illegal_vector_in_lvt_register_002(void)
{
	unsigned reg = APIC_LVTCMCI;
	unsigned long vec = LAPIC_TEST_INVALID_VEC1;
	volatile unsigned long val;
	unsigned long esr;

	val = apic_read(reg);
	val &= ~APIC_VECTOR_MASK;
	val &= ~APIC_LVT_MASKED;
	val &= ~APIC_MODE_MASK;/*make bit[10-8] to 0*/
	val |= vec;
	apic_write(reg, val);

	apic_write(APIC_ESR, 0U);
	esr = apic_read(APIC_ESR);

	report("%s", (esr & (0x1UL << 6)), __FUNCTION__);
}

/**
 * @brief case name illegal vector in LVT register
 *
 * Summary: When a vCPU writes LAPIC LVT register and the new guest LAPIC LVT register [bit
 *  7:0] is less than 10H, ACRN hypervisor shall guarantee that the guest LAPIC of
 *  the vCPU detects a received illegal vector error.
 *  Keep the behavior the same with the native.
 *
 * check APIC_LVTTHMR register
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38520_illegal_vector_in_lvt_register_003(void)
{
	unsigned reg = APIC_LVTTHMR;
	unsigned long vec = LAPIC_TEST_INVALID_VEC1;
	volatile unsigned long val;
	unsigned long esr;

	val = apic_read(reg);
	val &= ~APIC_VECTOR_MASK;
	val &= ~APIC_LVT_MASKED;
	val &= ~APIC_MODE_MASK;/*make bit[10-8] to 0*/
	val |= vec;
	apic_write(reg, val);

	apic_write(APIC_ESR, 0U);
	esr = apic_read(APIC_ESR);

	report("%s", (esr & (0x1UL << 6)), __FUNCTION__);
}

/**
 * @brief case name illegal vector in LVT register
 *
 * Summary: When a vCPU writes LAPIC LVT register and the new guest LAPIC LVT register [bit
 *  7:0] is less than 10H, ACRN hypervisor shall guarantee that the guest LAPIC of
 *  the vCPU detects a received illegal vector error.
 *  Keep the behavior the same with the native.
 *
 *check APIC_LVTPC register
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38521_illegal_vector_in_lvt_register_004(void)
{
	unsigned reg = APIC_LVTPC;
	unsigned long vec = LAPIC_TEST_INVALID_VEC1;
	volatile unsigned long val;
	unsigned long esr;

	val = apic_read(reg);
	val &= ~APIC_VECTOR_MASK;
	val &= ~APIC_LVT_MASKED;
	val &= ~APIC_MODE_MASK;/*make bit[10-8] to 0*/
	val |= vec;
	apic_write(reg, val);

	apic_write(APIC_ESR, 0U);
	esr = apic_read(APIC_ESR);

	report("%s", (esr & (0x1UL << 6)), __FUNCTION__);

}

/**
 * @brief case name illegal vector in LVT register
 *
 * Summary: When a vCPU writes LAPIC LVT register and the new guest LAPIC LVT register [bit
 *  7:0] is less than 10H, ACRN hypervisor shall guarantee that the guest LAPIC of
 *  the vCPU detects a received illegal vector error.
 *  Keep the behavior the same with the native.
 *
 * check APIC_LVT0 register
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38522_illegal_vector_in_lvt_register_005(void)
{
	unsigned reg = APIC_LVT0;
	unsigned long vec = LAPIC_TEST_INVALID_VEC1;
	volatile unsigned long val;
	unsigned long esr;

	val = apic_read(reg);
	val &= ~APIC_VECTOR_MASK;
	val &= ~APIC_LVT_MASKED;
	val &= ~APIC_MODE_MASK;/*make bit[10-8] to 0*/
	val |= vec;
	apic_write(reg, val);

	apic_write(APIC_ESR, 0U);
	esr = apic_read(APIC_ESR);

	report("%s", (esr & (0x1UL << 6)), __FUNCTION__);
}

/**
 * @brief case name illegal vector in LVT register
 *
 * Summary: When a vCPU writes LAPIC LVT register and the new guest LAPIC LVT register [bit
 *  7:0] is less than 10H, ACRN hypervisor shall guarantee that the guest LAPIC of
 *  the vCPU detects a received illegal vector error.
 *  Keep the behavior the same with the native.
 *
 *	check APIC_LVT1 register
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38524_illegal_vector_in_lvt_register_006(void)
{
	unsigned reg = APIC_LVT1;
	unsigned long vec = LAPIC_TEST_INVALID_VEC1;
	volatile unsigned long val;
	unsigned long esr;

	val = apic_read(reg);
	val &= ~APIC_VECTOR_MASK;
	val &= ~APIC_LVT_MASKED;
	val &= ~APIC_MODE_MASK;/*make bit[10-8] to 0*/
	val |= vec;
	apic_write(reg, val);

	apic_write(APIC_ESR, 0U);
	esr = apic_read(APIC_ESR);

	report("%s", (esr & (0x1UL << 6)), __FUNCTION__);
}

/**
 * @brief case name illegal vector in LVT register
 *
 * Summary: When a vCPU writes LAPIC LVT register and the new guest LAPIC LVT register [bit
 *  7:0] is less than 10H, ACRN hypervisor shall guarantee that the guest LAPIC of
 *  the vCPU detects a received illegal vector error.
 *  Keep the behavior the same with the native.
 *
 *	check APIC_LVTERR register
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38526_illegal_vector_in_lvt_register_007(void)
{
	unsigned reg = APIC_LVTERR;
	unsigned long vec = LAPIC_TEST_INVALID_VEC1;
	volatile unsigned long val;
	u32 esr;

	val = apic_read(reg);
	val &= ~APIC_VECTOR_MASK;
	val &= ~APIC_LVT_MASKED;
	val &= ~APIC_MODE_MASK;/*make bit[10-8] to 0*/
	val |= vec;
	apic_write(reg, val);

	apic_write(APIC_ESR, 0U);
	esr = apic_read(APIC_ESR);

	report("%s", (esr & (0x1UL << 6)), __FUNCTION__);
}
/**
 * @brief case name Write the DCR
 *
 * Summary: If a vCPU write a value to the DCR, ACRN hypervisor shall guarantee CCR
 *  count-down restart and the value in DCR is immediately used.
 *  Keep the behavior the same with the native.
 *
 * Arm one-shot timer, before timer reaches 0, read value of register APIC_TMCCT,
 * and write APIC_DCR with 0x0A (110: devided by 128),
 * the count-down restarts and the value in guest LAPIC DCR is immediately used.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27554_write_the_dcr_001(void)
{
	u32 tmcct;
	u64 tsc1, tsc2;
	u32 eax, ebx, ecx_crystal_hz, reserved;
	u32 ratio;
	u32 delta;

	__asm volatile("cpuid"
				   : "=a"(eax), "=b"(ebx), "=c"(ecx_crystal_hz), "=d"(reserved)
				   : "0" (0x15), "2" (0)
				   : "memory");
	ratio = ebx / eax;/*APIC timer Hz : TSC Hz*/

	lapic_timer_cnt = 0;
	handle_irq(LAPIC_ONESHOT_TIMER_VEC, lapic_timer_handler);
	apic_write(APIC_TDCR, 0xB);
	apic_write(APIC_LVTT, APIC_LVT_TIMER_ONESHOT | LAPIC_ONESHOT_TIMER_VEC);
	apic_write(APIC_TMICT, LAPIC_TIMER_INITIAL_VAL);
	sleep(LAPIC_TIMER_INITIAL_VAL / 10);

	apic_write(APIC_TDCR, 0x0A);/*devided = 128*/
	tmcct = apic_read(APIC_TMCCT);/*get the left time */
	tsc1 = rdmsr(MSR_IA32_TSC);
	while (!lapic_timer_cnt);
	tsc2 = rdmsr(MSR_IA32_TSC);
	delta = (tsc2 - tsc1) / (128 * ratio);/*calculate the elapsed time */
	/*the 3 is the result of testing experience*/
	report("%s", (delta > tmcct) && (delta < (tmcct + 3)), __FUNCTION__);
	apic_write(APIC_TDCR, 0xB);/*devided = 1*/
}

/**
 * @brief case name illegal vector in ICR
 *
 * Summary: When a vCPU writes LAPIC ICR and the new guest LAPIC ICR [bit 7:0] is less than
 *  10H, ACRN hypervisor shall guarantee that the guest LAPIC of the vCPU detects a
 *  send illegal vector error.
 *  Keep the behavior the same with the native.
 *
 * while LAPIC ICR[bit 10:8] is 0,writing LAPIC ICR register to send a SELF IPI with its [bit 7:0] smaller than 10H,
 * which means an illegal vector. Verify that bit 5 of LAPIC ESR register is 1,
 * which means LAPIC detects a send illegal vector error.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27805_illegal_vector_in_icr_001(void)
{
	u32 vec = 1;
	u32 mode = APIC_DEST_PHYSICAL | APIC_DM_FIXED;
	u32 esr;

	irq_enable();
	apic_icr_write(mode | vec, 0);/*APIC_DM_FIXED: [bit 10:8] is 0*/
	sleep(100);

	apic_write(APIC_ESR, 0U);
	esr = apic_read(APIC_ESR);

	report("%s", (esr & (1 << 5)), __FUNCTION__);
}

/**
 * @brief case name mask bit in LVT when Software re-enabling
 *
 * Summary:When a vCPU writes LAPIC SVR, the old guest LAPIC SVR[bit 8] is 1 and the new guest LAPIC SVR[bit 8] is 0,
 * ACRN hypervisor shall guarantee that guest LAPIC LVT [bit 16] is unchanged.
 *
 * 1.Set bit 8 of LAPIC SVR register to software enable LAPIC.Set mask bit 16 of an LVT register.
 * 2.Change bit 8 of LAPIC SVR register from 1 to 0.
 * 3.Verify that mask bit 16 of the LVT register keeps unchanged.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27544_mask_bit_in_lvt_when_software_re_enabling_001(void)
{
	const unsigned reg = APIC_LVTT;
	unsigned val;

	val = apic_read(APIC_SPIV);
	val |= APIC_VECTOR_MASK;
	val |= APIC_SPIV_APIC_ENABLED;/*bit8 is 1*/
	apic_write(APIC_SPIV, val);

	val = apic_read(reg);
	val |= APIC_LVT_MASKED;/*bit 16 of LVT is 1*/
	apic_write(reg, val);

	val = apic_read(APIC_SPIV);
	val &= ~APIC_SPIV_APIC_ENABLED;/*change bit8 to 0*/
	apic_write(APIC_SPIV, val);

	val = apic_read(reg);

	report("%s", (val & APIC_LVT_MASKED), __FUNCTION__);/*bit 16 of LVT should be unchanged*/
	apic_write(APIC_SPIV, 0x1FF);
}

/**
 * @brief case name delivery status bit of LVT
 *
 * Summary: When a vCPU attempts to write LAPIC LVT [bit 12], ACRN hypervisor shall ignore
 *  the write to this bit.
 *  Keep the behavior the same with the native.
 *
 * Read bit 12 of a APIC_LVTT register and then write a different value to the register.
 * Verify that the bit keeps unchanged.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27520_delivery_status_bit_of_lvt_001(void)
{
	u32 val1, val2;
	u32 reg = APIC_LVTT;

	val1 = apic_read(reg);
	/* change bit12 only and write to LVT register*/
	apic_write(reg, val1 ^ (1UL << 12));
	/*the bit12 does not change*/
	val2 = apic_read(reg);
	report("%s", (val1 & (1UL << 12)) == (val2 & (1UL << 12)), __FUNCTION__);
}

/**
 * @brief case name delivery status bit of LVT
 *
 * Summary: When a vCPU attempts to write LAPIC LVT [bit 12], ACRN hypervisor shall ignore
 *  the write to this bit.
 *  Keep the behavior the same with the native.
 *
 * Read bit 12 of a APIC_LVTCMCI register and then write a different value to the register.
 * Verify that the bit keeps unchanged.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38568_delivery_status_bit_of_lvt_002(void)
{
	u32 val1, val2;
	u32 reg = APIC_LVTCMCI;

	val1 = apic_read(reg);
	/* change bit12 only and write to LVT register*/
	apic_write(reg, val1 ^ (1UL << 12));
	/*the bit12 does not change*/
	val2 = apic_read(reg);
	report("%s", (val1 & (1UL << 12)) == (val2 & (1UL << 12)), __FUNCTION__);
}

/**
 * @brief case name delivery status bit of LVT
 *
 * Summary: When a vCPU attempts to write LAPIC LVT [bit 12], ACRN hypervisor shall ignore
 *  the write to this bit.
 *  Keep the behavior the same with the native.
 *
 * Read bit 12 of a APIC_LVTTHMR register and then write a different value to the register.
 * Verify that the bit keeps unchanged.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38569_delivery_status_bit_of_lvt_003(void)
{
	u32 val1, val2;
	u32 reg = APIC_LVTTHMR;

	val1 = apic_read(reg);
	/* change bit12 only and write to LVT register*/
	apic_write(reg, val1 ^ (1UL << 12));
	/*the bit12 does not change*/
	val2 = apic_read(reg);
	report("%s", (val1 & (1UL << 12)) == (val2 & (1UL << 12)), __FUNCTION__);
}

/**
 * @brief case name delivery status bit of LVT
 *
 * Summary: When a vCPU attempts to write LAPIC LVT [bit 12], ACRN hypervisor shall ignore
 *  the write to this bit.
 *  Keep the behavior the same with the native.
 *
 * Read bit 12 of a APIC_LVTPC register and then write a different value to the register.
 * Verify that the bit keeps unchanged.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38570_delivery_status_bit_of_lvt_004(void)
{
	u32 val1, val2;
	u32 reg = APIC_LVTPC;

	val1 = apic_read(reg);
	/* change bit12 only and write to LVT register*/
	apic_write(reg, val1 ^ (1UL << 12));
	/*the bit12 does not change*/
	val2 = apic_read(reg);
	report("%s", (val1 & (1UL << 12)) == (val2 & (1UL << 12)), __FUNCTION__);
}

/**
 * @brief case name delivery status bit of LVT
 *
 * Summary: When a vCPU attempts to write LAPIC LVT [bit 12], ACRN hypervisor shall ignore
 *  the write to this bit.
 *  Keep the behavior the same with the native.
 *
 * Read bit 12 of a APIC_LVT0 register and then write a different value to the register.
 * Verify that the bit keeps unchanged.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38571_delivery_status_bit_of_lvt_005(void)
{
	u32 val1, val2;
	u32 reg = APIC_LVT0;

	val1 = apic_read(reg);
	/* change bit12 only and write to LVT register*/
	apic_write(reg, val1 ^ (1UL << 12));
	/*the bit12 does not change*/
	val2 = apic_read(reg);
	report("%s", (val1 & (1UL << 12)) == (val2 & (1UL << 12)), __FUNCTION__);
}

/**
 * @brief case name delivery status bit of LVT
 *
 * Summary: When a vCPU attempts to write LAPIC LVT [bit 12], ACRN hypervisor shall ignore
 *  the write to this bit.
 *  Keep the behavior the same with the native.
 *
 * Read bit 12 of a APIC_LVT1 register and then write a different value to the register.
 * Verify that the bit keeps unchanged.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38573_delivery_status_bit_of_lvt_006(void)
{
	u32 val1, val2;
	u32 reg = APIC_LVT1;

	val1 = apic_read(reg);
	/* change bit12 only and write to LVT register*/
	apic_write(reg, val1 ^ (1UL << 12));
	/*the bit12 does not change*/
	val2 = apic_read(reg);
	report("%s", (val1 & (1UL << 12)) == (val2 & (1UL << 12)), __FUNCTION__);
}

/**
 * @brief case name delivery status bit of LVT
 *
 * Summary: When a vCPU attempts to write LAPIC LVT [bit 12], ACRN hypervisor shall ignore
 *  the write to this bit.
 *  Keep the behavior the same with the native.
 *
 * Read bit 12 of a APIC_LVTERR register and then write a different value to the register.
 * Verify that the bit keeps unchanged.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38574_delivery_status_bit_of_lvt_007(void)
{
	u32 val1, val2;
	u32 reg = APIC_LVTERR;

	val1 = apic_read(reg);
	/* change bit12 only and write to LVT register*/
	apic_write(reg, val1 ^ (1UL << 12));
	/*the bit12 does not change*/
	val2 = apic_read(reg);
	report("%s", (val1 & (1UL << 12)) == (val2 & (1UL << 12)), __FUNCTION__);
}

/**
 * @brief case name Set Initial Counter Register,
 * Current Count Register and IA32_TSCDEADLINE to 0H after setting time mode to be 3H
 *
 * Summary: When a vCPU writes LAPIC timer register [bit 18:17] is 3H,
 *  ACRN hypervisor shall set guest LAPIC initial count register,
 *	guest LAPIC current count register and guest IA32TSCDEADLINE to 0H.
 *  Keep the behavior the same with the native.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27725_set_TMICT_and_TSCDEADLINE_to_0_after_setting_time_mode_to_be_3h_001(void)
{
	u32 val;

	val = apic_read(APIC_LVTT);
	apic_write(APIC_LVTT, val | (3U << 17));

	report("%s", (!apic_read(APIC_TMICT)) && (!rdmsr(MSR_IA32_TSCDEADLINE)), __FUNCTION__);
	/*set back the timer mode*/
	apic_write(APIC_LVTT, val);
}

/**
 * @brief case name value of Initial Count Register when LVT timer bit 18 is 1
 *
 * Summary: When a vCPU reads LAPIC initial count register and guest LVT timer register [bit 18] is 1,
 * ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 * Set bit 18 of LVT timer register as 1 (TSC-Deadline mode or reserved).
 * Verify that reading from LAPIC initial count register is 0H.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27723_value_of_TMICT_when_lvt_timer_bit_18_is_1_001(void)
{
	u32 val;

	val = apic_read(APIC_LVTT);
	apic_write(APIC_LVTT, val | (1U << 18));

	report("%s", !apic_read(APIC_TMICT), __FUNCTION__);
	/*set back the timer mode*/
	apic_write(APIC_LVTT, val);

}

/**
 * @brief case name Interrupt Delivery with illegal vector
 *
 * Summary: When a fixed interrupt is triggered and the vector is less than 10H, ACRN
 *  hypervisor shall guarantee that the interrupt is ignored.
 *  Interrupt Delivery with illegal vector shall never been issued.
 *
 *  Trigger a fixed interrupt with the vector less than 10H.
 *  Verify that the interrupt is ignored by checking LAPIC ISR register.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27712_interrupt_delivery_with_illegal_vector_001(void)
{
	const unsigned int vector = LAPIC_TEST_INVALID_VEC;
	const unsigned int mode = APIC_DEST_PHYSICAL | APIC_DM_FIXED;
	bool result = true;

	eoi();
	apic_icr_write(mode | vector, 0); /*try to trigger a less than 10H interrupt */

	for (int i = 0; i < APIC_ISR_NR; i++) {
		if (apic_read(APIC_ISR + LAPIC_ISR_INDEX(i)) != 0U) {
			result = false;
			break;
		}
	}
	report("%s", result, __FUNCTION__);
}

/**
 * @brief case name x2APIC IPI with unsupported delivery mode
 *
 * Summary: When a vCPU writes LAPIC ICR, ACRN hypervisor shall guarantee that the IPI issue
 *  request is ignored if any of the following conditions is met.
 *
 *  The new guest LAPIC ICR [bit 10:8] is 7H.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27495_x2apic_ipi_with_unsupported_delivery_mode_001(void)
{
	const unsigned int destination = LAPIC_INTR_TARGET_SELF;
	const unsigned int vec = LAPIC_TEST_VEC;
	const unsigned int mode = SET_APIC_DELIVERY_MODE(APIC_DEST_PHYSICAL, 0x7);

	handle_irq(vec, lapic_self_ipi_isr);
	ipi_count = 0;
	apic_icr_write(mode | vec, destination);

	sleep(100);
	report("%s", !ipi_count, __FUNCTION__);
}

/**
 * @brief case name x2APIC IPI with unsupported delivery mode
 *
 * Summary: When a vCPU writes LAPIC ICR, ACRN hypervisor shall guarantee that the IPI issue
 *  request is ignored if any of the following conditions is met.
 *
 *  The new guest LAPIC ICR [bit 10:8] is 3H.
 *
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38607_x2apic_ipi_with_unsupported_delivery_mode_002(void)
{
	const unsigned int destination = LAPIC_INTR_TARGET_SELF;
	const unsigned int vec = LAPIC_TEST_VEC;
	const unsigned int mode = SET_APIC_DELIVERY_MODE(APIC_DEST_PHYSICAL, 0x3);

	handle_irq(vec, lapic_self_ipi_isr);
	ipi_count = 0;
	apic_icr_write(mode | vec, destination);

	sleep(100);
	report("%s", !ipi_count, __FUNCTION__);
}

/**
 * @brief case name x2APIC IPI with unsupported delivery mode
 *
 * Summary: When a vCPU writes LAPIC ICR, ACRN hypervisor shall guarantee that the IPI issue
 *  request is ignored if any of the following conditions is met.

 *  The new guest LAPIC ICR [bit 10:8] is 2H.
 *
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38608_x2apic_ipi_with_unsupported_delivery_mode_003(void)
{
	const unsigned int destination = LAPIC_INTR_TARGET_SELF;
	const unsigned int vec = LAPIC_TEST_VEC;
	const unsigned int mode = SET_APIC_DELIVERY_MODE(APIC_DEST_PHYSICAL, 0x2);

	handle_irq(vec, lapic_self_ipi_isr);
	ipi_count = 0;
	apic_icr_write(mode | vec, destination);

	sleep(100);
	report("%s", !ipi_count, __FUNCTION__);
}

/**
 * @brief case name x2APIC IPI with unsupported delivery mode
 *
 * Summary: When a vCPU writes LAPIC ICR, ACRN hypervisor shall guarantee that the IPI issue
 *  request is ignored if any of the following conditions is met.

 *  The new guest LAPIC ICR [bit 10:8] is 1H.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38609_x2apic_ipi_with_unsupported_delivery_mode_004(void)
{
	const unsigned int destination = LAPIC_INTR_TARGET_SELF;
	const unsigned int vec = LAPIC_TEST_VEC;
	const unsigned int mode = SET_APIC_DELIVERY_MODE(APIC_DEST_PHYSICAL, 0x1);

	handle_irq(vec, lapic_self_ipi_isr);
	ipi_count = 0;
	apic_icr_write(mode | vec, destination);

	sleep(100);
	report("%s", !ipi_count, __FUNCTION__);
}

/**
 * @brief case name x2APIC LVT with SMI Delivery Mode
 *
 * Summary: While a guest LAPIC LVT register [bit 9:8] is different from 0H, ACRN hypervisor
 *  shall guarantee that local interrupts controlled by the guest LVT register are
 *  ignored.
 *  Delivery mode with the 2 least significant bits being not 0H can be SMI, ExtINT,
 *  INIT or reserved. A VM is not supposed to trigger a SMI by local interrupts,
 *  considering the indeterministic behavior of SMI handling. Configuring an local
 *  interrupt to be ExtINT has no effect due to the lack of a legacy PIC or IOAPIC on
 *  the virtual platform. INIT is only valid for LINT0 for LINT1 which is not
 *  connected to any interrupt source. Thus configuring a local interrupt to be any
 *  of the delivery mode above shall essentially prevent the corresponding local
 *  interrupts from being delivered to the vCPU.
 *
 *  Write an LAPIC LVT register with its bit [10:8] as
 *	010 (SMI),
 *  Verify that the request is ignored by checking LAPIC ISR and ESR registers are not set.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27934_x2apic_lvt_with_smi_delivery_mode_001(void)
{
	u32 vec = 0;

	apic_write(APIC_ESR, 0);
	/*the vector filed should be 0. SDM vol.3 chapter 10.4.2*/
	apic_write(APIC_LVT0,  APIC_DM_SMI | vec);

	for (int i = 0; i < APIC_ISR_NR; i++) {
		if (apic_read(APIC_ISR + (i << 4))) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	apic_write(APIC_ESR, 0);
	if (apic_read(APIC_ESR) != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}


/**
 * @brief case name x2APIC LVT with SMI/INIT/ExtINT/reserved Delivery Mode 002
 *
 * Summary: While a guest LAPIC LVT register [bit 9:8] is different from 0H, ACRN hypervisor
 *  shall guarantee that local interrupts controlled by the guest LVT register are
 *  ignored.
 *  Delivery mode with the 2 least significant bits being not 0H can be SMI, ExtINT,
 *  INIT or reserved. A VM is not supposed to trigger a SMI by local interrupts,
 *  considering the indeterministic behavior of SMI handling. Configuring an local
 *  interrupt to be ExtINT has no effect due to the lack of a legacy PIC or IOAPIC on
 *  the virtual platform. INIT is only valid for LINT0 for LINT1 which is not
 *  connected to any interrupt source. Thus configuring a local interrupt to be any
 *  of the delivery mode above shall essentially prevent the corresponding local
 *  interrupts from being delivered to the vCPU.
 *
 *  Write an LAPIC LVT register with its bit [10:8] as
 *	101 (INIT)
 *  Verify that the request is ignored by checking LAPIC ISR and ESR registers are not set.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39318_x2apic_lvt_with_INIT_delivery_mode_002(void)
{
	u32 vec = 0;

	apic_write(APIC_ESR, 0);
	/* the vector filed should be 0. SDM vol.3 chapter 10.4.2*/
	apic_write(APIC_LVT0, APIC_DM_INIT | vec);

	/*the interrupt should not be devliverd */
	for (int i = 0; i < APIC_ISR_NR; i++) {
		if (apic_read(APIC_ISR + (i << 4))) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	apic_write(APIC_ESR, 0);
	if (apic_read(APIC_ESR) != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/**
 * @brief case name x2APIC LVT with SMI/INIT/ExtINT/reserved Delivery Mode 003
 *
 * Summary: While a guest LAPIC LVT register [bit 9:8] is different from 0H, ACRN hypervisor
 *  shall guarantee that local interrupts controlled by the guest LVT register are
 *  ignored.
 *  Delivery mode with the 2 least significant bits being not 0H can be SMI, ExtINT,
 *  INIT or reserved. A VM is not supposed to trigger a SMI by local interrupts,
 *  considering the indeterministic behavior of SMI handling. Configuring an local
 *  interrupt to be ExtINT has no effect due to the lack of a legacy PIC or IOAPIC on
 *  the virtual platform. INIT is only valid for LINT0 for LINT1 which is not
 *  connected to any interrupt source. Thus configuring a local interrupt to be any
 *  of the delivery mode above shall essentially prevent the corresponding local
 *  interrupts from being delivered to the vCPU.
 *
 *  Write an LAPIC LVT register with its bit [10:8] as
 *	110 (reserved)
 *  Verify that the request is ignored by checking LAPIC ISR and ESR registers are not set.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39319_x2apic_lvt_with_reserved_delivery_mode_003(void)
{
	u32 vec = 0;

	apic_write(APIC_ESR, 0);
	/*the vector filed should be 0. SDM vol.3 chapter 10.4.2*/
	apic_write(APIC_LVT0,  APIC_DM_RESERVED | vec); /*reserved mode*/

	for (int i = 0; i < APIC_ISR_NR; i++) {
		if (apic_read(APIC_ISR + (i << 4))) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	apic_write(APIC_ESR, 0);
	if (apic_read(APIC_ESR) != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/**
 * @brief case name x2APIC LVT with SMI/INIT/ExtINT/reserved Delivery Mode 004
 *
 * Summary: While a guest LAPIC LVT register [bit 9:8] is different from 0H, ACRN hypervisor
 *  shall guarantee that local interrupts controlled by the guest LVT register are
 *  ignored.
 *  Delivery mode with the 2 least significant bits being not 0H can be SMI, ExtINT,
 *  INIT or reserved. A VM is not supposed to trigger a SMI by local interrupts,
 *  considering the indeterministic behavior of SMI handling. Configuring an local
 *  interrupt to be ExtINT has no effect due to the lack of a legacy PIC or IOAPIC on
 *  the virtual platform. INIT is only valid for LINT0 for LINT1 which is not
 *  connected to any interrupt source. Thus configuring a local interrupt to be any
 *  of the delivery mode above shall essentially prevent the corresponding local
 *  interrupts from being delivered to the vCPU.
 *
 *  Write an LAPIC LVT register with its bit [10:8] as
 *	111 (ExtINT)
 *  Verify that the request is ignored by checking LAPIC ISR and ESR registers are not set.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39320_x2apic_lvt_with_ExtINT_delivery_mode_004(void)
{
	u32 vec = 0;

	apic_write(APIC_ESR, 0);
	/* the vector filed should be 0. SDM vol.3 chapter 10.4.2*/
	apic_write(APIC_LVT0, APIC_DM_EXTINT | vec);

	for (int i = 0; i < APIC_ISR_NR; i++) {
		if (apic_read(APIC_ISR + (i << 4))) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	apic_write(APIC_ESR, 0);
	if (apic_read(APIC_ESR) != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/**
 * @brief case name CR8 is a mirror of LAPIC TPR
 *
 * Summary:
 *  CR8 is a mirror of LAPIC TPR.CR8 and Task Priority Register will be shadow.
 *
 * APIC.TPR[bits 7:4] = CR8[bits 3:0], APIC.TPR[bits 3:0] = 0.
 * A read of CR8 returns a 64-bit value which is the value of TPR[bits 7:4], zero extended to 64 bits.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27926_cr8_is_mirror_of_tpr(void)
{
	ulong first_cr8, sec_cr8;
	u32 old_tpr, tmp_tpr;

	old_tpr = apic_read(APIC_TASKPRI);

	apic_write(APIC_TASKPRI, LAPIC_TPR_MAX);
	asm volatile ("nop");
	first_cr8 = read_cr8();

	asm volatile ("nop");/* cr8 and TPR are not serialized*/
	asm volatile ("nop");

	apic_write(APIC_TASKPRI, LAPIC_TPR_MID);
	asm volatile ("nop");
	sec_cr8 = read_cr8();

	asm volatile ("nop");/* cr8 and TPR are not serialized*/
	asm volatile ("nop");

	write_cr8(0x5);
	tmp_tpr = apic_read(APIC_TASKPRI);
	/*write old tpr back*/
	apic_write(APIC_TASKPRI, old_tpr);
	report("%s", ((LAPIC_TPR_MAX >> 4) == first_cr8) && ((LAPIC_TPR_MID >> 4) == sec_cr8)	\
		   && ((tmp_tpr & 0xf) == 0U) && ((tmp_tpr >> 4) == 0x5),	\
		   __FUNCTION__);
}
#ifdef IN_NON_SAFETY_VM
/**
 * @brief case name APIC Base field state following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest MSR_IA32_APICBASE of AP
 *  to 0FEE00C00H following INIT.
 *  In compliance with Chapter 10.4.4, Vol.3, SDM. INIT reset only occurs on APs,
 *  thus the BSP bit shall be cleared.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27681_apic_base_field_state_of_ap_following_init_001(void)
{
	volatile u32 *ia32_apic_base = (u32 *)IA32_APIC_BASE_INIT_ADDR;

	report("%s", *ia32_apic_base == 0xfee00c00U, __FUNCTION__);
}
/**
 * @brief case name LVT CMCI state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC LVT CMCI to 00010000H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38686_LVT_CMCI_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (u32 *)LVT_CMCI_INIT_ADDR;

	if (*init_addr != 0x00010000U) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(38686);
	init_addr = (u32 *)LVT_CMCI_INIT_ADDR;

	if (*init_addr != 0x00010000U) {
		is_pass = false;
		printf("%d: init_addr=%d\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name LVT LINT0  state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC LVT LINT0 to 00010000H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38687_LVT_LINT0_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (u32 *)LINT0_INIT_ADDR;

	if (*init_addr != 0x00010000U) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(38687);

	if (*init_addr != 0x00010000U) {
		is_pass = false;
		printf("%d: init_addr=%d\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name LVT LINT0  state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC LVT LINT1 to 00010000H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38688_LVT_LINT1_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (u32 *)LINT1_INIT_ADDR;

	if (*init_addr != 0x00010000U) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(38688);

	if (*init_addr != 0x00010000U) {
		is_pass = false;
		printf("%d: init_addr=%d\n", __LINE__, *init_addr);
	}

	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name LVT Error register  state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC LVT Error register to 00010000H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38691_LVT_ERROR_register_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (u32 *)LVT_ERROR_INIT_ADDR;

	if (*init_addr != 0x00010000U) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(38691);

	if (*init_addr != 0x00010000U) {
		is_pass = false;
		printf("%d: init_addr=%d\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name  LVT Thermal Monitor  state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC  LVT Thermal Monitor  to 00010000H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38692_LVT_Thermal_Monitor_register_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (u32 *)LVTTHMR_INIT_ADDR;

	if (*init_addr != 0x00010000U) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(38692);

	if (*init_addr != 0x00010000U) {
		is_pass = false;
		printf("%d: init_addr=%d\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name
 * LVT Performance Counter Register state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC LVT Performance Counter Register to 00010000H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38693_LVT_Performance_Counter_register_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (u32 *)LVTPC_INIT_ADDR;

	if (*init_addr != 0x00010000U) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(38693);

	if (*init_addr != 0x00010000U) {
		is_pass = false;
		printf("%d: init_addr=%d\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name
 * LVT Timer Register state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC LVT Timer Register to 00010000H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38694_LVT_timer_register_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (u32 *)LVT_TIMER_INIT_ADDR;

	if (*init_addr != 0x00010000U) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(38694);

	if (*init_addr != 0x00010000U) {
		is_pass = false;
		printf("%d: init_addr=%d\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name
 * ESR state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC ESR to 0H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38697_ESR_state_of_following_init(void)
{
	volatile u32 *init_addr = (u32 *)ESR_INIT_ADDR;

	report("%s", *init_addr == 0x0U, __FUNCTION__);
}
/**
 * @brief case name
 * SVR state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC SVR to 0FFH following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38698_SVR_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (u32 *)SVR_INIT_ADDR;

	if (*init_addr != 0x0FFU) {
		is_pass = false;
		printf("%d: init_addr=0x%x\n", __LINE__, *init_addr);
	}

	notify_modify_and_read_init_value(38698);
	if (*init_addr != 0x0FFU) {
		is_pass = false;
		printf("%d: init_addr=0x%x\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name
 * ICR state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC ICR  to 0H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38699_ICR_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (u32 *)ICR_INIT_ADDR;

	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: init_addr=0x%x\n", __LINE__, *init_addr);
	}

	notify_modify_and_read_init_value(38699);
	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: init_addr=0x%x\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name
 * TPR state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC TPR to 0H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38700_TPR_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (u32 *)TPR_INIT_ADDR;

	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: TPR=0x%x\n", __LINE__, *init_addr);
	}

	notify_modify_and_read_init_value(38700);

	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: TPR=0x%x\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name
 * PPR state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC PPR to 0H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38701_PPR_state_of_following_init(void)
{
	volatile u32 *init_addr = (u32 *)PPR_INIT_ADDR;

	report("%s", *init_addr == 0x0U, __FUNCTION__);
}
/**
 * @brief case name
 * IRR state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC IRR to 0H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38704_IRR_state_of_following_init(void)
{
	volatile u32 *init_addr = (u32 *)IRR_INIT_ADDR;

	report("%s", *init_addr == 0x0U, __FUNCTION__);
}
/**
 * @brief case name
 * ISR state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC ISR to 0H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38705_ISR_state_of_following_init(void)
{
	volatile u32 *init_addr = (u32 *)ISR_INIT_ADDR;

	report("%s", *init_addr == 0x0U, __FUNCTION__);
}
/**
 * @brief case name
 * TMR state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC TMR to 0H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38706_TMR_state_of_following_init(void)
{
	volatile u32 *init_addr = (u32 *)TMR_INIT_ADDR;

	report("%s", *init_addr == 0x0U, __FUNCTION__);
}
/**
 * @brief case name
 * CR8 state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC CR8 to 0H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38707_CR8_state_of_following_init(void)
{
	volatile ulong *init_addr = (volatile ulong *)CR8_INIT_ADDR;
	bool is_pass = true;

	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: cr8=0x%lx\n", __LINE__, *init_addr);
	}

	notify_modify_and_read_init_value(38707);

	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: cr8=0x%lx\n", __LINE__, *init_addr);
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name
 * Timer initial count register state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC timer initial count register to 0H following INIT
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38708_timer_initial_count_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (volatile u32 *)TICR_INIT_ADDR;

	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: TICR_INIT=0x%x\n", __LINE__, *init_addr);
	}

	notify_modify_and_read_init_value(38708);

	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: TICR_INIT=0x%x\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name
 * Timer current count register state of following INIT
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC timer current count register to 0H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38709_timer_current_count_state_of_following_init(void)
{
	volatile u32 *init_addr = (u32 *)TCCR_INIT_ADDR;

	report("%s", *init_addr == 0x0U, __FUNCTION__);
}

/**
 * @brief case name
 * DCR state of following INIT
 *
 * Summary:ACRN hypervisor shall set initial guest LAPIC DCR to 0H following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38710_DCR_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (volatile u32 *)TDCR_INIT_ADDR;

	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: DCR=0x%x\n", __LINE__, *init_addr);
	}

	notify_modify_and_read_init_value(38710);

	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: DCR=0x%x\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name
 * IA32_TSC_DEADLINE state of following INIT
 *
 * Summary:ACRN hypervisor shall keep guest IA32_TSC_DEADLINE unchanged following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38711_IA32_TSC_DEADLINE_state_of_following_init(void)
{
	bool is_pass = true;
	volatile u32 *init_addr = (volatile u32 *)IA32_TSC_DEADLINE_INIT_ADDR;

	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: tsc_deadline=0x%x\n", __LINE__, *init_addr);
	}

	notify_modify_and_read_init_value(38711);

	if (*init_addr != 0x0U) {
		is_pass = false;
		printf("%d: tsc_deadline=0x%x\n", __LINE__, *init_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name
 * IA32_APIC_BASE state of following INIT
 *
 * Summary:ACRN hypervisor shall keep guest IA32_APIC_BASE unchanged following INIT.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38713_IA32_APIC_BASE_state_of_following_init(void)
{

	volatile u32 *init_addr = (volatile u32 *)IA32_APIC_BASE_INIT_ADDR;
	bool is_pass = true;

	if (*init_addr != 0xfee00c00U) {
		is_pass = false;
		printf("%d: init_addr=0x%x\n", __LINE__, *init_addr);
	}

	notify_modify_and_read_init_value(38713);
	if (*init_addr != 0xfee00c00U) {
		is_pass = false;
		printf("%d: init_addr=0x%x\n", __LINE__, *init_addr);
	}

	report("%s", is_pass, __FUNCTION__);

}

#endif
/**
 * @brief case name APIC Base field state following start-up
 *
 * Summary: ACRN hypervisor shall set initial guest MSR_IA32_APICBASE of BP to 0FEE00D00H
 *  following start-up.
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27922_apic_base_field_state_of_bp_following_start_up_001(void)
{
	volatile u32 *ia32_apic_base = (u32 *)IA32_APIC_BASE_STARTUP_ADDR;

	report("%s", *ia32_apic_base == 0xfee00d00U, __FUNCTION__);
}
/**
 * @brief case name LVT CMCI state following start-up
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC LVT CMCI to 00010000H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38737_LVT_CMCI_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)LVT_CMCI_STARTUP_ADDR;

	report("%s", *addr == 0x00010000U, __FUNCTION__);
}

/**
 * @brief case name LVT LINT0 state following start-up
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC LVT LINT0 to 00010000H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38738_LVT_LINT0_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)LINT0_STARTUP_ADDR;

	report("%s", *addr == 0x00010000U, __FUNCTION__);
}

/**
 * @brief case name LVT LINT1 state following start-up
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC LVT LINT1 to 00010000H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38739_LVT_LINT1_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)LINT1_STARTUP_ADDR;

	report("%s", *addr == 0x00010000U, __FUNCTION__);
}
/**
 * @brief case name LVT Error Register state following start-up
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC LVT Error Register to 00010000H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38740_LVT_ERROR_Register_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)LVT_ERROR_STARTUP_ADDR;

	report("%s", *addr == 0x00010000U, __FUNCTION__);
}

/**
 * @brief case name LVT Thermal Monitor Register state following start-up
 *
 * Summary: ACRN hypervisor shall set initial guest LAPIC LVT Thermal Monitor Register to 00010000H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38741_LVT_Thermal_Monitor_Register_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)LVT_THMR_STARTUP_ADDR;

	report("%s", *addr == 0x00010000U, __FUNCTION__);
}

/**
 * @brief case name  LVT Performance Counter Register state following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC LVT Performance Counter Register to 00010000H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38743_LVT_Performance_Conuter_Register_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)LVT_PCR_STARTUP_ADDR;

	report("%s", *addr == 0x00010000U, __FUNCTION__);
}

/**
 * @brief case name  LVT Timer Register state following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC LVT Timer Register to 00010000H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39455_LVT_Timer_Register_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)LVT_TIMER_STARTUP_ADDR;

	report("%s", *addr == 0x00010000U, __FUNCTION__);
}
/**
 * @brief case name  ESR state following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC ESR to 00010000H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38745_ESR_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)ESR_STARTUP_ADDR;

	report("%s", *addr == 0x0U, __FUNCTION__);
}

/**
 * @brief case name  SVR state following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC SVR to 0FFH following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38746_SVR_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)SVR_STARTUP_ADDR;

	report("%s", *addr == 0x0FFU, __FUNCTION__);
}

/**
 * @brief case name  ICR state following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC ICR to 0H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38747_ICR_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)ICR_STARTUP_ADDR;

	report("%s", *addr == 0x0U, __FUNCTION__);
}

/**
 * @brief case name  TPR state following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC TPR to 0H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38748_TPR_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)TPR_STARTUP_ADDR;

	report("%s", *addr == 0x0U, __FUNCTION__);
}

/**
 * @brief case name  PPR state following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC PPR to 0H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38749_PPR_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)PPR_STARTUP_ADDR;

	report("%s", *addr == 0x0U, __FUNCTION__);
}

/**
 * @brief case name  IRR state following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC IRR to 0H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38750_IRR_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)IRR_STARTUP_ADDR;

	report("%s", *addr == 0x0U, __FUNCTION__);
}

/**
 * @brief case name  ISR state following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC ISR to 0H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38753_ISR_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)ISR_STARTUP_ADDR;

	report("%s", *addr == 0x0U, __FUNCTION__);
}

/**
 * @brief case name  TMR state following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC TMR to 0H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38754_TMR_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)TMR_STARTUP_ADDR;

	report("%s", *addr == 0x0U, __FUNCTION__);
}

/**
 * @brief case name  CR8 state following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC CR8 to 0H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38755_CR8_state_of_following_start_up(void)
{
	volatile ulong *addr = (ulong *)CR8_STARTUP_ADDR;
	report("%s", *addr == 0x0U, __FUNCTION__);
}

/**
 * @brief case name  Timer initial count register state of following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC Timer initial count register to 0H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38756_TICR_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)TICR_STARTUP_ADDR;

	report("%s", *addr == 0x0U, __FUNCTION__);
}
/**
 * @brief case name  Timer current count register state of following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC Timer current count register to 0H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38757_TCCR_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)TCCR_STARTUP_ADDR;

	report("%s", *addr == 0x0U, __FUNCTION__);
}
/**
 * @brief case name  DCR state of following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC DCR to 0H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38758_DCR_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)DCR_STARTUP_ADDR;

	report("%s", *addr == 0x0U, __FUNCTION__);
}
/**
 * @brief case name  IA32_TSC_DEADLINE  state of following start-up
 *
 * Summary:
 * ACRN hypervisor shall set initial guest LAPIC IA32_TSC_DEADLINE to 0H following start-up.
 *
 *  In compliance with Chapter 10.4.4, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38759_IA32_TSC_DEADLINE_state_of_following_start_up(void)
{
	volatile u32 *addr = (u32 *)IA32_TSC_DEADLINE_STARTUP_ADDR;

	report("%s", *addr == 0x0U, __FUNCTION__);
}

/**
 * @brief case name Expose TSC deadline timer mode support
 *
 * Summary: ACRN hypervisor shall expose TSC deadline timer mode support to any VM, in
 *  compliance with Chapter 10.5.4.1, Vol.3, SDM. TSC-deadline timer mode can be used
 *  by software to use the local APIC timer to signal an interrupt at an absolute
 *  time.
 *
 * TSC deadline mode timer is supported if CPUID.01H:ECX.TSC_Deadline[bit 24] is set to 1.
 * Configure the timer LVT register to TSC deadline mode.
 * Write a non-zero 64-bit value into IA32_TSC_DEADLINE MSR to arm the timer.
 * Verify that corresponding timer interrupt is captured.
 *
 *
 */
bool local_apic_rqmid_27651_expose_tsc_deadline_timer_mode_support_001(bool print_flag)
{
	if (!(cpuid(1).c & (1 << 24))) {
		report("%s", 0, __FUNCTION__);
		return false;
	}

	tdt_isr_cnt = 0;
	handle_irq(TSC_DEADLINE_TIMER_VECTOR, tsc_deadline_timer_isr);
	apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | TSC_DEADLINE_TIMER_VECTOR);
	start_dtimer(1000);

	sleep(1100);
	if (print_flag) {
		report("%s", tdt_isr_cnt == 1, __FUNCTION__);
	}

	return (tdt_isr_cnt == 1);
}

/**
 * @brief case name Expose TSC deadline timer mode support
 *
 * Summary: ACRN hypervisor shall expose TSC deadline timer mode support to any VM, in
 *  compliance with Chapter 10.5.4.1, Vol.3, SDM. TSC-deadline timer mode can be used
 *  by software to use the local APIC timer to signal an interrupt at an absolute
 *  time.
 *
 * when change timer mode ,it should set IA32_TSCDEADLINE MSR to 0H.
 * To change timer mode to one shot mode 
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38781_expose_tsc_deadline_timer_mode_support_002(void)
{
	const unsigned int vec = TSC_DEADLINE_TIMER_VECTOR;
	unsigned int lvtt;

	wrmsr(MSR_IA32_TSCDEADLINE, (rdtsc() + 1000));

	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;

	lvtt |= APIC_LVT_TIMER_ONESHOT;
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	report("%s", !rdmsr(MSR_IA32_TSCDEADLINE), __FUNCTION__);
}

/**
 * @brief case name Expose TSC deadline timer mode support
 *
 * Summary: ACRN hypervisor shall expose TSC deadline timer mode support to any VM, in
 *  compliance with Chapter 10.5.4.1, Vol.3, SDM. TSC-deadline timer mode can be used
 *  by software to use the local APIC timer to signal an interrupt at an absolute
 *  time.
 *
 * when change timer mode ,it should set IA32_TSCDEADLINE MSR to 0H.
 * To change timer mode to Periodic.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38782_expose_tsc_deadline_timer_mode_support_003(void)
{
	const unsigned int vec = TSC_DEADLINE_TIMER_VECTOR;
	unsigned int lvtt;

	wrmsr(MSR_IA32_TSCDEADLINE, (rdtsc() + 1000));

	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;
	lvtt |= APIC_LVT_TIMER_PERIODIC;
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	report("%s", !rdmsr(MSR_IA32_TSCDEADLINE), __FUNCTION__);
}

/**
 * @brief case name Expose TSC deadline timer mode support
 *
 * Summary: ACRN hypervisor shall expose TSC deadline timer mode support to any VM, in
 *  compliance with Chapter 10.5.4.1, Vol.3, SDM. TSC-deadline timer mode can be used
 *  by software to use the local APIC timer to signal an interrupt at an absolute
 *  time.
 *
 *  when set APIC_LVTT[18:17] to 2H, the Initial Counter Register and Current Count Register will become 0H
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38783_expose_tsc_deadline_timer_mode_support_004(void)
{
	const unsigned int vec = TSC_DEADLINE_TIMER_VECTOR;
	unsigned int lvtt;

	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;

	lvtt |= APIC_LVT_TIMER_TSCDEADLINE;/*set APIC_LVTT[18:17] to 2H*/
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	report("%s", !apic_read(APIC_TMICT) && !apic_read(APIC_TMCCT), __FUNCTION__);
}

/**
 * @brief case name Expose TSC deadline timer mode support
 *
 * Summary: ACRN hypervisor shall expose TSC deadline timer mode support to any VM, in
 *  compliance with Chapter 10.5.4.1, Vol.3, SDM. TSC-deadline timer mode can be used
 *  by software to use the local APIC timer to signal an interrupt at an absolute
 *  time.
 *
 *   when APIC_LVTT[18] is 1H, writing to Initial Count Register will be ignored. In Chapter 10.5.4.1 of SDM vol3.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38784_expose_tsc_deadline_timer_mode_support_005(void)
{
	const unsigned int vec = TSC_DEADLINE_TIMER_VECTOR;
	unsigned int lvtt;
	u32 val;

	val = apic_read(APIC_TMICT);

	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;
	lvtt |= (1 << 18); /*set bit[18] to 1*/
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	apic_write(APIC_TMICT, val + 1); /*writing to Initial Count Register will be ignored*/
	report("%s", apic_read(APIC_TMICT) == val, __FUNCTION__);
}

/**
 * @brief case name Expose TSC deadline timer mode support
 *
 * Summary: ACRN hypervisor shall expose TSC deadline timer mode support to any VM, in
 *  compliance with Chapter 10.5.4.1, Vol.3, SDM. TSC-deadline timer mode can be used
 *  by software to use the local APIC timer to signal an interrupt at an absolute
 *  time.
 *
 *   when APIC_LVTT[18] is 1H, reading current count register will get 0H. In Chapter 10.5.4.1 of SDM vol3.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38785_expose_tsc_deadline_timer_mode_support_006(void)
{
	const unsigned int vec = TSC_DEADLINE_TIMER_VECTOR;
	unsigned int lvtt;

	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;

	lvtt |= (1 << 18);
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	report("%s", apic_read(APIC_TMCCT) == 0, __FUNCTION__);
}

/**
 * @brief case name Expose TSC deadline timer mode support
 *
 * Summary: ACRN hypervisor shall expose TSC deadline timer mode support to any VM, in
 *  compliance with Chapter 10.5.4.1, Vol.3, SDM. TSC-deadline timer mode can be used
 *  by software to use the local APIC timer to signal an interrupt at an absolute
 *  time.
 *
 * if APIC_LVTT [16] is 1H and APIC_LVTT[18:17] is 2H, when DEADLINE timer time up,
 * the IA32_TSC_DEADLINE_MSR  will be 0 and the timer interrupt request does not send.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38786_expose_tsc_deadline_timer_mode_support_007(void)
{
	const unsigned int vec = TSC_DEADLINE_TIMER_VECTOR;
	unsigned int lvtt;

	if (!(cpuid(1).c & (1 << 24))) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	tdt_isr_cnt = 0;
	handle_irq(vec, tsc_deadline_timer_isr);

	/*clear all the related bits*/
	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;

	lvtt |= APIC_LVT_MASKED;/* APIC_LVTT [16] is 1H*/
	lvtt |= APIC_LVT_TIMER_TSCDEADLINE;/*APIC_LVTT[18:17] is 2H*/
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	start_dtimer(1000);
	sleep(1100);

	report("%s", (tdt_isr_cnt == 0) && !rdmsr(MSR_IA32_TSCDEADLINE), __FUNCTION__);
}
/**
 * @brief case name Expose TSC deadline timer mode support
 *
 * Summary: ACRN hypervisor shall expose TSC deadline timer mode support to any VM, in
 *  compliance with Chapter 10.5.4.1, Vol.3, SDM. TSC-deadline timer mode can be used
 *  by software to use the local APIC timer to signal an interrupt at an absolute
 *  time.
 *
 * when APIC_LVTT[18:17]  is  not 2H, writing to TSC_DEADLINE_MSR will be ignored.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38787_expose_tsc_deadline_timer_mode_support_008(void)
{
	const unsigned int vec = TSC_DEADLINE_TIMER_VECTOR;
	unsigned int lvtt;
	u64 val;

	/*clear all the related bits*/
	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;

	lvtt |= APIC_LVT_TIMER_MASK;/*set APIC_LVTT[18:17] to 3*/
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	val = rdmsr(MSR_IA32_TSCDEADLINE);

	wrmsr(MSR_IA32_TSCDEADLINE, val + 10);/*writing to TSC_DEADLINE_MSR will be ignored */
	report("%s", rdmsr(MSR_IA32_TSCDEADLINE) == val, __FUNCTION__);
}

/**
 * @brief case name Expose TSC deadline timer mode support
 *
 * Summary: ACRN hypervisor shall expose TSC deadline timer mode support to any VM, in
 *  compliance with Chapter 10.5.4.1, Vol.3, SDM. TSC-deadline timer mode can be used
 *  by software to use the local APIC timer to signal an interrupt at an absolute
 *  time.
 *
 * when APIC_LVTT[18:17]  is  not 2H, reading TSC_DEADLINE_MSR will get 0H.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38788_expose_tsc_deadline_timer_mode_support_009(void)
{
	const unsigned int vec = TSC_DEADLINE_TIMER_VECTOR;
	unsigned int lvtt;

	/*clear all the related bits firstly*/
	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;

	lvtt |= APIC_LVT_TIMER_MASK; /*set APIC_LVTT[18:17]  is  not 3H */
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	report("%s", rdmsr(MSR_IA32_TSCDEADLINE) == 0, __FUNCTION__);
}

/**
 * @brief case name Expose One-shot timer mode support
 *
 * Summary: ACRN hypervisor shall expose one-shot timer mode support to any VM, in
 *  compliance with Chapter 10.5.4, Vol.3, SDM. The timer shall can be configured
 *  through the timer LVT entry for one-shot mode. In one-shot mode, the timer is
 *  started by programming its initial-count register. The initial count value is
 *  then copied into the currentcount register and count-down begins. After the timer
 *  reaches zero, an timer interrupt is generated and the timer remains at its 0
 *  value until reprogrammed.
 *
 * Configure the timer LVT register to one-shot mode.
 * Trigger the APIC timer by writing an initial value to the initial-count register.
 * Verify that corresponding timer interrupt is captured only one time.
 * @param print_flag; true ----report the result; false ----not report the result
 *
 * @retval true: PASS; false: failed
 *
 */
bool local_apic_rqmid_27648_expose_one_shot_timer_mode_support_001(bool print_flag)
{
	const unsigned int vec = LAPIC_TEST_VEC;
	unsigned initial_value = LAPIC_TIMER_INITIAL_VAL;
	unsigned int lvtt;

	lapic_timer_cnt = 0;
	handle_irq(vec, lapic_timer_handler);

	/*clear all the related bits firstly*/
	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;

	lvtt |= APIC_LVT_TIMER_ONESHOT;
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	apic_write(APIC_TMICT, initial_value);
	sleep(initial_value);

	if (print_flag) {
		report("%s", lapic_timer_cnt == 1, __FUNCTION__);
	}
	return (lapic_timer_cnt == 1);
}


/**
 * @brief case name Expose One-shot timer mode support
 *
 * Summary: ACRN hypervisor shall expose one-shot timer mode support to any VM, in
 *  compliance with Chapter 10.5.4, Vol.3, SDM. The timer shall can be configured
 *  through the timer LVT entry for one-shot mode. In one-shot mode, the timer is
 *  started by programming its initial-count register. The initial count value is
 *  then copied into the currentcount register and count-down begins. After the timer
 *  reaches zero, an timer interrupt is generated and the timer remains at its 0
 *  value until reprogrammed.
 *
 * if APIC_LVTT[16] is 1H,the one-shot timer interrupt should not be sent.when timer reach 0;
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38789_expose_one_shot_timer_mode_support_002(void)
{
	const unsigned int vec = LAPIC_TEST_VEC;
	unsigned initial_value = LAPIC_TIMER_INITIAL_VAL;
	unsigned int lvtt;

	lapic_timer_cnt = 0;
	handle_irq(vec, lapic_timer_handler);

	/*clear all the related bits firstly*/
	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;

	/*set APIC_LVTT[16] to 1 and arm oneshot timer*/
	lvtt |= APIC_LVT_MASKED;
	lvtt |= APIC_LVT_TIMER_ONESHOT;
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	apic_write(APIC_TMICT, initial_value);
	sleep(initial_value);

	report("%s", lapic_timer_cnt == 0, __FUNCTION__);
}

/**
 * @brief case name Expose Periodic timer mode support
 *
 * Summary: ACRN hypervisor shall expose periodic timer mode support to any VM, in
 *  compliance with Chapter 10.5.4, Vol.3, SDM. The timer shall can be configured
 *  through the timer LVT entry for periodic mode. In periodic mode, the
 *  current-count register is automatically reloaded from the initial-count register
 *  when the count reaches 0 and a timer interrupt is generated, and the count-down
 *  is repeated. If during the count-down process the initial-count register is set,
 *  counting will restart, using the new initial-count value. The initial-count
 *  register is a read-write register; the current-count register is read only.
 *
 * Configure the timer LVT register to periodic mode.
 * Trigger the APIC timer by writing an initial value to the initial-count register.
 * Verify that corresponding timer interrupt is captured at least 2 times.
 *
 * @param print_flag
 * true: printf the test result;false: don't printf the test result;
 *
 * @retval true:pass; false:failed
 *
 */
bool  local_apic_rqmid_27645_expose_periodic_timer_mode_support_001(bool print_flag)
{
	const unsigned int vec = LAPIC_TEST_VEC;
	unsigned initial_value = LAPIC_TIMER_INITIAL_VAL;
	unsigned int lvtt;

	lapic_timer_cnt = 0;
	handle_irq(vec, lapic_timer_handler);

	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;

	lvtt |= vec;
	lvtt |= APIC_LVT_TIMER_PERIODIC;
	apic_write(APIC_LVTT, lvtt);

	apic_write(APIC_TDCR, 0x0000000b);
	apic_write(APIC_TMICT, initial_value);
	sleep(initial_value * 3 + 100);

	if (print_flag) {
		report("%s", lapic_timer_cnt >= 2, __FUNCTION__);
	}

	apic_write(APIC_TMICT, 0);
	return (lapic_timer_cnt >= 2);
}

/**
 * @brief case name Expose Periodic timer mode support
 *
 * Summary: ACRN hypervisor shall expose periodic timer mode support to any VM, in
 *  compliance with Chapter 10.5.4, Vol.3, SDM. The timer shall can be configured
 *  through the timer LVT entry for periodic mode. In periodic mode, the
 *  current-count register is automatically reloaded from the initial-count register
 *  when the count reaches 0 and a timer interrupt is generated, and the count-down
 *  is repeated. If during the count-down process the initial-count register is set,
 *  counting will restart, using the new initial-count value. The initial-count
 *  register is a read-write register; the current-count register is read only.
 *
 * if APIC_LVTT[16] is 1, the Periodic timer interrupt should not send.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_38790_expose_periodic_timer_mode_support_002(void)
{
	const unsigned int vec = LAPIC_TEST_VEC;
	unsigned initial_value = LAPIC_TIMER_INITIAL_VAL;
	unsigned int lvtt;

	lapic_timer_cnt = 0;
	handle_irq(vec, lapic_timer_handler);

	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;

	lvtt |= vec;
	lvtt |= APIC_LVT_MASKED;/*APIC_LVTT[16] is 1*/
	lvtt |= APIC_LVT_TIMER_PERIODIC;
	apic_write(APIC_LVTT, lvtt);

	apic_write(APIC_TMICT, initial_value);
	sleep(initial_value * 3 + 100);

	report("%s", lapic_timer_cnt == 0, __FUNCTION__);
	apic_write(APIC_TMICT, 0);
}

/**
 * @brief case name LAPIC Timer Mode Configration
 *
 * Summary: ACRN hypervisor shall expose timer mode configration support to any VM, in
 *  compliance with Chapter 10.5.4, Vol.3, SDM. The timer can be configured through
 *  the timer LVT entry for one-shot mode, periodic mode or TSC-Deadline Mode, in
 *  compliance with Chapter 10.5.4, Vol.3, SDM.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27697_lapic_timer_mode_configuration(void)
{
	bool ret1, ret2, ret3;

	ret1 = local_apic_rqmid_27648_expose_one_shot_timer_mode_support_001(false);
	ret2 = local_apic_rqmid_27645_expose_periodic_timer_mode_support_001(false);
	ret3 = local_apic_rqmid_27651_expose_tsc_deadline_timer_mode_support_001(false);

	report("%s", ret1 && ret2 && ret3, __FUNCTION__);
}
/*strategy----480:Local APIC IPI support----
 *Test if the Hypervisor follows the SPEC/Requirement to make local APIC expose IPI support to VM./
 */
void lapic_ipi_support_isr(isr_regs_t *regs)
{
	++ipi_count;
	asm volatile("nop");
}
/**
 * @brief case name Expose IPI support
 *
 * Summary: ACRN hypervisor shall expose IPI support to any VM, in compliance with Chapter
 *  10.6, Chapter 10.12.10 and Figure 10-28, Vol.3, SDM.
 *  IPI mechanism will be used by VM to issuing inter-processor interrupts. This
 *  include ICR and LDR. As only x2apic mode is supported, DFR is not available to
 *  VMs.
 *
 * Enable x2APIC on current logical processor.
 * Write APIC ICR register to issue a SELF IPI with physical destination mode and no shorthand.
 * Verify that the IPI is delivered by checking corresponding bit of APIC ISR is set.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27642_expose_ipi_support_001(void)
{
	u32 isr_n;

	ipi_count = 0;
	handle_irq(LAPIC_TEST_VEC, lapic_ipi_support_isr);

	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_FIXED | LAPIC_TEST_VEC, 0);
	sleep(100);

	/*find the vector of corresponding ISR*/
	isr_n = apic_read(APIC_ISR + LAPIC_ISR_INDEX(LAPIC_TEST_VEC / 32));
	eoi();
	/*check the corresponding bit in ISR*/
	report("%s", (ipi_count == 1) && (isr_n & (1U << (LAPIC_TEST_VEC % 32))), __FUNCTION__);
}
#ifdef IN_NON_SAFETY_VM
static bool IPI_RESULT = false;
void lapic_ipi_delivery_handler(isr_regs_t *regs)
{
	u32 isr_n;
	isr_n = apic_read(APIC_ISR + LAPIC_ISR_INDEX(LAPIC_TEST_VEC / 32));
	IPI_RESULT = isr_n & (1U << (LAPIC_TEST_VEC % 32));
	eoi();
}

/**
 * @brief case name x2APIC IPI Delivery in Physical Destination Mode
 *
 * Summary: ACRN hypervisor shall expose IPI delivery in physical destination mode, in
 *  compliance with Chapter 10.12.9, Vol.3, SDM.
 *  x2APIC IPI behavior in compliance with Chapter 10.12.9, Vol.3, SDM.
 *
 * Write APIC ICR register with its bit 11 as 0H (physical destination mode).
 * Verify that the IPI is delivered by checking corresponding bit of APIC ISR is set.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27620_x2apic_ipi_delivery_in_physical_destination_mode(void)
{
	handle_irq(LAPIC_TEST_VEC, lapic_ipi_delivery_handler);
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_FIXED | LAPIC_TEST_VEC, 1);

	sleep(1000);
	report("%s", IPI_RESULT, __FUNCTION__);
	IPI_RESULT = false;
}
#endif
/**
 * @brief case name x2APIC IPI Delivery in Logical Destination Mode
 *
 * Summary: When a vCPU writes LAPIC ICR and the new guest LAPIC ICR [bit 11] is 1H, ACRN
 *  hypervisor shall guarantee that the IPI issue request is ignored.
 *  Logical destination can be used neither in Zephyr nor Linux in the current
 *  scope.
 *
 * Enable x2APIC on current logical processor.
 * Write APIC ICR register with its bit 11 as 1H (logical destination mode).
 * Verify that the IPI is ignored by checking APIC ISR is not set.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27619_x2apic_ipi_delivery_in_logical_destination_mode(void)
{
	u32 isr_n;

	handle_irq(LAPIC_TEST_VEC, lapic_ipi_support_isr);
	apic_icr_write(APIC_DEST_LOGICAL | APIC_DM_FIXED | LAPIC_TEST_VEC, 0);

	sleep(100);
	isr_n = apic_read(APIC_ISR + LAPIC_ISR_INDEX(LAPIC_TEST_VEC / 32));
	report("%s", (isr_n & (1U << (LAPIC_TEST_VEC % 32))) == 0, __FUNCTION__);

}
/**
 * @brief case name x2APIC IPI Delivery in Self/All including Self/All Excluding Self Mode 001
 *
 * Summary:When a vCPU writes LAPIC ICR and the new guest LAPIC ICR [bit 19:18] is different from 0H,
 * ACRN hypervisor shall guarantee that the IPI issue request is ignored.
 *
 * Enable x2APIC on current logical processor.
 * Write APIC ICR register with its bit [19:18] as 01 (self), 10 (all including self) and 11 (all excluding self).
 * Verify that the IPI is ignored by checking APIC ISR is not set.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39301_x2apic_ipi_delivery_in_self_all_including_all_excluding_mode_001(void)
{
	u32 isr_n;

	handle_irq(LAPIC_TEST_VEC, lapic_ipi_support_isr);
	apic_icr_write(APIC_DEST_SELF | APIC_DM_FIXED | LAPIC_TEST_VEC, 0);

	sleep(100);
	isr_n = apic_read(APIC_ISR + LAPIC_ISR_INDEX(LAPIC_TEST_VEC / 32));
	report("%s", (isr_n & (1U << (LAPIC_TEST_VEC % 32))) == 0, __FUNCTION__);
}

/**
 * @brief case name x2APIC IPI Delivery in Self/All including Self/All Excluding Self Mode 002
 *
 * Summary:When a vCPU writes LAPIC ICR and the new guest LAPIC ICR [bit 19:18] is different from 0H,
 * ACRN hypervisor shall guarantee that the IPI issue request is ignored.
 *
 * Enable x2APIC on current logical processor.
 * Write APIC ICR register with its bit [19:18] as 01 (self), 10 (all including self) and 11 (all excluding self).
 * Verify that the IPI is ignored by checking APIC ISR is not set.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39306_x2apic_ipi_delivery_in_self_all_including_all_excluding_mode_002(void)
{
	u32 isr_n;

	handle_irq(LAPIC_TEST_VEC, lapic_ipi_support_isr);
	apic_icr_write(APIC_DEST_ALLINC | APIC_DM_FIXED | LAPIC_TEST_VEC, 0);

	sleep(100);
	isr_n = apic_read(APIC_ISR + LAPIC_ISR_INDEX(LAPIC_TEST_VEC / 32));
	report("%s", (isr_n & (1U << (LAPIC_TEST_VEC % 32))) == 0, __FUNCTION__);
}

/**
 * @brief case name x2APIC IPI Delivery in Self/All including Self/All Excluding Self Mode 003
 *
 * Summary:When a vCPU writes LAPIC ICR and the new guest LAPIC ICR [bit 19:18] is different from 0H,
 * ACRN hypervisor shall guarantee that the IPI issue request is ignored.
 *
 * Enable x2APIC on current logical processor.
 * Write APIC ICR register with its bit [19:18] as 01 (self), 10 (all including self) and 11 (all excluding self).
 * Verify that the IPI is ignored by checking APIC ISR is not set.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39308_x2apic_ipi_delivery_in_self_all_including_all_excluding_mode_003(void)
{
	u32 isr_n;

	handle_irq(LAPIC_TEST_VEC, lapic_ipi_support_isr);
	apic_icr_write(APIC_DEST_ALLBUT | APIC_DM_FIXED | LAPIC_TEST_VEC, 0);

	sleep(100);
	isr_n = apic_read(APIC_ISR + LAPIC_ISR_INDEX(LAPIC_TEST_VEC / 32));
	report("%s", (isr_n & (1U << (LAPIC_TEST_VEC % 32))) == 0, __FUNCTION__);
}

/**
 * @brief case name ignore ICR write of level bit
 *
 * Summary: When a vCPU attempts to write ICR[bit 14], ACRN hypervisor shall guarantee that
 *  the write to the bit is ignored.
 *  This flag has no meaning in Pentium 4 and Intel Xeon processors and will always
 *  be issued as a 1. Write this bit on native will be ignored. Keep the behavior the
 *  same with the native.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27716_ignore_icr_write_of_level_bit(void)
{
	u32 val0;
	u32 val1;

	val0 = apic_read(APIC_ICR);
	apic_icr_write((val0 ^ (1U << 14)), 0);
	val1 = apic_read(APIC_ICR);
	report("%s", val0 == val1, __FUNCTION__);
}

/**
 * @brief case name ignore ICR write of trigger mode bit
 *
 * Summary: When a vCPU attempts to write ICR[bit 15], ACRN hypervisor shall guarantee that
 *  the write to the bit is ignored.
 *  This flag has no meaning in Pentium 4 and Intel Xeon processors, and will always
 *  be issued as a 0. Write this bit on native will be ignored. Keep the behavior
 *  the same with the native.
 *
 * Set trigger mode bit 15 of APIC ICR register as 1. Read the register and verify that bit 15 keeps 0,
 * which means the write is ignored.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27714_ignore_icr_write_of_trigger_mode_bit(void)
{
	u32 val;

	val = apic_read(APIC_ICR);
	val |= (1U << 15);
	apic_icr_write(val, 0);

	report("%s", !(apic_read(APIC_ICR) & (1U << 15)), __FUNCTION__);
}

/**
 * @brief case name ignore write of EOI when ISR is 0H
 *
 * Summary: When a vCPU attempts to write LAPIC EOI register and the guest LAPIC ISR is 0H,
 *  ACRN hypervisor shall guarantee that the write is ignored.
 *
 *
 * 1. When reading from APIC ISR register is 0H, which means no interrupt is in-service,
 * write APIC EOI register with a zero value and check again if ISR is unchanged (0).
 * 2. When reading from APIC ISR register is 1H,
 * write APIC EOI register with a zero value and check again if ISR is unchanged (0)
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27531_ignore_write_of_eoi_when_isr_is_0h(void)
{
	eoi();
	/* there is no interrupt, so ISR should be 0, */
	for (int i = 0; i < APIC_ISR_NR; i++) {
		if (apic_read(APIC_ISR + (i << 4))) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	eoi();
	/* send EOI, ISR should be unchanged (0) */
	for (int i = 0; i < APIC_ISR_NR; i++) {
		if (apic_read(APIC_ISR + (i << 4))) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	handle_irq(LAPIC_TEST_VEC, lapic_ipi_isr_no_eoi);
	apic_write(APIC_SELF_IPI, LAPIC_TEST_VEC);
	sleep(100);
	/* the ISR bit should be 1*/
	if (apic_read(APIC_ISR + ((LAPIC_TEST_VEC / 32) << 4)) != (1U << (LAPIC_TEST_VEC % 32))) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	eoi();
	/* send EOI, all ISRs should be 0 */
	for (int i = 0; i < APIC_ISR_NR; i++) {
		if (apic_read(APIC_ISR + (i << 4))) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}


/**
 * @brief case name Ignore MSI with vector 0FFH
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 0FFH,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *	Trigger a fixed message signaled interrupt (MSI) with the vector as 0FFH in the MSI message data register.
 * Verify that the MSI is ignored.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39017_ignore_MSI_with_vector_0FFH()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0xFFU);/*invalid vector 0xff*/
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name Ignore MSI while software-disabled
 *
 * Summary: When a message signalled interrupt is triggered and the guest LAPIC SVR [bit 8] is 0,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *	Clear bit 8 of APIC SVR register to 0 to software disable local APIC. Trigger an MSI interrupt.
 * Verify that the MSI is ignored.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39040_ignore_MSI_while_software_diabled()
{
	u32 svr_val;
	bool ret = false;
	bool is_pass = false;
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	union pci_bdf bdf = {0};

	svr_val = apic_read(APIC_SPIV);
	apic_write(APIC_SPIV, svr_val & (~APIC_SPIV_APIC_ENABLED));

	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0xFFU);/*invalid vector 0xff*/
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}

	apic_write(APIC_SPIV, 0x1FF);
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name local_apic_rqmid_27600_read_only_ppr
 *
 * Summary: ACRN hypervisor shall guarantee the PPR is read-only,
 * in compliance with Chapter 10.8.3.1, Vol.3, SDM.
 *
 *	Try to write PPR register, and check the register value.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27600_read_only_ppr()
{
	u32 val;

	val = apic_read(APIC_PROCPRI);

	asm volatile (ASM_TRY("1f")
				  "wrmsr \n"
				  "1:"
				  : : "a"(val + 1), "d"(0), "c"(APIC_BASE_MSR + APIC_PROCPRI/16));

	report("%s", apic_read(APIC_PROCPRI) == val, __FUNCTION__);
}

/**
 * @brief case name Read-Only APIC Base
 *
 * Summary: ACRN hypervisor shall guarantee guest MSR_IA32_APICBASE is read-only.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27593_read_only_apic_base(void)
{
	u64 val;

	val = rdmsr(MSR_IA32_APICBASE);

	wrmsr_checking(MSR_IA32_APICBASE, val + (1U << 12));/*Try to change Base physical address*/

	report("%s", rdmsr(MSR_IA32_APICBASE) == val, __FUNCTION__);
}
/**
 * @brief case name Read-Only Local APIC Version Register
 *
 * Summary: ACRN hypervisor shall guarantee the Local APIC Version Register is read-only.
 *  In compliance with Table 10-1 and Table 10-6, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27592_read_only_local_apic_version_register(void)
{
	u64 val;

	val = apic_read(APIC_LVR);
	wrmsr_checking(APIC_BASE_MSR + APIC_LVR / 16, val + 1);

	report("%s", apic_read(APIC_LVR) == val, __FUNCTION__);
}
/**
 * @brief case name Read-Only ISR
 *
 * Summary: ACRN hypervisor shall guarantee the ISR is read-only.
 *  In compliance with Table 10-1 and Table 10-6, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27589_read_only_isr(void)
{
	u32 val;

	val = apic_read(APIC_ISR);
	wrmsr_checking(APIC_BASE_MSR + APIC_ISR / 16, val + 1);
	report("%s",  apic_read(APIC_ISR) == val, __FUNCTION__);
}

/**
 * @brief case name Read-Only TMR
 *
 * Summary: ACRN hypervisor shall guarantee the TMR is read-only.
 *  In compliance with Table 10-1 and Table 10-6, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27588_read_only_tmr(void)
{
	u32 val;

	val = apic_read(APIC_TMR);
	wrmsr_checking(APIC_BASE_MSR + APIC_TMR / 16, val + 1);
	report("%s", apic_read(APIC_TMR) == val, __FUNCTION__);
}

/**
 * @brief case name Read-Only IRR
 *
 * Summary: ACRN hypervisor shall guarantee the IRR is read-only.
 *  In compliance with Table 10-1 and Table 10-6, Vol.3, SDM.
 *
 * Try to write IRR register , and it will generate exception.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27587_read_only_irr(void)
{
	u32 val;

	val = apic_read(APIC_IRR);
	wrmsr_checking(APIC_BASE_MSR + APIC_IRR / 16, val + 1);
	report("%s",  apic_read(APIC_IRR) == val, __FUNCTION__);
}

/**
 * @brief case name Read-Only Current Count Register
 *
 * Summary: ACRN hypervisor shall guarantee the Current Count Register is read-only.
 *  In compliance with Table 10-1 and Table 10-6, Vol.3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27585_read_only_current_count_register(void)
{
	u32 val = 0;

	val = apic_read(APIC_TMCCT);
	wrmsr_checking(APIC_BASE_MSR + APIC_TMCCT / 16, val + 1);
	report("%s",  apic_read(APIC_TMCCT) == val, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr x2APIC read only register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the LAPIC register is read-only,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *Try to write PPR register, and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38921_inject_gp_when_wrmsr_x2apic_read_only_PPR_register()
{
	u64 val = 0;

	wrmsr_checking(APIC_BASE_MSR + APIC_PROCPRI / 16, val);

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr x2APIC read only register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the LAPIC register is read-only,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *Try to write IA32_APIC_BASE register, and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38962_inject_gp_when_wrmsr_x2apic_read_only_IA32_APIC_BASE_register()
{
	u64 val = 0;

	wrmsr_checking(MSR_IA32_APICBASE, val);
	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr x2APIC read only register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the LAPIC register is read-only,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *Try to write local APIC version register, and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38963_inject_gp_when_wrmsr_x2apic_read_only_APIC_version_register()
{
	u64 val = 0;

	wrmsr_checking(APIC_BASE_MSR + APIC_LVR / 16, val);
	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr x2APIC read only register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the LAPIC register is read-only,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *Try to write APIC ISR register, and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38964_inject_gp_when_wrmsr_x2apic_read_only_ISR_register()
{
	u64 val = 0;

	wrmsr_checking(APIC_BASE_MSR + APIC_ISR / 16, val);
	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr x2APIC read only register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the LAPIC register is read-only,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *Try to write APIC TMR register, and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38965_inject_gp_when_wrmsr_x2apic_read_only_TMR_register()
{
	u64 val = 0;

	wrmsr_checking(APIC_BASE_MSR + APIC_TMR / 16, val);
	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr x2APIC read only register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the LAPIC register is read-only,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *Try to write APIC IRR register, and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38966_inject_gp_when_wrmsr_x2apic_read_only_IRR_register()
{
	u64 val = 0;

	wrmsr_checking(APIC_BASE_MSR + APIC_IRR / 16, val);
	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr x2APIC read only register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the LAPIC register is read-only,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 * Try to write current-count register, and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38967_inject_gp_when_wrmsr_x2apic_read_only_Current_Count_register()
{
	u64 val = 0;

	wrmsr_checking(APIC_BASE_MSR + APIC_TMCCT / 16, val);
	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr x2APIC read only register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the LAPIC register is read-only,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *Try to write local APIC ID register, and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38968_inject_gp_when_wrmsr_x2apic_read_only_LAPIC_ID_register()
{
	u64 val = 0;

	wrmsr_checking(APIC_BASE_MSR + APIC_ID / 16, val);
	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr x2APIC read only register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the LAPIC register is read-only,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *Try to write local APIC_LDR register, and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38969_inject_gp_when_wrmsr_x2apic_read_only_APIC_LDR_register()
{
	u64 val = 0;

	wrmsr_checking(APIC_BASE_MSR + APIC_LDR / 16, val);
	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr attempts to set a reserved bit to 1 in a read/write register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the new value has any reserved bit set to 1H,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *	Write 1 to APIC SVR register reserved bits , and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38970_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_001()
{
	u32 val;
	u32 resv_bits = 0xFFFFEE00;/*APIC SVR reserved bits mask*/

	val = apic_read(APIC_SPIV);
	for (int i = 0; i < 32; i++) {
		if (resv_bits & (1U < i)) {
			wrmsr_checking(APIC_BASE_MSR + APIC_SPIV / 16, val | (resv_bits & (1U < i)));
			if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr attempts to set a reserved bit to 1 in a read/write register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the new value has any reserved bit set to 1H,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *	Write 1 to APIC LVT timer register reserved bits , and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38976_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_002()
{
	u32 val;
	u32 resv_bits = 0xFFF8EF00;/*reserved bits mask*/

	val = apic_read(APIC_LVTT);
	for (int i = 0; i < 32; i++) {
		if (resv_bits & (1U < i)) {
			wrmsr_checking(APIC_BASE_MSR + APIC_LVTT / 16, val | (resv_bits & (1U < i)));
			if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr attempts to set a reserved bit to 1 in a read/write register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the new value has any reserved bit set to 1H,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *	Write 1 to APIC LVT CMCI register reserved bits , and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38981_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_003()
{
	u32 val;
	u32 resv_bits = 0xFFFEE800;/*reserved bits mask*/

	val = apic_read(APIC_LVTCMCI);
	for (int i = 0; i < 32; i++) {
		if (resv_bits & (1U < i)) {
			wrmsr_checking(APIC_BASE_MSR + APIC_LVTCMCI / 16, val | (resv_bits & (1U < i)));
			if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr attempts to set a reserved bit to 1 in a read/write register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the new value has any reserved bit set to 1H,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *	Write 1 to APIC Thermal register reserved bits , and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38983_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_004()
{
	u32 val;
	u32 resv_bits = 0xFFFEE800;/*reserved bits mask*/

	val = apic_read(APIC_LVTTHMR);
	for (int i = 0; i < 32; i++) {
		if (resv_bits & (1U < i)) {
			wrmsr_checking(APIC_BASE_MSR + APIC_LVTTHMR / 16, val | (resv_bits & (1U < i)));
			if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr attempts to set a reserved bit to 1 in a read/write register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the new value has any reserved bit set to 1H,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *	Write 1 to APIC LVT PMC register reserved bits , and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38984_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_005()
{
	u32 val;
	u32 resv_bits = 0xFFFEE800;/*reserved bits mask*/

	val = apic_read(APIC_LVTPC);
	for (int i = 0; i < 32; i++) {
		if (resv_bits & (1U < i)) {
			wrmsr_checking(APIC_BASE_MSR + APIC_LVTPC / 16, val | (resv_bits & (1U < i)));
			if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr attempts to set a reserved bit to 1 in a read/write register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the new value has any reserved bit set to 1H,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *	Write 1 to APIC LVT LINT0 register reserved bits , and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38985_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_006()
{
	u32 val;
	u32 resv_bits = 0xFFFE0800;/*reserved bits mask*/

	val = apic_read(APIC_LVT0);
	for (int i = 0; i < 32; i++) {
		if (resv_bits & (1U < i)) {
			wrmsr_checking(APIC_BASE_MSR + APIC_LVT0 / 16, val | (resv_bits & (1U < i)));
			if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr attempts to set a reserved bit to 1 in a read/write register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the new value has any reserved bit set to 1H,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *	Write 1 to APIC LVT LINT1 register reserved bits , and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38988_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_007()
{
	u32 val;
	u32 resv_bits = 0xFFFE0800;/*reserved bits mask*/

	val = apic_read(APIC_LVT1);
	for (int i = 0; i < 32; i++) {
		if (resv_bits & (1U < i)) {
			wrmsr_checking(APIC_BASE_MSR + APIC_LVT1 / 16, val | (resv_bits & (1U < i)));
			if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr attempts to set a reserved bit to 1 in a read/write register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the new value has any reserved bit set to 1H,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *	Write 1 to APIC LVT Error register reserved bits , and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38989_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_008()
{
	u32 val;
	u32 resv_bits = 0xFFFEEF00;/*reserved bits mask*/

	val = apic_read(APIC_LVTERR);
	for (int i = 0; i < 32; i++) {
		if (resv_bits & (1U < i)) {
			wrmsr_checking(APIC_BASE_MSR + APIC_LVTERR / 16, val | (resv_bits & (1U < i)));
			if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr attempts to set a reserved bit to 1 in a read/write register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the new value has any reserved bit set to 1H,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *	Write 1 to APIC divide configuration register reserved bits , and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38990_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_009()
{
	u32 val;
	u32 resv_bits = 0xFFFFFF04;/*reserved bits mask*/

	val = apic_read(APIC_TDCR);
	for (int i = 0; i < 32; i++) {
		if (resv_bits & (1U < i)) {
			wrmsr_checking(APIC_BASE_MSR + APIC_TDCR / 16, val | (resv_bits & (1U < i)));
			if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr attempts to set a reserved bit to 1 in a read/write register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the new value has any reserved bit set to 1H,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *	Write 1 to APIC ICR register reserved bits , and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38992_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_010()
{
	u64 val;
	u64 resv_bits = 0xFFFFFF04;/*reserved bits mask*/

	val = rdmsr(APIC_BASE_MSR + APIC_ICR / 16);
	for (int i = 0; i < 63; i++) {
		if (resv_bits & (1UL < i)) {
			wrmsr_checking(APIC_BASE_MSR + APIC_ICR / 16, val | (resv_bits & (1U < i)));
			if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when wrmsr attempts to set a reserved bit to 1 in a read/write register
 *
 * Summary: When a vCPU attempts to write a LAPIC register and the new value has any reserved bit set to 1H,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 *	Write 1 to APIC TPR register reserved bits , and will generate #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38995_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_011()
{
	u64 val;
	u64 resv_bits = 0xFFFFFFF0;/*reserved bits mask*/

	val = rdmsr(APIC_BASE_MSR + APIC_TASKPRI / 16);
	for (int i = 0; i < 63; i++) {
		if (resv_bits & (1UL < i)) {
			wrmsr_checking(APIC_BASE_MSR + APIC_TASKPRI / 16, val | (resv_bits & (1U < i)));
			if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when rdmsr causes #GP(0) for write-only registers
 *
 * Summary: When a vCPU attempts to read an MSR and the MSR is write-only,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0)
 *
 *	Try to rdmsr APIC EOI register, vCPU shall receive #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_38999_inject_gp_when_rdmsr_causes_GP0_for_write_only_EOI_registers()
{
	u64 val;

	rdmsr_checking(APIC_BASE_MSR + APIC_EOI / 16, &val);
	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name Inject #GP(0) when rdmsr causes #GP(0) for write-only registers
 *
 * Summary: When a vCPU attempts to read an MSR and the MSR is write-only,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0)
 *
 *	Try to rdmsr SELF IPI register, vCPU shall receive #GP(0)
 * @param None
 *
 * @retval None
 *
 */
void lapic_apic_rqmid_39001_inject_gp_when_rdmsr_causes_GP0_for_write_only_SELF_IPI_registers()
{
	u64 val;

	rdmsr_checking(APIC_BASE_MSR + APIC_SELF_IPI / 16, &val);
	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name Physical LAPIC Support
 *
 * Summary: LAPIC shall be available on the physical platform
 *
 * Try to write LAPIC ID register, and check the register value.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39007_read_only_local_apic_version_register(void)
{
	u64 val;

	val = apic_read(APIC_ID);
	wrmsr_checking(APIC_BASE_MSR + APIC_ID / 16, val + 1);

	report("%s", apic_read(APIC_ID) == val, __FUNCTION__);
}
#endif
#ifdef IN_NATIVE
/**
 * @brief case name Physical LAPIC Support
 *
 * Summary: LAPIC shall be available on the physical platform
 *
 *Enumerate CPUID.1:EDX[bit 9] on the physical platform. Verify the result is 1.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_36496_physical_lapic_support(void)
{
	report("%s", cpuid(1).d & (1U << 9), __FUNCTION__);
}
/**
 * @brief case name Physical x2APIC Support
 *
 * Summary: x2APIC support shall be available on the physical platform.
 *
 * Enumerate CPUID.1:ECX[bit 21] on the physical platform. Verify the result is 1.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_36492_physical_x2apic_support(void)
{
	report("%s", cpuid(1).c & (1U << 21), __FUNCTION__);
}
#define MAX_CPU 64
static struct spinlock lock;
static int phy_cpu_nr;
static u32 phy_x2apic_id[MAX_CPU];
void physical_x2apic_id_hander(isr_regs_t *regs)
{
	spin_lock(&lock);
	phy_cpu_nr++;
	phy_x2apic_id[phy_cpu_nr] = cpuid(0xb).d;
	spin_unlock(&lock);

	eoi();
}
/**
 * @brief case name Different Physical x2APIC ID
 *
 * Summary: Different LAPICs on the physical platform shall have different x2APIC IDs.
 *
 * Enumerate CPUID.0BH:EDX on each pCPU on the physical platform to get its x2APIC ID.
 * Verify that the result is different from each other.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39081_different_physical_x2apic_id(void)
{
	u32 cpu_nr = fwcfg_get_nb_cpus();

	phy_x2apic_id[0] = cpuid(0xb).d;

#define PHYSICAL_TEST_VECTOR 0xE0
	handle_irq(PHYSICAL_TEST_VECTOR, physical_x2apic_id_hander);
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_FIXED | APIC_INT_ASSERT |
				   PHYSICAL_TEST_VECTOR | APIC_DEST_ALLBUT, 0);
	sleep(1000);

	for (u32 i = 0; i < cpu_nr; i++) {
		for (u32 j = i + 1; j < cpu_nr; j++) {
			if (phy_x2apic_id[i] == phy_x2apic_id[j]) {
				report("%s", 0, __FUNCTION__);
				return;
			}
		}
	}
	report("%s", 1, __FUNCTION__);
}

/**
 * @brief case name Physical arat Support
 *
 * Summary: APIC Timer always running feature shall be supported on the physical platform.
 *
 *  Enumerate CPUID.06H.EAX.ARAT[bit 2] on the physical platform. Verify the result is 1.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_36493_physical_arat_support(void)
{
	report("%s", cpuid(6).a & (1U << 2), __FUNCTION__);
}
/**
 * @brief case name Physical TSC deadline timer mode support
 *
 * Summary:TSC deadline timer mode support shall be available on the physical platform.
 *  Verify that TSC deadline mode timer is supported by checking CPUID.01H:ECX.TSC_Deadline[bit 24] is set to 1.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39010_physical_tsc_deadline_timer_mode_support_001(void)
{
	report("%s", cpuid(1).c & (1U << 24), __FUNCTION__);
}

/**
 * @brief case name Physical TSC deadline timer mode support
 *
 * Summary:TSC deadline timer mode support shall be available on the physical platform.
 *
 * Configure the timer LVT register to TSC deadline mode.
 * Write a non-zero 64-bit value into IA32_TSC_DEADLINE MSR to arm the timer.
 * Verify that corresponding timer interrupt is captured.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39011_physical_tsc_deadline_timer_mode_support_002(void)
{
	tdt_isr_cnt = 0;

	init_rtsc_dtimer();
	start_dtimer(3000);
	sleep(3010);
	report("%s", tdt_isr_cnt == 1, __FUNCTION__);
}

/**
 * @brief case name Physical one-shot timer mode support
 *
 * Summary:One-shot timer mode support shall be available on the physical platform.
 *
 *  Configure the timer LVT register to one-shot mode.
 *Trigger the APIC timer by writing an initial value to the initial-count register.
 *Verify that corresponding timer interrupt is captured only one time.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39012_physical_one_shot_timer_mode_support(void)
{
	const unsigned int vec = LAPIC_TEST_VEC;
	unsigned initial_value = LAPIC_TIMER_INITIAL_VAL;
	unsigned int lvtt;

	lapic_timer_cnt = 0;
	handle_irq(vec, lapic_timer_handler);

	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt &= ~APIC_LVT_TIMER_MASK;
	lvtt |= APIC_LVT_TIMER_ONESHOT;
	lvtt |= vec;
	apic_write(APIC_LVTT, lvtt);

	apic_write(APIC_TMICT, initial_value);
	sleep(initial_value + 10);
	report("%s", lapic_timer_cnt == 1, __FUNCTION__);
}
/**
 * @brief case name Physical Periodic timer mode support
 *
 * Summary:Periodic timer support shall be available on the physical platform.
 *
 *  Configure the timer LVT register to periodic mode.
 * Trigger the APIC timer by writing an initial value to the initial-count register.
 * Verify that corresponding timer interrupt is captured at least 2 times.
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39013_physical_Periodic_timer_mode_support(void)
{
	const unsigned int vec = LAPIC_TEST_VEC;
	unsigned initial_value = LAPIC_TIMER_INITIAL_VAL;
	unsigned int lvtt;

	lapic_timer_cnt = 0;
	handle_irq(vec, lapic_timer_handler);

	lvtt = apic_read(APIC_LVTT);
	lvtt &= ~APIC_LVT_MASKED;
	lvtt &= ~APIC_VECTOR_MASK;
	lvtt |= vec;
	lvtt |= APIC_LVT_TIMER_PERIODIC;
	apic_write(APIC_LVTT, lvtt);

	apic_write(APIC_TMICT, initial_value);
	sleep(initial_value * 3 + 100);

	report("%s", lapic_timer_cnt >= 2, __FUNCTION__);
	apic_write(APIC_TMICT, 0);/*stop the timer*/
}
/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 0FFH,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *	Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 00H in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39052_filter_the_MSI_interrupt_with_illegal_vector_001()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x0U);/*invalid vector */
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 01H,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 01H in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39064_filter_the_MSI_interrupt_with_illegal_vector_002()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x01U);/*invalid vector */
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 02H,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 02H in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39065_filter_the_MSI_interrupt_with_illegal_vector_003()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x02U);/*invalid vector */
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 03H,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 03H in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39066_filter_the_MSI_interrupt_with_illegal_vector_004()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x03U);/*invalid vector */
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 04H,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 04H in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39067_filter_the_MSI_interrupt_with_illegal_vector_005()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x04U);/*invalid vector*/
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 05H,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 05H in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39068_filter_the_MSI_interrupt_with_illegal_vector_006()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x05U);/*invalid vector*/
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 06H,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 06H in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39069_filter_the_MSI_interrupt_with_illegal_vector_007()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x06U);/*invalid vector */
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 07H,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 07H in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39070_filter_the_MSI_interrupt_with_illegal_vector_008()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x07U);/*invalid vector 0xff*/
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 08H,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 08H in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39071_filter_the_MSI_interrupt_with_illegal_vector_009()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x08U);/*invalid vector*/
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 09H,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 09H in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39072_filter_the_MSI_interrupt_with_illegal_vector_010()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x09U);/*invalid vector */
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 0AH,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 0AH in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39073_filter_the_MSI_interrupt_with_illegal_vector_011()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x0aU);
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 0bH,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 0BH in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39074_filter_the_MSI_interrupt_with_illegal_vector_012()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0xbU);
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 0cH,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 0CH in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39075_filter_the_MSI_interrupt_with_illegal_vector_013()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0xcU);/*invalid vector */
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 0dH,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 0DH in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39076_filter_the_MSI_interrupt_with_illegal_vector_014()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0xdU);/*invalid vector */
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 0eH,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 0EH in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39077_filter_the_MSI_interrupt_with_illegal_vector_015()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0xeU);/*invalid vector */
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}
/**
 * @brief case name Filter the MSI interrupt with illegal vector
 *
 * Summary: When a fixed message signalled interrupt is triggered and the vector is 0fH,
 * ACRN hypervisor shall guarantee that the external interrupt is ignored.
 *
 *  Test on the physical platform. Trigger a fixed message signaled interrupt (MSI)
 * with the vector from 0FH in the MSI message data register successively.
 * Verify that no MSI is delivered.
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_39078_filter_the_MSI_interrupt_with_illegal_vector_016()
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	bool ret = false;
	union pci_bdf bdf = {0};
	if (get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf)) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == true) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0xfU);/*invalid vector */
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
			is_pass = !(g_get_msi_int & SHIFT_LEFT(1UL, lapic_id));
		}
	}
	report("%s", is_pass, __FUNCTION__);
}
#endif
static void print_case_list()
{
	printf("Local APIC feature case list:\n\r");
#ifndef IN_NATIVE
	/*strategy---------558:Local APIC Expose Registers Support------------*/
	printf("\t Case ID:%d case name:%s\n\r", 27696,
		   "local_apic_rqmid_27696_apic_capability");
	printf("\t Case ID:%d case name:%s\n\r", 27694,
		   "local_apic_rqmid_27694_x2apic_capability_001");
	printf("\t Case ID:%d case name:%s\n\r", 27685,
		   "local_apic_rqmid_27685_different_x2apic_id_001");
	printf("\t Case ID:%d case name:%s\n\r", 27677,
		   "local_apic_rqmid_27677_software_disable_support_001");
	printf("\t Case ID:%d case name:%s\n\r", 27675,
		   "local_apic_rqmid_27675_encoding_of_lapic_version_register_001");
	printf("\t Case ID:%d case name:%s\n\r", 27674,
		   "local_apic_rqmid_27674_expose_lvt_support_001");
	printf("\t Case ID:%d case name:%s\n\r", 27659,
		   "local_apic_rqmid_27659_expose_lapic_error_handling_001");
	printf("\t Case ID:%d case name:%s\n\r", 27731,
		   "local_apic_rqmid_27731_expose_lapic_svr_support");
	printf("\t Case ID:%d case name:%s\n\r", 27654,
		   "local_apic_rqmid_27654_expose_arat_support_001");
	printf("\t Case ID:%d case name:%s\n\r", 27932,
		   "local_apic_rqmid_27932_expose_interrupt_priority_feature_001");
	printf("\t Case ID:%d case name:%s\n\r", 27930,
		   "local_apic_rqmid_27930_the_value_of_ppr_001");
	printf("\t Case ID:%d case name:%s\n\r", 27927,
		   "local_apic_rqmid_27927_expose_interrupt_acceptance_for_fixed_interrupts_infrastructure_001");
	printf("\t Case ID:%d case name:%s\n\r", 27599,
		   "local_apic_rqmid_27599_expose_eoi_001");
	printf("\t Case ID:%d case name:%s\n\r", 27595,
		   "local_apic_rqmid_27595_expose_self_ipi_register_in_x2apic_mode_001");
	printf("\t Case ID:%d case name:%s\n\r", 27571,
		   "local_apic_rqmid_27571_illegal_vector_in_lvt_register_001");
	printf("\t Case ID:%d case name:%s\n\r", 38519,
		   "local_apic_rqmid_38519_illegal_vector_in_lvt_register_002");
	printf("\t Case ID:%d case name:%s\n\r", 38520,
		   "local_apic_rqmid_38520_illegal_vector_in_lvt_register_003");
	printf("\t Case ID:%d case name:%s\n\r", 38521,
		   "local_apic_rqmid_38521_illegal_vector_in_lvt_register_004");
	printf("\t Case ID:%d case name:%s\n\r", 38522,
		   "local_apic_rqmid_38522_illegal_vector_in_lvt_register_005");
	printf("\t Case ID:%d case name:%s\n\r", 38524,
		   "local_apic_rqmid_38524_illegal_vector_in_lvt_register_006");
	printf("\t Case ID:%d case name:%s\n\r", 38526,
		   "local_apic_rqmid_38526_illegal_vector_in_lvt_register_007");
	printf("\t Case ID:%d case name:%s\n\r", 27554,
		   "local_apic_rqmid_27554_write_the_dcr_001");
	printf("\t Case ID:%d case name:%s\n\r", 27805,
		   "local_apic_rqmid_27805_illegal_vector_in_icr_001");
	printf("\t Case ID:%d case name:%s\n\r", 27544,
		   "local_apic_rqmid_27544_mask_bit_in_lvt_when_software_re_enabling_001");
	printf("\t Case ID:%d case name:%s\n\r", 27520,
		   "local_apic_rqmid_27520_delivery_status_bit_of_lvt_001");
	printf("\t Case ID:%d case name:%s\n\r", 38568,
		   "local_apic_rqmid_38568_delivery_status_bit_of_lvt_002");
	printf("\t Case ID:%d case name:%s\n\r", 38569,
		   "local_apic_rqmid_38569_delivery_status_bit_of_lvt_003");
	printf("\t Case ID:%d case name:%s\n\r", 38570,
		   "local_apic_rqmid_38570_delivery_status_bit_of_lvt_004");
	printf("\t Case ID:%d case name:%s\n\r", 38571,
		   "local_apic_rqmid_38571_delivery_status_bit_of_lvt_005");
	printf("\t Case ID:%d case name:%s\n\r", 38573,
		   "local_apic_rqmid_38573_delivery_status_bit_of_lvt_006");
	printf("\t Case ID:%d case name:%s\n\r", 38574,
		   "local_apic_rqmid_38574_delivery_status_bit_of_lvt_007");
	printf("\t Case ID:%d case name:%s\n\r", 27725,
		   "local_apic_rqmid_27725_set_TMICT_and_TSCDEADLINE_to_0_after_setting_time_mode_to_be_3h_001");
	printf("\t Case ID:%d case name:%s\n\r", 27723,
		   "local_apic_rqmid_27723_value_of_TMICT_when_lvt_timer_bit_18_is_1_001");
	printf("\t Case ID:%d case name:%s\n\r", 27712,
		   "local_apic_rqmid_27712_interrupt_delivery_with_illegal_vector_001");
	printf("\t Case ID:%d case name:%s\n\r", 27495,
		   "local_apic_rqmid_27495_x2apic_ipi_with_unsupported_delivery_mode_001");
	printf("\t Case ID:%d case name:%s\n\r", 38607,
		   "local_apic_rqmid_38607_x2apic_ipi_with_unsupported_delivery_mode_002");
	printf("\t Case ID:%d case name:%s\n\r", 38608,
		   "local_apic_rqmid_38608_x2apic_ipi_with_unsupported_delivery_mode_003");
	printf("\t Case ID:%d case name:%s\n\r", 38609,
		   "local_apic_rqmid_38609_x2apic_ipi_with_unsupported_delivery_mode_004");
	printf("\t Case ID:%d case name:%s\n\r", 27934,
		   "local_apic_rqmid_27934_x2apic_lvt_with_smi_delivery_mode_001");
	printf("\t Case ID:%d case name:%s\n\r", 39318,
		   "local_apic_rqmid_39318_x2apic_lvt_with_INIT_delivery_mode_002");
	printf("\t Case ID:%d case name:%s\n\r", 39319,
		   "local_apic_rqmid_39319_x2apic_lvt_with_reserved_delivery_mode_003");
	printf("\t Case ID:%d case name:%s\n\r", 39320,
		   "local_apic_rqmid_39320_x2apic_lvt_with_ExtINT_delivery_mode_004");
	printf("\t Case ID:%d case name:%s\n\r", 27926,
		   "local_apic_rqmid_27926_cr8_is_mirror_of_tpr");
	/*strategy---------474:Local APIC init & start-up------------*/
#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 27681,
		   "local_apic_rqmid_27681_apic_base_field_state_of_ap_following_init_001");
	printf("\t Case ID:%d case name:%s\n\r", 38686,
		   "local_apic_rqmid_38686_LVT_CMCI_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38687,
		   "local_apic_rqmid_38687_LVT_LINT0_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38688,
		   "local_apic_rqmid_38688_LVT_LINT1_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38691,
		   "local_apic_rqmid_38691_LVT_ERROR_register_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38692,
		   "local_apic_rqmid_38692_LVT_Thermal_Monitor_register_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38693,
		   "local_apic_rqmid_38693_LVT_Performance_Counter_register_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38694,
		   "local_apic_rqmid_38694_LVT_timer_register_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38697,
		   "local_apic_rqmid_38697_ESR_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38698,
		   "local_apic_rqmid_38698_SVR_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38699,
		   "local_apic_rqmid_38699_ICR_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38700,
		   "local_apic_rqmid_38700_TPR_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38701,
		   "local_apic_rqmid_38701_PPR_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38704,
		   "local_apic_rqmid_38704_IRR_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38705,
		   "local_apic_rqmid_38705_ISR_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38706,
		   "local_apic_rqmid_38706_TMR_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38707,
		   "local_apic_rqmid_38707_CR8_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38708,
		   "local_apic_rqmid_38708_timer_initial_count_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38709,
		   "local_apic_rqmid_38709_timer_current_count_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38710,
		   "local_apic_rqmid_38710_DCR_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38711,
		   "local_apic_rqmid_38711_IA32_TSC_DEADLINE_state_of_following_init");
	printf("\t Case ID:%d case name:%s\n\r", 38713,
		   "local_apic_rqmid_38713_IA32_APIC_BASE_state_of_following_init");
#endif
	printf("\t Case ID:%d case name:%s\n\r", 27922,
		   "local_apic_rqmid_27922_apic_base_field_state_of_bp_following_start_up_001");
	printf("\t Case ID:%d case name:%s\n\r", 38737,
		   "local_apic_rqmid_38737_LVT_CMCI_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38738,
		   "local_apic_rqmid_38738_LVT_LINT0_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38739,
		   "local_apic_rqmid_38739_LVT_LINT1_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38740,
		   "local_apic_rqmid_38740_LVT_ERROR_Register_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38741,
		   "local_apic_rqmid_38741_LVT_Thermal_Monitor_Register_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38743,
		   "local_apic_rqmid_38743_LVT_Performance_Conuter_Register_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 39455,
		   "local_apic_rqmid_39455_LVT_Timer_Register_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38745,
		   "local_apic_rqmid_38745_ESR_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38746,
		   "local_apic_rqmid_38746_SVR_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38747,
		   "local_apic_rqmid_38747_ICR_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38748,
		   "local_apic_rqmid_38748_TPR_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38749,
		   "local_apic_rqmid_38749_PPR_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38750,
		   "local_apic_rqmid_38750_IRR_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38753,
		   "local_apic_rqmid_38753_ISR_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38754,
		   "local_apic_rqmid_38754_TMR_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38755,
		   "local_apic_rqmid_38755_CR8_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38756,
		   "local_apic_rqmid_38756_TICR_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38757,
		   "local_apic_rqmid_38757_TCCR_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38758,
		   "local_apic_rqmid_38758_DCR_state_of_following_start_up");
	printf("\t Case ID:%d case name:%s\n\r", 38759,
		   "local_apic_rqmid_38759_IA32_TSC_DEADLINE_state_of_following_start_up");
	/*strategy---------481:Local APIC Timer support------------*/
	printf("\t Case ID:%d case name:%s\n\r", 27651,
		   "local_apic_rqmid_27651_expose_tsc_deadline_timer_mode_support_001");
	printf("\t Case ID:%d case name:%s\n\r", 38781,
		   "local_apic_rqmid_38781_expose_tsc_deadline_timer_mode_support_002");
	printf("\t Case ID:%d case name:%s\n\r", 38782,
		   "local_apic_rqmid_38782_expose_tsc_deadline_timer_mode_support_003");
	printf("\t Case ID:%d case name:%s\n\r", 38783,
		   "local_apic_rqmid_38783_expose_tsc_deadline_timer_mode_support_004");
	printf("\t Case ID:%d case name:%s\n\r", 38784,
		   "local_apic_rqmid_38784_expose_tsc_deadline_timer_mode_support_005");
	printf("\t Case ID:%d case name:%s\n\r", 38785,
		   "local_apic_rqmid_38785_expose_tsc_deadline_timer_mode_support_006");
	printf("\t Case ID:%d case name:%s\n\r", 38786,
		   "local_apic_rqmid_38786_expose_tsc_deadline_timer_mode_support_007");
	printf("\t Case ID:%d case name:%s\n\r", 38787,
		   "local_apic_rqmid_38787_expose_tsc_deadline_timer_mode_support_008");
	printf("\t Case ID:%d case name:%s\n\r", 38788,
		   "local_apic_rqmid_38788_expose_tsc_deadline_timer_mode_support_009");
	printf("\t Case ID:%d case name:%s\n\r", 27648,
		   "local_apic_rqmid_27648_expose_one_shot_timer_mode_support_001");
	printf("\t Case ID:%d case name:%s\n\r", 38789,
		   "local_apic_rqmid_38789_expose_one_shot_timer_mode_support_002");
	printf("\t Case ID:%d case name:%s\n\r", 27645,
		   "local_apic_rqmid_27645_expose_periodic_timer_mode_support_001");
	printf("\t Case ID:%d case name:%s\n\r", 38790,
		   "local_apic_rqmid_38790_expose_periodic_timer_mode_support_002");
	printf("\t Case ID:%d case name:%s\n\r", 27697,
		   "local_apic_rqmid_27697_lapic_timer_mode_configuration");
	/*strategy---------480: Local APIC IPI support-----------------------*/
	printf("\t Case ID:%d case name:%s\n\r", 27642,
		   "local_apic_rqmid_27642_expose_ipi_support_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 27620,
		   "local_apic_rqmid_27620_x2apic_ipi_delivery_in_physical_destination_mode");
#endif
	printf("\t Case ID:%d case name:%s\n\r", 27619,
		   "local_apic_rqmid_27619_x2apic_ipi_delivery_in_logical_destination_mode");
	printf("\t Case ID:%d case name:%s\n\r", 39301,
		   "local_apic_rqmid_39301_x2apic_ipi_delivery_in_self_all_including_all_excluding_mode_001");
	printf("\t Case ID:%d case name:%s\n\r", 39306,
		   "local_apic_rqmid_39306_x2apic_ipi_delivery_in_self_all_including_all_excluding_mode_002");
	printf("\t Case ID:%d case name:%s\n\r", 39308,
		   "local_apic_rqmid_39308_x2apic_ipi_delivery_in_self_all_including_all_excluding_mode_003");
	/*strategy---------557: Local APIC ignore write of registers-----------------------*/
	printf("\t Case ID:%d case name:%s\n\r", 27716,
		   "local_apic_rqmid_27716_ignore_icr_write_of_level_bit");
	printf("\t Case ID:%d case name:%s\n\r", 27714,
		   "local_apic_rqmid_27714_ignore_icr_write_of_trigger_mode_bit");
	printf("\t Case ID:%d case name:%s\n\r", 27531,
		   "local_apic_rqmid_27531_ignore_write_of_eoi_when_isr_is_0h");
	printf("\t Case ID:%d case name:%s\n\r", 39017,
		   "local_apic_rqmid_39017_ignore_MSI_with_vector_0FFH");
	printf("\t Case ID:%d case name:%s\n\r", 39040,
		   "local_apic_rqmid_39040_ignore_MSI_while_software_diabled");
	/*strategy---------475: Local APIC Inject #GP(0)-----------------------*/
	printf("\t Case ID:%d case name:%s\n\r", 27600,
		   "local_apic_rqmid_27600_read_only_ppr");
	printf("\t Case ID:%d case name:%s\n\r", 27593,
		   "local_apic_rqmid_27593_read_only_apic_base");
	printf("\t Case ID:%d case name:%s\n\r", 27592,
		   "local_apic_rqmid_27592_read_only_local_apic_version_register");
	printf("\t Case ID:%d case name:%s\n\r", 27589,
		   "local_apic_rqmid_27589_read_only_isr");
	printf("\t Case ID:%d case name:%s\n\r", 27588,
		   "local_apic_rqmid_27588_read_only_tmr");
	printf("\t Case ID:%d case name:%s\n\r", 27587,
		   "local_apic_rqmid_27587_read_only_irr");
	printf("\t Case ID:%d case name:%s\n\r", 27585,
		   "local_apic_rqmid_27585_read_only_current_count_register");
	printf("\t Case ID:%d case name:%s\n\r", 38921,
		   "lapic_apic_rqmid_38921_inject_gp_when_wrmsr_x2apic_read_only_register_001");
	printf("\t Case ID:%d case name:%s\n\r", 38962,
		   "lapic_apic_rqmid_38962_inject_gp_when_wrmsr_x2apic_read_only_register_002");
	printf("\t Case ID:%d case name:%s\n\r", 38963,
		   "lapic_apic_rqmid_38963_inject_gp_when_wrmsr_x2apic_read_only_register_003");
	printf("\t Case ID:%d case name:%s\n\r", 38964,
		   "lapic_apic_rqmid_38964_inject_gp_when_wrmsr_x2apic_read_only_register_004");
	printf("\t Case ID:%d case name:%s\n\r", 38965,
		   "lapic_apic_rqmid_38965_inject_gp_when_wrmsr_x2apic_read_only_register_005");
	printf("\t Case ID:%d case name:%s\n\r", 38966,
		   "lapic_apic_rqmid_38966_inject_gp_when_wrmsr_x2apic_read_only_register_006");
	printf("\t Case ID:%d case name:%s\n\r", 38967,
		   "lapic_apic_rqmid_38967_inject_gp_when_wrmsr_x2apic_read_only_register_007");
	printf("\t Case ID:%d case name:%s\n\r", 38968,
		   "lapic_apic_rqmid_38968_inject_gp_when_wrmsr_x2apic_read_only_register_008");
	printf("\t Case ID:%d case name:%s\n\r", 38969,
		   "lapic_apic_rqmid_38969_inject_gp_when_wrmsr_x2apic_read_only_register_009");
	printf("\t Case ID:%d case name:%s\n\r", 38970,
		   "lapic_apic_rqmid_38970_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_001");
	printf("\t Case ID:%d case name:%s\n\r", 38976,
		   "lapic_apic_rqmid_38976_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_002");
	printf("\t Case ID:%d case name:%s\n\r", 38981,
		   "lapic_apic_rqmid_38981_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_003");
	printf("\t Case ID:%d case name:%s\n\r", 38983,
		   "lapic_apic_rqmid_38983_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_004");
	printf("\t Case ID:%d case name:%s\n\r", 38984,
		   "lapic_apic_rqmid_38984_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_005");
	printf("\t Case ID:%d case name:%s\n\r", 38985,
		   "lapic_apic_rqmid_38985_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_006");
	printf("\t Case ID:%d case name:%s\n\r", 38988,
		   "lapic_apic_rqmid_38988_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_007");
	printf("\t Case ID:%d case name:%s\n\r", 38989,
		   "lapic_apic_rqmid_38989_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_008");
	printf("\t Case ID:%d case name:%s\n\r", 38990,
		   "lapic_apic_rqmid_38990_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_009");
	printf("\t Case ID:%d case name:%s\n\r", 38992,
		   "lapic_apic_rqmid_38992_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_010");
	printf("\t Case ID:%d case name:%s\n\r", 38995,
		   "lapic_apic_rqmid_38995_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_011");
	printf("\t Case ID:%d case name:%s\n\r", 38999,
		   "lapic_apic_rqmid_38999_inject_gp_when_rdmsr_causes_GP0_for_write_only_registers_001");
	printf("\t Case ID:%d case name:%s\n\r", 39001,
		   "lapic_apic_rqmid_39001_inject_gp_when_rdmsr_causes_GP0_for_write_only_registers_002");
	printf("\t Case ID:%d case name:%s\n\r", 39007,
		   "local_apic_rqmid_39007_read_only_local_apic_version_register");
#endif
#ifdef IN_NATIVE
	/*strategy---------618: LAPIC Application Constraint-----------------------*/
	printf("\t Case ID:%d case name:%s\n\r", 36496,
		   "local_apic_rqmid_36496_physical_lapic_support");
	printf("\t Case ID:%d case name:%s\n\r", 36492,
		   "local_apic_rqmid_36492_physical_x2apic_support");
	printf("\t Case ID:%d case name:%s\n\r", 39081,
		   "local_apic_rqmid_39081_different_physical_x2apic_id");
	printf("\t Case ID:%d case name:%s\n\r", 36493,
		   "local_apic_rqmid_36493_physical_arat_support");
	printf("\t Case ID:%d case name:%s\n\r", 39010,
		   "local_apic_rqmid_39010_physical_tsc_deadline_timer_mode_support_001");
	printf("\t Case ID:%d case name:%s\n\r", 39011,
		   "local_apic_rqmid_39011_physical_tsc_deadline_timer_mode_support_002");
	printf("\t Case ID:%d case name:%s\n\r", 39012,
		   "local_apic_rqmid_39012_physical_one_shot_timer_mode_support");
	printf("\t Case ID:%d case name:%s\n\r", 39013,
		   "local_apic_rqmid_39013_physical_Periodic_timer_mode_support");
	printf("\t Case ID:%d case name:%s\n\r", 39052,
		   "local_apic_rqmid_39052_filter_the_MSI_interrupt_with_illegal_vector_001");
	printf("\t Case ID:%d case name:%s\n\r", 39064,
		   "local_apic_rqmid_39064_filter_the_MSI_interrupt_with_illegal_vector_002");
	printf("\t Case ID:%d case name:%s\n\r", 39065,
		   "local_apic_rqmid_39065_filter_the_MSI_interrupt_with_illegal_vector_003");
	printf("\t Case ID:%d case name:%s\n\r", 39066,
		   "local_apic_rqmid_39066_filter_the_MSI_interrupt_with_illegal_vector_004");
	printf("\t Case ID:%d case name:%s\n\r", 39067,
		   "local_apic_rqmid_39067_filter_the_MSI_interrupt_with_illegal_vector_005");
	printf("\t Case ID:%d case name:%s\n\r", 39068,
		   "local_apic_rqmid_39068_filter_the_MSI_interrupt_with_illegal_vector_006");
	printf("\t Case ID:%d case name:%s\n\r", 39069,
		   "local_apic_rqmid_39069_filter_the_MSI_interrupt_with_illegal_vector_007");
	printf("\t Case ID:%d case name:%s\n\r", 39070,
		   "local_apic_rqmid_39070_filter_the_MSI_interrupt_with_illegal_vector_008");
	printf("\t Case ID:%d case name:%s\n\r", 39071,
		   "local_apic_rqmid_39071_filter_the_MSI_interrupt_with_illegal_vector_009");
	printf("\t Case ID:%d case name:%s\n\r", 39072,
		   "local_apic_rqmid_39072_filter_the_MSI_interrupt_with_illegal_vector_010");
	printf("\t Case ID:%d case name:%s\n\r", 39073,
		   "local_apic_rqmid_39073_filter_the_MSI_interrupt_with_illegal_vector_011");
	printf("\t Case ID:%d case name:%s\n\r", 39074,
		   "local_apic_rqmid_39074_filter_the_MSI_interrupt_with_illegal_vector_012");
	printf("\t Case ID:%d case name:%s\n\r", 39075,
		   "local_apic_rqmid_39075_filter_the_MSI_interrupt_with_illegal_vector_013");
	printf("\t Case ID:%d case name:%s\n\r", 39076,
		   "local_apic_rqmid_39076_filter_the_MSI_interrupt_with_illegal_vector_014");
	printf("\t Case ID:%d case name:%s\n\r", 39077,
		   "local_apic_rqmid_39077_filter_the_MSI_interrupt_with_illegal_vector_015");
	printf("\t Case ID:%d case name:%s\n\r", 39078,
		   "local_apic_rqmid_39078_filter_the_MSI_interrupt_with_illegal_vector_016");
#endif
}
int main(int ac, char **av)
{
#ifndef IN_NATIVE
	ulong cr8 = 0;

	asm("mov %%cr8, %%rax;mov %%rax, %0":"=m"(cr8));
	*(ulong *) CR8_STARTUP_ADDR = cr8;
#endif
	setup_idt();
	print_case_list();

	pci_pdev_enumerate_dev(pci_devs, &nr_pci_devs);
	if (nr_pci_devs == 0) {
		DBG_ERRO("pci_pdev_enumerate_dev finds no devs!");
		return 0;
	}
	union pci_bdf bdf = {0};
	bool is = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is) {
		e1000e_init(bdf);
	}
#ifndef IN_NATIVE
	/*strategy---------558:Local APIC Expose Registers Support------------*/
	local_apic_rqmid_27696_apic_capability();
	local_apic_rqmid_27694_x2apic_capability_001();
	local_apic_rqmid_27685_different_x2apic_id_001();
	local_apic_rqmid_27677_software_disable_support_001();
	local_apic_rqmid_27675_encoding_of_lapic_version_register_001();
	local_apic_rqmid_27674_expose_lvt_support_001();
	local_apic_rqmid_27659_expose_lapic_error_handling_001();
	local_apic_rqmid_27731_expose_lapic_svr_support();
	local_apic_rqmid_27654_expose_arat_support_001();
	local_apic_rqmid_27932_expose_interrupt_priority_feature_001();
	local_apic_rqmid_27930_the_value_of_ppr_001();
	local_apic_rqmid_27927_expose_interrupt_acceptance_for_fixed_interrupts_infrastructure_001();
	local_apic_rqmid_27599_expose_eoi_001();
	local_apic_rqmid_27595_expose_self_ipi_register_in_x2apic_mode_001();
	local_apic_rqmid_27571_illegal_vector_in_lvt_register_001();
	local_apic_rqmid_38519_illegal_vector_in_lvt_register_002();
	local_apic_rqmid_38520_illegal_vector_in_lvt_register_003();
	local_apic_rqmid_38521_illegal_vector_in_lvt_register_004();
	local_apic_rqmid_38522_illegal_vector_in_lvt_register_005();
	local_apic_rqmid_38524_illegal_vector_in_lvt_register_006();
	local_apic_rqmid_38526_illegal_vector_in_lvt_register_007();
	local_apic_rqmid_27554_write_the_dcr_001();
	local_apic_rqmid_27805_illegal_vector_in_icr_001();
	local_apic_rqmid_27544_mask_bit_in_lvt_when_software_re_enabling_001();
	local_apic_rqmid_27520_delivery_status_bit_of_lvt_001();
	local_apic_rqmid_38568_delivery_status_bit_of_lvt_002();
	local_apic_rqmid_38569_delivery_status_bit_of_lvt_003();
	local_apic_rqmid_38570_delivery_status_bit_of_lvt_004();
	local_apic_rqmid_38571_delivery_status_bit_of_lvt_005();
	local_apic_rqmid_38573_delivery_status_bit_of_lvt_006();
	local_apic_rqmid_38574_delivery_status_bit_of_lvt_007();
	local_apic_rqmid_27725_set_TMICT_and_TSCDEADLINE_to_0_after_setting_time_mode_to_be_3h_001();
	local_apic_rqmid_27723_value_of_TMICT_when_lvt_timer_bit_18_is_1_001();
	local_apic_rqmid_27712_interrupt_delivery_with_illegal_vector_001();
	local_apic_rqmid_27495_x2apic_ipi_with_unsupported_delivery_mode_001();
	local_apic_rqmid_38607_x2apic_ipi_with_unsupported_delivery_mode_002();
	local_apic_rqmid_38608_x2apic_ipi_with_unsupported_delivery_mode_003();
	local_apic_rqmid_38609_x2apic_ipi_with_unsupported_delivery_mode_004();
	local_apic_rqmid_27934_x2apic_lvt_with_smi_delivery_mode_001();
	local_apic_rqmid_39318_x2apic_lvt_with_INIT_delivery_mode_002();
	local_apic_rqmid_39319_x2apic_lvt_with_reserved_delivery_mode_003();
	local_apic_rqmid_39320_x2apic_lvt_with_ExtINT_delivery_mode_004();
	local_apic_rqmid_27926_cr8_is_mirror_of_tpr();
	/*strategy---------474:Local APIC init & start-up------------*/
#ifdef IN_NON_SAFETY_VM
	local_apic_rqmid_27681_apic_base_field_state_of_ap_following_init_001();
	local_apic_rqmid_38686_LVT_CMCI_state_of_following_init();
	local_apic_rqmid_38687_LVT_LINT0_state_of_following_init();
	local_apic_rqmid_38688_LVT_LINT1_state_of_following_init();
	local_apic_rqmid_38691_LVT_ERROR_register_state_of_following_init();
	local_apic_rqmid_38692_LVT_Thermal_Monitor_register_state_of_following_init();
	local_apic_rqmid_38693_LVT_Performance_Counter_register_state_of_following_init();
	local_apic_rqmid_38694_LVT_timer_register_state_of_following_init();
	local_apic_rqmid_38697_ESR_state_of_following_init();
	local_apic_rqmid_38698_SVR_state_of_following_init();
	local_apic_rqmid_38699_ICR_state_of_following_init();
	local_apic_rqmid_38700_TPR_state_of_following_init();
	local_apic_rqmid_38701_PPR_state_of_following_init();
	local_apic_rqmid_38704_IRR_state_of_following_init();
	local_apic_rqmid_38705_ISR_state_of_following_init();
	local_apic_rqmid_38706_TMR_state_of_following_init();
	local_apic_rqmid_38707_CR8_state_of_following_init();
	local_apic_rqmid_38708_timer_initial_count_state_of_following_init();
	local_apic_rqmid_38709_timer_current_count_state_of_following_init();
	local_apic_rqmid_38710_DCR_state_of_following_init();
	local_apic_rqmid_38711_IA32_TSC_DEADLINE_state_of_following_init();
	local_apic_rqmid_38713_IA32_APIC_BASE_state_of_following_init();
#endif
	local_apic_rqmid_27922_apic_base_field_state_of_bp_following_start_up_001();
	local_apic_rqmid_38737_LVT_CMCI_state_of_following_start_up();
	local_apic_rqmid_38738_LVT_LINT0_state_of_following_start_up();
	local_apic_rqmid_38739_LVT_LINT1_state_of_following_start_up();
	local_apic_rqmid_38740_LVT_ERROR_Register_state_of_following_start_up();
	local_apic_rqmid_38741_LVT_Thermal_Monitor_Register_state_of_following_start_up();
	local_apic_rqmid_38743_LVT_Performance_Conuter_Register_state_of_following_start_up();
	local_apic_rqmid_39455_LVT_Timer_Register_state_of_following_start_up();
	local_apic_rqmid_38745_ESR_state_of_following_start_up();
	local_apic_rqmid_38746_SVR_state_of_following_start_up();
	local_apic_rqmid_38747_ICR_state_of_following_start_up();
	local_apic_rqmid_38748_TPR_state_of_following_start_up();
	local_apic_rqmid_38749_PPR_state_of_following_start_up();
	local_apic_rqmid_38750_IRR_state_of_following_start_up();
	local_apic_rqmid_38753_ISR_state_of_following_start_up();
	local_apic_rqmid_38754_TMR_state_of_following_start_up();
	local_apic_rqmid_38755_CR8_state_of_following_start_up();
	local_apic_rqmid_38756_TICR_state_of_following_start_up();
	local_apic_rqmid_38757_TCCR_state_of_following_start_up();
	local_apic_rqmid_38758_DCR_state_of_following_start_up();
	local_apic_rqmid_38759_IA32_TSC_DEADLINE_state_of_following_start_up();
	/*strategy---------481:Local APIC Timer support------------*/
	local_apic_rqmid_27651_expose_tsc_deadline_timer_mode_support_001(true);
	local_apic_rqmid_38781_expose_tsc_deadline_timer_mode_support_002();
	local_apic_rqmid_38782_expose_tsc_deadline_timer_mode_support_003();
	local_apic_rqmid_38783_expose_tsc_deadline_timer_mode_support_004();
	local_apic_rqmid_38784_expose_tsc_deadline_timer_mode_support_005();
	local_apic_rqmid_38785_expose_tsc_deadline_timer_mode_support_006();
	local_apic_rqmid_38786_expose_tsc_deadline_timer_mode_support_007();
	local_apic_rqmid_38787_expose_tsc_deadline_timer_mode_support_008();
	local_apic_rqmid_38788_expose_tsc_deadline_timer_mode_support_009();
	local_apic_rqmid_27648_expose_one_shot_timer_mode_support_001(true);
	local_apic_rqmid_38789_expose_one_shot_timer_mode_support_002();
	local_apic_rqmid_27645_expose_periodic_timer_mode_support_001(true);
	local_apic_rqmid_38790_expose_periodic_timer_mode_support_002();
	local_apic_rqmid_27697_lapic_timer_mode_configuration();
	/*strategy---------480: Local APIC IPI support-----------------------*/
	local_apic_rqmid_27642_expose_ipi_support_001();
#ifdef IN_NON_SAFETY_VM
	local_apic_rqmid_27620_x2apic_ipi_delivery_in_physical_destination_mode();
#endif
	local_apic_rqmid_27619_x2apic_ipi_delivery_in_logical_destination_mode();
	local_apic_rqmid_39301_x2apic_ipi_delivery_in_self_all_including_all_excluding_mode_001();
	local_apic_rqmid_39306_x2apic_ipi_delivery_in_self_all_including_all_excluding_mode_002();
	local_apic_rqmid_39308_x2apic_ipi_delivery_in_self_all_including_all_excluding_mode_003();
	/*strategy---------557: Local APIC ignore write of registers-----------------------*/
	local_apic_rqmid_27716_ignore_icr_write_of_level_bit();
	local_apic_rqmid_27714_ignore_icr_write_of_trigger_mode_bit();
	local_apic_rqmid_27531_ignore_write_of_eoi_when_isr_is_0h();
	local_apic_rqmid_39017_ignore_MSI_with_vector_0FFH();
	local_apic_rqmid_39040_ignore_MSI_while_software_diabled();
	/*strategy---------475: Local APIC Inject #GP(0)-----------------------*/
	local_apic_rqmid_27600_read_only_ppr();
	local_apic_rqmid_27593_read_only_apic_base();
	local_apic_rqmid_27592_read_only_local_apic_version_register();
	local_apic_rqmid_27589_read_only_isr();
	local_apic_rqmid_27588_read_only_tmr();
	local_apic_rqmid_27587_read_only_irr();
	local_apic_rqmid_27585_read_only_current_count_register();
	lapic_apic_rqmid_38921_inject_gp_when_wrmsr_x2apic_read_only_PPR_register();
	lapic_apic_rqmid_38962_inject_gp_when_wrmsr_x2apic_read_only_IA32_APIC_BASE_register();
	lapic_apic_rqmid_38963_inject_gp_when_wrmsr_x2apic_read_only_APIC_version_register();
	lapic_apic_rqmid_38964_inject_gp_when_wrmsr_x2apic_read_only_ISR_register();
	lapic_apic_rqmid_38965_inject_gp_when_wrmsr_x2apic_read_only_TMR_register();
	lapic_apic_rqmid_38966_inject_gp_when_wrmsr_x2apic_read_only_IRR_register();
	lapic_apic_rqmid_38967_inject_gp_when_wrmsr_x2apic_read_only_Current_Count_register();
	lapic_apic_rqmid_38968_inject_gp_when_wrmsr_x2apic_read_only_LAPIC_ID_register();
	lapic_apic_rqmid_38969_inject_gp_when_wrmsr_x2apic_read_only_APIC_LDR_register();
	lapic_apic_rqmid_38970_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_001();
	lapic_apic_rqmid_38976_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_002();
	lapic_apic_rqmid_38981_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_003();
	lapic_apic_rqmid_38983_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_004();
	lapic_apic_rqmid_38984_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_005();
	lapic_apic_rqmid_38985_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_006();
	lapic_apic_rqmid_38988_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_007();
	lapic_apic_rqmid_38989_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_008();
	lapic_apic_rqmid_38990_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_009();
	lapic_apic_rqmid_38992_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_010();
	lapic_apic_rqmid_38995_inject_gp_when_wrmsr_attempts_set_reserved_bit_to_1_in_RW_register_011();
	lapic_apic_rqmid_38999_inject_gp_when_rdmsr_causes_GP0_for_write_only_EOI_registers();
	lapic_apic_rqmid_39001_inject_gp_when_rdmsr_causes_GP0_for_write_only_SELF_IPI_registers();
	local_apic_rqmid_39007_read_only_local_apic_version_register();
#endif
#ifdef IN_NATIVE
	/*strategy---------618: LAPIC Application Constraint-----------------------*/
	local_apic_rqmid_36496_physical_lapic_support();
	local_apic_rqmid_36492_physical_x2apic_support();
	local_apic_rqmid_39081_different_physical_x2apic_id();
	local_apic_rqmid_36493_physical_arat_support();
	local_apic_rqmid_39010_physical_tsc_deadline_timer_mode_support_001();
	local_apic_rqmid_39011_physical_tsc_deadline_timer_mode_support_002();
	local_apic_rqmid_39012_physical_one_shot_timer_mode_support();
	local_apic_rqmid_39013_physical_Periodic_timer_mode_support();
	local_apic_rqmid_39052_filter_the_MSI_interrupt_with_illegal_vector_001();
	local_apic_rqmid_39064_filter_the_MSI_interrupt_with_illegal_vector_002();
	local_apic_rqmid_39065_filter_the_MSI_interrupt_with_illegal_vector_003();
	local_apic_rqmid_39066_filter_the_MSI_interrupt_with_illegal_vector_004();
	local_apic_rqmid_39067_filter_the_MSI_interrupt_with_illegal_vector_005();
	local_apic_rqmid_39068_filter_the_MSI_interrupt_with_illegal_vector_006();
	local_apic_rqmid_39069_filter_the_MSI_interrupt_with_illegal_vector_007();
	local_apic_rqmid_39070_filter_the_MSI_interrupt_with_illegal_vector_008();
	local_apic_rqmid_39071_filter_the_MSI_interrupt_with_illegal_vector_009();
	local_apic_rqmid_39072_filter_the_MSI_interrupt_with_illegal_vector_010();
	local_apic_rqmid_39073_filter_the_MSI_interrupt_with_illegal_vector_011();
	local_apic_rqmid_39074_filter_the_MSI_interrupt_with_illegal_vector_012();
	local_apic_rqmid_39075_filter_the_MSI_interrupt_with_illegal_vector_013();
	local_apic_rqmid_39076_filter_the_MSI_interrupt_with_illegal_vector_014();
	local_apic_rqmid_39077_filter_the_MSI_interrupt_with_illegal_vector_015();
	local_apic_rqmid_39078_filter_the_MSI_interrupt_with_illegal_vector_016();
#endif
	return report_summary();
}
