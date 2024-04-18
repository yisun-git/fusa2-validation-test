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
    echo "make x86/"${i}".bzimage"
    make x86/${i}.bzimage VM=safety
    result=$?
    if [[ $result != 0 ]]; then
        echo "FAILED TO MAKE ${i}"
        exit $result
    fi
    mkdir -p x86/obj
    mv x86/${i}.bzimage x86/obj/${i}_${BIT}_safety.bzimage
done
