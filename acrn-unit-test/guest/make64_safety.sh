#!/usr/bin/env bash

make clean
./configure --arch=x86_64
export FILE_NAME=$1
echo "make x86/"$1".bzimage"
make x86/$1.bzimage VM=safety
make_result=$?
if [ $make_result != 0 ]; then
    exit $make_result
fi
mv x86/$1.bzimage x86/obj/$1_$2_safety.bzimage
