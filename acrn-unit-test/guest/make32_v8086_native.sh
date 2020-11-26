#!/usr/bin/env bash
# usage: ./make32_v8086_native.sh v8086_main

#cd ../..
#patch -p1 < native.patch
#cd  acrn-unit-test/guest
echo "Please make the patch of native first"
make clean
./configure --arch=i386
echo "make x86/v8086/"$1".raw"

make x86/v8086/$1.raw NATIVE=yes V8086=v8086_mode V8086_MAIN=$1

make_result=$?
if [ $make_result != 0 ]; then
    exit $make_result
fi
mv x86/v8086/$1.elf x86/obj/$1_native.elf
