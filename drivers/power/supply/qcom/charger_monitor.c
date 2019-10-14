/*******************************************************************************
 *  Copyright (C), 2008-2018, OPPO Mobile Comm Corp., Ltd.
 *  ODM_WT_EDIT
 *  FILE: - charger_monitor.c
 *  Description : Add charger monitor function
 *  Version: 1.0
 *  Date : 2018/6/20
 *  Author: Bin2.Zhang@ODM_WT.BSP.Charger.Basic
 *
 *  -------------------------Revision History:----------------------------------
 *   <author>	 <data> 	<version >			<desc>
 *  Bin2.Zhang	2018/6/20	1.0				Add charger monitor function
 ******************************************************************************/
#ifdef ODM_WT_EDIT
/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1372106, 20180620, Add for charger monitor */
//#define DEBUG
#define pr_fmt(fmt) "CHG-CHK: " fmt

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/iio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/qpnp/qpnp-revid.h>
#include "smb5-reg.h"
#include "smb5-lib.h"
#include "storm-watch.h"
#include <linux/pmic-voter.h>

/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1372106, 20180622, Add for proc node */
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1372106, 20180615, Add lcd on-off control usb icl */
#include <linux/notifier.h>
#include <linux/fb.h>
#include <linux/msm_drm_notify.h>
/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190521, Add for kernel shutdown when high temperature */
#include <linux/kthread.h>
#include <linux/reboot.h>
#include <uapi/linux/sched/types.h>

/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1372106, 20180610, Add for charger notify code */
#include <linux/ktime.h>
#define MAX_CHARGER_TIMEOUT (10*3600) // 10H

#define CHARGER_MONITOR_DELAY_MS (5000)

#define MAX_USB_ICL_CURRENT (2000000)
#define BATTERY_ITERM_CURRENT (260)
#define FULL_COUNTS_BY_CURRENT (5)
#define FULL_COUNTS_BY_VOLTAGE (4)

#define USB_ICL_LCD_ON_ABOVE_350 (900000)
#define USB_ICL_LCD_ON  (1200000)
#define USB_ICL_LCD_OFF (MAX_USB_ICL_CURRENT)

#define TEMP_SKIN_LEVEL_2 (390)
#define TEMP_SKIN_LEVEL_1 (370)
#define TEMP_SKIN_LEVEL_0 (-400)
#define USB_ICL_SKIN_LEVEL_2 (1200000)
#define USB_ICL_SKIN_LEVEL_1 (1500000)
#define USB_ICL_SKIN_LEVEL_0 (MAX_USB_ICL_CURRENT)

#define TEMP_CPU_LEVEL_2 (460)
#define TEMP_CPU_LEVEL_1 (430)
#define TEMP_CPU_LEVEL_0 (-400)
#define USB_ICL_CPU_LEVEL_2 (1200000)
#define USB_ICL_CPU_LEVEL_1 (1500000)
#define USB_ICL_CPU_LEVEL_0 (MAX_USB_ICL_CURRENT)

#define BAT_VOL_HIGH_PARAMS (4550)

#define CHG_VOL_HIGH_PARAMS (5800)
#define CHG_VOL_LOW_PARAMS (4300)

#define BAT_MIN_FV	(3600000)

typedef enum {
    CRITICAL_LOG_NORMAL = 0,
    CRITICAL_LOG_UNABLE_CHARGING,
    CRITICAL_LOG_BATTTEMP_ABNORMAL,
    CRITICAL_LOG_VCHG_ABNORMAL,
    CRITICAL_LOG_VBAT_TOO_HIGH,
    CRITICAL_LOG_CHARGING_OVER_TIME,
} OPPO_CHG_CRITICAL_LOG;

enum battery_status_index {
    BATTERY_STATUS__REMOVED = 0,                     /* < -19 */
    BATTERY_STATUS__LOW_TEMP,                        /* -19 ~ -2C  */
    BATTERY_STATUS__COLD_TEMP,                       /* -2 ~ 0 */
    BATTERY_STATUS__LITTLE_COLD_TEMP,                /* 0 ~ 5 */
    BATTERY_STATUS__COOL_TEMP,                       /* 5 ~ 12 */
    BATTERY_STATUS__LITTLE_COOL_TEMP,                /* 12 ~ 22 */
    BATTERY_STATUS__NORMAL,                          /* 22 ~ 45 */
    BATTERY_STATUS__WARM_TEMP,                       /* 45 ~ 53 */
    BATTERY_STATUS__HIGH_TEMP,                       /* > 53 */
    BATTERY_STATUS__INVALID
};

enum notify_code {
    CHG_VOL_LOW                 = 0,
    CHG_VOL_HIGH                = 1,
    REVERSE_0                   = 2,
    BAT_TEMP_HIGH               = 3,
    BAT_TEMP_LOW                = 4,
    BAT_TEMP_DISCONNECT         = 5,
    BAT_VOL_HIGH                = 6,
    CHARGER_FULL                = 7,
    REVERSE_1                   = 8,
    CHARGER_OVERTIME            = 9,
    CHARGER_FULL_HIGHTEMP       = 10,
    CHARGER_FULL_LOWTEMP2       = 11,
    REVERSE_2                   = 14,
    CHARGER_UNKNOW              = 20,
    CHARGER_LOW_BATTERY         = 21,
    CHARGER_LOW_BATTERY2        = 22,
    CHARGER_LOW_BATTERY3        = 23,
    CHARGER_POC                 = 24,
    LOW_BATTERY                 = 26,
    MAX_NOTIFY_CODE             = 31,
};

struct smb_step_chg {
	int low_threshold;
	int bound_temp;
	//int hyst;

	int fcc;
	int fv;
	int rechr_v;
	int chg_fv;
	int max_fv;

	int health;
};

struct smb_chg_para {
	int init_first;

	int lcd_is_on;
	int lcd_on_chg_curr_proc;

	int batt_temp;
	int	batt_vol;
	int	batt_curr;
	int	batt_soc;
	int	batt_status;
	int	batt_healthd;
	int	batt_healthd_save;
	int	batt_present;
	int batt_auth;

	int full_cnt_c;
	int full_cnt_v;
	int batt_is_full;

	int sum_cnt_warm;
	int sum_cnt_good;
	int sum_cnt_cool;
	int sum_cnt_good_down_fv;
	int sum_cnt_up_fv;
	int batt_fv_up_status;

	int need_do_icl;
	int	chg_vol;
	int chg_vol_low;
	//int	chg_curr;
	int	chg_present;
	int chg_present_save;
	unsigned long chg_times;
	long chg_time_out;

	int need_stop_usb_chg;
	int need_stop_bat_chg;
	int need_output_log;

	int chg_ranges_now;
	int chg_ranges_save;
	int chg_ranges_len;
	//struct smb_step_chg *p_chg_ranges;
};
struct smb_chg_para g_charger_para;

struct smb_charger *g_chg = NULL;

static int charger_abnormal_log = CRITICAL_LOG_NORMAL;
//#define BAT_TEMP_DISCONNECT_PARAMS (-190)
#define BAT_TEMP_HYST (20)
#define BAT_FULL_FV_ABOVE (-5) //mV
struct smb_step_chg g_chg_ranges[] = {
									   /* low        fcc, fv,rechr_v,chg_fv,max_fv,               health */
	[         BATTERY_STATUS__REMOVED] = {-3000, -3000,      0, 3980, 3700, 3990, 4000, POWER_SUPPLY_HEALTH_DEAD},
	[        BATTERY_STATUS__LOW_TEMP] = { -190,  -190,       0, 3980, 3700, 3990, 4000, POWER_SUPPLY_HEALTH_COLD},
	[       BATTERY_STATUS__COLD_TEMP] = {  -20,   -20,  350000, 3980, 3700, 3990, 4000, POWER_SUPPLY_HEALTH_COOL},
	[BATTERY_STATUS__LITTLE_COLD_TEMP] = {    0,     0,  650000, 4430, 4200, 4435, 4440, POWER_SUPPLY_HEALTH_GOOD},
	[       BATTERY_STATUS__COOL_TEMP] = {   50,    50, 1150000, 4430, 4320, 4435, 4440, POWER_SUPPLY_HEALTH_GOOD},
	[BATTERY_STATUS__LITTLE_COOL_TEMP] = {  120,   120, 2000000, 4430, 4320, 4435, 4440, POWER_SUPPLY_HEALTH_GOOD},
	[          BATTERY_STATUS__NORMAL] = {  220,   220, 2000000, 4430, 4320, 4435, 4440, POWER_SUPPLY_HEALTH_GOOD},
	[       BATTERY_STATUS__WARM_TEMP] = {  450,   450, 1350000, 4130, 3980, 4150, 4180, POWER_SUPPLY_HEALTH_WARM},
	[       BATTERY_STATUS__HIGH_TEMP] = {  530,   530,       0, 4130, 3980, 4150, 4180, POWER_SUPPLY_HEALTH_OVERHEAT},
};

//#define BAT_TEMP_TOO_HOT_PARAMS (g_chg_ranges[g_charger_para.chg_ranges_len - 1].high_threshold)
//#define BAT_TEMP_TOO_COLD_PARAMS (g_chg_ranges[0].low_threshold)

static bool charger_monitor_work_init = false;
static int charger_need_reset = 0;

void reset_battery_voltage_current(struct smb_charger *chg);
void update_chg_monitor_statu(struct smb_charger *chg);

