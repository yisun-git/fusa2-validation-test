#!/usr/bin/env bash

#cd ../..
#patch -p1 < native.patch
#cd  acrn-unit-test/guest
echo “Please make the patch of native first”
make clean
./configure --arch=x86_64
echo "make x86/"$1".raw"
make x86/$1.raw NATIVE=yes
mv x86/$1.elf x86/obj/$1_native_$2.elf
