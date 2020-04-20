#!/usr/bin/env bash

BUILD_64BIT_FEATURE="smx sgx smm hyperthread rtc mem_cache segmentation multiboot \
	memory_order tsc mp_initialization xsave local_apic paging general_purpose pmu_fu rdt branch_profile \
	device_passthrough mmx locked_atomic machine_check"

#BUILD_64BIT_FEATURE="mem_cache"
BUILD_32BIT_FEATURE="segmentation paging general_purpose mmx cpumode"
BUILD_REAL_MODE_FEATURE=""
BUILD_V8086_FEATURE=""
BUILD_NATIVE_64_FEATURE="xsave device_passthrough"
BUILD_NATIVE_32_FEATURE=""

RESULT=0

	find . -name *.raw |xargs rm -rf
	find . -name *.elf |xargs rm -rf
	find . -name *.bzimage |xargs rm -rf
	rm -rf x86/obj
	mkdir x86/obj
for i in $BUILD_32BIT_FEATURE;
do
        echo "start build $i protected mode file"
        ./make32_and_real_mode_non_safety.sh $i 32;
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
        ./make32_and_real_mode_non_safety.sh $i real_mode;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
	    exit $RESULT
        fi
done

#for i in $BUILD_V8086_FEATURE;
#do
#        echo "start build $i v8086 mode file"
#        ./make32_and_real_mode.sh $i;
#done

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
