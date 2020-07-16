#ifndef __REGISTER_H
#define __REGISTER_H

int write_cr3_checking(unsigned long val);
int write_cr4_checking(unsigned long val);
int wrmsr_checking(u32 MSR_ADDR, u64 value);
int rdmsr_checking(u32 MSR_ADDR, u64 *result);
int write_cr4_osxsave(u32 bitvalue);
int xsetbv_checking(u32 index, u64 value);
int xgetbv_checking(u32 index, u64 *result);

#endif
