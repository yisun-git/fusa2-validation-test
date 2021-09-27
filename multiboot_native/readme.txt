This code verifies that multiboot is in native case:

The completion code is in this package (acrn-hypervisor_multiboot_native.tar.gz)

This code is modified on the Fusa branch of acrn hypervisor(https://gitlab.devtools.intel.com/projectacrn/acrn-static-toolbox/acrn-sliced-mainline.git commit b502d88be8ca65896e6c290559f06b2e261b60ea)

It can also be obtained by typing the following patch on the commit ID b502d88be8ca65896e6c290559f06b2e261b60ea of acrn hypervisor
 
multiboot_native.tar.gz (The compressed package of patch and checksum image files)

checksum (checksum executable file)
checksum.c (cehcksum source file)
multiboo_native.patch (patch)
multiboot_64.raw (safety kernel image file)
multiboot.bzimage (non-safety kernel image file)

Test method:
Use multiboot.bzimage, multiboot_64.raw, acrn.32.out (acrn.32.out compiled by acrn hypervisor) for testing, and refer to unit test method for startup method

