/*
 * Enable e1000e MSI function
 * NUC e1000e MAC is 82574IT ethernet controller
 * NET framework is  82574IT MAC + I219_LM phy
 * chunlin.wang
 */

#ifndef __E1000E_H__
#define __E1000E_H__

#include "libcflat.h"
#include "pci_util.h"

typedef int (*int_func)(void *p_arg);

#define MAKE_NET_MSI_MESSAGE_ADDR(base, apic_id)	((base) | SHIFT_LEFT((apic_id), 12))
#define MAKE_NET_MSI_MESSAGE_DATA(vector)	(SHIFT_LEFT(0, 8) | (vector))

/* WARNing:
 * Must call msi_int_connect before e1000e_init
 * Max number that connect msi interrupt functon is 4
 * Not support int disable connect
 */
extern int msi_int_connect(int_func int_ck, void *p_arg);
extern int e1000e_init(union pci_bdf bdf);
extern void msi_int_simulate(void);
extern int delay_loop_ms(uint32_t ms);
extern uint64_t msi_int_cnt_get(void);

extern int msi_int_demo(uint32_t msi_trigger_num);
extern void e1000e_reset(void);
extern void e1000e_msi_config(uint64_t msi_msg_addr, uint32_t msi_msg_data);
extern void e1000e_msi_enable(void);
extern void e1000e_msi_disable(void);
extern void e1000e_msi_reset(void);

#endif
