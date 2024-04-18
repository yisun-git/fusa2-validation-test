#!/usr/bin/env bash

usage() {
    echo "$(basename $0): wrong arguments!" 1>&2
    echo "  Usage: $(basename $0) test [PHASE_X]" 1>&2
}

if [[ $# < 1 ]]; then
    usage
    exit 1
fi

./configure --arch=i386
make clean

echo "make x86/realmode/"$1".bzimage"
export PHASE_N=$2
echo "$PHASE_N"
make x86/realmode/$1.bzimage VM=non-safety REAL_MODE=yes
make_result=$?
if [ $make_result != 0 ]; then
    echo "FAILED TO MAKE ${i}"
    exit $make_result
fi

mkdir -p x86/obj
if [[ $2 != "" ]]; then
    mv x86/realmode/$1.bzimage x86/obj/$1_$2.bzimage
else
    mv x86/realmode/$1.bzimage x86/obj/$1.bzimage
fi
