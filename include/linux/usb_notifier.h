/*******************************************************************************
** Copyright (C), 2008-2016, OPPO Mobile Comm Corp., Ltd.
** ODM_WT_EDIT
** FILE: - usb_notifier.h
** Description : This program is for usb notifier
** Version: 1.0
** Date : 2018/11/27
** Author: Zhonghua Hu@ODM_WT.BSP.TP-FP.TP
**
** -------------------------Revision History:----------------------------------
**  <author>	 <data> 	<version >			<desc>
**
**
*******************************************************************************/
 
#include <linux/notifier.h>
#include <linux/export.h>




extern struct atomic_notifier_head usb_notifier_list;

extern int usb_register_client(struct notifier_block *nb);
extern int usb_unregister_client(struct notifier_block *nb);
extern int usb_notifier_call_chain(unsigned long val, void *v);

