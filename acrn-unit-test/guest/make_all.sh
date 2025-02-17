#!/usr/bin/env bash

set -e

BUILD_64BIT_FEATURE="smx sgx smm hyperthread rtc mem_cache segmentation multiboot \
    memory_order tsc mp_initialization xsave local_apic paging general_purpose pmu_fu rdt branch_profile \
    device_passthrough mmx locked_atomic machine_check vmx fpu info_leakage idle_block sse \
    taskmanagement interrupt pt pci power_thermal avx mpx \
    safety_analysis_high_direct safety_analysis_high_dma safety_analysis_low_direct safety_analysis_low_dma debug_features \
    misc_msr interrupt_mc misc_cpuid tsx multiboot_low cpumode exception_avx_b6 exception_avx_pt_b6 exception_avx_pt_ra_b6 \
    exception_fpu_b6 exception_fpu_pt_b6 exception_fpu_pt_ra_b6 exception_fpu_ra_b6 exception_gp_b6 exception_gp_pt_b6 \
    exception_gp_pt_ra_b6 exception_mmx_b6 exception_mmx_pt_ra_b6 exception_mpx_b6 exception_mpx_pt_b6 exception_mpx_pt_ra_b6 \
    exception_mpx_ra_b6 exception_sse_b6 exception_sse_pt_ra_b6 interrupt_df_pf hypercall user_interrupts"

BUILD_32BIT_FEATURE="mem_cache segmentation paging general_purpose mmx cpumode fpu sse taskmanagement interrupt pt avx mpx \
    interrupt_triple_fault v8086_protect exception_avx_pt exception_avx_pt_b6 exception_avx_pt_ra_b6 exception_fpu_pt \
    exception_fpu_pt_b6 exception_fpu_pt_ra_b6 exception_gp_pt exception_gp_pt_b6 exception_gp_pt_ra exception_gp_pt_ra_b6 \
    exception_mmx_pt exception_mmx_pt_ra_b6 exception_mpx_pt exception_mpx_pt_b6 exception_mpx_pt_ra_b6 exception_sse_pt exception_sse_pt_ra_b6 \
    hypercall"
BUILD_REAL_MODE_FEATURE="rmode_v8086 rmode_seg"
BUILD_V8086_FEATURE="v8086_part1 v8086_part2 v8086_part3 v8086_seg"

BUILD_NATIVE_64_FEATURE="pmu_fu tsc xsave device_passthrough sse pt info_leakage safety_analysis_cat machine_check debug_features \
    mem_cache taskmanagement idle_block local_apic rtc segmentation paging memory_order misc_cpuid tsx locked_atomic \
    fpu mmx general_purpose sgx avx application_constraints exception_gp_pt_b6 exception_gp_pt_ra_b6"

BUILD_NATIVE_64_FEATURE="$BUILD_NATIVE_64_FEATURE hsi_mem_paging_access_low hsi_multi_proc_mgmt hsi_mem_mgmt \
    hsi_inter_mgmt hsi_inter_control hsi_local_apic hsi_peripherals hsi_gp hsi_virtual_spec hsi_inter_remap \
    hsi_16_64_cpumode hsi_gp_cpumode"

BUILD_NATIVE_32_FEATURE="paging taskmanagement general_purpose exception_gp_pt_b6 exception_gp_pt_ra_b6"
BUILD_NATIVE_32_FEATURE="$BUILD_NATIVE_32_FEATURE hsi_mem_mgmt hsi_gp"
BUILD_NATIVE_REAL_MODE_FEATURE="hsi_mem_mgmt hsi_gp_rmode"

# The image size cannot be more than 64K in 16-bit mode, so split the cases in one file into several parts if needed.
# the last character of the feature name, is the max index number of the splitted parts.
# for example, 'exception_sse_ra_3' means exception_sse_ra.c can be used for compiling 4 images for test.
BUILD_AUTO_GEN_REAL_MODE_FEATURE="exception_avx_pt_ra_b6_0 exception_avx_ra_0 exception_fpu_pt_ra_b6_0 exception_fpu_ra_0 \
    exception_fpu_ra_b6_0 exception_gp_pt_ra_0 exception_gp_pt_ra_b6_0 exception_gp_ra_1 exception_mmx_pt_ra_b6_0 \
    exception_mmx_ra_0 exception_mpx_pt_ra_b6_0 exception_mpx_ra_1 exception_mpx_ra_b6_0 exception_sse_pt_ra_b6_1 \
    exception_sse_ra_3"
BUILD_AUTO_GEN_V8086_FEATURE="exception_gp_v8_0 exception_sse_v8_1"


rm -rf x86/obj
mkdir -p x86/obj

# handle the v8086 features of auto-gen
for i in $BUILD_AUTO_GEN_V8086_FEATURE;
do
    echo "start build $i v8086 mode file"
    for ((j=0;j<=${i:0-1};j++))
    do
        ./make32_v8086_non_safety.sh ${i%??} PHASE_$j
    done
done

# handle the real mode features of auto-gen
for i in $BUILD_AUTO_GEN_REAL_MODE_FEATURE;
do
    echo "start build $i real mode file"
    for ((j=0;j<=${i:0-1};j++))
    do
        ./make32_real_mode_non_safety.sh ${i%??} PHASE_$j
    done
done
#end

./make_non_safety.sh 32 $BUILD_32BIT_FEATURE
./make_safety.sh 32 $BUILD_32BIT_FEATURE

for i in $BUILD_REAL_MODE_FEATURE;
do
    echo "start build $i real mode file"
    ./make32_real_mode_non_safety.sh $i
done

for i in $BUILD_V8086_FEATURE;
do
    echo "start build $i v8086 mode file"
    ./make32_v8086_non_safety.sh $i
done

./make_native.sh 64 $BUILD_NATIVE_64_FEATURE
./make_native.sh 32 $BUILD_NATIVE_32_FEATURE

for i in $BUILD_NATIVE_REAL_MODE_FEATURE;
do
    echo "start build $i native real mode file"
    ./make32_real_mode_native.sh $i
done

./make_non_safety.sh 64 $BUILD_64BIT_FEATURE
./make_safety.sh 64 $BUILD_64BIT_FEATURE

./make_qemu_safety.sh machine_check

./make32_real_mode_native.sh rmode_v8086
./make32_real_mode_native.sh exception_gp_pt_ra_b6
./make32_v8086_native.sh v8086_part1

# Section for special cases
#

# compile those cases with the macro PHASE_N defined.
mv x86/obj/memory_order_64.bzimage x86/obj/memory_order_multi_case_64.bzimage
mv x86/obj/memory_order_native_64.elf x86/obj/memory_order_multi_case.elf

export PHASE_N=PHASE_RUN_SINGLE_CASE
BUILD_FEATURE="memory_order"

./make_non_safety.sh 64 $BUILD_FEATURE
mv x86/obj/memory_order_64.bzimage x86/obj/memory_order_single_case_64.bzimage

./make_native.sh 64 $BUILD_FEATURE
mv x86/obj/memory_order_native_64.elf x86/obj/memory_order_single_case.elf

export PHASE_N=

# End of section for special cases

echo "Finish making all images"
exit 0
