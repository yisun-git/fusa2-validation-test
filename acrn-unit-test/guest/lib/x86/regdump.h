#ifndef _REG_DUMP_H_
#define _REG_DUMP_H_
#include "msr.h"

#define	IA32_P5_MC_ADDR                       0x0
#define	IA32_P5_MC_TYPE                       0x1
#define	IA32_MONITOR_FILTER_SIZE              0x6
#define	IA32_TIME_STAMP_COUNTER               0x10
#define	IA32_PLATFORM_ID                      0x17
#define	IA32_APIC_BASE                        0x1B
#define	MSR_SMI_COUNT                         0x34
#define	IA32_FEATURE_CONTROL                  0x3A
#define	IA32_TSC_ADJUST                       0x3B
#define	IA32_SPEC_CTRL                        0x48
#define	IA32_PRED_CMD                         0x49
#define	IA32_BIOS_UPDT_TRIG                   0x79
#define	MSR_TRACE_HUB_STH_ACPIBAR_BASE        0x80
#define	IA32_BIOS_SIGN_ID                     0x8B
#define	IA32_SGXLEPUBKEYHASH0                 0x8C
#define	IA32_SGXLEPUBKEYHASH1                 0x8D
#define	IA32_SGXLEPUBKEYHASH2                 0x8E
#define	IA32_SGXLEPUBKEYHASH3                 0x8F
#define	IA32_SMM_MONITOR_CTL                  0x9B
#define	IA32_SMBASE                           0x9E
#define	IA32_PMC0                             0xC1
#define	IA32_PMC1                             0xC2
#define	IA32_PMC2                             0xC3
#define	IA32_PMC3                             0xC4
#define	IA32_PMC4                             0xC5
#define	IA32_PMC5                             0xC6
#define	IA32_PMC6                             0xC7
#define	IA32_PMC7                             0xC8
#define	MSR_PLATFORM_INFO                     0xCE
#define	MSR_PKG_CST_CONFIG_CONTROL            0xE2
#define	MSR_PMG_IO_CAPTURE_BASE               0xE4
#define	IA32_MPERF                            0xE7
#define	IA32_APERF                            0xE8
#define	IA32_MTRRCAP                          0xFE
#define	IA32_FLUSH_CMD                        0x10B
#define	MSR_FEATURE_CONFIG                    0x13C
#define	IA32_SYSENTER_CS                      0x174
#define	IA32_SYSENTER_ESP                     0x175
#define	IA32_SYSENTER_EIP                     0x176
#define	IA32_MCG_CAP                          0x179
#define	IA32_MCG_STATUS                       0x17A
#define	IA32_MCG_CTL                          0x17B
#define	IA32_PERFEVTSEL0                      0x186
#define	IA32_PERFEVTSEL1                      0x187
#define	IA32_PERFEVTSEL2                      0x188
#define	IA32_PERFEVTSEL3                      0x189
#define	MSR_PERF_STATUS                       0x198
#define	IA32_PERF_CTL                         0x199
#define	IA32_CLOCK_MODULATION                 0x19A
#define	IA32_THERM_INTERRUPT                  0x19B
#define	IA32_THERM_STATUS                     0x19C
#define	IA32_MISC_ENABLE                      0x1A0
#define	MSR_TEMPERATURE_TARGET                0x1A2
#define	MSR_MISC_FEATURE_CONTROL              0x1A4
#define	MSR_OFFCORE_RSP_0                     0x1A6
#define	MSR_OFFCORE_RSP_1                     0x1A7
#define	MSR_MISC_PWR_MGMT                     0x1AA
#define	MSR_TURBO_RATIO_LIMIT                 0x1AD
#define	IA32_ENERGY_PERF_BIAS                 0x1B0
#define	IA32_PACKAGE_THERM_STATUS             0x1B1
#define	IA32_PACKAGE_THERM_INTERRUPT          0x1B2
#define	MSR_LBR_SELECT                        0x1C8
#define	MSR_LASTBRANCH_TOS                    0x1C9
#define	IA32_DEBUGCTL                         0x1D9
#define	MSR_LER_FROM_LIP                      0x1DD
#define	MSR_LER_TO_LIP                        0x1DE
#define	IA32_SMRR_PHYSBASE                    0x1F2
#define	IA32_SMRR_PHYSMASK                    0x1F3
#define	MSR_PRMRR_PHYS_BASE                   0x1F4
#define	MSR_PRMRR_PHYS_MASK                   0x1F5
#define	IA32_PLATFORM_DCA_CAP                 0x1F8
#define	IA32_CPU_DCA_CAP                      0x1F9
#define	IA32_DCA_0_CAP                        0x1FA
#define	MSR_PRMRR_VALID_CONFIG                0x1FB
#define	MSR_POWER_CTL                         0x1FC
#define	IA32_MTRR_PHYSBASE0                   0x200
#define	IA32_MTRR_PHYSMASK0                   0x201
#define	IA32_MTRR_PHYSBASE1                   0x202
#define	IA32_MTRR_PHYSMASK1                   0x203
#define	IA32_MTRR_PHYSBASE2                   0x204
#define	IA32_MTRR_PHYSMASK2                   0x205
#define	IA32_MTRR_PHYSBASE3                   0x206
#define	IA32_MTRR_PHYSMASK3                   0x207
#define	IA32_MTRR_PHYSBASE4                   0x208
#define	IA32_MTRR_PHYSMASK4                   0x209
#define	IA32_MTRR_PHYSBASE5                   0x20A
#define	IA32_MTRR_PHYSMASK5                   0x20B
#define	IA32_MTRR_PHYSBASE6                   0x20C
#define	IA32_MTRR_PHYSMASK6                   0x20D
#define	IA32_MTRR_PHYSBASE7                   0x20E
#define	IA32_MTRR_PHYSMASK7                   0x20F
#define	IA32_MTRR_PHYSBASE8                   0x210
#define	IA32_MTRR_PHYSMASK8                   0x211
#define	IA32_MTRR_PHYSBASE9                   0x212
#define	IA32_MTRR_PHYSMASK9                   0x213
#define	IA32_MTRR_FIX64K_00000                0x250
#define	IA32_MTRR_FIX16K_80000                0x258
#define	IA32_MTRR_FIX16K_A0000                0x259
#define	IA32_MTRR_FIX4K_C0000                 0x268
#define	IA32_MTRR_FIX4K_C8000                 0x269
#define	IA32_MTRR_FIX4K_D0000                 0x26A
#define	IA32_MTRR_FIX4K_D8000                 0x26B
#define	IA32_MTRR_FIX4K_E0000                 0x26C
#define	IA32_MTRR_FIX4K_E8000                 0x26D
#define	IA32_MTRR_FIX4K_F0000                 0x26E
#define	IA32_MTRR_FIX4K_F8000                 0x26F
#define	IA32_PAT                              0x277
#define	IA32_MC4_CTL2                         0x284
#define	IA32_MC5_CTL2                         0x285
#define	IA32_MC6_CTL2                         0x286
#define	IA32_MC7_CTL2                         0x287
#define	IA32_MC8_CTL2                         0x288
#define	IA32_MC9_CTL2                         0x289
#define	IA32_MC10_CTL2                        0x28A
#define	IA32_MC11_CTL2                        0x28B
#define	IA32_MC12_CTL2                        0x28C
#define	IA32_MC13_CTL2                        0x28D
#define	IA32_MC14_CTL2                        0x28E
#define	IA32_MC15_CTL2                        0x28F
#define	IA32_MC16_CTL2                        0x290
#define	IA32_MC17_CTL2                        0x291
#define	IA32_MC18_CTL2                        0x292
#define	IA32_MC19_CTL2                        0x293
#define	IA32_MC20_CTL2                        0x294
#define	IA32_MC21_CTL2                        0x295
#define	IA32_MC22_CTL2                        0x296
#define	IA32_MC23_CTL2                        0x297
#define	IA32_MC24_CTL2                        0x298
#define	IA32_MC25_CTL2                        0x299
#define	IA32_MC26_CTL2                        0x29A
#define	IA32_MC27_CTL2                        0x29B
#define	IA32_MC28_CTL2                        0x29C
#define	IA32_MC29_CTL2                        0x29D
#define	IA32_MC30_CTL2                        0x29E
#define	IA32_MC31_CTL2                        0x29F
#define	MSR_UNCORE_PRMRR_PHYS_BASE            0x2F4
#define	MSR_UNCORE_PRMRR_PHYS_MASK            0x2F5
#define	IA32_MTRR_DEF_TYPE                    0x2FF
#define	MSR_SGXOWNEREPOCH0                    0x300
#define	MSR_SGXOWNEREPOCH1                    0x301
#define	IA32_FIXED_CTR0                       0x309
#define	IA32_FIXED_CTR1                       0x30A
#define	IA32_FIXED_CTR2                       0x30B
#define	IA32_PERF_CAPABILITIES                0x345
#define	IA32_FIXED_CTR_CTRL                   0x38D
#define	IA32_PERF_GLOBAL_STATUS               0x38E
#define	IA32_PERF_GLOBAL_CTRL                 0x38F
#define	IA32_PERF_GLOBAL_STATUS_RESET         0x390
#define	IA32_PERF_GLOBAL_STATUS_SET           0x391
#define	IA32_PERF_GLOBAL_INUSE                0x392
#define	MSR_UNC_PERF_FIXED_CTRL               0x394
#define	MSR_UNC_PERF_FIXED_CTR                0x395
#define	MSR_UNC_CBO_CONFIG                    0x396
#define	MSR_UNC_ARB_PERFCTR0                  0x3B0
#define	MSR_UNC_ARB_PERFCTR1                  0x3B1
#define	MSR_UNC_ARB_PERFEVTSEL0               0x3B2
#define	MSR_UNC_ARB_PERFEVTSEL1               0x3B3
#define	IA32_PEBS_ENABLE                      0x3F1
#define	MSR_PEBS_LD_LAT                       0x3F6
#define	MSR_PEBS_FRONTEND                     0x3F7
#define	MSR_PKG_C3_RESIDENCY                  0x3F8
#define	MSR_PKG_C6_RESIDENCY                  0x3F9
#define	MSR_PKG_C7_RESIDENCY                  0x3FA
#define	MSR_CORE_C3_RESIDENCY                 0x3FC
#define	MSR_CORE_C6_RESIDENCY                 0x3FD
#define	MSR_CORE_C7_RESIDENCY                 0x3FE
#define	IA32_MC0_ADDR                         0x402
#define	IA32_MC0_MISC                         0x403
#define	IA32_MC1_ADDR                         0x406
#define	IA32_MC1_MISC                         0x407
#define	IA32_MC2_ADDR                         0x40A
#define	IA32_MC2_MISC                         0x40B
#define	IA32_MC3_ADDR                         0x40E
#define	IA32_MC3_MISC                         0x40F
#define	IA32_MC4_ADDR                         0x412
#define	IA32_MC4_MISC                         0x413
#define	IA32_MC5_ADDR                         0x416
#define	IA32_MC5_MISC                         0x417
#define	IA32_MC6_ADDR                         0x41A
#define	IA32_MC6_MISC                         0x41B
#define	IA32_MC7_ADDR                         0x41E
#define	IA32_MC7_MISC                         0x41F
#define	IA32_MC8_ADDR                         0x422
#define	IA32_MC8_MISC                         0x423
#define	IA32_MC9_ADDR                         0x426
#define	IA32_MC9_MISC                         0x427
#define	IA32_MC10_CTL                         0x428
#define	IA32_MC10_STATUS                      0x429
#define	IA32_MC10_ADDR                        0x42A
#define	IA32_MC10_MISC                        0x42B
#define	IA32_MC11_CTL                         0x42C
#define	IA32_MC11_STATUS                      0x42D
#define	IA32_MC11_ADDR                        0x42E
#define	IA32_MC11_MISC                        0x42F
#define	IA32_MC12_CTL                         0x430
#define	IA32_MC12_STATUS                      0x431
#define	IA32_MC12_ADDR                        0x432
#define	IA32_MC12_MISC                        0x433
#define	IA32_MC13_CTL                         0x434
#define	IA32_MC13_STATUS                      0x435
#define	IA32_MC13_ADDR                        0x436
#define	IA32_MC13_MISC                        0x437
#define	IA32_MC14_CTL                         0x438
#define	IA32_MC14_STATUS                      0x439
#define	IA32_MC14_ADDR                        0x43A
#define	IA32_MC14_MISC                        0x43B
#define	IA32_MC15_CTL                         0x43C
#define	IA32_MC15_STATUS                      0x43D
#define	IA32_MC15_ADDR                        0x43E
#define	IA32_MC15_MISC                        0x43F
#define	IA32_MC16_CTL                         0x440
#define	IA32_MC16_STATUS                      0x441
#define	IA32_MC16_ADDR                        0x442
#define	IA32_MC16_MISC                        0x443
#define	IA32_MC17_CTL                         0x444
#define	IA32_MC17_STATUS                      0x445
#define	IA32_MC17_ADDR                        0x446
#define	IA32_MC17_MISC                        0x447
#define	IA32_MC18_CTL                         0x448
#define	IA32_MC18_STATUS                      0x449
#define	IA32_MC18_ADDR                        0x44A
#define	IA32_MC18_MISC                        0x44B
#define	IA32_MC19_CTL                         0x44C
#define	IA32_MC19_STATUS                      0x44D
#define	IA32_MC19_ADDR                        0x44E
#define	IA32_MC19_MISC                        0x44F
#define	IA32_MC20_CTL                         0x450
#define	IA32_MC20_STATUS                      0x451
#define	IA32_MC20_ADDR                        0x452
#define	IA32_MC20_MISC                        0x453
#define	IA32_MC21_CTL                         0x454
#define	IA32_MC21_STATUS                      0x455
#define	IA32_MC21_ADDR                        0x456
#define	IA32_MC21_MISC                        0x457
#define	IA32_MC22_CTL                         0x458
#define	IA32_MC22_STATUS                      0x459
#define	IA32_MC22_ADDR                        0x45A
#define	IA32_MC22_MISC                        0x45B
#define	IA32_MC23_CTL                         0x45C
#define	IA32_MC23_STATUS                      0x45D
#define	IA32_MC23_ADDR                        0x45E
#define	IA32_MC23_MISC                        0x45F
#define	IA32_MC24_CTL                         0x460
#define	IA32_MC24_STATUS                      0x461
#define	IA32_MC24_ADDR                        0x462
#define	IA32_MC24_MISC                        0x463
#define	IA32_MC25_CTL                         0x464
#define	IA32_MC25_STATUS                      0x465
#define	IA32_MC25_ADDR                        0x466
#define	IA32_MC25_MISC                        0x467
#define	IA32_MC26_CTL                         0x468
#define	IA32_MC26_STATUS                      0x469
#define	IA32_MC26_ADDR                        0x46A
#define	IA32_MC26_MISC                        0x46B
#define	IA32_MC27_CTL                         0x46C
#define	IA32_MC27_STATUS                      0x46D
#define	IA32_MC27_ADDR                        0x46E
#define	IA32_MC27_MISC                        0x46F
#define	IA32_MC28_CTL                         0x470
#define	IA32_MC28_STATUS                      0x471
#define	IA32_MC28_ADDR                        0x472
#define	IA32_MC28_MISC                        0x473
#define	IA32_VMX_BASIC                        0x480
#define	IA32_VMX_PINBASED_CTLS                0x481
#define	IA32_VMX_PROCBASED_CTLS               0x482
#define	IA32_VMX_EXIT_CTLS                    0x483
#define	IA32_VMX_ENTRY_CTLS                   0x484
#define	IA32_VMX_MISC                         0x485
#define	IA32_VMX_CR0_FIXED0                   0x486
#define	IA32_VMX_CR0_FIXED1                   0x487
#define	IA32_VMX_CR4_FIXED0                   0x488
#define	IA32_VMX_CR4_FIXED1                   0x489
#define	IA32_VMX_VMCS_ENUM                    0x48A
#define	IA32_VMX_PROCBASED_CTLS2              0x48B
#define	IA32_VMX_EPT_VPID_CAP                 0x48C
#define	IA32_VMX_TRUE_PINBASED_CTLS           0x48D
#define	IA32_VMX_TRUE_PROCBASED_CTLS          0x48E
#define	IA32_VMX_TRUE_EXIT_CTLS               0x48F
#define	IA32_VMX_TRUE_ENTRY_CTLS              0x490
#define	IA32_VMX_VMFUNC                       0x491
#define	IA32_A_PMC0                           0x4C1
#define	IA32_A_PMC1                           0x4C2
#define	IA32_A_PMC2                           0x4C3
#define	IA32_A_PMC3                           0x4C4
#define	IA32_A_PMC4                           0x4C5
#define	IA32_A_PMC5                           0x4C6
#define	IA32_A_PMC6                           0x4C7
#define	IA32_A_PMC7                           0x4C8
#define	IA32_MCG_EXT_CTL                      0x4D0
#define	IA32_SGX_SVN_STATUS                   0x500
#define	IA32_RTIT_OUTPUT_BASE                 0x560
#define	IA32_RTIT_OUTPUT_MASK_PTRS            0x561
#define	IA32_RTIT_CTL                         0x570
#define	IA32_RTIT_STATUS                      0x571
#define	IA32_RTIT_CR3_MATCH                   0x572
#define	IA32_RTIT_ADDR0_A                     0x580
#define	IA32_RTIT_ADDR0_B                     0x581
#define	IA32_RTIT_ADDR1_A                     0x582
#define	IA32_RTIT_ADDR1_B                     0x583
#define	IA32_RTIT_ADDR2_A                     0x584
#define	IA32_RTIT_ADDR2_B                     0x585
#define	IA32_RTIT_ADDR3_A                     0x586
#define	IA32_RTIT_ADDR3_B                     0x587
#define	IA32_DS_AREA                          0x600
#define	MSR_RAPL_POWER_UNIT                   0x606
#define	MSR_PKGC3_IRTL                        0x60A
#define	MSR_PKGC6_IRTL                        0x60B
#define	MSR_PKGC7_IRTL                        0x60C
#define	MSR_PKG_C2_RESIDENCY                  0x60D
#define	MSR_PKG_POWER_LIMIT                   0x610
#define	MSR_PKG_ENERGY_STATUS                 0x611
#define	MSR_PKG_PERF_STATUS                   0x613
#define	MSR_PKG_POWER_INFO                    0x614
#define	MSR_DRAM_ENERGY_STATUS                0x619
#define	MSR_DRAM_PERF_STATUS                  0x61B
#define	MSR_RING_RATIO_LIMIT                  0x620
#define	MSR_PP0_POWER_LIMIT                   0x638
#define	MSR_PP0_ENERGY_STATUS                 0x639
#define	MSR_PP0_POLICY                        0x63A
#define	MSR_PP1_POWER_LIMIT                   0x640
#define	MSR_PP1_ENERGY_STATUS                 0x641
#define	MSR_PP1_POLICY                        0x642
#define	MSR_CONFIG_TDP_NOMINAL                0x648
#define	MSR_CONFIG_TDP_LEVEL1                 0x649
#define	MSR_CONFIG_TDP_LEVEL2                 0x64A
#define	MSR_CONFIG_TDP_CONTROL                0x64B
#define	MSR_TURBO_ACTIVATION_RATIO            0x64C
#define	MSR_PLATFORM_ENERGY_COUNTER           0x64D
#define	MSR_PPERF                             0x64E
#define	MSR_CORE_PERF_LIMIT_REASONS           0x64F
#define	MSR_PKG_HDC_CONFIG                    0x652
#define	MSR_CORE_HDC_RESIDENCY                0x653
#define	MSR_PKG_HDC_SHALLOW_RESIDENCY         0x655
#define	MSR_PKG_HDC_DEEP_RESIDENCY            0x656
#define	MSR_WEIGHTED_CORE_C0                  0x658
#define	MSR_ANY_CORE_C0                       0x659
#define	MSR_ANY_GFXE_C0                       0x65A
#define	MSR_CORE_GFXE_OVERLAP_C0              0x65B
#define	MSR_PLATFORM_POWER_LIMIT              0x65C
#define	MSR_LASTBRANCH_0_FROM_IP              0x680
#define	MSR_LASTBRANCH_1_FROM_IP              0x681
#define	MSR_LASTBRANCH_2_FROM_IP              0x682
#define	MSR_LASTBRANCH_3_FROM_IP              0x683
#define	MSR_LASTBRANCH_4_FROM_IP              0x684
#define	MSR_LASTBRANCH_5_FROM_IP              0x685
#define	MSR_LASTBRANCH_6_FROM_IP              0x686
#define	MSR_LASTBRANCH_7_FROM_IP              0x687
#define	MSR_LASTBRANCH_8_FROM_IP              0x688
#define	MSR_LASTBRANCH_9_FROM_IP              0x689
#define	MSR_LASTBRANCH_10_FROM_IP             0x68A
#define	MSR_LASTBRANCH_11_FROM_IP             0x68B
#define	MSR_LASTBRANCH_12_FROM_IP             0x68C
#define	MSR_LASTBRANCH_13_FROM_IP             0x68D
#define	MSR_LASTBRANCH_14_FROM_IP             0x68E
#define	MSR_LASTBRANCH_15_FROM_IP             0x68F
#define	MSR_LASTBRANCH_16_FROM_IP             0x690
#define	MSR_LASTBRANCH_17_FROM_IP             0x691
#define	MSR_LASTBRANCH_18_FROM_IP             0x692
#define	MSR_LASTBRANCH_19_FROM_IP             0x693
#define	MSR_LASTBRANCH_20_FROM_IP             0x694
#define	MSR_LASTBRANCH_21_FROM_IP             0x695
#define	MSR_LASTBRANCH_22_FROM_IP             0x696
#define	MSR_LASTBRANCH_23_FROM_IP             0x697
#define	MSR_LASTBRANCH_24_FROM_IP             0x698
#define	MSR_LASTBRANCH_25_FROM_IP             0x699
#define	MSR_LASTBRANCH_26_FROM_IP             0x69A
#define	MSR_LASTBRANCH_27_FROM_IP             0x69B
#define	MSR_LASTBRANCH_28_FROM_IP             0x69C
#define	MSR_LASTBRANCH_29_FROM_IP             0x69D
#define	MSR_LASTBRANCH_30_FROM_IP             0x69E
#define	MSR_LASTBRANCH_31_FROM_IP             0x69F
#define	MSR_GRAPHICS_PERF_LIMIT_REASONS       0x6B0
#define	MSR_RING_PERF_LIMIT_REASONS           0x6B1
#define	MSR_LASTBRANCH_0_TO_IP                0x6C0
#define	MSR_LASTBRANCH_1_TO_IP                0x6C1
#define	MSR_LASTBRANCH_2_TO_IP                0x6C2
#define	MSR_LASTBRANCH_3_TO_IP                0x6C3
#define	MSR_LASTBRANCH_4_TO_IP                0x6C4
#define	MSR_LASTBRANCH_5_TO_IP                0x6C5
#define	MSR_LASTBRANCH_6_TO_IP                0x6C6
#define	MSR_LASTBRANCH_7_TO_IP                0x6C7
#define	MSR_LASTBRANCH_8_TO_IP                0x6C8
#define	MSR_LASTBRANCH_9_TO_IP                0x6C9
#define	MSR_LASTBRANCH_10_TO_IP               0x6CA
#define	MSR_LASTBRANCH_11_TO_IP               0x6CB
#define	MSR_LASTBRANCH_12_TO_IP               0x6CC
#define	MSR_LASTBRANCH_13_TO_IP               0x6CD
#define	MSR_LASTBRANCH_14_TO_IP               0x6CE
#define	MSR_LASTBRANCH_15_TO_IP               0x6CF
#define	MSR_LASTBRANCH_16_TO_IP               0x6D0
#define	MSR_LASTBRANCH_17_TO_IP               0x6D1
#define	MSR_LASTBRANCH_18_TO_IP               0x6D2
#define	MSR_LASTBRANCH_19_TO_IP               0x6D3
#define	MSR_LASTBRANCH_20_TO_IP               0x6D4
#define	MSR_LASTBRANCH_21_TO_IP               0x6D5
#define	MSR_LASTBRANCH_22_TO_IP               0x6D6
#define	MSR_LASTBRANCH_23_TO_IP               0x6D7
#define	MSR_LASTBRANCH_24_TO_IP               0x6D8
#define	MSR_LASTBRANCH_25_TO_IP               0x6D9
#define	MSR_LASTBRANCH_26_TO_IP               0x6DA
#define	MSR_LASTBRANCH_27_TO_IP               0x6DB
#define	MSR_LASTBRANCH_28_TO_IP               0x6DC
#define	MSR_LASTBRANCH_29_TO_IP               0x6DD
#define	MSR_LASTBRANCH_30_TO_IP               0x6DE
#define	MSR_LASTBRANCH_31_TO_IP               0x6DF
#define	IA32_TSC_DEADLINE                     0x6E0
#define	MSR_UNC_CBO_0_PERFEVTSEL0             0x700
#define	MSR_UNC_CBO_0_PERFEVTSEL1             0x701
#define	MSR_UNC_CBO_0_PERFCTR0                0x706
#define	MSR_UNC_CBO_0_PERFCTR1                0x707
#define	MSR_UNC_CBO_1_PERFEVTSEL0             0x710
#define	MSR_UNC_CBO_1_PERFEVTSEL1             0x711
#define	MSR_UNC_CBO_1_PERFCTR0                0x716
#define	MSR_UNC_CBO_1_PERFCTR1                0x717
#define	MSR_UNC_CBO_2_PERFEVTSEL0             0x720
#define	MSR_UNC_CBO_2_PERFEVTSEL1             0x721
#define	MSR_UNC_CBO_2_PERFCTR0                0x726
#define	MSR_UNC_CBO_2_PERFCTR1                0x727
#define	MSR_UNC_CBO_3_PERFEVTSEL0             0x730
#define	MSR_UNC_CBO_3_PERFEVTSEL1             0x731
#define	MSR_UNC_CBO_3_PERFCTR0                0x736
#define	MSR_UNC_CBO_3_PERFCTR1                0x737
#define	IA32_PM_ENABLE                        0x770
#define	IA32_HWP_CAPABILITIES                 0x771
#define	IA32_HWP_REQUEST_PKG                  0x772
#define	IA32_HWP_INTERRUPT                    0x773
#define	IA32_HWP_REQUEST                      0x774
#define	IA32_HWP_STATUS                       0x777
#define	IA32_X2APIC_APICID                    0x802
#define	IA32_X2APIC_VERSION                   0x803
#define	IA32_X2APIC_TPR                       0x808
#define	IA32_X2APIC_PPR                       0x80A
#define	IA32_X2APIC_EOI                       0x80B
#define	IA32_X2APIC_LDR                       0x80D
#define	IA32_X2APIC_SIVR                      0x80F
#define	IA32_X2APIC_ISR0                      0x810
#define	IA32_X2APIC_ISR1                      0x811
#define	IA32_X2APIC_ISR2                      0x812
#define	IA32_X2APIC_ISR3                      0x813
#define	IA32_X2APIC_ISR4                      0x814
#define	IA32_X2APIC_ISR5                      0x815
#define	IA32_X2APIC_ISR6                      0x816
#define	IA32_X2APIC_ISR7                      0x817
#define	IA32_X2APIC_TMR0                      0x818
#define	IA32_X2APIC_TMR1                      0x819
#define	IA32_X2APIC_TMR2                      0x81A
#define	IA32_X2APIC_TMR3                      0x81B
#define	IA32_X2APIC_TMR4                      0x81C
#define	IA32_X2APIC_TMR5                      0x81D
#define	IA32_X2APIC_TMR6                      0x81E
#define	IA32_X2APIC_TMR7                      0x81F
#define	IA32_X2APIC_IRR0                      0x820
#define	IA32_X2APIC_IRR1                      0x821
#define	IA32_X2APIC_IRR2                      0x822
#define	IA32_X2APIC_IRR3                      0x823
#define	IA32_X2APIC_IRR4                      0x824
#define	IA32_X2APIC_IRR5                      0x825
#define	IA32_X2APIC_IRR6                      0x826
#define	IA32_X2APIC_IRR7                      0x827
#define	IA32_X2APIC_ESR                       0x828
#define	IA32_X2APIC_LVT_CMCI                  0x82F
#define	IA32_X2APIC_ICR                       0x830
#define	IA32_X2APIC_LVT_TIMER                 0x832
#define	IA32_X2APIC_LVT_THERMAL               0x833
#define	IA32_X2APIC_LVT_PMI                   0x834
#define	IA32_X2APIC_LVT_LINT0                 0x835
#define	IA32_X2APIC_LVT_LINT1                 0x836
#define	IA32_X2APIC_LVT_ERROR                 0x837
#define	IA32_X2APIC_INIT_COUNT                0x838
#define	IA32_X2APIC_CUR_COUNT                 0x839
#define	IA32_X2APIC_DIV_CONF                  0x83E
#define	IA32_X2APIC_SELF_IPI                  0x83F
#define	IA32_DEBUG_INTERFACE                  0xC80
#define	IA32_L3_QOS_CFG                       0xC81
#define	IA32_L2_QOS_CFG                       0xC82
#define	IA32_QM_EVTSEL                        0xC8D
#define	IA32_QM_CTR                           0xC8E
#define	IA32_PQR_ASSOC                        0xC8F
#define	IA32_BNDCFGS                          0xD90
#define	IA32_XSS                              0xDA0
#define	IA32_PKG_HDC_CTL                      0xDB0
#define	IA32_PM_CTL1                          0xDB1
#define	IA32_THREAD_STALL                     0xDB2
#define	MSR_LBR_INFO_0                        0xDC0
#define	MSR_LBR_INFO_1                        0xDC1
#define	MSR_LBR_INFO_2                        0xDC2
#define	MSR_LBR_INFO_3                        0xDC3
#define	MSR_LBR_INFO_4                        0xDC4
#define	MSR_LBR_INFO_5                        0xDC5
#define	MSR_LBR_INFO_6                        0xDC6
#define	MSR_LBR_INFO_7                        0xDC7
#define	MSR_LBR_INFO_8                        0xDC8
#define	MSR_LBR_INFO_9                        0xDC9
#define	MSR_LBR_INFO_10                       0xDCA
#define	MSR_LBR_INFO_11                       0xDCB
#define	MSR_LBR_INFO_12                       0xDCC
#define	MSR_LBR_INFO_13                       0xDCD
#define	MSR_LBR_INFO_14                       0xDCE
#define	MSR_LBR_INFO_15                       0xDCF
#define	MSR_LBR_INFO_16                       0xDD0
#define	MSR_LBR_INFO_17                       0xDD1
#define	MSR_LBR_INFO_18                       0xDD2
#define	MSR_LBR_INFO_19                       0xDD3
#define	MSR_LBR_INFO_20                       0xDD4
#define	MSR_LBR_INFO_21                       0xDD5
#define	MSR_LBR_INFO_22                       0xDD6
#define	MSR_LBR_INFO_23                       0xDD7
#define	MSR_LBR_INFO_24                       0xDD8
#define	MSR_LBR_INFO_25                       0xDD9
#define	MSR_LBR_INFO_26                       0xDDA
#define	MSR_LBR_INFO_27                       0xDDB
#define	MSR_LBR_INFO_28                       0xDDC
#define	MSR_LBR_INFO_29                       0xDDD
#define	MSR_LBR_INFO_30                       0xDDE
#define	MSR_LBR_INFO_31                       0xDDF
#define	MSR_UNC_PERF_GLOBAL_CTRL              0xE01
#define	MSR_UNC_PERF_GLOBAL_STATUS            0xE02
#define	IA32_EFER                             0xC0000080
#define	IA32_STAR                             0xC0000081
#define	IA32_LSTAR                            0xC0000082
#define	IA32_CSTAR                            0xC0000083
#define	IA32_FMASK                            0xC0000084
#define	IA32_FS_BASE                          0xC0000100
#define	IA32_GS_BASE                          0xC0000101
#define	IA32_KERNEL_GS_BASE                   0xC0000102
#define	IA32_TSC_AUX                          0xC0000103
#define	IA32_MC0_CTL2                         0x280
#define	IA32_MC1_CTL2                         0x281
#define	IA32_MC2_CTL2                         0x282
#define	IA32_MC3_CTL2                         0x283
#define	IA32_MC0_CTL                          0x400
#define	IA32_MC0_STATUS                       0x401
#define	IA32_MC1_CTL                          0x404
#define	IA32_MC1_STATUS                       0x405
#define	IA32_MC2_CTL                          0x408
#define	IA32_MC2_STATUS                       0x409
#define	IA32_MC3_CTL                          0x40C
#define	IA32_MC3_STATUS                       0x40D
#define	IA32_MC4_CTL                          0x410
#define	IA32_MC4_STATUS                       0x411
#define	IA32_MC5_CTL                          0x414
#define	IA32_MC5_STATUS                       0x415
#define	IA32_MC6_CTL                          0x418
#define	IA32_MC6_STATUS                       0x419
#define	IA32_MC7_CTL                          0x41C
#define	IA32_MC7_STATUS                       0x41D
#define	IA32_MC8_CTL                          0x420
#define	IA32_MC8_STATUS                       0x421
#define	IA32_MC9_CTL                          0x424
#define	IA32_MC9_STATUS                       0x425

