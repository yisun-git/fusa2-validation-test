#ifndef HSI_16_64_CPUMODE_H
#define HSI_16_64_CPUMODE_H

#define INIT_CR0_ADDR 0xa000
#define X86_CR0_PE     0x00000001

/**
 * @brief The register address of IA32_EFER MSR.
 */
#define MSR_IA32_EFER                 0xC0000080
#define IA32_EFER_LME	(1 << 8)
#endif

