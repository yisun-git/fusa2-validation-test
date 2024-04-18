#!/usr/bin/env bash

usage() {
    echo "$(basename $0): wrong arguments!" 1>&2
    echo "  Usage: $(basename $0) 64|32 test..." 1>&2
}

if [[ $# < 2 ]]; then
    usage
    exit 1
fi

BIT=$1

if [[ ${BIT} != "64" && ${BIT} != "32" ]]; then
    usage
    exit 1
fi

# FIXME is this warning still needed?
#
#cd ../..
#patch -p1 < native.patch
#cd  acrn-unit-test/guest
echo "Please make the patch of native first"

if [[ ${BIT} == "64" ]]; then
    ./configure --arch=x86_64
else
    ./configure --arch=i386
fi

make clean

shift
for i in "$@";
do
    if [[ ${BIT} == "64" ]]; then
        export FILE_NAME=${i}
    fi
    echo "make x86/"${i}".raw"
    make x86/${i}.raw NATIVE=yes
    result=$?
    if [[ $result != 0 ]]; then
        echo "FAILED TO MAKE ${i}"
        exit $result
    fi
    mkdir -p x86/obj
    mv x86/${i}.elf x86/obj/${i}_native_${BIT}.elf
done