#define USBIN_AICL_EN_BIT	BIT(2)
#define USBIN_25MA			(25000)
#define USB_ICL_POINT_HIGH		(4550)
#define USB_ICL_POINT_LOW		(4500)
/* Add of 1200mA back, avoid can't charge with 1200mA when lcd was on. 0719*/
int usb_icl_step[] = {500, 900, 1200, 1500, 1750, 2000}; // Remove 1200mA, avoid 1000mA dcp charged with 1200mA current
/* Must be sure to call this funtion,it will cause charger current to be a low level !!! */
/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1372106, 20180625, Add for do ICL */
int do_charger_icl(struct smb_charger *chg)
{
	int usb_current_now = 0;
	int current_temp = 0;
	int usb_icl_step_lens = sizeof(usb_icl_step) / sizeof(int);
	int i = 0;
	int rc = 0;
	int usb_icl_point = 0;
	union power_supply_propval val = {0, };
	int save_icl = g_charger_para.need_do_icl;

	g_charger_para.need_do_icl = 0;
	if ((g_charger_para.chg_present != 1) || (chg->real_charger_type == POWER_SUPPLY_TYPE_USB) || (chg->real_charger_type == POWER_SUPPLY_TYPE_USB_CDP)
			|| (chg->real_charger_type == POWER_SUPPLY_TYPE_UNKNOWN) || (g_charger_para.batt_status == POWER_SUPPLY_STATUS_FULL)
			|| (get_client_vote(chg->usb_icl_votable, CHG_CHK_VOTER) == 0)
			|| (get_effective_result(chg->chg_disable_votable) == true)) {
		pr_debug("USB don't need do charger ICL.\n");
		return 0;
	}

	power_supply_get_property(chg->batt_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
	if (val.intval / 1000 < 4100) {
		usb_icl_point = USB_ICL_POINT_LOW;
	} else {
		usb_icl_point = USB_ICL_POINT_HIGH;
	}

	/* clean last setting first */
	vote(chg->usb_icl_votable, CHG_CHK_VOTER, false, 0);
	//usb_current_now = get_effective_result(chg->usb_icl_votable);
	smblib_get_charge_param(chg, &chg->param.usb_icl, &usb_current_now);

	/* Disable AICL first */
	rc = smblib_masked_write(chg, USBIN_AICL_OPTIONS_CFG_REG, USBIN_AICL_EN_BIT, 0);
	if (rc < 0)
		pr_info("Clear USBIN_AICL_OPTIONS_CFG_REG error!");

	for (i = 0; i < usb_icl_step_lens; i++) {
		if (i == 0) {
			if (usb_current_now < usb_icl_step[i] * 1000) {
				break;
			}
		} else if (usb_current_now < usb_icl_step[i - 1] * 1000) {
			i--;
			break;
		}
		//vote(chg->usb_icl_votable, CHG_CHK_VOTER, true, usb_icl_step[i] * 1000);
		smblib_set_charge_param(chg, &chg->param.usb_icl, usb_icl_step[i] * 1000);
		msleep(90);

		if ((get_client_vote(chg->usb_icl_votable, BOOST_BACK_VOTER) == 0)
				&& (get_effective_result(chg->usb_icl_votable) < USBIN_25MA)) {
			i--;
			break;
		}

		power_supply_get_property(chg->usb_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
		if (val.intval / 1000 < usb_icl_point) {
			i--;
			break;
		}
	}
	i = (i > 0 ? i : 0);
	current_temp = get_effective_result(chg->usb_icl_votable);
	smblib_set_charge_param(chg, &chg->param.usb_icl, current_temp);
	if (usb_icl_step_lens == i) {
		i = usb_icl_step_lens - 1;
	}
	pr_info("USB 0x%x do charger ICL %d->(Temp:%d)->%d,USB type:%d, Vbus:%d\n", save_icl, usb_current_now / 1000, current_temp / 1000, usb_icl_step[i], chg->real_charger_type, val.intval);
	if (usb_icl_step[i] == 1200) {
		i--;
	}
	g_charger_para.need_output_log = 1;
	vote(chg->usb_icl_votable, CHG_CHK_VOTER, true, usb_icl_step[i] * 1000);
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1519424, 20180810, Add for Vbus recovery */
	msleep(120);

	/* Enable AICL */
	rc = smblib_masked_write(chg, USBIN_AICL_OPTIONS_CFG_REG, USBIN_AICL_EN_BIT, USBIN_AICL_EN_BIT);
	if (rc < 0) {
		pr_info("Set USBIN_AICL_OPTIONS_CFG_REG error!");
	}

	return 0;
}

int get_battery_status_modify(struct smb_charger *chg)
{
	int batt_healthd = g_chg_ranges[g_charger_para.chg_ranges_now].health;
	union power_supply_propval val = {0, };
	//int rc = 0;

	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.2055056, 20190530, Add for Vbus recovery */
	/* Need get real batt_status before update battery status for UI. */
	smblib_get_prop_batt_status(chg, &val);
	g_charger_para.batt_status = val.intval;
	if (charger_monitor_work_init == false) {
		return val.intval;
	}

	/* Hold full after had already charged to full! Change to NOT_CHARGING when plug-out. */
	if (/*(chg->prop_status == POWER_SUPPLY_STATUS_FULL) && */(batt_healthd == g_charger_para.batt_healthd_save)
				&& (g_charger_para.chg_present == 1) && (g_charger_para.batt_is_full == 1)) {
		if ((g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_GOOD) && (chg->ui_soc < 100)) {
			return POWER_SUPPLY_STATUS_CHARGING;
		} else if ((g_charger_para.batt_status == POWER_SUPPLY_STATUS_NOT_CHARGING)
				&& (get_any_voter_value_without_client(chg->chg_disable_votable, CHG_FULL_VOTER) == false)
				&& (is_client_vote_enabled(chg->chg_disable_votable, CHG_FULL_VOTER) == true)) {
			return POWER_SUPPLY_STATUS_FULL;
		} else if ((g_charger_para.batt_status == POWER_SUPPLY_STATUS_CHARGING)
				|| (g_charger_para.batt_status == POWER_SUPPLY_STATUS_FULL)) {
			return POWER_SUPPLY_STATUS_FULL;
		} else if ((chg->prop_status == POWER_SUPPLY_STATUS_FULL)
				&& (get_any_voter_value_without_client(chg->chg_disable_votable, CHG_FULL_VOTER) == false)) {
			return POWER_SUPPLY_STATUS_FULL;
		}
		pr_info("Status error, health:%d, prop_status:%d, batt_status:%d\n", g_charger_para.batt_healthd,
				chg->prop_status, g_charger_para.batt_status);
	}
	chg->prop_status = g_charger_para.batt_status;

	if ((g_charger_para.chg_present == 1) && (g_charger_para.batt_status == POWER_SUPPLY_STATUS_FULL)) {
		if (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_GOOD) {
			if (chg->ui_soc >= 100) {
				chg->prop_status = POWER_SUPPLY_STATUS_FULL;
				g_charger_para.batt_is_full = 1;
			} else {
				chg->prop_status = POWER_SUPPLY_STATUS_CHARGING;
			}
		} else {
			chg->prop_status = POWER_SUPPLY_STATUS_FULL;
			g_charger_para.batt_is_full = 1;
		}
	}

	if (g_charger_para.batt_status == POWER_SUPPLY_STATUS_DISCHARGING) {
		chg->prop_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		g_charger_para.batt_is_full = 0;
	}

	return chg->prop_status;
}

static void soc_reduce_slow_when_1(struct smb_charger *chg)
{
	static int reduce_count = 0;
	int reduce_count_limit = 0;

	if (chg->ui_soc >= 2) {
		reduce_count = 0;
		return ;
	}

	if (g_charger_para.chg_present == 1) {
		reduce_count_limit = 12;
	} else {
		reduce_count_limit = 4;
	}
	if (g_charger_para.batt_vol < 3410) {
		reduce_count++;
	} else if ((reduce_count > reduce_count_limit) && (chg->ui_soc == 0)) {
		reduce_count++;
	} else {
		reduce_count = 0;
	}

	g_charger_para.need_output_log = 1;
	if (reduce_count > reduce_count_limit) {
		reduce_count = reduce_count_limit + 1;
		chg->ui_soc = 0;
	} else {
		chg->ui_soc = 1;
	}
}

#define SOC_SYNC_DOWN_RATE_300S (60) // 300/5
#define SOC_SYNC_DOWN_RATE_150S (30) // 150/5
#define SOC_SYNC_DOWN_RATE_90S  (18) //  90/5
#define SOC_SYNC_DOWN_RATE_60S  (12) //  60/5
#define SOC_SYNC_DOWN_RATE_40S  ( 8) //  40/5
#define SOC_SYNC_DOWN_RATE_15S  ( 3) //  15/5

#define SOC_SYNC_UP_RATE_60S  (12) //  60/5
#define SOC_SYNC_UP_RATE_10S  ( 2) //  10/5

#define TEN_MINUTES (10 * 60)

static int soc_down_count = 0;
static int soc_up_count = 0;
int update_battery_ui_soc(struct smb_charger *chg)
{
	int soc_down_limit = 0;
	int soc_up_limit = 0;
	unsigned long soc_reduce_margin = 0;
	bool vbatt_too_low = false;
	union power_supply_propval val = {0, };

	if ((chg->ui_soc < 0) || (chg->ui_soc > 100)) {
		pr_info("Don't need sync ui_soc:%d, msoc:%d.\n", chg->ui_soc, g_charger_para.batt_soc);
		chg->ui_soc = g_charger_para.batt_soc;
		return -1;
	}

	if (chg->ui_soc == 100) {
		soc_down_limit = SOC_SYNC_DOWN_RATE_300S;
	} else if (chg->ui_soc >= 95) {
		soc_down_limit = SOC_SYNC_DOWN_RATE_150S;
	} else if (chg->ui_soc >= 60) {
		soc_down_limit = SOC_SYNC_DOWN_RATE_60S;
	} else if ((g_charger_para.chg_present == 1) && (chg->ui_soc == 1)) {
		soc_down_limit = SOC_SYNC_DOWN_RATE_90S;
	} else {
		soc_down_limit = SOC_SYNC_DOWN_RATE_40S;
	}
	if ((g_charger_para.batt_vol < 3300) && (g_charger_para.batt_vol > 2500)) {
		soc_down_limit = SOC_SYNC_DOWN_RATE_15S;
		vbatt_too_low = true;
		pr_info("batt_volt:%d, vbatt_too_low:%d\n", g_charger_para.batt_vol, vbatt_too_low);
	}

	if ((g_charger_para.batt_status == POWER_SUPPLY_STATUS_FULL) || (g_charger_para.batt_is_full == 1)) {
		soc_up_limit = SOC_SYNC_UP_RATE_60S;
	} else {
		soc_up_limit = SOC_SYNC_UP_RATE_10S;
	}

	if ((g_charger_para.chg_present == 1) && (g_charger_para.batt_status == POWER_SUPPLY_STATUS_FULL)) {
		chg->sleep_tm_sec = 0;
		if (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_GOOD) {
			soc_down_count = 0;
			soc_up_count++;
			if (soc_up_count >= soc_up_limit) {
				soc_up_count = 0;
				chg->ui_soc++;
				if (chg->ui_soc <= 100) {
					g_charger_para.need_output_log = 1;
					pr_debug("full ui_soc:%d soc:%d up_limit:%d\n", chg->ui_soc, g_charger_para.batt_soc, soc_up_limit * 5);
				}
			}
			if (chg->ui_soc >= 100) {
				chg->ui_soc = 100;
				chg->prop_status = POWER_SUPPLY_STATUS_FULL;
				vote(chg->chg_disable_votable, CHG_FULL_VOTER, true, 0);
				g_charger_para.batt_is_full = 1;
				val.intval = 100;
				power_supply_set_property(chg->bms_psy, POWER_SUPPLY_PROP_UI_SOC, &val);
			} else {
				g_charger_para.need_output_log = 1;
				pr_debug("full ui_soc:%d soc:%d up_limit:%d up_count:%d\n", chg->ui_soc, g_charger_para.batt_soc, soc_up_limit * 5, soc_up_count);
				chg->prop_status = POWER_SUPPLY_STATUS_CHARGING;
			}
		} else {
			chg->prop_status = POWER_SUPPLY_STATUS_FULL;
			vote(chg->chg_disable_votable, CHG_FULL_VOTER, true, 0);
			g_charger_para.batt_is_full = 1;
		}
	} else if ((g_charger_para.chg_present == 1) && (chg->prop_status == POWER_SUPPLY_STATUS_FULL)
				/*	&& (get_any_voter_value_without_client(chg->chg_disable_votable, CHG_FULL_VOTER) == false)
					&& (is_client_vote_enabled(chg->chg_disable_votable, CHG_FULL_VOTER) == true) */) {
				//g_charger_para.need_output_log = 1;
				chg->prop_status = POWER_SUPPLY_STATUS_FULL;
				g_charger_para.batt_is_full = 1;
				soc_down_count = 0;
				soc_up_count = 0;
				return 1;
	} else if ((g_charger_para.chg_present == 1) && (g_charger_para.batt_status == POWER_SUPPLY_STATUS_CHARGING)) {
		chg->sleep_tm_sec = 0;
		if (chg->prop_status != POWER_SUPPLY_STATUS_FULL) {
			chg->prop_status = POWER_SUPPLY_STATUS_CHARGING;
		}
		if (g_charger_para.batt_soc == chg->ui_soc) {
			soc_down_count = 0;
			soc_up_count = 0;
		} else if (g_charger_para.batt_soc > chg->ui_soc) {
			soc_down_count = 0;
			soc_up_count++;
			if (soc_up_count >= soc_up_limit) {
				soc_up_count = 0;
				chg->ui_soc++;
			}
			if (chg->ui_soc >= 100) {
				chg->ui_soc = 100;
			}
		} else if (g_charger_para.batt_soc < chg->ui_soc) {
			soc_up_count = 0;
			soc_down_count++;
			if (soc_down_count >= soc_down_limit) {
				soc_down_count = 0;
				chg->ui_soc--;
			}
		}
		if (chg->ui_soc != g_charger_para.batt_soc) {
			g_charger_para.need_output_log = 1;
			pr_debug("charging ui_soc:%d soc:%d down_limit:%d down_count:%d up_limit:%d up_count:%d\n", chg->ui_soc, g_charger_para.batt_soc,
						soc_down_limit * 5, soc_down_count, soc_up_limit * 5, soc_up_count);
		}
	} else if ((g_charger_para.chg_present == 1) && (chg->prop_status == POWER_SUPPLY_STATUS_CHARGING)
					&& (get_any_voter_value_without_client(chg->chg_disable_votable, CHG_FULL_VOTER) == false)
					&& (is_client_vote_enabled(chg->chg_disable_votable, CHG_FULL_VOTER) == true)) {
		chg->sleep_tm_sec = 0;
		if (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_GOOD) {
			soc_down_count = 0;
			soc_up_count++;
			if (soc_up_count >= soc_up_limit) {
				soc_up_count = 0;
				chg->ui_soc++;
				if (chg->ui_soc <= 100) {
					g_charger_para.need_output_log = 1;
					pr_debug("full ui_soc:%d soc:%d up_limit:%d\n", chg->ui_soc, g_charger_para.batt_soc, soc_up_limit * 5);
				}
			}
			if (chg->ui_soc >= 100) {
				chg->ui_soc = 100;
				chg->prop_status = POWER_SUPPLY_STATUS_FULL;
				g_charger_para.batt_is_full = 1;
				val.intval = 100;
				power_supply_set_property(chg->bms_psy, POWER_SUPPLY_PROP_UI_SOC, &val);
			} else {
				g_charger_para.need_output_log = 1;
				pr_debug("full ui_soc:%d soc:%d up_limit:%d up_count:%d\n", chg->ui_soc, g_charger_para.batt_soc, soc_up_limit * 5, soc_up_count);
				chg->prop_status = POWER_SUPPLY_STATUS_CHARGING;
			}
		} else {
			chg->prop_status = POWER_SUPPLY_STATUS_FULL;
			g_charger_para.batt_is_full = 1;
		}
	} else {
		chg->prop_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		g_charger_para.batt_is_full = 0;
		soc_up_count = 0;
		if (g_charger_para.batt_soc <= chg->ui_soc || vbatt_too_low) {
			if (soc_down_count > soc_down_limit) {
				soc_down_count = soc_down_limit + 1;
			} else {
				soc_down_count++;
			}
			if (chg->sleep_tm_sec > 0) {
				soc_reduce_margin = chg->sleep_tm_sec / TEN_MINUTES;
				if (soc_reduce_margin == 0) {
					if ((chg->ui_soc - g_charger_para.batt_soc) > 2) {
						chg->ui_soc--;
						soc_down_count = 0;
						chg->sleep_tm_sec = 0;
					}
				} else if (soc_reduce_margin < (chg->ui_soc - g_charger_para.batt_soc)) {
					chg->ui_soc -= soc_reduce_margin;
					soc_down_count = 0;
					chg->sleep_tm_sec = 0;
				} else if (soc_reduce_margin >= (chg->ui_soc - g_charger_para.batt_soc)) {
					chg->ui_soc = g_charger_para.batt_soc;
					soc_down_count = 0;
					chg->sleep_tm_sec = 0;
				}
			}
			if (soc_down_count >= soc_down_limit && (g_charger_para.batt_soc < chg->ui_soc || vbatt_too_low)) {
				chg->sleep_tm_sec = 0;
				soc_down_count = 0;
				chg->ui_soc--;
			}
		}
		if (chg->ui_soc != g_charger_para.batt_soc) {
			g_charger_para.need_output_log = 1;
			pr_debug("discharging ui_soc:%d soc:%d down_limit:%d soc_down_count:%d sleep time:%ld\n", chg->ui_soc, g_charger_para.batt_soc,
						soc_down_limit * 5, soc_down_count, chg->sleep_tm_sec);
		}
	}

	soc_reduce_slow_when_1(chg);

	return 0;
}

int get_charger_timeout(struct smb_charger *chg)
{
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1435729, 20180625, Modify for timeout notify */
	return (chg->notify_code & (1 << CHARGER_OVERTIME)) ? 1 : 0;
}

void reset_charge_modify_setting(struct smb_charger *chg, int chg_triggle)
{
	if (chg_triggle == 1) {
		chg->prop_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		g_charger_para.batt_is_full = 0;
	}

	if (charger_monitor_work_init == false) {
		charger_need_reset = 1;
		return ;
	}

	g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COLD_TEMP].low_threshold; /* -2 ~ 0 */
	g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].low_threshold; /* 0 ~ 5 */
	g_chg_ranges[BATTERY_STATUS__COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COOL_TEMP].low_threshold; /* 5 ~ 12 */
	g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].low_threshold; /* 12 ~ 22 */
	g_chg_ranges[BATTERY_STATUS__NORMAL].bound_temp = g_chg_ranges[BATTERY_STATUS__NORMAL].low_threshold; /* 22 ~ 45 */
	g_chg_ranges[BATTERY_STATUS__WARM_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__WARM_TEMP].low_threshold; /* 45 ~ 53 */
	g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].low_threshold; /* > 53 */

	g_charger_para.full_cnt_c = 0;
	g_charger_para.full_cnt_v = 0;
	g_charger_para.batt_is_full = 0;
	g_charger_para.sum_cnt_warm = 0;
	g_charger_para.sum_cnt_good = 0;
	g_charger_para.sum_cnt_cool = 0;
	g_charger_para.sum_cnt_good_down_fv = 0;
	g_charger_para.sum_cnt_up_fv = 0;
	g_charger_para.batt_fv_up_status = 0;
	g_charger_para.chg_vol_low = 0;
	g_charger_para.need_output_log = 1;
	g_charger_para.need_do_icl = 0;
	reset_battery_voltage_current(chg);

	vote(chg->usb_icl_votable, CHG_CHK_VOTER, false, 0);
	vote(chg->chg_disable_votable, CHG_FULL_VOTER, false, 0);
	//update_chg_monitor_statu(chg);
	pr_info("Resest charge modify setting.\n");
}

