#!/usr/bin/env bash

#cd ../..
#patch -p1 < native.patch
#cd  acrn-unit-test/guest
echo “Please make the patch of native first”
make clean
./configure --arch=i386
echo "make x86/realmode/"$1".raw"
export PHASE_N=$2
echo "$PHASE_N"
make x86/realmode/$1.raw NATIVE=yes REAL_MODE=yes
make_result=$?
if [ $make_result != 0 ]; then
    exit $make_result
fi
mv x86/realmode/$1.elf x86/obj/$1_native_$2.elf
