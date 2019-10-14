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

#include <linux/tpd_summer.h>
#include <linux/gpio.h>

int lcd_id_action(int gpio){
	
	int value;
	if (gpio_is_valid(gpio)){
		value = gpio_get_value(gpio);
		pr_info("THE VALUE OF LCD ID PIN input is %d\n", value);
		if(value == 1){
			gpio_direction_output(gpio, 1);
		}
		gpio_direction_output(gpio, 0);
		value = gpio_get_value(gpio);
		pr_info("GPIO LCD lcd_id output : %d\n", value);
		return value;
	}else{
		pr_err("Invalid lcd_id0 gpio: %d\n", gpio);
		return -1;
	}
} 

void tpd_summer_lcd_id(struct device_node *dev_node){
	int lcd_id0;
	int lcd_id1;
	int lcd_id2;
	uint32_t flag;
	
	lcd_id0 = of_get_named_gpio_flags(dev_node, DTS_LCD_ID0, 0, &flag);
	lcd_id1 = of_get_named_gpio_flags(dev_node, DTS_LCD_ID1, 0, &flag);
	lcd_id2 = of_get_named_gpio_flags(dev_node, DTS_LCD_ID2, 0, &flag);
	
	pr_info("GPIO LCD lcd_id0: %d\n", lcd_id0);
	pr_info("GPIO LCD lcd_id1: %d\n", lcd_id1);
	pr_info("GPIO LCD lcd_id2: %d\n", lcd_id2);
	
	pr_info("the result of lcd_id_action is %d\n",lcd_id_action(lcd_id0));
	pr_info("the result of lcd_id_action is %d\n",lcd_id_action(lcd_id1));
	pr_info("the result of lcd_id_action is %d\n",lcd_id_action(lcd_id2));
}
//EXPORT_SYMBOL(tpd_summer_lcd_id);