#define SUPPORTED_MSR_NUM	(sizeof(supported_msr_list)/sizeof(supported_msr_list[0]))

typedef struct gen_reg_struct {
	u64 rax, rbx, rcx, rdx;
	u64 rsi, rdi, rsp, rbp;
	u64 rip, rflags;
#ifdef __x86_64__
	u64 r8, r9, r10, r11;
	u64 r12, r13, r14, r15;
#endif
	u64 cr0, cr1, cr2, cr3, cr4;
#ifdef __x86_64__
	u64 cr8;
#endif
} gen_reg_t;

u32 supported_msr_list[] = {
	IA32_P5_MC_ADDR,
	IA32_P5_MC_TYPE,
	IA32_MONITOR_FILTER_SIZE,
	IA32_TIME_STAMP_COUNTER,
	IA32_PLATFORM_ID,
	IA32_APIC_BASE,
	MSR_SMI_COUNT,
	IA32_FEATURE_CONTROL,
	IA32_TSC_ADJUST,
	IA32_SPEC_CTRL,
	IA32_BIOS_SIGN_ID,
	MSR_PLATFORM_INFO,
	MSR_FEATURE_CONFIG,
	IA32_SYSENTER_CS,
	IA32_SYSENTER_ESP,
	IA32_SYSENTER_EIP,
	IA32_MCG_CAP,
	IA32_MCG_STATUS,
	IA32_MISC_ENABLE,
	IA32_PAT,
	IA32_X2APIC_APICID,
	IA32_X2APIC_VERSION,
	IA32_X2APIC_TPR,
	IA32_X2APIC_PPR,
	IA32_X2APIC_LDR,
	IA32_X2APIC_SIVR,
	IA32_X2APIC_ISR0,
	IA32_X2APIC_ISR1,
	IA32_X2APIC_ISR2,
	IA32_X2APIC_ISR3,
	IA32_X2APIC_ISR4,
	IA32_X2APIC_ISR5,
	IA32_X2APIC_ISR6,
	IA32_X2APIC_ISR7,
	IA32_X2APIC_TMR0,
	IA32_X2APIC_TMR1,
	IA32_X2APIC_TMR2,
	IA32_X2APIC_TMR3,
	IA32_X2APIC_TMR4,
	IA32_X2APIC_TMR5,
	IA32_X2APIC_TMR6,
	IA32_X2APIC_TMR7,
	IA32_X2APIC_IRR0,
	IA32_X2APIC_IRR1,
	IA32_X2APIC_IRR2,
	IA32_X2APIC_IRR3,
	IA32_X2APIC_IRR4,
	IA32_X2APIC_IRR5,
	IA32_X2APIC_IRR6,
	IA32_X2APIC_IRR7,
	IA32_X2APIC_ESR,
	IA32_X2APIC_LVT_CMCI,
	IA32_X2APIC_ICR,
	IA32_X2APIC_LVT_TIMER,
	IA32_X2APIC_LVT_THERMAL,
	IA32_X2APIC_LVT_PMI,
	IA32_X2APIC_LVT_LINT0,
	IA32_X2APIC_LVT_LINT1,
	IA32_X2APIC_LVT_ERROR,
	IA32_X2APIC_INIT_COUNT,
	IA32_X2APIC_CUR_COUNT,
	IA32_X2APIC_DIV_CONF,
	IA32_EFER,
	IA32_STAR,
	IA32_LSTAR,
	IA32_CSTAR,
	IA32_FMASK,
	IA32_FS_BASE,
	IA32_GS_BASE,
	IA32_KERNEL_GS_BASE,
#ifdef IN_SAFETY_VM
	IA32_MC4_CTL2,
	IA32_MC5_CTL2,
	IA32_MC6_CTL2,
	IA32_MC7_CTL2,
	IA32_MC8_CTL2,
	IA32_MC9_CTL2,
	IA32_MC0_CTL2,
	IA32_MC1_CTL2,
	IA32_MC2_CTL2,
	IA32_MC3_CTL2,
	IA32_MC0_CTL,
	IA32_MC0_STATUS,
	IA32_MC1_CTL,
	IA32_MC1_STATUS,
	IA32_MC2_CTL,
	IA32_MC2_STATUS,
	IA32_MC3_CTL,
	IA32_MC3_STATUS,
	IA32_MC4_CTL,
	IA32_MC4_STATUS,
	IA32_MC5_CTL,
	IA32_MC5_STATUS,
	IA32_MC6_CTL,
	IA32_MC6_STATUS,
	IA32_MC7_CTL,
	IA32_MC7_STATUS,
	IA32_MC8_CTL,
	IA32_MC8_STATUS,
	IA32_MC9_CTL,
	IA32_MC9_STATUS,
#endif
	IA32_TSC_AUX
};


