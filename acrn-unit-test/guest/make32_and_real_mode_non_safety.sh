#!/usr/bin/env bash

make clean
./configure --arch=i386
echo "make x86/"$1".bzimage"
make x86/$1.bzimage VM=non-safety
if [ $? != 0 ]; then
    exit $?
fi
mv x86/$1.bzimage x86/obj/$1_$2.bzimage
