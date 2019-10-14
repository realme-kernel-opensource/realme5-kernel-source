/*******************************************************************************
** Copyright (C), 2008-2016, OPPO Mobile Comm Corp., Ltd.
** ODM_WT_EDIT
** FILE: - tpd_summer.c
** Description : This program is for touch common function
** Version: 1.0
** Date : 2019/01/11
** Author: Bin Su@ODM_WT.BSP.TP-FP.TP
**
** -------------------------Revision History:----------------------------------
**  <author>	 <data> 	<version >			<desc>
**
**
*******************************************************************************/
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>

#ifdef CONFIG_OF
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

#define DTS_LCD_ID0    "touchpanel,id0-gpio"
#define DTS_LCD_ID1    "touchpanel,id1-gpio"
#define DTS_LCD_ID2    "touchpanel,id2-gpio"

extern void tpd_summer_lcd_id(struct device_node *dev_node);