bool xsave_reg_dump(void *ptr);

/*-------------------------------------------------*
 *Genaral register dump
 *
 *
 */
static inline __attribute__((always_inline)) void gen_reg_dump(void *ptr)
{
	struct gen_reg_struct *reg_dump;

	reg_dump = (struct gen_reg_struct *)ptr;
	asm volatile ("mov %%" R "ax," "%0" : "=m"(reg_dump->rax)::"memory");
	asm volatile ("mov %%" R "bx," "%0" : "=m"(reg_dump->rbx)::"memory");
	asm volatile ("mov %%" R "cx," "%0" : "=m"(reg_dump->rcx)::"memory");
	asm volatile ("mov %%" R "dx," "%0" : "=m"(reg_dump->rdx)::"memory");
	asm volatile ("mov %%" R "si," "%0" : "=m"(reg_dump->rsi)::"memory");
	asm volatile ("mov %%" R "di," "%0" : "=m"(reg_dump->rdi)::"memory");
	asm volatile ("mov %%" R "sp," "%0" : "=m"(reg_dump->rsp)::"memory");
	asm volatile ("mov %%" R "bp," "%0" : "=m"(reg_dump->rbp)::"memory");
	asm volatile ("pushf; pop %0" : "=m"(reg_dump->rflags)::"memory");
#ifdef __x86_64__
	asm volatile ("mov %%r8, %0" : "=m"(reg_dump->r8)::"memory");
	asm volatile ("mov %%r9, %0" : "=m"(reg_dump->r9)::"memory");
	asm volatile ("mov %%r10," "%0" : "=m"(reg_dump->r10)::"memory");
	asm volatile ("mov %%r11," "%0" : "=m"(reg_dump->r11)::"memory");
	asm volatile ("mov %%r12," "%0" : "=m"(reg_dump->r12)::"memory");
	asm volatile ("mov %%r13," "%0" : "=m"(reg_dump->r13)::"memory");
	asm volatile ("mov %%r14," "%0" : "=m"(reg_dump->r14)::"memory");
	asm volatile ("mov %%r15," "%0" : "=m"(reg_dump->r15)::"memory");
#endif
	asm volatile ("mov %%cr0,%%" R "ax \n"
				  "mov %%" R "ax," "%0"
				  : "=m"(reg_dump->cr0)::"memory");
	asm volatile ("mov %%cr2,%%" R "ax \n"
				  "mov %%" R "ax," "%0"
				  : "=m"(reg_dump->cr2)::"memory");
	asm volatile ("mov %%cr3,%%" R "ax \n"
				  "mov %%" R "ax," "%0"
				  : "=m"(reg_dump->cr3)::"memory");
	asm volatile ("mov %%cr4,%%" R "ax \n"
				  "mov %%" R "ax," "%0"
				  : "=m"(reg_dump->cr4)::"memory");
#ifdef __x86_64__
	asm volatile ("mov %%cr8,%%" R "ax \n"
				  "mov %%" R "ax," "%0"
				  : "=m"(reg_dump->cr8)::"memory");
#endif
	/*restore eax/rax value*/
	asm volatile ("mov %0, %%" R "ax"::"m"(reg_dump->rax):"memory");

}
#endif
