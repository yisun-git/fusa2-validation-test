#!/usr/bin/env bash

usage() {
    echo "$(basename $0): wrong arguments!" 1>&2
    echo "  Usage: $(basename $0) test [PHASE_X]" 1>&2
}

if [[ $# < 1 ]]; then
    usage
    exit 1
fi

# FIXME is this warning still needed?
#cd ../..
#patch -p1 < native.patch
#cd  acrn-unit-test/guest
echo "Please make the patch of native first"

./configure --arch=i386
make clean

echo "make x86/v8086/"$1".raw"
export PHASE_N=$2
echo "$PHASE_N"
make x86/v8086/$1.raw NATIVE=yes V8086=v8086_mode V8086_MAIN=$1
make_result=$?
if [ $make_result != 0 ]; then
    echo "FAILED TO MAKE ${i}"
    exit $make_result
fi

mkdir -p x86/obj
if [[ $2 != "" ]]; then
    mv x86/v8086/$1.elf x86/obj/$1_native_$2.elf
else
    mv x86/v8086/$1.elf x86/obj/$1_native.elf
fi