void update_chg_monitor_statu(struct smb_charger *chg)
{
	union power_supply_propval val = {0, };
	int rc = 0;

	power_supply_get_property(chg->batt_psy, POWER_SUPPLY_PROP_TEMP, &val);
	g_charger_para.batt_temp = val.intval;
	power_supply_get_property(chg->batt_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
	g_charger_para.batt_vol = (val.intval + 500) / 1000;
	power_supply_get_property(chg->batt_psy, POWER_SUPPLY_PROP_CURRENT_NOW, &val);
	g_charger_para.batt_curr = val.intval;
	//power_supply_get_property(chg->batt_psy, POWER_SUPPLY_PROP_CAPACITY, &val);
	smblib_get_prop_batt_capacity(chg, &val);
	g_charger_para.batt_soc = val.intval;
	//power_supply_get_property(chg->batt_psy, POWER_SUPPLY_PROP_STATUS, &val);
	smblib_get_prop_batt_status(chg, &val);
	g_charger_para.batt_status = val.intval;
	power_supply_get_property(chg->batt_psy, POWER_SUPPLY_PROP_PRESENT, &val);
	g_charger_para.batt_present = val.intval;
	power_supply_get_property(chg->batt_psy, POWER_SUPPLY_PROP_AUTHENTICATE, &val);
	g_charger_para.batt_auth = val.intval;

	power_supply_get_property(chg->usb_psy, POWER_SUPPLY_PROP_PRESENT, &val);
	g_charger_para.chg_present = val.intval;

	if (g_charger_para.chg_present == 1) {
		power_supply_get_property(chg->usb_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
		g_charger_para.chg_vol = val.intval / 1000;
		chg->charger_mv = g_charger_para.chg_vol;

		if (chg->iio.skin_temp_chan) {
			rc = iio_read_channel_processed(chg->iio.skin_temp_chan, &chg->skin_temp);
			if (rc < 0) {
				pr_debug("Couldn't read SKIN TEMP channel, rc=%d\n", rc);
				chg->skin_temp = 25000;
			}
			chg->skin_temp /= 100;
		}

		if (chg->iio.cpu_temp_chan) {
			rc = iio_read_channel_processed(chg->iio.cpu_temp_chan, &chg->cpu_temp);
			if (rc < 0) {
				pr_debug("Couldn't read CPU0 TEMP channel, rc=%d\n", rc);
				chg->cpu_temp = 25000;
			}
			chg->cpu_temp /= 100;
		}

		//g_charger_para.chg_times = (u32)ktime_get_seconds(); // Ktime will be error when system suspend
		if ((g_charger_para.batt_status != POWER_SUPPLY_STATUS_CHARGING)
				&& ((get_any_voter_value_without_client(chg->chg_disable_votable, CHG_TIMEOUT_VOTER) == true)
					|| (is_client_vote_enabled(chg->chg_disable_votable, CHG_TIMEOUT_VOTER) == false))) {
			chg->charger_times = 0;
			g_charger_para.chg_times = 0;
		} else if (get_rtc_time(&g_charger_para.chg_times) < 0) {
			g_charger_para.chg_times = 0;
		} else if (chg->charger_times == 0) {
			chg->charger_times = g_charger_para.chg_times;
		}
		//pr_info("SMB charger notify from %ldS to %ldS.\n", chg->charger_times, g_charger_para.chg_times);
	} else {
		g_charger_para.chg_times = 0;
		chg->charger_times = 0;
		g_charger_para.chg_vol = 0;
		chg->charger_mv = 0;
	}
}

static void update_battery_healthd(struct smb_charger *chg)
{
	//int i = 0;
	int high_hyst = 0;
	int low_hyst = 0;
	static int batt_temp_chg = -12345;
	int chg_ranges_now_tmp = BATTERY_STATUS__NORMAL;

	if (g_charger_para.chg_present != 1) {
		batt_temp_chg = -12345;
	}

	if (g_charger_para.batt_temp <= g_chg_ranges[BATTERY_STATUS__LOW_TEMP].bound_temp) {
		chg_ranges_now_tmp = BATTERY_STATUS__REMOVED;
	} else if (g_charger_para.batt_temp < g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp) {
		chg_ranges_now_tmp = BATTERY_STATUS__LOW_TEMP;
	} else if (g_charger_para.batt_temp > g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].bound_temp) {
		chg_ranges_now_tmp = BATTERY_STATUS__HIGH_TEMP;
	} else if (g_charger_para.batt_temp >= g_chg_ranges[BATTERY_STATUS__WARM_TEMP].bound_temp) {
		chg_ranges_now_tmp = BATTERY_STATUS__WARM_TEMP;
	} else if (g_charger_para.batt_temp >= g_chg_ranges[BATTERY_STATUS__NORMAL].bound_temp) {
		chg_ranges_now_tmp = BATTERY_STATUS__NORMAL;
	} else if (g_charger_para.batt_temp >= g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].bound_temp) {
		chg_ranges_now_tmp = BATTERY_STATUS__LITTLE_COOL_TEMP;
	} else if (g_charger_para.batt_temp >= g_chg_ranges[BATTERY_STATUS__COOL_TEMP].bound_temp) {
		chg_ranges_now_tmp = BATTERY_STATUS__COOL_TEMP;
	} else if (g_charger_para.batt_temp >= g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].bound_temp) {
		chg_ranges_now_tmp = BATTERY_STATUS__LITTLE_COLD_TEMP;
	} else if (g_charger_para.batt_temp >= g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp) {
		chg_ranges_now_tmp = BATTERY_STATUS__COLD_TEMP;
	} else {
		g_charger_para.need_output_log = 1;
	}

	if (chg_ranges_now_tmp == g_charger_para.chg_ranges_now) {
		if ((g_charger_para.chg_present == 1) && (batt_temp_chg == -12345)) {
			batt_temp_chg = g_charger_para.batt_temp;
		}
		return ;
	}

	g_charger_para.chg_ranges_now = chg_ranges_now_tmp;
	g_charger_para.batt_healthd = g_chg_ranges[g_charger_para.chg_ranges_now].health;
	if (g_charger_para.chg_present != 1) {
		return ;
	} else if (batt_temp_chg == -12345) {
		batt_temp_chg = g_charger_para.batt_temp;
		return ;
	}

	if (g_charger_para.batt_temp > batt_temp_chg) { /*get warm*/
		low_hyst = -BAT_TEMP_HYST;
		high_hyst = 0;
	} else { /*get cool*/
		low_hyst = 0;
		high_hyst = BAT_TEMP_HYST;
	}

	if ((g_charger_para.chg_ranges_now == BATTERY_STATUS__NORMAL) /* 22 ~ 45 */
			|| (g_charger_para.chg_ranges_now == BATTERY_STATUS__REMOVED) /* < -19 */
			|| (g_charger_para.chg_ranges_now == BATTERY_STATUS__INVALID)) {
		g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COLD_TEMP].low_threshold; /* -2 ~ 0 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].low_threshold; /* 0 ~ 5 */
		g_chg_ranges[BATTERY_STATUS__COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COOL_TEMP].low_threshold; /* 5 ~ 12 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].low_threshold; /* 12 ~ 22 */
		g_chg_ranges[BATTERY_STATUS__NORMAL].bound_temp = g_chg_ranges[BATTERY_STATUS__NORMAL].low_threshold; /* 22 ~ 45 */
		g_chg_ranges[BATTERY_STATUS__WARM_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__WARM_TEMP].low_threshold; /* 45 ~ 53 */
		g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].low_threshold; /* > 53 */
	} else if (g_charger_para.chg_ranges_now == BATTERY_STATUS__LOW_TEMP) { /* -19 ~ -2C  */
		g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COLD_TEMP].low_threshold  + high_hyst; /* -2 ~ 0 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].low_threshold; /* 0 ~ 5 */
		g_chg_ranges[BATTERY_STATUS__COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COOL_TEMP].low_threshold; /* 5 ~ 12 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].low_threshold; /* 12 ~ 22 */
		g_chg_ranges[BATTERY_STATUS__NORMAL].bound_temp = g_chg_ranges[BATTERY_STATUS__NORMAL].low_threshold; /* 22 ~ 45 */
		g_chg_ranges[BATTERY_STATUS__WARM_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__WARM_TEMP].low_threshold; /* 45 ~ 53 */
		g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].low_threshold; /* > 53 */
	} else if (g_charger_para.chg_ranges_now == BATTERY_STATUS__COLD_TEMP) { /* -2 ~ 0 */
		g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COLD_TEMP].low_threshold + low_hyst; /* -2 ~ 0 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].low_threshold + high_hyst; /* 0 ~ 5 */
		g_chg_ranges[BATTERY_STATUS__COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COOL_TEMP].low_threshold; /* 5 ~ 12 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].low_threshold; /* 12 ~ 22 */
		g_chg_ranges[BATTERY_STATUS__NORMAL].bound_temp = g_chg_ranges[BATTERY_STATUS__NORMAL].low_threshold; /* 22 ~ 45 */
		g_chg_ranges[BATTERY_STATUS__WARM_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__WARM_TEMP].low_threshold; /* 45 ~ 53 */
		g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].low_threshold; /* > 53 */
	} else if (g_charger_para.chg_ranges_now == BATTERY_STATUS__LITTLE_COLD_TEMP) { /* 0 ~ 5 */
		g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COLD_TEMP].low_threshold; /* -2 ~ 0 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].low_threshold + low_hyst; /* 0 ~ 5 */
		g_chg_ranges[BATTERY_STATUS__COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COOL_TEMP].low_threshold + high_hyst; /* 5 ~ 12 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].low_threshold; /* 12 ~ 22 */
		g_chg_ranges[BATTERY_STATUS__NORMAL].bound_temp = g_chg_ranges[BATTERY_STATUS__NORMAL].low_threshold; /* 22 ~ 45 */
		g_chg_ranges[BATTERY_STATUS__WARM_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__WARM_TEMP].low_threshold; /* 45 ~ 53 */
		g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].low_threshold; /* > 53 */
	} else if (g_charger_para.chg_ranges_now == BATTERY_STATUS__COOL_TEMP) { /* 5 ~ 12 */
		g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COLD_TEMP].low_threshold; /* -2 ~ 0 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].low_threshold; /* 0 ~ 5 */
		g_chg_ranges[BATTERY_STATUS__COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COOL_TEMP].low_threshold  + low_hyst; /* 5 ~ 12 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].low_threshold + high_hyst; /* 12 ~ 22 */
		g_chg_ranges[BATTERY_STATUS__NORMAL].bound_temp = g_chg_ranges[BATTERY_STATUS__NORMAL].low_threshold; /* 22 ~ 45 */
		g_chg_ranges[BATTERY_STATUS__WARM_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__WARM_TEMP].low_threshold; /* 45 ~ 53 */
		g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].low_threshold; /* > 53 */
	} else if (g_charger_para.chg_ranges_now == BATTERY_STATUS__LITTLE_COOL_TEMP) { /* 12 ~ 22 */
		g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COLD_TEMP].low_threshold; /* -2 ~ 0 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].low_threshold; /* 0 ~ 5 */
		g_chg_ranges[BATTERY_STATUS__COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COOL_TEMP].low_threshold; /* 5 ~ 12 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].low_threshold  + low_hyst; /* 12 ~ 22 */
		g_chg_ranges[BATTERY_STATUS__NORMAL].bound_temp = g_chg_ranges[BATTERY_STATUS__NORMAL].low_threshold + high_hyst; /* 22 ~ 45 */
		g_chg_ranges[BATTERY_STATUS__WARM_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__WARM_TEMP].low_threshold; /* 45 ~ 53 */
		g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].low_threshold; /* > 53 */
	} else if (g_charger_para.chg_ranges_now == BATTERY_STATUS__WARM_TEMP) { /* 45 ~ 53 */
		g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COLD_TEMP].low_threshold; /* -2 ~ 0 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].low_threshold; /* 0 ~ 5 */
		g_chg_ranges[BATTERY_STATUS__COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COOL_TEMP].low_threshold; /* 5 ~ 12 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].low_threshold; /* 12 ~ 22 */
		g_chg_ranges[BATTERY_STATUS__NORMAL].bound_temp = g_chg_ranges[BATTERY_STATUS__NORMAL].low_threshold; /* 22 ~ 45 */
		g_chg_ranges[BATTERY_STATUS__WARM_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__WARM_TEMP].low_threshold + low_hyst; /* 45 ~ 53 */
		g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].low_threshold + high_hyst; /* > 53 */
	} else if (g_charger_para.chg_ranges_now == BATTERY_STATUS__HIGH_TEMP) { /* > 53 */
		g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COLD_TEMP].low_threshold; /* -2 ~ 0 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].low_threshold; /* 0 ~ 5 */
		g_chg_ranges[BATTERY_STATUS__COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__COOL_TEMP].low_threshold; /* 5 ~ 12 */
		g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].low_threshold; /* 12 ~ 22 */
		g_chg_ranges[BATTERY_STATUS__NORMAL].bound_temp = g_chg_ranges[BATTERY_STATUS__NORMAL].low_threshold; /* 22 ~ 45 */
		g_chg_ranges[BATTERY_STATUS__WARM_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__WARM_TEMP].low_threshold; /* 45 ~ 53 */
		g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].bound_temp = g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].low_threshold + low_hyst; /* > 53 */
	}

	pr_info("Temp %d -> %d, Healthd:%d, Temp Bound = [%d %d %d %d %d %d %d %d]", batt_temp_chg, g_charger_para.batt_temp, g_charger_para.chg_ranges_now,
		g_chg_ranges[BATTERY_STATUS__LOW_TEMP].bound_temp, g_chg_ranges[BATTERY_STATUS__COLD_TEMP].bound_temp, g_chg_ranges[BATTERY_STATUS__LITTLE_COLD_TEMP].bound_temp,
		g_chg_ranges[BATTERY_STATUS__COOL_TEMP].bound_temp, g_chg_ranges[BATTERY_STATUS__LITTLE_COOL_TEMP].bound_temp, g_chg_ranges[BATTERY_STATUS__NORMAL].bound_temp,
		g_chg_ranges[BATTERY_STATUS__WARM_TEMP].bound_temp, g_chg_ranges[BATTERY_STATUS__HIGH_TEMP].bound_temp);
	batt_temp_chg = g_charger_para.batt_temp;
}

