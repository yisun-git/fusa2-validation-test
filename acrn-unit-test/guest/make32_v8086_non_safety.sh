#!/usr/bin/env bash
# usage: ./make32_v8086_non_safety.sh v8086_main

make clean
./configure --arch=i386
echo "make x86/v8086/"$1".bzimage"

export PHASE_N=$2
echo "$PHASE_N"

make x86/v8086/$1.bzimage VM=non-safety V8086=v8086_mode V8086_MAIN=$1

make_result=$?
if [ $make_result != 0 ]; then
    exit $make_result
fi
mv x86/v8086/$1.bzimage x86/obj/$1_$2.bzimage
