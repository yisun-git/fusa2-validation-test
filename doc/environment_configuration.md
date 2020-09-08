# Fusa Environment Configuration
This tutorial describes how to run Zephyr and Clear Linux as the User VM on the ACRN hypervisor. We are using Kaby Lake-based NUC (model NUC7i7DNH) in this tutorial. 

###  Prerequisites
- Use the Intel NUC Kit NUC7i7DNH.
- Connect to the serial port to your host.
- Verify that you have already compiled such files: acrn.32.out, *.raw, *.bzimage.

### Update GRUB
Perform the following to update GRUB so it can boot the hypervisor and load the kernel image:
1. Append the following configuration in the /etc/default/grub  file:

```
# Uncomment to disable graphical terminal (grub-pc only)

GRUB_TERMINAL=console

GRUB_CMDLINE_LINUX="console=tty0 console=ttyS0,115200n8"

GRUB_TERMINAL=serial

GRUB_SERIAL_COMMAND="serial --speed=115200 --unit=0 --word=8 --parity=no --stop==1"

```
2. Modify /etc/grub.d/40_custom, add:

```
    menuentry 'ACRN FuSa LOGICAL 2VMs' --class ubuntu --class gnu-linux --class gnu --class os $menuentry_id_option 'gnulinux-simple-e23c76ae-b06d-4a6e-ad42-46b8eedfd7d3' {

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
```
> **Note:** The module /boot/xxx.raw is the VM0 (Zephyr) kernel file; The module /boot/xxx.bzimage is the Service VM kernel file.

3. Update GRUB:

```
$ sudo update-grub
```
4. Modify /boot/grub/grub.cfg:

```
Comment out this line: set timeout_style=hidden
```
5. Reboot the NUC. Select the **ACRN FuSa LOGICAL 2VMs** entry to boot the ACRN hypervisor on the NUC’s display. The GRUB loader will boot the hypervisor, and the hypervisor will start the VMs automatically

```
+----------------------------------------------------------------------------+
 | Ubuntu                                                                     |
 | Advanced options for Ubuntu                                                |
 | System setup                                                               |
 |*ACRN FuSa LOGICAL 2VMs                                                     |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 |                                                                            |
 +----------------------------------------------------------------------------+
```



### Zephry and Clear Linux startup checking
1. Use these steps to verify that the hypervisor is properly running:
- Log in to the ACRN hypervisor shell from the serial console.
- Use the vm_list command to verify that the vm0 and Service VM are launched successfully.

```
root@mahaixin-Z97X-UD5H:~# picocom -b 115200 /dev/ttyUSB0
picocom v1.7

port is        : /dev/ttyUSB0
flowcontrol    : none
baudrate is    : 115200
...
Terminal ready

ACRN:\>
ACRN:\>vm_list

VM_UUID                          VM_ID VM_NAME                          VM_STATE
================================ ===== ================================ ========
00000000000000000000000000000000   0   ACRN PRE-LAUNCHED VM0            Started
00000000000000000000000000000000   1   ACRN PRE-LAUNCHED VM1            Started

```

2. Use these steps to verify all VMs are running properly:
- Use the vm_console 0 to switch to VM0 (Zephyr) console. It will display Zephyrs' case log.
- Enter Ctrl+Spacebar to return to the ACRN hypervisor shell.
- Use the vm_console 1 command to switch to the VM1 (Clear Linux) console.
- Verify that the case's log in Clear Linux dump out.

```
ACRN:\>vm_console 0

----- Entering VM 0 Shell -----
enabling apic
                 safety_analysis feature case list:
                 Case ID:33882 case name:mitigation-Spatial Interface_Memory isolation - DMA_001

...

ACRN:\>vm_console 1

----- Entering VM 1 Shell -----
enabling apic
enabling apic
enabling apic
                 safety_analysis feature case list:
                 Case ID:33882 case name:mitigation-Spatial Interface_Memory isolation - DMA_001

...
```





