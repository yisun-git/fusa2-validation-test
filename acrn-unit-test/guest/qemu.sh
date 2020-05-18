#!/usr/bin/env bash

killall qemu-system-x86_64

./grub_iso acrn-unit_test.iso ../../../../acrn-hypervisor/hypervisor/build/acrn.32.out ./x86/obj/$1.raw ./x86/obj/$2.bzimage

qemu-system-x86_64 -machine q35,kernel_irqchip=split,accel=kvm -cpu max,level=22,invtsc -m 4G -smp cpus=8,cores=4,threads=2 -enable-kvm -device isa-debug-exit -device intel-iommu,intremap=on,aw-bits=48,caching-mode=on,device-iotlb=on -debugcon file:/dev/stdout -serial mon:stdio -display none  -boot d -cdrom ./acrn-unit_test.iso || true


