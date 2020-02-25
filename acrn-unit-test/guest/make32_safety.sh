#!/usr/bin/env bash

make clean
./configure --arch=i386
echo "make x86/"$1".raw"
make x86/$1.raw VM=safety
if [ $? != 0 ]; then
    exit $?
fi
mv x86/$1.raw x86/obj/$1_$2.raw
