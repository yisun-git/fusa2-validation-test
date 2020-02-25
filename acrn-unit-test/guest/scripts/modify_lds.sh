#! /bin/bash
set -x

pwd
start_address="    . = $@;"

sed -i "3c${start_address}"  x86/flat.lds
sed -i "3s/^/    /" x86/flat.lds

