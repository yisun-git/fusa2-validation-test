#!/usr/bin/env bash

make clean
./configure --arch=x86_64
export FILE_NAME=$1
echo "make x86/"$1".raw"
make x86/$1.raw VM=safety BP_HLT=1
make_result=$?
if [ $make_result != 0 ]; then
	exit $make_result
fi
mv x86/$1.raw x86/obj/$1.raw