int get_chg_battery_healthd(void)
{
	return g_charger_para.batt_healthd;
}

void reset_battery_voltage_current(struct smb_charger *chg)
{
	vote(chg->fv_votable, CHG_CHK_VOTER, true, g_chg_ranges[g_charger_para.chg_ranges_now].fv * 1000);
	vote(chg->fcc_votable, CHG_CHK_VOTER, true, g_chg_ranges[g_charger_para.chg_ranges_now].fcc);
}

static void check_up_battery_float_voltage(struct smb_charger *chg)
{
	int set_fv = 0;

	if (g_charger_para.chg_present != 1) {
		/* Modify FV at charging only */
		return ;
	}

	if (g_charger_para.sum_cnt_good_down_fv != 0) {
		return;
	}
	if ((g_charger_para.chg_present == 1) && (g_charger_para.batt_fv_up_status == 0) && (g_charger_para.sum_cnt_good_down_fv == 0)
			&& (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_GOOD) && (g_charger_para.batt_soc > 85)) {
		g_charger_para.need_output_log = 1;
		set_fv = get_client_vote(chg->fv_votable, CHG_CHK_VOTER);
		pr_info("Set Battery FV [%d-> 4410].\n", set_fv / 1000);
		if (set_fv > 4410000) {
			vote(chg->fv_votable, CHG_CHK_VOTER, true, 4410000);
			//g_charger_para.sum_cnt_good_down_fv += 1;
			g_charger_para.batt_fv_up_status = 1;
		}
		return;
	}

	set_fv = get_client_vote(chg->fv_votable, CHG_CHK_VOTER);
	if ((g_charger_para.batt_fv_up_status == 1) && (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_GOOD)
			&& (g_charger_para.batt_vol < 4435) && (set_fv < 4430000)) {
		g_charger_para.need_output_log = 1;
		g_charger_para.sum_cnt_up_fv += 1;
		if (g_charger_para.sum_cnt_up_fv >= 3) {
			g_charger_para.sum_cnt_up_fv = 0;
			if (set_fv >= BAT_MIN_FV) {
				set_fv = (set_fv > 4420000) ? 4420000 : set_fv; //Max value can't out of 4.430V after add 0.01V
				vote(chg->fv_votable, CHG_CHK_VOTER, true, set_fv + 10000);
			}
			pr_info("Battery FV now:%dmV, + 10mV. Up good!\n", set_fv / 1000);
		}
	} else {
		g_charger_para.sum_cnt_up_fv = 0;
	}
}

