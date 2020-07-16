/*
 * Test for unittest
 *
 * Copyright (c) intel, 2020
 *
 * Authors:
 *  wenwumax <wenwux.ma@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 *
 */

#ifndef HOST_REGISTER_H
#define HOST_REGISTER_H


/*
 * This file contains the initial values of the registers and MSRs when the host start-up,
 * which can be used to compare with the corresponding registers or MSRs values when the guest init.
 *
 */

#define HOST_IA32_SPEC_CTRL_VAL		0x0
#define HOST_PLATFORM_INFO_VAL		0x804043DF1011500

#endif
