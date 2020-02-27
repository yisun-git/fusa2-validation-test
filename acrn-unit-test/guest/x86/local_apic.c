/*
 * Copyright (c) 2019 Intel Corporation. All rights reserved.
 * Test mode: 64-bit
 */

#include "libcflat.h"
#include "apic.h"
#include "vm.h"
#include "smp.h"
#include "desc.h"
#include "isr.h"
#include "msr.h"
#include "atomic.h"

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

#ifndef LAPIC_SAFETY_STRING
#define LAPIC_SAFETY_STRING "[Safety VM for Local APIC]: "
#endif
#define LAPIC_FIRST_VEC     16
#define LAPIC_MAX_VEC       255
#define LAPIC_TIMER_WAIT_MULTIPLIER 1000U
#define LAPIC_TIMER_INITIAL_VAL 1U
#define LAPIC_TEST_VEC      0x0E0U
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
#define LAPIC_NO_EXEC(fmt, cond, arg...) do { report(fmt, 0, ##arg); } while (0)

#define LAPIC_TEST_INVALID_MAX_VEC 15UL
#define LAPIC_ILLEGAL_VECTOR_LOOP_START(var, end) for ( ; (var) <= (end); (vec) += 1) {
#define LAPIC_ILLEGAL_VECTOR_LOOP_END }

static atomic_t lapic_timer_isr_count;
static void lapic_timer_isr(isr_regs_t *regs)
{
    (void) regs;
    atomic_inc(&lapic_timer_isr_count);

    eoi();
}

/* Summary: 2 Case for Requirement: 140249 Expose Periodic timer mode support */
/*    <1: 140249 - 27645> Local APIC_Expose Periodic timer mode support_001 */

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
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27645_expose_periodic_timer_mode_support_001(void)
{
    const unsigned int vec = LAPIC_TEST_VEC;
    unsigned initial_value = LAPIC_TIMER_INITIAL_VAL;
    unsigned int lvtt;

    irq_disable();
    atomic_set(&lapic_timer_isr_count, 0);

    wrmsr(MSR_IA32_TSCDEADLINE, 0);
    apic_write(APIC_TMICT, 0);

    handle_irq(vec, lapic_timer_isr);
    lvtt = apic_read(APIC_LVTT);
    lvtt &= ~APIC_LVT_MASKED;
    lvtt &= ~APIC_VECTOR_MASK;
    lvtt |= vec;

    apic_write(APIC_LVTT, lvtt);
    mb();

    irq_enable();

    lvtt &= ~APIC_LVT_TIMER_MASK;
    lvtt |= APIC_LVT_TIMER_PERIODIC;

    apic_write(APIC_LVTT, lvtt);
    mb();

    initial_value *= LAPIC_TIMER_WAIT_MULTIPLIER;
    apic_write(APIC_TMICT, initial_value);
    mb();
    asm volatile ("nop");

    initial_value *= LAPIC_TIMER_WAIT_MULTIPLIER;
	while (atomic_read(&lapic_timer_isr_count) < 2)
    {
		--initial_value;
		if (initial_value == 0U)
		{
			apic_write(APIC_TMICT, 0);
			break;
		}
	}
	report("%s", atomic_read(&lapic_timer_isr_count) >= 2,
	       "local_apic_rqmid_27645_expose_periodic_timer_mode_support_001");
}

/**
 * @brief case name x2APIC capability
 *
 * Summary: When a vCPU reads CPUID.1H, ACRN hypervisor shall set guest ECX[bit 21] to 1H.
 *  x2APIC mode is used to provide extended processor addressability to the VM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void local_apic_rqmid_27694_x2apic_capability_001(void)
{
	struct cpuid id;
	id = cpuid(LAPIC_CPUID_APIC_CAPABILITY);
	report("%s", !!(id.c & LAPIC_CPUID_APIC_CAP_X2APCI), \
		"local_apic_rqmid_27694_x2apic_capability_001");
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

    asm volatile ("nop");// cr8 and TPR are not serialized
    asm volatile ("nop");

    apic_write(APIC_TASKPRI, LAPIC_TPR_MID);
    asm volatile ("nop");
    sec_cr8 = read_cr8();

	asm volatile ("nop");// cr8 and TPR are not serialized
    asm volatile ("nop");

	write_cr8(0x5);
	tmp_tpr = apic_read(APIC_TASKPRI);
    //write old tpr back
    apic_write(APIC_TASKPRI, old_tpr);
    report("%s", ((LAPIC_TPR_MAX >> 4) == first_cr8) && ((LAPIC_TPR_MID >> 4) == sec_cr8)	\
					&& ((tmp_tpr & 0xf) == 0U) && ((tmp_tpr >> 4) == 0x5),	\
					"local_apic_rqmid_27926_cr8_is_mirror_of_tpr");
}

int main(int ac, char **av)
{
    (void) ac;
    (void) av;

    /* setup_vm(); */
    /* smp_init(); */
    setup_idt();

    /* API to select one or all case to run */
    local_apic_rqmid_27645_expose_periodic_timer_mode_support_001();
    local_apic_rqmid_27694_x2apic_capability_001();
    local_apic_rqmid_27926_cr8_is_mirror_of_tpr();
    return report_summary();
}