static void check_charger_full(struct smb_charger *chg)
{
	bool battery_is_full = false;
	int set_fv = get_client_vote(chg->fv_votable, CHG_CHK_VOTER) / 1000;
	union power_supply_propval val = {0, };

	if (g_charger_para.chg_present != 1) {
		return ;
	}
	if (g_charger_para.batt_status != POWER_SUPPLY_STATUS_CHARGING) {
		return ;
	}

	//if (g_charger_para.batt_vol > g_chg_ranges[g_charger_para.chg_ranges_now].fv)
	if (g_charger_para.batt_vol >= set_fv + BAT_FULL_FV_ABOVE) {
		if ((g_charger_para.batt_curr < 0) && ((g_charger_para.batt_curr * -1) < BATTERY_ITERM_CURRENT)) {
			g_charger_para.need_output_log = 1;
			g_charger_para.full_cnt_c += 1;
			if (g_charger_para.full_cnt_c > FULL_COUNTS_BY_CURRENT) {
				battery_is_full |= true;
				g_charger_para.full_cnt_c = 0;
			}
		} else if (g_charger_para.batt_curr >= 0) {
			g_charger_para.need_output_log = 1;
			g_charger_para.full_cnt_c += 1;
			if (g_charger_para.full_cnt_c > 2 * FULL_COUNTS_BY_CURRENT) {
				battery_is_full |= true;
				g_charger_para.full_cnt_c = 0;
			}
		} else {
			battery_is_full |= false;
			g_charger_para.full_cnt_c = 0;
		}
	}

	if (g_charger_para.batt_vol >= g_chg_ranges[g_charger_para.chg_ranges_now].max_fv) {
		g_charger_para.need_output_log = 1;
		g_charger_para.full_cnt_v += 1;
		if (g_charger_para.full_cnt_v > FULL_COUNTS_BY_VOLTAGE) {
			battery_is_full |= true;
			g_charger_para.full_cnt_v = 0;
		}
	}

	if (battery_is_full == true) {
		g_charger_para.batt_is_full = 1;
		if (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_GOOD) {
			if (chg->ui_soc >= 100) {
				chg->prop_status = POWER_SUPPLY_STATUS_FULL;
				vote(chg->chg_disable_votable, CHG_FULL_VOTER, true, 0);
				val.intval = 100;
				power_supply_set_property(chg->bms_psy, POWER_SUPPLY_PROP_UI_SOC, &val);
				pr_info("check_charger_full GOOD full_cnt_c:%d, full_cnt_v:%d.\n", g_charger_para.full_cnt_c, g_charger_para.full_cnt_v);
			} else {
				vote(chg->chg_disable_votable, CHG_FULL_VOTER, true, 0);
				pr_info("check_charger_full GOOD need wait ui_soc:%d full_cnt_c:%d, full_cnt_v:%d.\n", chg->ui_soc, g_charger_para.full_cnt_c, g_charger_para.full_cnt_v);
			}
		} else {
				chg->prop_status = POWER_SUPPLY_STATUS_FULL;
				vote(chg->chg_disable_votable, CHG_FULL_VOTER, true, 0);
				pr_info("check_charger_full COOL_WARM full_cnt_c:%d, full_cnt_v:%d.\n", g_charger_para.full_cnt_c, g_charger_para.full_cnt_v);
		}
	}
}

static void check_recharger(struct smb_charger *chg)
{
	union power_supply_propval val = {0, };

	if ((g_charger_para.chg_present != 1) || (chg->prop_status != POWER_SUPPLY_STATUS_FULL)
			|| (is_client_vote_enabled(chg->chg_disable_votable, CHG_FULL_VOTER) == false)) {
		return ;
	}

	val.intval = g_chg_ranges[g_charger_para.chg_ranges_now].rechr_v;
	if (g_charger_para.batt_vol < val.intval) {
		g_charger_para.full_cnt_c = 0;
		g_charger_para.full_cnt_v = 0;
		val.intval = val.intval * 1000;
		power_supply_set_property(chg->batt_psy, POWER_SUPPLY_PROP_RECHARGE_UV, &val);
		vote(chg->chg_disable_votable, CHG_FULL_VOTER, false, 0);
	}
}

void set_icl_flags(struct smb_charger *chg, int val)
{
	g_charger_para.need_do_icl |= (1 << val);
}

void update_state_healthd_changed(struct smb_charger *chg)
{
	union power_supply_propval val = {0, };

	g_charger_para.full_cnt_c = 0;
	g_charger_para.full_cnt_v = 0;
	g_charger_para.batt_is_full = 0;

	if (!((g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_GOOD) || (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_COOL)
			|| (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_WARM))) {
		return ;
	}

	val.intval = g_chg_ranges[g_charger_para.chg_ranges_now].rechr_v * 1000;
	power_supply_set_property(chg->batt_psy, POWER_SUPPLY_PROP_RECHARGE_UV, &val);
	vote(chg->chg_disable_votable, CHG_FULL_VOTER, false, 0);

	//smblib_get_prop_batt_status(chg, &val);
	//g_charger_para.batt_status = val.intval;
	get_battery_status_modify(chg);
}

void update_battery_float_voltage(struct smb_charger *chg)
{
	int set_fv = 0;

	if (g_charger_para.init_first == 1) {
		g_charger_para.need_output_log = 1;
		reset_battery_voltage_current(chg);
	}

	if (g_charger_para.chg_ranges_save != g_charger_para.chg_ranges_now) {
		g_charger_para.need_output_log = 1;
		reset_battery_voltage_current(chg);
		g_charger_para.sum_cnt_good_down_fv = 0;
	}

	if (g_charger_para.chg_present != 1) {
		/* Modify FV at charging only */
		return ;
	}

	if (g_charger_para.batt_healthd_save != g_charger_para.batt_healthd) {
		g_charger_para.need_output_log = 1;
		update_state_healthd_changed(chg);
		g_charger_para.sum_cnt_good_down_fv = 0;
	}

	if ((g_charger_para.chg_ranges_now == 4)
			&& (g_charger_para.batt_vol >= 4180)) {
		vote(chg->fcc_votable, CHG_CHK_VOTER, true, g_chg_ranges[3].fcc); // 0.15C
	}

	if (g_charger_para.batt_status == POWER_SUPPLY_STATUS_FULL) {
		/* Setting FV only at Full */
		return ;
	}

	if ((g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_GOOD)
			&& (g_charger_para.batt_vol >= g_chg_ranges[g_charger_para.chg_ranges_now].chg_fv)) {
		g_charger_para.need_output_log = 1;
		g_charger_para.sum_cnt_good += 1;
		if (g_charger_para.sum_cnt_good >= 3) {
			g_charger_para.sum_cnt_good = 0;
			set_fv = get_client_vote(chg->fv_votable, CHG_CHK_VOTER);
			pr_info("Battery FV now:%dmV, - 10mV. Down good!\n", set_fv / 1000);
			if (set_fv >= BAT_MIN_FV) {
				vote(chg->fv_votable, CHG_CHK_VOTER, true, set_fv - 10000);
				g_charger_para.sum_cnt_good_down_fv += 1;
			}
		}
	} else {
		g_charger_para.sum_cnt_good = 0;
	}

	if ((g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_WARM)
			&& (g_charger_para.batt_vol >= g_chg_ranges[g_charger_para.chg_ranges_now].chg_fv)) {
		g_charger_para.need_output_log = 1;
		g_charger_para.sum_cnt_warm += 1;
		if (g_charger_para.sum_cnt_warm >= 3) {
			g_charger_para.sum_cnt_warm = 0;
			set_fv = get_client_vote(chg->fv_votable, CHG_CHK_VOTER);
			pr_info("Battery FV now:%dmV, - 10mV. Down warm!\n", set_fv / 1000);
			if (set_fv >= BAT_MIN_FV) {
				vote(chg->fv_votable, CHG_CHK_VOTER, true, set_fv - 10000);
			}
		}
	} else {
		g_charger_para.sum_cnt_warm = 0;
	}

	if ((g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_COOL)
			&& (g_charger_para.batt_vol >= g_chg_ranges[g_charger_para.chg_ranges_now].chg_fv)) {
		g_charger_para.need_output_log = 1;
		g_charger_para.sum_cnt_cool += 1;
		if (g_charger_para.sum_cnt_cool >= 3) {
			g_charger_para.sum_cnt_cool = 0;
			set_fv = get_client_vote(chg->fv_votable, CHG_CHK_VOTER);
			pr_info("Battery FV now:%dmV, - 10mV. Down cool!\n", set_fv / 1000);
			if (set_fv >= BAT_MIN_FV) {
				vote(chg->fv_votable, CHG_CHK_VOTER, true, set_fv - 10000);
			}
		}
	} else {
		g_charger_para.sum_cnt_cool = 0;
	}
}

