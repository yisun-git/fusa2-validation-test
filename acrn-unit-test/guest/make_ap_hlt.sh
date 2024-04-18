#!/usr/bin/env bash

export AP_HLT=1
$(dirname $0)/make_non_safety.sh 64 $1
