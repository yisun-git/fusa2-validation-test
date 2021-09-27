#!/usr/bin/env bash

make clean
./configure --arch=i386
echo "make x86/"$1".raw"
make x86/$1.raw VM=safety
make_result=$?
if [ $make_result != 0 ]; then
    exit $make_result
fi
mv x86/$1.raw x86/obj/$1_$2.raw
