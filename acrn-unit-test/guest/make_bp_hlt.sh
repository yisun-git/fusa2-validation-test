#!/usr/bin/env bash

export BP_HLT=1
$(dirname $0)/make_safety.sh 64 $1
