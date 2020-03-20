
This file is to give information about coding style scans tools which are implemented on this repo.

=====================================================

Coding Rules for FuSa test code:  https://wiki.ith.intel.com/display/OTCCWPQA/Coding+Rules+for+FuSa+SRS+test+code


--------------------------------
1. checkpatch.pl tool
--------------------------------

### 15 coding rules are covered:
* C-CS-01
* C-CS-03
* C-CS-05
* C-CS-07
* C-CS-08
* C-CS-09
* C-CS-11
* C-CS-13
* C-CS-14
* C-CS-15
* C-CS-17
* C-CS-18
* ASM-CS-02
* ASM-CS-03
* ASM-CS-07

### Usage
$ cd acrn-unit-test/guest/scripts

If to scan by folder:
$ ./checkfolder <folder_name>

If to scan by sigle file:
$ ./checkpatch.pl -f <file_name>

If you want to fix a file automatically, use --fix.


--------------------------------
2. acrn-clang-tidy tool
--------------------------------

### 4 coding rules are covered:
* C-ST-02
* C-EP-05
* C-FN-05
* C-FN-06

### Build && Install
Install LLVM/Clang 9.0. Refer to https://apt.llvm.org/ on how to install LLVM/Clang 9.0 via apt. 
The README.txt at the top of acrn-static-tool has more details on the configuration of LLVM/Clang 9.0 after installation.
Recommended command:
$ bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"
$ apt-get install clang-9 clang-tools-9 clang-9-doc libclang-common-9-dev libclang-9-dev libclang1-9 clang-format-9 python-clang-9 clangd-9

Clone acrn-clang-tidy from https://gitlab.devtools.intel.com/projectacrn/acrn-static-toolbox/acrn-clang-tidy.

Build and install the tool using following commands:
$ mkdir build && cd build
$ cmake ..
$ make
$ sudo make install

### Usage
Under acrn-validation-test, 
$ mkdir github && cd github && git clone https://github.com/projectacrn/acrn-unit-test.git
$ cp -r ../acrn-unit-test/* acrn-unit-test/
$ cd acrn-unit-test/guest/
$ make codescan


