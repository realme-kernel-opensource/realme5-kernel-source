/*************************************************************************
 * ** Copyright (C), 2008-2016, OPPO Mobile Comm Corp., Ltd.
 * ** ODM_WT_EDIT
 * ** File: - wt_system_main.c
 * ** Description: Add for bootloader log
 * ** Version: 1.0
 * ** Date : 2019/04/20
 * ** Author: Hui.Wang@ODM_WT.BSP.Kernel.Stability
 * **
 * ** ------------------- Revision History: -------------------------------
 * **    <author>       <data>         <version >      <desc>
 * **    Hui.Wang       2019/04/20     1.0             build this module
 * **********************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <wt_sys/wt_boot_reason.h>
#include <wt_sys/wt_bootloader_log_save.h>

static int __init system_monitor_init(void)
{
	int ret;
#ifdef CONFIG_WT_BOOTLOADER_LOG
	ret = wt_bootloader_log_init();
	if (ret) {
		printk(KERN_ERR"wt_bootloader_log_init error!\n");
		return -1;
	}
#endif
#ifdef CONFIG_WT_BOOT_REASON
	ret = wt_boot_reason_init();
	if (ret) {
		printk(KERN_ERR"wt_boot_reason_init error!\n");
		return -1;
	}
#endif
#ifndef WT_FINAL_RELEASE
/*
	ret = wt_bootloader_log_handle();
	if (ret) {
		printk(KERN_ERR"wt_bootloader_log_handle error!\n");
		return -1;
	}
*/
#endif
	return ret;
}

static void __exit system_monitor_exit(void)
{
#ifdef CONFIG_WT_BOOTLOADER_LOG
	wt_bootloader_log_exit();
#endif
#ifdef CONFIG_WT_BOOT_REASON
	wt_boot_reason_exit();
#endif
}

module_init(system_monitor_init);
module_exit(system_monitor_exit);
MODULE_LICENSE("GPL");
