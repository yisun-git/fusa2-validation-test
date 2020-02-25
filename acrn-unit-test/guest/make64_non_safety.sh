#!/usr/bin/env bash

make clean
./configure --arch=x86_64
echo "make x86/"$1".bzimage"
make x86/$1.bzimage VM=non-safety
mv x86/$1.bzimage x86/obj/$1_$2.bzimage
