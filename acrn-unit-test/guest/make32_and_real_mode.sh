#!/usr/bin/env bash

make clean
./configure --arch=i386
echo "make x86/"$1".bzimage"
make x86/$1.bzimage