void update_charger_status(struct smb_charger *chg)
{
	int need_stop_usb_chg_tmp = 0;
	int need_stop_bat_chg_tmp = 0;
	static int chg_vol_low_out = 0;
	int chg_vol_low_rechr = 0;
	int rc = 0;
	union power_supply_propval val = {0, };

	if (g_charger_para.chg_present != 1) {
		return ;
	}

	if (g_charger_para.chg_vol > CHG_VOL_HIGH_PARAMS) {
		need_stop_usb_chg_tmp = 1;
		need_stop_bat_chg_tmp = 1;
	}
	/* Don't need stop charger when Vbus was below 4.3V */
	chg_vol_low_rechr = CHG_VOL_LOW_PARAMS > g_charger_para.batt_vol ? CHG_VOL_LOW_PARAMS : g_charger_para.batt_vol;
	if ((g_charger_para.chg_vol < CHG_VOL_LOW_PARAMS) && (g_charger_para.chg_vol_low != 1)) {
		msleep(10);
		power_supply_get_property(chg->usb_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
		if (val.intval / 1000 < CHG_VOL_LOW_PARAMS) {
			g_charger_para.chg_vol_low = 1;
			pr_info("VBUS was still low after read the seconds times.%d\n", val.intval);
		}
	} else if ((g_charger_para.chg_vol > chg_vol_low_rechr + 350) && (g_charger_para.chg_vol_low == 1)) {
		chg_vol_low_out += 1;
	} else {
		chg_vol_low_out = 0;
	}
	if (g_charger_para.chg_vol_low == 1) {
		g_charger_para.need_output_log = 1;
	}

	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1445507, 20180625, Modify timeout setting */
	if ((chg->charger_times > 0) && (g_charger_para.chg_times >= chg->charger_times)
			&& (g_charger_para.chg_times - chg->charger_times >= g_charger_para.chg_time_out)
			&& (g_charger_para.chg_time_out >= 0)) {
		//need_stop_usb_chg_tmp = 1;
		//need_stop_bat_chg_tmp = 1;
		vote(chg->chg_disable_votable, CHG_TIMEOUT_VOTER, true, 0);
	} else {
		vote(chg->chg_disable_votable, CHG_TIMEOUT_VOTER, false, 0);
	}
	if (g_charger_para.batt_vol > BAT_VOL_HIGH_PARAMS) {
		need_stop_bat_chg_tmp = 1;
	}
	if (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_OVERHEAT) {
		need_stop_bat_chg_tmp = 1;
	}
	if ((g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_COLD) || (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_DEAD)
			|| (g_charger_para.batt_auth != 1)) {
		need_stop_bat_chg_tmp = 1;
	}

	vote(chg->usb_icl_votable, FB_BLANK_VOTER, false, 0);
	if (g_charger_para.lcd_is_on == 1) {
	/*
		if (g_charger_para.batt_temp >= 350)
			vote(chg->usb_icl_votable, FB_BLANK_VOTER, true, min(g_charger_para.lcd_on_chg_curr_proc, USB_ICL_LCD_ON_ABOVE_350));
		else
			vote(chg->usb_icl_votable, FB_BLANK_VOTER, true, min(g_charger_para.lcd_on_chg_curr_proc, USB_ICL_LCD_ON));
	*/

		if (chg->skin_temp > TEMP_SKIN_LEVEL_2)
			vote(chg->usb_icl_votable, SKIN_TEMP_VOTER, true, USB_ICL_SKIN_LEVEL_2);
		else if (chg->skin_temp > TEMP_SKIN_LEVEL_1)
			vote(chg->usb_icl_votable, SKIN_TEMP_VOTER, true, USB_ICL_SKIN_LEVEL_1);
		else
			vote(chg->usb_icl_votable, SKIN_TEMP_VOTER, false, 0);

		if (chg->cpu_temp > TEMP_CPU_LEVEL_2)
			vote(chg->usb_icl_votable, CPU_TEMP_VOTER, true, USB_ICL_CPU_LEVEL_2);
		else if (chg->cpu_temp > TEMP_CPU_LEVEL_1)
			vote(chg->usb_icl_votable, CPU_TEMP_VOTER, true, USB_ICL_CPU_LEVEL_1);
		else
			vote(chg->usb_icl_votable, CPU_TEMP_VOTER, false, 0);
	} else if (g_charger_para.lcd_is_on == 0) {
		//vote(chg->usb_icl_votable, FB_BLANK_VOTER, false, 0);
		vote(chg->usb_icl_votable, SKIN_TEMP_VOTER, false, 0);
		vote(chg->usb_icl_votable, CPU_TEMP_VOTER, false, 0);
	}

	if (need_stop_usb_chg_tmp != g_charger_para.need_stop_usb_chg) {
		g_charger_para.need_output_log = 1;
		g_charger_para.need_stop_usb_chg = need_stop_usb_chg_tmp;
		if (g_charger_para.need_stop_usb_chg == 1) {
			vote(chg->usb_icl_votable, CHG_STOP_VOTER, true, 0);
			//smblib_set_usb_suspend(chg, true);
		} else {
			vote(chg->usb_icl_votable, CHG_STOP_VOTER, false, 0);
			//smblib_set_usb_suspend(chg, false);
		}
	}
	if (need_stop_bat_chg_tmp != g_charger_para.need_stop_bat_chg) {
		g_charger_para.need_output_log = 1;
		g_charger_para.need_stop_bat_chg = need_stop_bat_chg_tmp;
		if (g_charger_para.need_stop_bat_chg == 1) {
			vote(chg->chg_disable_votable, CHG_STOP_VOTER, true, 0);
		} else {
			vote(chg->chg_disable_votable, CHG_STOP_VOTER, false, 0);
		}
	}

	if (/*need_stop_bat_chg_tmp || */need_stop_usb_chg_tmp) {
		return ;
	}


	if ((g_charger_para.chg_ranges_save != g_charger_para.chg_ranges_now) || (g_charger_para.need_do_icl != 0)) {
		do_charger_icl(chg);
	}
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1372106, 20180625, Add for do ICL when USB was plug-in */
	/*
	if ((g_charger_para.chg_present == 1) && (g_charger_para.chg_present_save != g_charger_para.chg_present)) {
		do_charger_icl(chg);
	}
	*/

	/* Need do charging in FULL sometime, case about full status changed! */
	if ((g_charger_para.chg_vol_low == 1) && (chg_vol_low_out >= 2)/* && (chg->prop_status != POWER_SUPPLY_STATUS_FULL)*/) {
		g_charger_para.need_output_log = 1;
		g_charger_para.chg_vol_low = 0;
		pr_info("Reset from 4.3V VBUS\n");
		vote(chg->usb_icl_votable, WEAK_CHARGER_VOTER, false, 0);

		rc = smblib_masked_write(chg, USBIN_CMD_IL_REG, USBIN_SUSPEND_BIT, 1);
		if (rc < 0) {
			pr_info("Couldn't set USBIN_SUSPEND_BIT 1 rc=%d\n", rc);
		}
		//msleep(50);
		rc = smblib_masked_write(chg, USBIN_CMD_IL_REG, USBIN_SUSPEND_BIT, 0);
		if (rc < 0) {
			pr_info("Couldn't set USBIN_SUSPEND_BIT 0 rc=%d\n", rc);
		}
		msleep(5);
		//do_charger_icl(chg);
		rc = smblib_masked_write(chg, USBIN_AICL_OPTIONS_CFG_REG, USBIN_AICL_EN_BIT, 0);
		if (rc < 0) {
			pr_info("Couldn't set USBIN_AICL_OPTIONS_CFG_REG 0 rc=%d\n", rc);
		}
		msleep(5);
		rc = smblib_masked_write(chg, USBIN_AICL_OPTIONS_CFG_REG, USBIN_AICL_EN_BIT, USBIN_AICL_EN_BIT);
		if (rc < 0) {
			pr_info("Couldn't set USBIN_AICL_OPTIONS_CFG_REG 1 rc=%d\n", rc);
		}
		msleep(5);
		rc = smblib_masked_write(chg, AICL_CMD_REG, BIT(1), BIT(1));
		if (rc < 0) {
			pr_info("Couldn't set AICL_CMD_REG 1 rc=%d\n", rc);
		}
	}
}

#define CHARGER_ABNORMAL_DETECT_TIME 24
/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1372106, 20180610, Add for charger notify code */
void update_charger_notify_code(struct smb_charger *chg)
{
	int tmp_notify_code = 0;
	static int chg_abnormal_count = 0;

	if (g_charger_para.chg_present != 1) {
		chg->notify_code = 0;
		charger_abnormal_log = CRITICAL_LOG_NORMAL;
		chg_abnormal_count = 0;
		return ;
	}

	if ((get_effective_result(chg->chg_disable_votable) == false) && (chg->real_charger_type == POWER_SUPPLY_TYPE_USB_DCP)
			&& (chg->ui_soc <= 75) && (g_charger_para.batt_curr >= -20)) {
		chg_abnormal_count++;
		if (chg_abnormal_count >= CHARGER_ABNORMAL_DETECT_TIME) {
			chg_abnormal_count = CHARGER_ABNORMAL_DETECT_TIME;
			charger_abnormal_log = CRITICAL_LOG_UNABLE_CHARGING;
		}
		pr_err("unable charging, count=%d\n", chg_abnormal_count);
	} else {
		chg_abnormal_count = 0;
	}

	if (g_charger_para.chg_vol > CHG_VOL_HIGH_PARAMS) {
		tmp_notify_code |= (1 << CHG_VOL_HIGH);
		charger_abnormal_log = CRITICAL_LOG_VCHG_ABNORMAL;
	} else {
		tmp_notify_code &= ~(1 << CHG_VOL_HIGH);
	}
	if (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_OVERHEAT) {
		tmp_notify_code |= (1 << BAT_TEMP_HIGH);
		charger_abnormal_log = CRITICAL_LOG_BATTTEMP_ABNORMAL;
	} else {
		tmp_notify_code &= ~(1 << BAT_TEMP_HIGH);
	}
	if (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_COLD) {
		/*(g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_COLD)*/
		tmp_notify_code |= (1 << BAT_TEMP_LOW);
		charger_abnormal_log = CRITICAL_LOG_BATTTEMP_ABNORMAL;
	} else {
		tmp_notify_code &= ~(1 << BAT_TEMP_LOW);
	}
	if (g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_DEAD) {
		charger_abnormal_log = CRITICAL_LOG_BATTTEMP_ABNORMAL;
	}
	if ((g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_DEAD) || (g_charger_para.batt_auth != 1))/*(batt_present != 1)*/ {
		tmp_notify_code |= (1 << BAT_TEMP_DISCONNECT);
	} else {
		tmp_notify_code &= ~(1 << BAT_TEMP_DISCONNECT);
	}
	if (g_charger_para.batt_vol > BAT_VOL_HIGH_PARAMS) {
		tmp_notify_code |= (1 << BAT_VOL_HIGH);
		charger_abnormal_log = CRITICAL_LOG_VBAT_TOO_HIGH;
	} else {
		tmp_notify_code &= ~(1 << BAT_VOL_HIGH);
	}
	if ((g_charger_para.batt_healthd  == POWER_SUPPLY_HEALTH_GOOD) && (/*g_charger_para.batt_status*/chg->prop_status == POWER_SUPPLY_STATUS_FULL)) {
		tmp_notify_code |= (1 << CHARGER_FULL);
	} else {
		tmp_notify_code &= ~(1 << CHARGER_FULL);
	}
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1445507, 20180625, Modify timeout setting */
	if ((chg->charger_times > 0) && (g_charger_para.chg_times >= chg->charger_times)
			&& (g_charger_para.chg_times - chg->charger_times >= g_charger_para.chg_time_out)
			&& (g_charger_para.chg_time_out >= 0) && (g_charger_para.chg_present == 1)) {
		tmp_notify_code |= (1 << CHARGER_OVERTIME);
		charger_abnormal_log = CRITICAL_LOG_CHARGING_OVER_TIME;
	} else {
		tmp_notify_code &= ~(1 << CHARGER_OVERTIME);
	}
	if ((g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_WARM) && (/*g_charger_para.batt_status*/chg->prop_status == POWER_SUPPLY_STATUS_FULL)
			&& (g_charger_para.chg_present == 1)) {
		tmp_notify_code |= (1 << CHARGER_FULL_HIGHTEMP);
	} else {
		tmp_notify_code &= ~(1 << CHARGER_FULL_HIGHTEMP);
	}
	if ((g_charger_para.batt_healthd == POWER_SUPPLY_HEALTH_COOL) && (/*g_charger_para.batt_status*/chg->prop_status == POWER_SUPPLY_STATUS_FULL)
			&& (g_charger_para.chg_present == 1)) {
		tmp_notify_code |= (1 << CHARGER_FULL_LOWTEMP2);
	} else {
		tmp_notify_code &= ~(1 << CHARGER_FULL_LOWTEMP2);
	}

	if (chg->notify_code != tmp_notify_code) {
		pr_info("SMB charger notify code from 0x%x to 0x%x,charger time %ldS (%ldS %ldS).\n", chg->notify_code, tmp_notify_code,
					(g_charger_para.chg_times ? (g_charger_para.chg_times - chg->charger_times) : 0), g_charger_para.chg_times, chg->charger_times);
		g_charger_para.need_output_log = 1;
		chg->notify_code = tmp_notify_code;
		//power_supply_changed(chg->batt_psy);
	}
	//pr_info("SMB charger notify code 0x%x with charge time %ldS.\n", chg->notify_code, chg_times - chg->charger_times);
}

void update_charger_output_log(struct smb_charger *chg)
{
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1445507, 20180625, Modify timeout setting */
	unsigned long c_time =
		(((g_charger_para.chg_times > 0) && (g_charger_para.chg_times >= chg->charger_times)) ? (g_charger_para.chg_times - chg->charger_times) : 0);

	if ((chg->prop_status != g_charger_para.batt_status) && (g_charger_para.batt_status != POWER_SUPPLY_STATUS_DISCHARGING)) {
		g_charger_para.need_output_log = 1;
	}

	if (g_charger_para.need_output_log == 0) {
		//return ;
	}

	pr_info("Battery:[UI%d/%d/%dmV/%dmA/T%d/S O(%d)->N(%d)/H O(%d:%d)->N(%d:%d)/F%d]; Vbus:[P%d/T%d/%dmV/%ldS]; Notify:[0x%x]; Sleep:[%ld].T:[C%d/S%d]\n",
				chg->ui_soc, g_charger_para.batt_soc, g_charger_para.batt_vol, g_charger_para.batt_curr, g_charger_para.batt_temp,
				g_charger_para.batt_status, chg->prop_status, g_charger_para.batt_healthd_save, g_charger_para.chg_ranges_save,
				g_charger_para.batt_healthd, g_charger_para.chg_ranges_now, g_charger_para.batt_is_full,
				g_charger_para.chg_present, chg->real_charger_type, g_charger_para.chg_vol, c_time, chg->notify_code, chg->sleep_tm_sec,
				chg->cpu_temp, chg->skin_temp);

	return ;
}

static void smblib_charger_monitor_work(struct work_struct *work)
{
	struct smb_charger *chg = container_of(work, struct smb_charger,
						charger_monitor_work.work);

	charger_monitor_work_init = true;
	if (charger_need_reset == 1) {
		charger_need_reset = 0;
		reset_charge_modify_setting(chg, 0);
	}

	update_chg_monitor_statu(chg);

	update_battery_healthd(chg);
	update_battery_ui_soc(chg);
	update_battery_float_voltage(chg);
	check_up_battery_float_voltage(chg);

	check_charger_full(chg);
	check_recharger(chg);

	update_charger_status(chg);
	update_charger_notify_code(chg);

	update_charger_output_log(chg);

	g_charger_para.init_first = 0;
	g_charger_para.need_output_log = 0;
	g_charger_para.chg_present_save = g_charger_para.chg_present;
	g_charger_para.batt_healthd_save = g_charger_para.batt_healthd;
	g_charger_para.chg_ranges_save = g_charger_para.chg_ranges_now;

	power_supply_changed(chg->batt_psy);
	schedule_delayed_work(&chg->charger_monitor_work, msecs_to_jiffies(CHARGER_MONITOR_DELAY_MS));
}

/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1372106, 20180615, Add lcd on-off control usb icl */
static int fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct smb_charger *chg= container_of(self, struct smb_charger, fb_notif);
	//union power_supply_propval val = {0, };

	if (event == MSM_DRM_EARLY_EVENT_BLANK)
		return 0;
	if (evdata && evdata->data) {
		blank = evdata->data;
		switch (*blank) {
		case MSM_DRM_BLANK_UNBLANK:
			g_charger_para.lcd_is_on = 1;
			set_icl_flags(chg, 10);
			break;
		case MSM_DRM_BLANK_POWERDOWN:
			g_charger_para.lcd_is_on = 0;
			set_icl_flags(chg, 11);
			break;
		default :
			pr_info("Something error when run lcd set usb icl!\n");
			break;
		}
	}

    return 0;
}

