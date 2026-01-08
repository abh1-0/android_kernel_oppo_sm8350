#ifndef __KSU_H_SUS_SU
#define __KSU_H_SUS_SU

// #include "../../drivers/kernelsu/core_hook.h"
#include "../../KernelSU/kernel/sucompat.h"

void ksu_susfs_enable_sus_su(void);
void ksu_susfs_disable_sus_su(void);
bool susfs_is_allow_su(void);
void escape_to_root(void);

int sus_su_fifo_init(int *maj_dev_num, char *drv_path);
int sus_su_fifo_exit(int *maj_dev_num, char *drv_path);

#endif
