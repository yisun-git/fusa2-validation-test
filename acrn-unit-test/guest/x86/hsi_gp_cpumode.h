#ifndef HSI_GP_H
#define HSI_GP_H
/* BP at protect mode save cr0 value */
#define SAVE_CR0	0x6004UL
#define SAVE_IA32_EFER 0x600cUL

/**
 * @brief The register address of IA32_EFER MSR.
 */
#define MSR_IA32_EFER                 0xC0000080U
#define IA32_EFER_LME	(1ULL << 8U)
#endif