static ssize_t charger_cycle_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	char buf[16] = {0};

	if (copy_from_user(buf, buff, (len > 16) ? 16 : len)) {
		pr_err("proc write error.\n");
		return -EFAULT;
	}
	if (!strncmp(buf, "en808", strlen("en808"))) {
		vote(g_chg->usb_icl_votable, MMI_CHG_VOTER, false, 0);
		vote(g_chg->chg_disable_votable, MMI_CHG_VOTER, false, 0);
	} else if (!strncmp(buf, "dis808", strlen("dis808"))) {
		vote(g_chg->usb_icl_votable, MMI_CHG_VOTER, true, 0);
		vote(g_chg->chg_disable_votable, MMI_CHG_VOTER, true, 0);
	} else if (!strncmp(buf, "wakelock", strlen("wakelock"))) {
		vote(g_chg->usb_icl_votable, MMI_CHG_VOTER, false, 0);
		vote(g_chg->chg_disable_votable, MMI_CHG_VOTER, false, 0);
		__pm_stay_awake(&g_chg->charge_wake_lock);
	} else if (!strncmp(buf, "unwakelock", strlen("unwakelock"))) {
		vote(g_chg->usb_icl_votable, MMI_CHG_VOTER, true, 0);
		vote(g_chg->chg_disable_votable, MMI_CHG_VOTER, true, 0);
		__pm_relax(&g_chg->charge_wake_lock);
	}
	pr_info("proc cycle string:%s!\n", buf);

	return len;
}
#if 0
static ssize_t charger_cycle_read(struct file *filp, char __user *buff, size_t len, loff_t *data)
{
	char value[2] = {0};
	snprintf(value, sizeof(value), "%d", flash_mode);
	return simple_read_from_buffer(buff, len, data, value,1);
}
#endif
static const struct file_operations charger_cycle_fops = {
	.owner		= THIS_MODULE,
	//.read		= NULL,
	.write		= charger_cycle_write,
};

static ssize_t critical_log_read(struct file *filp, char __user *buff, size_t count, loff_t *off)
{
	char page[256] = {0};
	char read_data[32] = {0};
	int len = 0;

	if(charger_abnormal_log >= 10){
		charger_abnormal_log = 10;
	}
	read_data[0] = '0' + charger_abnormal_log % 10;
	len = sprintf(page, "%s", read_data);
	if (len > *off) {
		len -= *off;
	} else {
		len = 0;
	}
	if (copy_to_user(buff, page, (len < count ? len : count))) {
		return -EFAULT;
	}
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

static ssize_t critical_log_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	char write_data[32] = {0};
	int critical_log = 0;

	if(copy_from_user(&write_data, buff, len)) {
		pr_err("bat_log_write error.\n");
		return -EFAULT;
	}

	write_data[len] = '\0';
	if (write_data[len - 1] == '\n') {
		write_data[len - 1] = '\0';
	}
	critical_log = (int)simple_strtol(write_data, NULL, 10);
	if(critical_log > 256) {
		critical_log = 256;
	}
	charger_abnormal_log = critical_log;

	return len;
}

static const struct file_operations chg_critical_log_proc_fops = {
	.owner		= THIS_MODULE,
	.read		= critical_log_read,
	.write		= critical_log_write,
};

