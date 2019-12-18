#!/usr/bin/env bash

BUILD_64BIT_FEATURE="smx sgx smm hyperthread"
BUILD_32BIT_FEATURE=""
BUILD_REAL_MODE_FEATURE=""
BUILD_V8086_FEATURE=""
BUILD_NATIVE_FEATURE=""

for i in $BUILD_64BIT_FEATURE;  
do  
	echo "start build $i 64bit mode file"
	./make64.sh $i;
done

for i in $BUILD_32BIT_FEATURE;
do
        echo "start build $i protected mode file"
        ./make32_and_real_mode.sh $i;
done

for i in $BUILD_REAL_MODE_FEATURE;
do
        echo "start build $i real mode file"
        ./make32_and_real_mode.sh $i;
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
done
