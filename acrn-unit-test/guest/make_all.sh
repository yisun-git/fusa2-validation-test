#!/usr/bin/env bash

BUILD_64BIT_FEATURE="smx sgx smm hyperthread rtc_test mem_cache segmentation"
#BUILD_64BIT_FEATURE="mem_cache"
BUILD_32BIT_FEATURE="segmentation"
BUILD_REAL_MODE_FEATURE=""
BUILD_V8086_FEATURE=""
BUILD_NATIVE_FEATURE=""

RESULT=0

for i in $BUILD_32BIT_FEATURE;
do
        echo "start build $i protected mode file"
        ./make32_and_real_mode.sh $i 32;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
        fi
done

for i in $BUILD_REAL_MODE_FEATURE;
do
        echo "start build $i real mode file"
        ./make32_and_real_mode.sh $i real_mode;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
        fi
done

#for i in $BUILD_V8086_FEATURE;
#do
#        echo "start build $i v8086 mode file"
#        ./make32_and_real_mode.sh $i;
#done

for i in $BUILD_NATIVE_FEATURE;
do
        echo "start build $i native mode file"
        ./make_native.sh $i;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
        fi
done

for i in $BUILD_64BIT_FEATURE;  
do  
	echo "start build $i 64bit mode file"
	./make64.sh $i 64;
        make_result=$?
        if [ $make_result -ne 0 ]; then
            RESULT=$make_result
            echo "FAILED TO MAKE $i"
        fi
done


echo "final make result for all images:"
echo $RESULT

exit $RESULT
