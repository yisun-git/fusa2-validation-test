#!/bin/bash

# Check if the user is root
if [ "$(id -u)" != "0" ]; then
	echo "This script must be run as root" 1>&2
	exit 1
fi

# Check if msr-tools is installed
if ! command -v rdmsr &> /dev/null
then
	echo "msr-tools could not be found"
	exit
fi

# Define the output file
OUTPUT_FILE="machine_check_info.h"

# Check if the file exists
if [ -f "$OUTPUT_FILE" ]; then
    # Clear the content of the file
    echo -n "" > $OUTPUT_FILE
    echo "The content of $OUTPUT_FILE has been cleared."
fi

CPUIndex=0

#write the header
echo "#ifndef __MACHINE_CHECK_INFO_H__" >> $OUTPUT_FILE
echo "#define __MACHINE_CHECK_INFO_H__" >> $OUTPUT_FILE
echo -e "\n" >> $OUTPUT_FILE
echo "#include \"libcflat.h\"" >> $OUTPUT_FILE

# Get bank num
MSR_VALUE=$(rdmsr -p${CPUIndex} 0x179)
bank_num=$((0x${MSR_VALUE} & 0xFF))
echo "int bank_num = $bank_num;" >> $OUTPUT_FILE

# Read the MSR IA32_MCG_CAP
echo "u64 IA32_MCG_CAP = 0x$(rdmsr -p${CPUIndex} 0x179);" >> $OUTPUT_FILE

# Read the MSR IA32_MCG_STATUS
echo "u64 IA32_MCG_STATUS = 0x$(rdmsr -p${CPUIndex} 0x17A);" >> $OUTPUT_FILE

echo "u64 IA32_MCi_CTL[${bank_num}] =" >> $OUTPUT_FILE
echo "{" >> $OUTPUT_FILE
# Read IA32_MCi_CTL
for ((i=0; i<$bank_num; i++))
do
	#enable all error reporting flag
	wrmsr -p${CPUIndex} $((0x400+i*4)) 0xFFFFFFFFFFFFFFFF
	echo "0x$(rdmsr -p${CPUIndex} $((0x400+i*4)))," >> $OUTPUT_FILE
done
echo "};" >> $OUTPUT_FILE

if (( 0x${MSR_VALUE} & $((1 << 10)) == 0));
then
        echo "IA32_MCG_CAP[10] = 1 is not enabled"
	exit
fi

echo "u64 IA32_MCi_CTL2[${bank_num}] =" >> $OUTPUT_FILE
echo "{" >> $OUTPUT_FILE
# Read IA32_MCi_CTL2
for ((i=0; i<$bank_num; i++))
do
	value=0x$(rdmsr -p${CPUIndex} $((0x280+i)))
	#enable CMCI_EN, set 0x7FFF to corrected error count threshold
	wrmsr -p${CPUIndex} $((0x280+i)) $((value|(1<<30)|0x7FFF))
	echo "0x$(rdmsr -p${CPUIndex} $((0x280+i)))," >> $OUTPUT_FILE
done
echo "};" >> $OUTPUT_FILE

echo "u64 IA32_MCi_STATUS[${bank_num}] =" >> $OUTPUT_FILE
echo "{" >> $OUTPUT_FILE
# Read IA32_MCi_STATUS
for ((i=0; i<$bank_num; i++))
do
        echo "0x$(rdmsr -p${CPUIndex} $((0x401+i*4)))," >> $OUTPUT_FILE
done
echo "};" >> $OUTPUT_FILE

echo "u64 IA32_MCi_ADDR[${bank_num}] =" >> $OUTPUT_FILE
echo "{" >> $OUTPUT_FILE
# Read IA32_MCi_ADDR
for ((i=0; i<$bank_num; i++))
do
        echo "0x$(rdmsr -p${CPUIndex} $((0x402+i*4)))," >> $OUTPUT_FILE
done
echo "};" >> $OUTPUT_FILE

echo "u64 IA32_MCi_MISC[${bank_num}] =" >> $OUTPUT_FILE
echo "{" >> $OUTPUT_FILE
# Read IA32_MCi_MISC
for ((i=0; i<$bank_num; i++))
do
        echo "0x$(rdmsr -p${CPUIndex} $((0x403+i*4)))," >> $OUTPUT_FILE
done
echo "};" >> $OUTPUT_FILE


echo -e "\n" >> $OUTPUT_FILE
echo "#endif" >> $OUTPUT_FILE

