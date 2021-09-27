#!/usr/bin/env bash

make clean
./configure --arch=i386
echo "make x86/realmode/"$1".bzimage"
export PHASE_N=$2
echo "$PHASE_N"
make x86/realmode/$1.bzimage VM=non-safety REAL_MODE=yes
make_result=$?
if [ $make_result != 0 ]; then
    exit $make_result
fi
mv x86/realmode/$1.bzimage x86/obj/$1_$2.bzimage
