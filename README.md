# virtualization.hypervisors.acrn.acrn-fusa-cert.validation-test

# Fusa SRS Test building steps

## 1. Compile SRS unit-test

```bash
$ git clone https://github.com/intel-innersource/virtualization.hypervisors.acrn.acrn-fusa-cert.validation-test.git acrn-validation-test
$ git clone https://github.com/projectacrn/acrn-unit-test.git -b acrn_2022ww35
$ cp -r acrn-validation-test/acrn-unit-test/* acrn-unit-test/
$ cd acrn-unit-test/guest/
$ ./make_all.sh

# Path: acrn-unit-test/guest/x86/obj/
# (There are three kinds of files: *.bzimage for non-safety, *.raw for safety and *.elf for native)
```

## 2. Add grub cfg files for SRS unit test

Create these 3 grub cfg files and set up:
```bash
# In the files <xxx> are needed to be updated according to the local setup
$ touch 50_acrn-pre-raw
$ touch 50_acrn-pre-bzimage
$ touch 50_acrn-sos-bzimage
$ sudo cp 50_acrn-* /etc/grub.d/
$ sudo chmod +x /etc/grub.d/50_acrn-*
```

50_acrn-pre-raw
```sh
#!/bin/sh
exec tail -n +3 $0
menuentry 'ACRN SRS Pre-launched VM - raw' {
    load_video
    insmod gzio
    insmod part_gpt
    insmod ext2
    search --no-floppy --fs-uuid  --set=root <partition_UUID>

    echo 'Loading hypervisor srs module ...'
    multiboot --quirk-modules-after-kernel /boot/acrn.<config_name>.<board_name>.32.out root=UUID=<partition UUID> cpu_perf_policy=Performance
    module /boot/<target_test>.raw RawImage
    module /boot/ACPI_VM0.bin ACPI_VM0
}
```

50_acrn-pre-bzimage
```sh
#!/bin/sh
exec tail -n +3 $0
menuentry 'ACRN SRS Pre-launched VM - bzimage' {
    load_video
    insmod gzio
    insmod part_gpt
    insmod ext2
    search --no-floppy --fs-uuid  --set=root <partition_UUID>

    echo 'Loading hypervisor srs module ...'
    multiboot --quirk-modules-after-kernel /boot/acrn.<config_name>.<board_name>.32.out root=UUID=<partition UUID> cpu_perf_policy=Performance
    module /boot/<target_test>.bzimage Linux_bzImage
    module /boot/ACPI_VM0.bin ACPI_VM0
}
```

50_acrn-sos-bzimage
```sh
#!/bin/sh
exec tail -n +3 $0
menuentry 'ACRN SRS Service VM - bzimage' {
    load_video
    insmod gzio
    insmod part_gpt
    insmod ext2
    search --no-floppy --fs-uuid  --set=root <partition_UUID>

    echo 'Loading hypervisor srs module ...'
    multiboot --quirk-modules-after-kernel /boot/acrn.<config_name>.<board_name>.32.out root=UUID=<partition_UUID> cpu_perf_policy=Performance
    module /boot/<target_test>.bzimage Linux_bzImage
}
```

## 4. Get and compile ACRN hypervisor

```bash
$ git clone https://github.com/projectacrn/acrn-hypervisor.git

# Please generate board.xml and scenario.xml files as usual.
# Notes: VM scenario config fragments:
# 1. Pre-launched VM - raw
#    <os_config>
#      <kern_type>KERNEL_RAWIMAGE</kern_type>
#      <kern_mod>RawImage</kern_mod>
#      <bootargs></bootargs>
#      <kern_load_addr>0x100000</kern_load_addr>
#      <kern_entry_addr>0x100000</kern_entry_addr>
#    </os_config>
# 2. Pre-launched VM - bzimage and Service VM - bzimage
#    <os_config>
#      <kern_type>KERNEL_BZIMAGE</kern_type>
#      <kern_mod>Linux_bzImage</kern_mod>
#      <bootargs></bootargs>
#    </os_config>

$ cd acrn-hypervisor
$ make BOARD=<board.xml> SCENARIO=<scenario.xml>
$ sudo apt install --reinstall ./build/acrn-board-*.deb
```

## 5. Run SRS unit test

Reboot machine, at grub menu, choose the target boot entry of the three:
- "ACRN SRS Pre-launched VM - raw"
- "ACRN SRS Pre-launched VM - bzimage"
- "ACRN SRS Service VM - bzimage"

# [TO BE UPDATED] Run in QEMU

    Compile hypervisor (see ### 4). Add compile option QEMU=1.
    e.g. 
    $make RELEASE=0 QEMU=1
    Compile unit-test (see ### 5).

    1) Run acrn-unit-test on hypervisor
    Under path: <path of acrn-validation-test>/github/acrn-unit-test/guest/, 
    $ ./qemu.sh <safety_raw_file> <non_safety_bzimage_file>
    e.g.
    $ ./qemu.sh xsave_64 xsave

    2) Run acrn-unit-test on native
    Under path: <path of acrn-validation-test>/github/acrn-unit-test/guest/,
    $ ./qemu_native.sh <native_elf_file>
    e.g.
    $ ./qemu_native.sh xsave_native_64
