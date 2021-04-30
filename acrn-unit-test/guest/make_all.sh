#!/usr/bin/env bash

BUILD_64BIT_FEATURE="smx sgx smm hyperthread rtc mem_cache segmentation multiboot \
	memory_order tsc mp_initialization xsave local_apic paging general_purpose pmu_fu rdt branch_profile \
	device_passthrough mmx locked_atomic machine_check vmx fpu info_leakage idle_block sse \
	taskmanagement interrupt pt pci power_thermal avx mpx segmentation \
	safety_analysis_high_direct safety_analysis_high_dma safety_analysis_low_direct safety_analysis_low_dma debug_features \
	misc_msr interrupt_mc misc_cpuid tsx multiboot_low cpumode exception_avx_b6 exception_avx_pt_b6 exception_avx_pt_ra_b6 \
        exception_fpu_b6 exception_fpu_pt_b6 exception_fpu_pt_ra_b6 exception_fpu_ra_b6 exception_gp_b6 exception_gp_pt_b6 \
        exception_gp_pt_ra_b6 exception_mmx_b6 exception_mmx_pt_ra_b6 exception_mpx_b6 exception_mpx_pt_b6 exception_mpx_pt_ra_b6 \
        exception_mpx_ra_b6 exception_sse_b6 exception_sse_pt_ra_b6"

#BUILD_64BIT_FEATURE="mem_cache"
BUILD_32BIT_FEATURE="segmentation paging general_purpose mmx cpumode fpu sse taskmanagement interrupt pt avx mpx \
	segmentation interrupt_triple_fault v8086_protect exception_avx_pt exception_avx_pt_b6 exception_avx_pt_ra_b6 exception_fpu_pt \
        exception_fpu_pt_b6 exception_fpu_pt_ra_b6 exception_gp_pt exception_gp_pt_b6 exception_gp_pt_ra exception_gp_pt_ra_b6 \
        exception_mmx_pt exception_mmx_pt_ra_b6 exception_mpx_pt exception_mpx_pt_b6 exception_mpx_pt_ra_b6 exception_sse_pt exception_sse_pt_ra_b6 \
        "
BUILD_REAL_MODE_FEATURE="rmode_v8086 rmode_seg"
BUILD_V8086_FEATURE="v8086_part1 v8086_part2 v8086_part3 v8086_seg"

BUILD_NATIVE_64_FEATURE="xsave device_passthrough sse pt info_leakage safety_analysis_cat machine_check debug_features \
	mem_cache taskmanagement idle_block local_apic rtc segmentation paging memory_order misc_cpuid tsx locked_atomic \
	fpu mmx general_purpose sgx avx application_constraints"

BUILD_NATIVE_64_FEATURE="$BUILD_NATIVE_64_FEATURE hsi_mem_paging_access_low hsi_multi_proc_mgmt hsi_mem_mgmt \
	hsi_inter_mgmt hsi_inter_control hsi_local_apic hsi_peripherals hsi_gp hsi_virtual_spec hsi_inter_remap"

BUILD_NATIVE_32_FEATURE="taskmanagement general_purpose"
BUILD_NATIVE_32_FEATURE="$BUILD_NATIVE_32_FEATURE hsi_mem_mgmt hsi_gp"
BUILD_NATIVE_REAL_MODE_FEATURE="hsi_mem_mgmt hsi_gp_rmode"

# The image size cannot be more than 64K in 16-bit mode, so split the cases in one file into several parts if needed.
# the last character of the feature name, is the max index number of the splitted parts.
# for example, 'exception_sse_ra_2' means exception_sse_ra.c can be used for compiling 3 images for test.
BUILD_AUTO_GEN_REAL_MODE_FEATURE="exception_avx_pt_ra_b6_0 exception_avx_ra_0 exception_fpu_pt_ra_b6_0 exception_fpu_ra_0 \
        exception_fpu_ra_b6_0 exception_gp_pt_ra_0 exception_gp_pt_ra_b6_0 exception_gp_ra_1 exception_mmx_pt_ra_b6_0 \
        exception_mmx_ra_0 exception_mpx_pt_ra_b6_0 exception_mpx_ra_0 exception_mpx_ra_b6_0 exception_sse_pt_ra_b6_1 \
        exception_sse_ra_2"
BUILD_AUTO_GEN_V8086_FEATURE="exception_gp_v8_0 exception_sse_v8_1"

RESULT=0

	find . -name *.raw |xargs rm -rf
	find . -name *.elf |xargs rm -rf
	find . -name *.bzimage |xargs rm -rf
	rm -rf x86/obj
	mkdir x86/obj
	mkdir x86/obj/realmode

# handle the v8086 features of auto-gen
for i in $BUILD_AUTO_GEN_V8086_FEATURE;
do
        echo "start build $i v8086 mode file"
        for ((j=0;j<=${i:0-1};j++))
        do
                ./make32_v8086_non_safety.sh ${i%??} PHASE_$j
                make_result=$?
                if [ $make_result -ne 0 ]; then
                        RESULT=$make_result
                        echo "FAILED TO MAKE $i"
	                exit $RESULT
                fi
        done
done

# handle the real mode features of auto-gen
for i in $BUILD_AUTO_GEN_REAL_MODE_FEATURE;
do
        echo "start build $i real mode file"
        for ((j=0;j<=${i:0-1};j++))
        do
                ./make32_real_mode_non_safety.sh ${i%??} PHASE_$j
                make_result=$?
                if [ $make_result -ne 0 ]; then
                        RESULT=$make_result
                        echo "FAILED TO MAKE $i"
	                exit $RESULT
                fi
        done
done
#end

for i in $BUILD_32BIT_FEATURE;
do
        echo "start build $i protected mode file"
        ./make32_non_safety.sh $i 32;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
	    exit $RESULT
        fi

	./make32_safety.sh $i 32;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
	    exit $RESULT
        fi
done

for i in $BUILD_REAL_MODE_FEATURE;
do
        echo "start build $i real mode file"
        ./make32_real_mode_non_safety.sh $i;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
	    exit $RESULT
        fi
done

for i in $BUILD_V8086_FEATURE;
do
        echo "start build $i v8086 mode file"
        ./make32_v8086_non_safety.sh $i;
	make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
	    exit $RESULT
        fi
done

for i in $BUILD_NATIVE_64_FEATURE;
do
        echo "start build $i native 64bit mode file"
        ./make64_native.sh $i 64;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
	    exit $RESULT
        fi
done

for i in $BUILD_NATIVE_32_FEATURE;
do
	echo "start build $i native 32bit mode file"
	./make32_native.sh $i 32;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
	    exit $RESULT
        fi

done

for i in $BUILD_NATIVE_REAL_MODE_FEATURE;
do
	echo "start build $i native real mode file"
	./make32_native.sh realmode/$i real_mode;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
	    exit $RESULT
        fi

done

for i in $BUILD_64BIT_FEATURE;  
do  
	echo "start build $i 64bit mode file"
	./make64_non_safety.sh $i 64;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
	    exit $RESULT
        fi

	./make64_safety.sh $i 64;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
	    exit $RESULT
        fi
done

./make_qemu.sh;
       make_result=$?
       if [ $make_result -ne 0 ]; then
           RESULT=$make_result
           echo "FAILED TO MAKE $i"
       exit $RESULT
       fi

cp x86/obj/*  x86/

echo "final make result for all images:"
echo $RESULT

exit $RESULT
