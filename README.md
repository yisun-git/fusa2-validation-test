### Readme

### Fusa SRS Test building step.

#### 1.setup your local building environment firstly. 

    setup docker environment, pls refer http://sit-image.sh.intel.com/FUSA/docker_20200421/ReadMe.txt

#### 2.get SRS code 

    $git clone https://gitlab.devtools.intel.com/projectacrn/acrn-static-toolbox/acrn-validation-test.git  # branch: master

#### 3.get Hypervisor code

    $git clone https://gitlab.devtools.intel.com/projectacrn/acrn-static-toolbox/acrn-sliced-mainline.git  # branch: master

#### 4.compile hypervisor

    $cd acrn-sliced-mainline/hypervisor
    $make clean
    $make RELEASE=0

    path: acrn-sliced-mainline/hypervisor/build/acrn.32.out

#### 5.compile unit-test

    $docker run -v -it clearlinux_acrn:latest /bin/bash
    $cd acrn-validation-test
    $git reset --hard && git checkout master && git pull
    $mkdir github && cd github && git clone https://github.com/projectacrn/acrn-unit-test.git
    $cp -r ../acrn-unit-test/* acrn-unit-test/
    $cd acrn-unit-test/guest/
    $./make_all.sh

    path: acrn-validation-test/github/acrn-unit-test/guest/x86/obj/
    (There are three kinds of files: *.bzimage for non-safety, *.raw for safety and *.elf for native)

### 6.BTW, you need to append "ACRN_unit_test_image" after the unit test module for the unit-test to boot.
```
    40_custom_logic_partition_2_vm:

    menuentry 'ACRN SRS LOGICAL 2VMs' --class ubuntu --class gnu-linux --class gnu --class os $menuentry_id_option 'gnulinux-simple-e23c76ae-b06d-4a6e-ad42-46b8eedfd7d3' {
    recordfail
    load_video
    gfxmode $linux_gfx_mode
    insmod gzio
    insmod part_gpt
    insmod ext2

    echo 'Loading hypervisor srs module ...'
    multiboot --quirk-modules-after-kernel /boot/acrn.32.out
    module /boot/xxx.raw zephyr
    module /boot/xxx.bzimage linux
 }

    40_custom_native_elf:

    menuentry 'UNIT-TEST NATIVE ELF' --class ubuntu --class gnu-linux --class gnu --class os $menuentry_id_option 'gnulinux-simple-e23c76ae-b06d-4a6e-ad42-46b8eedfd7d3' {
    recordfail
    load_video
    gfxmode $linux_gfx_mode
    insmod gzio
    insmod part_gpt
    insmod ext2

    echo 'Loading native unit test ...'
    multiboot /boot/xxx.elf
 }
 ```