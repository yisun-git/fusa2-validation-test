#!/usr/bin/env bash

make clean
./configure --arch=x86_64
echo "make x86/"$1".raw"
make x86/$1.raw VM=safety
mv x86/$1.raw x86/obj/$1_$2.raw
