#!/bin/bash
#The pci devs
pci_devs=("USB" "Ethernet")
pci_bdf="00:00.0"
h_name="pci_native_config.h"
h_head="#ifndef __PCI_NATIVE_CONFIG_H__\n#define __PCI_NATIVE_CONFIG_H__\n"
h_tail="\n\n\n#endif"
pci_interrupt_line="00"
pci_interrupt_pin="00"
pci_cache_line="00"

function get_dev_bdf(){
#echo $1
pci_bdf=$(lspci|grep "$1"|awk '{print $1}')
return 0
}

function get_pci_config_interrupt_line(){
pci_interrupt_line=$(lspci -xxx -s $1|grep "30:" |awk \
	'{print "#define NATIVE_'$2'_INTERRUPT_LINE " "(0x" $14 ")"}')
return 0
}

function get_pci_config_interrupt_pin(){
pci_interrupt_pin=$(lspci -xxx -s $1|grep "30:" |awk \
        '{print "#define NATIVE_'$2'_INTERRUPT_PIN " "(0x" $15 ")"}')
return 0
}

function get_pci_config_cache_line(){
pci_cache_line=$(lspci -xxx -s $1|sed -n "2p;"|grep "00:" |awk \
        '{print "#define NATIVE_'$2'_CACHE_LINE " "(0x" $14 ")"}')
return 0
}


function __main__(){
echo `rm -fr ${h_name}`
echo -e ${h_head} > ${h_name}
#For each pci dev, do lspci
for((i=0;i<${#pci_devs[@]};i++)) do
name=${pci_devs[i]}
get_dev_bdf ${name}
#echo ${pci_bdf} >> ${h_name}
get_pci_config_interrupt_line ${pci_bdf} ${name}
get_pci_config_interrupt_pin ${pci_bdf} ${name}
get_pci_config_cache_line ${pci_bdf} ${name}
echo ${pci_interrupt_line} >> ${h_name}
echo ${pci_interrupt_pin} >> ${h_name}
echo ${pci_cache_line} >> ${h_name}
done
echo -e ${h_tail} >> ${h_name}
return 0
}

__main__

