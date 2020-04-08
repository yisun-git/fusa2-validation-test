#!/usr/bin/env bash

make clean
./configure --arch=x86_64
make x86/machine_check.bzimage QEMU=1
make_result=$?
if [ $make_result != 0 ]; then
	exit $make_result
fi

mv x86/machine_check.bzimage x86/obj/machine_check_qemu.bzimage