static int rtc_reset_check(void)
{
	struct rtc_time tm;
	struct rtc_device *rtc;
	int rc = 0;

	rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	if (rtc == NULL) {
		pr_err("%s: unable to open rtc device (%s)\n", __FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
		return 0;
	}

	rc = rtc_read_time(rtc, &tm);
	if (rc) {
		pr_err("Error reading rtc device (%s) : %d\n", CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}

	rc = rtc_valid_tm(&tm);
	if (rc) {
		pr_err("Invalid RTC time (%s): %d\n", CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}

	if ((tm.tm_year == 70) && (tm.tm_mon == 0) && (tm.tm_mday <= 1)) {
		pr_err(": Sec: %d, Min: %d, Hour: %d, Day: %d, Mon: %d, Year: %d  @@@ wday: %d, yday: %d, isdst: %d\n",
				tm.tm_sec, tm.tm_min, tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year,
				tm.tm_wday, tm.tm_yday, tm.tm_isdst);
		rtc_class_close(rtc);
		return 1;
	}

	pr_err(": Sec: %d, Min: %d, Hour: %d, Day: %d, Mon: %d, Year: %d  ###  wday: %d, yday: %d, isdst: %d\n",
			tm.tm_sec, tm.tm_min, tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year,
			tm.tm_wday, tm.tm_yday, tm.tm_isdst);

close_time:
	rtc_class_close(rtc);
	return 0;
}

static ssize_t rtc_reset_read(struct file *filp, char __user *buff, size_t count, loff_t *off)
{
	char page[256] = {0};
	char read_data[32] = {0};
	int len = 0;
	int rc = 0;

	rtc_reset_check();
	read_data[0] = '0' + rc % 10;
	len = sprintf(page, "%s", read_data);
	if (len > *off) {
		len -= *off;
	} else {
		len = 0;
	}
	if (copy_to_user(buff, page, (len < count ? len : count))) {
		return -EFAULT;
	}
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

static const struct file_operations rtc_reset_det_fops = {
	.read = rtc_reset_read,
};

static ssize_t charging_limit_current_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	char buf[8] = {0};
	int tmp = 0;

	if (copy_from_user(buf, buff, (len > 8) ? 8 : len)) {
		pr_err("proc write error.\n");
		return -EFAULT;
	}
	tmp = simple_strtoul(buf, NULL, 10);
	g_charger_para.lcd_on_chg_curr_proc = tmp > 2000 ? 2000000 : tmp * 1000;
/*
	if ((g_charger_para.lcd_is_on == 1) && (g_charger_para.batt_temp >= 350)) {
		vote(g_chg->usb_icl_votable, FB_BLANK_VOTER, true, min(g_charger_para.lcd_on_chg_curr_proc, USB_ICL_LCD_ON_ABOVE_350));
	} else if (g_charger_para.lcd_is_on == 1) {
		vote(g_chg->usb_icl_votable, FB_BLANK_VOTER, true, min(g_charger_para.lcd_on_chg_curr_proc, USB_ICL_LCD_ON));
	}
*/
	return len;
}
static ssize_t charging_limit_current_read(struct file *filp, char __user *buff, size_t len, loff_t *data)
{
	char value[8] = {0};
	int str_len = 0;

	str_len = snprintf(value, sizeof(value), "%d", g_charger_para.lcd_on_chg_curr_proc);

	return simple_read_from_buffer(buff, len, data, value, str_len);
}

static const struct file_operations charging_limit_current_fops = {
	.owner		= THIS_MODULE,
	.read		= charging_limit_current_read,
	.write		= charging_limit_current_write,
};

static ssize_t charging_limit_time_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	char buf[16] = {0};

	if (copy_from_user(buf, buff, (len > 16) ? 16 : len)) {
		pr_err("proc write error.\n");
		return -EFAULT;
	}
	g_charger_para.chg_time_out = simple_strtol(buf, NULL, 10);
	pr_info("Modify charger time out to %ldS.\n", g_charger_para.chg_time_out);

	return len;
}
static ssize_t charging_limit_time_read(struct file *filp, char __user *buff, size_t len, loff_t *data)
{
	char value[8] = {0};
	int str_len = 0;

	str_len = snprintf(value, sizeof(value), "%ld", g_charger_para.chg_time_out);

	return simple_read_from_buffer(buff, len, data, value, str_len);
}

static const struct file_operations charging_limit_time_fops = {
	.owner		= THIS_MODULE,
	.read		= charging_limit_time_read,
	.write		= charging_limit_time_write,
};


int tbatt_pwroff_enable = 1;
#define OPPO_TBATT_HIGH_PWROFF_COUNT            (18)
#define OPPO_TBATT_EMERGENCY_PWROFF_COUNT        (6)
#define OPCHG_PWROFF_HIGH_BATT_TEMP		   770
#define OPCHG_PWROFF_EMERGENCY_BATT_TEMP      850
DECLARE_WAIT_QUEUE_HEAD(oppo_tbatt_pwroff_wq);
void oppo_tbatt_power_off_task_wakeup(void);

static ssize_t proc_tbatt_pwroff_write(struct file *filp, const char __user *buf, size_t len, loff_t *data)
{
	char buffer[2] = {0};

	if (len > 2) {
		return -EFAULT;
	}

	if (copy_from_user(buffer, buf, 2)) {
		printk("%s:  error.\n", __func__);
		return -EFAULT;
	}

	if (buffer[0] == '0') {
		tbatt_pwroff_enable = 0;
	} else {
		tbatt_pwroff_enable = 1;
		oppo_tbatt_power_off_task_wakeup();
	}
	printk("%s:tbatt_pwroff_enable = %d.\n", __func__, tbatt_pwroff_enable);

	return len;
}

static ssize_t proc_tbatt_pwroff_read(struct file *filp, char __user *buff, size_t count, loff_t *off)
{
	char page[256] = {0};
	char read_data[3] = {0};
	int len = 0;

	if (tbatt_pwroff_enable == 1) {
		read_data[0] = '1';
	} else {
	read_data[0] = '0';
	}
	read_data[1] = '\0';
	len = sprintf(page, "%s", read_data);
	if (len > *off) {
		len -= *off;
	} else {
		len = 0;
	}

	if (copy_to_user(buff, page, (len < count ? len : count))) {
		return -EFAULT;
	}
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

static const struct file_operations tbatt_pwroff_proc_fops = {
	.write = proc_tbatt_pwroff_write,
	.read = proc_tbatt_pwroff_read,
};

static int init_proc_tbatt_pwroff(void)
{
	struct proc_dir_entry *p = NULL;

	p = proc_create("tbatt_pwroff", 0664, NULL, &tbatt_pwroff_proc_fops);
	if (!p) {
		printk("proc_create  fail!\n");
	}
	return 0;
}

static int oppo_tbatt_power_off_kthread(void *arg)
{
	int over_temp_count = 0, emergency_count = 0;
	int batt_temp = 0;
	//struct oppo_chg_chip *chip = (struct oppo_chg_chip *)arg;
	struct sched_param param = {.sched_priority = MAX_RT_PRIO-1};
	union power_supply_propval val = {0, };

	power_supply_get_property(g_chg->batt_psy, POWER_SUPPLY_PROP_TEMP, &val);
	g_charger_para.batt_temp = val.intval;


	sched_setscheduler(current, SCHED_FIFO, &param);
	tbatt_pwroff_enable = 1;

	while (!kthread_should_stop()) {
		schedule_timeout_interruptible(round_jiffies_relative(msecs_to_jiffies(5*1000)));
		//printk(" tbatt_pwroff_enable:%d over_temp_count[%d] start\n", tbatt_pwroff_enable, over_temp_count);
		if (!tbatt_pwroff_enable) {
			emergency_count = 0;
			over_temp_count = 0;
			wait_event_interruptible(oppo_tbatt_pwroff_wq, tbatt_pwroff_enable == 1);
		}

		power_supply_get_property(g_chg->batt_psy, POWER_SUPPLY_PROP_TEMP, &val);
		batt_temp = val.intval;

		if (batt_temp > OPCHG_PWROFF_EMERGENCY_BATT_TEMP) {
			emergency_count++;
			printk(" emergency_count:%d \n", emergency_count);
		} else {
			emergency_count = 0;
		}
		if (batt_temp > OPCHG_PWROFF_HIGH_BATT_TEMP) {
			over_temp_count++;
			printk("over_temp_count[%d] \n", over_temp_count);
		} else {
			over_temp_count = 0;
		}
		//printk("over_temp_count[%d], chip->temperature[%d]\n", over_temp_count, batt_temp);

		if (over_temp_count >= OPPO_TBATT_HIGH_PWROFF_COUNT
				|| emergency_count >= OPPO_TBATT_EMERGENCY_PWROFF_COUNT) {
			printk("ERROR: battery temperature is too high, goto power off\n");
			///msleep(1000);
			machine_power_off();
		}
	}
	return 0;
}

int oppo_tbatt_power_off_task_init(struct smb_charger *chg)
{
	if (!chg) {
		return -1;
	}

	chg->tbatt_pwroff_task = kthread_create(oppo_tbatt_power_off_kthread, chg, "tbatt_pwroff");
	if (chg->tbatt_pwroff_task) {
	    wake_up_process(chg->tbatt_pwroff_task);
	} else {
		printk("ERROR: chg->tbatt_pwroff_task creat fail\n");
		return -1;
	}

	return 0;
}

void oppo_tbatt_power_off_task_wakeup(void)
{
	wake_up_interruptible(&oppo_tbatt_pwroff_wq);
	return;
}

void wake_chg_monitor_work(struct smb_charger *chg, int ms)
{
	if (charger_monitor_work_init == false) {
		return ;
	}
	g_charger_para.need_output_log = 1;
	ms = ms > 0 ? ms : 0;
	cancel_delayed_work_sync(&chg->charger_monitor_work);
	schedule_delayed_work(&chg->charger_monitor_work, msecs_to_jiffies(ms));
}

int init_chg_monitor_work(struct smb_charger *chg)
{
	struct proc_dir_entry *proc_entry_charger_cycle;
	struct proc_dir_entry *proc_entry_charging_limit_current;
	struct proc_dir_entry *proc_entry_charging_limit_time;
	struct proc_dir_entry *proc_entry_charger_critical_log;
	struct proc_dir_entry *p = NULL;

	g_chg = chg;

	memset(&g_charger_para, 0, sizeof(g_charger_para));
	g_charger_para.init_first = 1;
	g_charger_para.chg_ranges_len = sizeof(g_chg_ranges) / sizeof(struct smb_step_chg);
	g_charger_para.batt_healthd = POWER_SUPPLY_HEALTH_GOOD;
	g_charger_para.batt_healthd_save = -22;
	g_charger_para.chg_ranges_now = BATTERY_STATUS__NORMAL;
	g_charger_para.chg_ranges_save = -22;
	g_charger_para.chg_time_out = MAX_CHARGER_TIMEOUT;

	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190513, Modify lcd_is_on value to 1 at bootup */
	g_charger_para.lcd_is_on = 1;
	g_charger_para.lcd_on_chg_curr_proc = MAX_USB_ICL_CURRENT; //Set to max charger current
	chg->fb_notif.notifier_call = fb_notifier_callback;
	//fb_register_client(&chg->fb_notif);
	msm_drm_register_client(&chg->fb_notif);

	chg->notify_code = 0;
	//chg->charger_times = 0;// Can't clear here,maybe clear the boot-up value

	proc_entry_charger_cycle = proc_create_data("charger_cycle", S_IWUSR | S_IWGRP | S_IWOTH, NULL, &charger_cycle_fops, NULL);
	if (proc_entry_charger_cycle == NULL) {
		pr_err("[%s]: Error! Couldn't create proc entry charger_cycle!\n", __func__);
	}
	proc_entry_charging_limit_current = proc_create_data("charging_limit_current", 0666, NULL, &charging_limit_current_fops, NULL);
	if (proc_entry_charging_limit_current == NULL) {
		pr_err("[%s]: Error! Couldn't create proc entry charging_limit_current!\n", __func__);
	}
	proc_entry_charging_limit_time = proc_create_data("charging_limit_time", 0666, NULL, &charging_limit_time_fops, NULL);
	if (proc_entry_charging_limit_time == NULL) {
		pr_err("[%s]: Error! Couldn't create proc entry charging_limit_time!\n", __func__);
	}

	proc_entry_charger_critical_log = proc_create_data("charger_critical_log", 0666, NULL, &chg_critical_log_proc_fops, NULL);
	if (proc_entry_charger_cycle == NULL) {
		pr_err("[%s]: Error! Couldn't create proc entry charger_critical_log!\n", __func__);
	}

	p = proc_create("rtc_reset", 0664, NULL, &rtc_reset_det_fops);
	if (!p) {
		pr_err("proc_create rtc_reset_det_fops fail!\n");
	}
	INIT_DELAYED_WORK(&chg->charger_monitor_work, smblib_charger_monitor_work);
	schedule_delayed_work(&chg->charger_monitor_work, msecs_to_jiffies(500));

	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190521, Add for kernel shutdown when high temperature */
	init_proc_tbatt_pwroff();
	oppo_tbatt_power_off_task_init(chg);

	return 0;
}

int deinit_chg_monitor_work(struct smb_charger *chg)
{
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1372106, 20180615, Add lcd on-off control usb icl */
	fb_unregister_client(&chg->fb_notif);

	cancel_delayed_work_sync(&chg->charger_monitor_work);

	return 0;
}
#endif /* ODM_WT_EDIT */