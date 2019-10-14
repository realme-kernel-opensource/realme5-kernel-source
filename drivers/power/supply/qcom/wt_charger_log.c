/*******************************************************************************
 *  Copyright (C), 2008-2018, OPPO Mobile Comm Corp., Ltd.
 *  ODM_WT_EDIT
 *  FILE: - wt_charger_log.c
 *  Description : Add charger log
 *  Version: 1.0
 *  Date : 2018/5/22
 *  Author: Bin2.Zhang@ODM_WT.BSP.Charger.Basic
 *
 *  -------------------------Revision History:----------------------------------
 *   <author>	 <data> 	<version >			<desc>
 *  Bin2.Zhang	2018/5/22	1.0				Add charger log
 ******************************************************************************/
/*********************************************************************************************
 * V0.1 zhangbin2 20170817 add charger & battery log
 * V1.0 zhangbin2 20171027 Correct g_smbchg_chip to g_smbchg_chg, modify with power_supply.h
**********************************************************************************************/
#ifdef ODM_WT_EDIT
#ifdef __WT_BATTERY_CHARGER_LOG_OUTPUT__

//#define DEBUG
#define pr_fmt(fmt) "WT-FG-CHG: " fmt

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
//#include "fg-core.h"
//#include "fg-reg.h"

#define LOG_DELAY_MS 5000

// Kmesg max line was control by LOG_LINE_MAX, printk.c
//#define LOG_LINE_MAX		(1024 - PREFIX_MAX)
#define MAX_LOG_LEN (4096)
#define MAX_PROP_LEN (64)
static char fg_chg_log_buf[MAX_LOG_LEN];
static int fg_chg_log_buf_offset = 0;
struct smb_charger *g_smbchg_chg = NULL;
//struct smb138x *g_smb138x_chip = NULL;

static int log_delay_ms = 0;
module_param_named(
	log_delay_ms, log_delay_ms, int, S_IRUSR | S_IWUSR
);

/*
  0: Don't output
  1: Output to kernel log
  2: Output to class file only. log_delay_ms was un-used in this mode.
*/
static int log_output_mode = 1;

#define POWER_SUPPLY_NAME(_name) #_name
/* Must be in the same order as POWER_SUPPLY_PROP_* */
static char *power_supply_name[] = {
	/* Properties of type `int' */
	POWER_SUPPLY_NAME(status),
	POWER_SUPPLY_NAME(charge_type),
	POWER_SUPPLY_NAME(health),
	POWER_SUPPLY_NAME(present),
	POWER_SUPPLY_NAME(online),
	POWER_SUPPLY_NAME(authentic),
	POWER_SUPPLY_NAME(technology),
	POWER_SUPPLY_NAME(cycle_count),
	POWER_SUPPLY_NAME(voltage_max),
	POWER_SUPPLY_NAME(voltage_min),
	POWER_SUPPLY_NAME(voltage_max_design),
	POWER_SUPPLY_NAME(voltage_min_design),
	POWER_SUPPLY_NAME(voltage_now),
	POWER_SUPPLY_NAME(voltage_avg),
	POWER_SUPPLY_NAME(voltage_ocv),
	POWER_SUPPLY_NAME(voltage_boot),
	POWER_SUPPLY_NAME(current_max),
	POWER_SUPPLY_NAME(current_now),
	POWER_SUPPLY_NAME(current_avg),
	POWER_SUPPLY_NAME(current_boot),
	POWER_SUPPLY_NAME(power_now),
	POWER_SUPPLY_NAME(power_avg),
	POWER_SUPPLY_NAME(charge_full_design),
	POWER_SUPPLY_NAME(charge_empty_design),
	POWER_SUPPLY_NAME(charge_full),
	POWER_SUPPLY_NAME(charge_empty),
	POWER_SUPPLY_NAME(charge_now),
	POWER_SUPPLY_NAME(charge_avg),
	POWER_SUPPLY_NAME(charge_counter),
	POWER_SUPPLY_NAME(constant_charge_current),
	POWER_SUPPLY_NAME(constant_charge_current_max),
	POWER_SUPPLY_NAME(constant_charge_voltage),
	POWER_SUPPLY_NAME(constant_charge_voltage_max),
	POWER_SUPPLY_NAME(charge_control_limit),
	POWER_SUPPLY_NAME(charge_control_limit_max),
	POWER_SUPPLY_NAME(input_current_limit),
	POWER_SUPPLY_NAME(energy_full_design),
	POWER_SUPPLY_NAME(energy_empty_design),
	POWER_SUPPLY_NAME(energy_full),
	POWER_SUPPLY_NAME(energy_empty),
	POWER_SUPPLY_NAME(energy_now),
	POWER_SUPPLY_NAME(energy_avg),
	POWER_SUPPLY_NAME(capacity),
	POWER_SUPPLY_NAME(capacity_alert_min),
	POWER_SUPPLY_NAME(capacity_alert_max),
	POWER_SUPPLY_NAME(capacity_level),
	POWER_SUPPLY_NAME(temp),
	POWER_SUPPLY_NAME(temp_max),
	POWER_SUPPLY_NAME(temp_min),
	POWER_SUPPLY_NAME(temp_alert_min),
	POWER_SUPPLY_NAME(temp_alert_max),
	POWER_SUPPLY_NAME(temp_ambient),
	POWER_SUPPLY_NAME(temp_ambient_alert_min),
	POWER_SUPPLY_NAME(temp_ambient_alert_max),
	POWER_SUPPLY_NAME(time_to_empty_now),
	POWER_SUPPLY_NAME(time_to_empty_avg),
	POWER_SUPPLY_NAME(time_to_full_now),
	POWER_SUPPLY_NAME(time_to_full_avg),
	POWER_SUPPLY_NAME(type),
	POWER_SUPPLY_NAME(scope),
	POWER_SUPPLY_NAME(precharge_current),
	POWER_SUPPLY_NAME(charge_term_current),
	POWER_SUPPLY_NAME(calibrate),
	/* Local extensions */
	POWER_SUPPLY_NAME(usb_hc),
	POWER_SUPPLY_NAME(usb_otg),
	POWER_SUPPLY_NAME(charge_enabled),
	POWER_SUPPLY_NAME(set_ship_mode),
	POWER_SUPPLY_NAME(real_type),
	POWER_SUPPLY_NAME(charge_now_raw),
	POWER_SUPPLY_NAME(charge_now_error),
	POWER_SUPPLY_NAME(capacity_raw),
	POWER_SUPPLY_NAME(battery_charging_enabled),
	POWER_SUPPLY_NAME(charging_enabled),
	POWER_SUPPLY_NAME(step_charging_enabled),
	POWER_SUPPLY_NAME(step_charging_step),
	POWER_SUPPLY_NAME(pin_enabled),
	POWER_SUPPLY_NAME(input_suspend),
	POWER_SUPPLY_NAME(input_voltage_regulation),
	POWER_SUPPLY_NAME(input_current_max),
	POWER_SUPPLY_NAME(input_current_trim),
	POWER_SUPPLY_NAME(input_current_settled),
	POWER_SUPPLY_NAME(input_voltage_settled),
	POWER_SUPPLY_NAME(bypass_vchg_loop_debouncer),
	POWER_SUPPLY_NAME(charge_counter_shadow),
	POWER_SUPPLY_NAME(hi_power),
	POWER_SUPPLY_NAME(low_power),
	POWER_SUPPLY_NAME(temp_cool),
	POWER_SUPPLY_NAME(temp_warm),
	POWER_SUPPLY_NAME(temp_cold),
	POWER_SUPPLY_NAME(temp_hot),
	POWER_SUPPLY_NAME(system_temp_level),
	POWER_SUPPLY_NAME(resistance),
	POWER_SUPPLY_NAME(resistance_capacitive),
	POWER_SUPPLY_NAME(resistance_id),
	POWER_SUPPLY_NAME(resistance_now),
	POWER_SUPPLY_NAME(flash_current_max),
	POWER_SUPPLY_NAME(update_now),
	POWER_SUPPLY_NAME(esr_count),
	POWER_SUPPLY_NAME(buck_freq),
	POWER_SUPPLY_NAME(boost_current),
	POWER_SUPPLY_NAME(safety_timer_enabled),
	POWER_SUPPLY_NAME(charge_done),
	POWER_SUPPLY_NAME(flash_active),
	POWER_SUPPLY_NAME(flash_trigger),
	POWER_SUPPLY_NAME(force_tlim),
	POWER_SUPPLY_NAME(dp_dm),
	POWER_SUPPLY_NAME(input_current_limited),
	POWER_SUPPLY_NAME(input_current_now),
	POWER_SUPPLY_NAME(charge_qnovo_enable),
	POWER_SUPPLY_NAME(current_qnovo),
	POWER_SUPPLY_NAME(voltage_qnovo),
	POWER_SUPPLY_NAME(rerun_aicl),
	POWER_SUPPLY_NAME(cycle_count_id),
	POWER_SUPPLY_NAME(safety_timer_expired),
	POWER_SUPPLY_NAME(restricted_charging),
	POWER_SUPPLY_NAME(current_capability),
	POWER_SUPPLY_NAME(typec_mode),
	POWER_SUPPLY_NAME(typec_cc_orientation),
	POWER_SUPPLY_NAME(typec_power_role),
	POWER_SUPPLY_NAME(typec_src_rp),
	POWER_SUPPLY_NAME(pd_allowed),
	POWER_SUPPLY_NAME(pd_active),
	POWER_SUPPLY_NAME(pd_in_hard_reset),
	POWER_SUPPLY_NAME(pd_current_max),
	POWER_SUPPLY_NAME(pd_usb_suspend_supported),
	POWER_SUPPLY_NAME(charger_temp),
	POWER_SUPPLY_NAME(charger_temp_max),
	POWER_SUPPLY_NAME(parallel_disable),
	POWER_SUPPLY_NAME(pe_start),
	POWER_SUPPLY_NAME(soc_reporting_ready),
	POWER_SUPPLY_NAME(debug_battery),
	POWER_SUPPLY_NAME(fcc_delta),
	POWER_SUPPLY_NAME(icl_reduction),
	POWER_SUPPLY_NAME(parallel_mode),
	POWER_SUPPLY_NAME(die_health),
	POWER_SUPPLY_NAME(connector_health),
	POWER_SUPPLY_NAME(ctm_current_max),
	POWER_SUPPLY_NAME(hw_current_max),
#ifdef ODM_WT_EDIT
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190416, Add for charging sysfs. */
	POWER_SUPPLY_NAME(StopCharging_Test),
	POWER_SUPPLY_NAME(StartCharging_Test),
	POWER_SUPPLY_NAME(adapter_fw_update),
	POWER_SUPPLY_NAME(authenticate),
	POWER_SUPPLY_NAME(batt_cc),
	POWER_SUPPLY_NAME(batt_fcc),
	POWER_SUPPLY_NAME(batt_rm),
	POWER_SUPPLY_NAME(batt_soh),
	POWER_SUPPLY_NAME(charge_technology),
	POWER_SUPPLY_NAME(charge_timeout),
	POWER_SUPPLY_NAME(chargerid_volt),
	POWER_SUPPLY_NAME(fastcharger),
	POWER_SUPPLY_NAME(mmi_charging_enable),
	POWER_SUPPLY_NAME(voocchg_ing),
	POWER_SUPPLY_NAME(notify_code),
	POWER_SUPPLY_NAME(call_mode),
	POWER_SUPPLY_NAME(battery_info),
	POWER_SUPPLY_NAME(battery_info_id),
	POWER_SUPPLY_NAME(soc_notify_ready),
	POWER_SUPPLY_NAME(ui_soc),
	POWER_SUPPLY_NAME(real_status),
	POWER_SUPPLY_NAME(recharge_uv),
#endif /* ODM_WT_EDIT */
	#ifdef ODM_WT_EDIT
	/*Haibin1.zhang@ODM_WT.BSP.Storage.otg, 2019/04/18, Add for otg configuration */
	POWER_SUPPLY_NAME(otg_switch),
	POWER_SUPPLY_NAME(otg_online),
	#endif /* ODM_WT_EDIT */
	POWER_SUPPLY_NAME(pr_swap),
	POWER_SUPPLY_NAME(cc_step),
	POWER_SUPPLY_NAME(cc_step_sel),
	POWER_SUPPLY_NAME(sw_jeita_enabled),
	POWER_SUPPLY_NAME(pd_voltage_max),
	POWER_SUPPLY_NAME(pd_voltage_min),
	POWER_SUPPLY_NAME(sdp_current_max),
	POWER_SUPPLY_NAME(connector_type),
	POWER_SUPPLY_NAME(parallel_batfet_mode),
	POWER_SUPPLY_NAME(parallel_fcc_max),
	POWER_SUPPLY_NAME(min_icl),
	POWER_SUPPLY_NAME(moisture_detected),
	POWER_SUPPLY_NAME(batt_profile_version),
	POWER_SUPPLY_NAME(batt_full_current),
	POWER_SUPPLY_NAME(recharge_soc),
	POWER_SUPPLY_NAME(hvdcp_opti_allowed),
	POWER_SUPPLY_NAME(smb_en_mode),
	POWER_SUPPLY_NAME(smb_en_reason),
	POWER_SUPPLY_NAME(esr_actual),
	POWER_SUPPLY_NAME(esr_nominal),
	POWER_SUPPLY_NAME(soh),
	POWER_SUPPLY_NAME(clear_soh),
	POWER_SUPPLY_NAME(force_recharge),
	POWER_SUPPLY_NAME(fcc_stepper_enable),
	POWER_SUPPLY_NAME(toggle_stat),
	POWER_SUPPLY_NAME(main_fcc_max),
	POWER_SUPPLY_NAME(fg_reset),
	POWER_SUPPLY_NAME(qc_opti_disable),
	POWER_SUPPLY_NAME(cc_soc),
	POWER_SUPPLY_NAME(batt_age_level),
	POWER_SUPPLY_NAME(voltage_vph),
	POWER_SUPPLY_NAME(chip_version),
	POWER_SUPPLY_NAME(therm_icl_limit),
	POWER_SUPPLY_NAME(dc_reset),
	POWER_SUPPLY_NAME(voltage_max_limit),
	POWER_SUPPLY_NAME(real_capacity),
	/* Charge pump properties */
	POWER_SUPPLY_NAME(cp_status1),
	POWER_SUPPLY_NAME(cp_status2),
	POWER_SUPPLY_NAME(cp_enable),
	POWER_SUPPLY_NAME(cp_switcher_en),
	POWER_SUPPLY_NAME(cp_die_temp),
	POWER_SUPPLY_NAME(cp_isns),
	POWER_SUPPLY_NAME(cp_toggle_switcher),
	POWER_SUPPLY_NAME(cp_irq_status),
	POWER_SUPPLY_NAME(cp_ilim),
	/* Local extensions of type int64_t */
	POWER_SUPPLY_NAME(charge_counter_ext),
	/* Properties of type `const char *' */
	POWER_SUPPLY_NAME(model_name),
	POWER_SUPPLY_NAME(manufacturer),
	POWER_SUPPLY_NAME(serial_number),
	POWER_SUPPLY_NAME(battery_type),
	POWER_SUPPLY_NAME(cycle_counts),
};

static enum power_supply_property smb5_batt_props[] = {
	POWER_SUPPLY_PROP_INPUT_SUSPEND,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CHARGER_TEMP,
	POWER_SUPPLY_PROP_CHARGER_TEMP_MAX,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMITED,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_QNOVO,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_QNOVO,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
	POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_STEP_CHARGING_ENABLED,
	POWER_SUPPLY_PROP_SW_JEITA_ENABLED,
	POWER_SUPPLY_PROP_CHARGE_DONE,
	POWER_SUPPLY_PROP_PARALLEL_DISABLE,
	//POWER_SUPPLY_PROP_SET_SHIP_MODE,
	POWER_SUPPLY_PROP_DIE_HEALTH,
	POWER_SUPPLY_PROP_RERUN_AICL,
	POWER_SUPPLY_PROP_DP_DM,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
	//POWER_SUPPLY_PROP_CHARGE_COUNTER,
	//POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_RECHARGE_SOC,
	//POWER_SUPPLY_PROP_CHARGE_FULL,
	//POWER_SUPPLY_PROP_FORCE_RECHARGE,
	POWER_SUPPLY_PROP_FCC_STEPPER_ENABLE,
#ifdef ODM_WT_EDIT
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190416, Add for step re-charger vbatt setting */
	POWER_SUPPLY_PROP_RECHARGE_UV,
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190416, Add start/stop charging property */
	//POWER_SUPPLY_PROP_STOPCHARGING_TEST,
	//POWER_SUPPLY_PROP_STARTCHARGING_TEST,
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190416, Add for factory mode test */
	//POWER_SUPPLY_PROP_ADAPTER_FW_UPDATE,
	POWER_SUPPLY_PROP_AUTHENTICATE,
	//POWER_SUPPLY_PROP_BATT_CC,
	//POWER_SUPPLY_PROP_BATT_FCC,
	//POWER_SUPPLY_PROP_BATT_RM,
	//POWER_SUPPLY_PROP_BATT_SOH,
	//POWER_SUPPLY_PROP_CHARGE_NOW,
	//POWER_SUPPLY_PROP_CHARGE_TECHNOLOGY,
	POWER_SUPPLY_PROP_CHARGE_TIMEOUT,
	//POWER_SUPPLY_PROP_CHARGERID_VOLT,
	//POWER_SUPPLY_PROP_FASTCHARGER,
	POWER_SUPPLY_PROP_MMI_CHARGING_ENABLE,
	//POWER_SUPPLY_PROP_STEP_CHARGING_STEP,
	//POWER_SUPPLY_PROP_VOOCCHG_ING,
	POWER_SUPPLY_PROP_NOTIFY_CODE,
	POWER_SUPPLY_PROP_CALL_MODE,
	//POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_REAL_STATUS,
#endif /* ODM_WT_EDIT */
};

static enum power_supply_property smb5_usb_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	//POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_PD_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_TYPE,
	//POWER_SUPPLY_PROP_TYPEC_MODE,
	//POWER_SUPPLY_PROP_TYPEC_POWER_ROLE,
	//POWER_SUPPLY_PROP_TYPEC_CC_ORIENTATION,
	POWER_SUPPLY_PROP_LOW_POWER,
	POWER_SUPPLY_PROP_PD_ACTIVE,
	POWER_SUPPLY_PROP_INPUT_CURRENT_SETTLED,
	POWER_SUPPLY_PROP_INPUT_CURRENT_NOW,
	POWER_SUPPLY_PROP_BOOST_CURRENT,
	POWER_SUPPLY_PROP_PE_START,
	POWER_SUPPLY_PROP_CTM_CURRENT_MAX,
	POWER_SUPPLY_PROP_HW_CURRENT_MAX,
	POWER_SUPPLY_PROP_REAL_TYPE,
	POWER_SUPPLY_PROP_PD_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_PD_VOLTAGE_MIN,
	//POWER_SUPPLY_PROP_CONNECTOR_TYPE,
	//POWER_SUPPLY_PROP_CONNECTOR_HEALTH,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	//POWER_SUPPLY_PROP_SMB_EN_MODE,
	//POWER_SUPPLY_PROP_SMB_EN_REASON,
	POWER_SUPPLY_PROP_SCOPE,
	POWER_SUPPLY_PROP_MOISTURE_DETECTED,
	POWER_SUPPLY_PROP_HVDCP_OPTI_ALLOWED,
	POWER_SUPPLY_PROP_QC_OPTI_DISABLE,
	POWER_SUPPLY_PROP_VOLTAGE_VPH,
	POWER_SUPPLY_PROP_THERM_ICL_LIMIT,
};
#if 0
static enum power_supply_property smb2_dc_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_REAL_TYPE,
};
#endif
static enum power_supply_property qg_psy_props[] = {
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_REAL_CAPACITY,
	//POWER_SUPPLY_PROP_TEMP,
	//POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_OCV,
	//POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_RESISTANCE,
	POWER_SUPPLY_PROP_RESISTANCE_ID,
	POWER_SUPPLY_PROP_RESISTANCE_NOW,
	POWER_SUPPLY_PROP_SOC_REPORTING_READY,
	POWER_SUPPLY_PROP_RESISTANCE_CAPACITIVE,
	POWER_SUPPLY_PROP_DEBUG_BATTERY,
	POWER_SUPPLY_PROP_BATTERY_TYPE,
	POWER_SUPPLY_PROP_VOLTAGE_MIN,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_BATT_FULL_CURRENT,
	POWER_SUPPLY_PROP_BATT_PROFILE_VERSION,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_CYCLE_COUNTS,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_TIME_TO_FULL_AVG,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,
	POWER_SUPPLY_PROP_ESR_ACTUAL,
	POWER_SUPPLY_PROP_ESR_NOMINAL,
	POWER_SUPPLY_PROP_SOH,
	POWER_SUPPLY_PROP_FG_RESET,
	POWER_SUPPLY_PROP_CC_SOC,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
#ifdef ODM_WT_EDIT
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190416, Add for factory mode test */
	POWER_SUPPLY_PROP_AUTHENTICATE,
	//POWER_SUPPLY_PROP_BATT_CC,
	//POWER_SUPPLY_PROP_BATT_FCC,
	/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190416, Add for store SOC */
	//POWER_SUPPLY_PROP_SOC_REPORTING_READY,
	//POWER_SUPPLY_PROP_BATTERY_INFO,
	//POWER_SUPPLY_PROP_BATTERY_INFO_ID,
	POWER_SUPPLY_PROP_SOC_NOTIFY_READY,
#endif /* ODM_WT_EDIT */
};

static enum power_supply_property smb5_usb_main_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_INPUT_CURRENT_SETTLED,
	POWER_SUPPLY_PROP_INPUT_VOLTAGE_SETTLED,
	//POWER_SUPPLY_PROP_FCC_DELTA,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_FLASH_ACTIVE,
	POWER_SUPPLY_PROP_FLASH_TRIGGER,
	POWER_SUPPLY_PROP_TOGGLE_STAT,
	POWER_SUPPLY_PROP_MAIN_FCC_MAX,
};
#if 1
static enum power_supply_property smb5_usb_port_props[] = {
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CURRENT_MAX,
};
#endif
static enum power_supply_property smb138x_parallel_props[] = {
};

static int fg_chg_logout_head(void);
static int set_log_output_mode(const char *val, const struct kernel_param *kp)
{
	int rc;

	rc = param_set_int(val, kp);
	if (rc) {
		pr_err("Unable to set fg_restart: %d\n", rc);
		return rc;
	}

	if (log_output_mode < 0 || log_output_mode > 2) {
		pr_err("Bad log_output_mode value %d\n", log_output_mode);
		return -EINVAL;
	}

	if (log_output_mode == 0)
		cancel_delayed_work_sync(&g_smbchg_chg->log_output_work);
	else if (log_output_mode == 1) {
		cancel_delayed_work_sync(&g_smbchg_chg->log_output_work);
		fg_chg_logout_head();
		schedule_delayed_work(&g_smbchg_chg->log_output_work, 0);
	} if (log_output_mode == 2)
		cancel_delayed_work_sync(&g_smbchg_chg->log_output_work);

	pr_info("log_output_mode modify to %d done\n", log_output_mode);
	return rc;
}

static struct kernel_param_ops log_output_mode_ops = {
	.set = set_log_output_mode,
	.get = param_get_int,
};

module_param_cb(log_output_mode, &log_output_mode_ops, &log_output_mode, 0644);

static const char * const power_supply_type_text[] = {
	"Unknown", "Battery", "UPS", "Mains", "USB",
	"USB_DCP", "USB_CDP", "USB_ACA", "USB_C",
	"USB_PD", "USB_PD_DRP", "BrickID",
	"USB_HVDCP", "USB_HVDCP_3", "Wireless", "USB_FLOAT",
	"BMS", "Parallel", "Main", "Wipower", "USB_C_UFP", "USB_C_DFP",
	"Charge_Pump",
};

static const char * const power_supply_status_text[] = {
	"Unknown", "Charging", "Discharging", "Not charging", "Full"
};

static const char * const power_supply_charge_type_text[] = {
	"Unknown", "N/A", "Trickle", "Fast", "Taper"
};

static const char * const power_supply_health_text[] = {
	"Unknown", "Good", "Overheat", "Dead", "Over voltage",
	"Unspecified failure", "Cold", "Watchdog timer expire",
	"Safety timer expire",
	"Warm", "Cool", "Hot"
};

static const char * const power_supply_technology_text[] = {
	"Unknown", "NiMH", "Li-ion", "Li-poly", "LiFe", "NiCd",
	"LiMn"
};

static const char * const power_supply_capacity_level_text[] = {
	"Unknown", "Critical", "Low", "Normal", "High", "Full"
};

static const char * const power_supply_scope_text[] = {
	"Unknown", "System", "Device"
};

static const char * const power_supply_usbc_text[] = {
	"Nothing attached", "Sink attached", "Powered cable w/ sink",
	"Debug Accessory", "Audio Adapter", "Powered cable w/o sink",
	"Source attached (default current)",
	"Source attached (medium current)",
	"Source attached (high current)",
	"Non compliant",
};

static const char * const power_supply_usbc_pr_text[] = {
	"none", "dual power role", "sink", "source"
};

static const char * const power_supply_typec_src_rp_text[] = {
	"Rp-Default", "Rp-1.5A", "Rp-3A"
};

static int log_output_psy(struct power_supply *psy, enum power_supply_property *props, int len)
{
	ssize_t ret = 0;
	union power_supply_propval value;
	int j = 0;

	//if (psy == NULL)
	//	return -EFAULT;

	if ((psy != NULL) && (fg_chg_log_buf_offset + strlen(psy->desc->name) + 3 < MAX_LOG_LEN))
		fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "-%s-,", psy->desc->name);
	else
		fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "--,");
		//return -EINVAL;

	for (j = 0; j < len; j++) {
		if (psy == NULL) {
			fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, ",");
			pr_debug("power supply was NULL!\n");
			continue;
		}
		if (fg_chg_log_buf_offset + MAX_PROP_LEN < MAX_LOG_LEN) {
			//value.intval = 0; // Output space when read error,do not output 0.
			ret = power_supply_get_property(psy, props[j], &value);
			if (ret < 0) {
				if (ret == -ENODATA)
					pr_debug("driver has no data for `%s' property\n", power_supply_name[j]);
				else if (ret != -ENODEV)
					pr_debug("driver failed to report `%s' property: %zd\n", power_supply_name[j], ret);
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, ",");
				continue;
				//return ret;
			}

			if (props[j] == POWER_SUPPLY_PROP_STATUS)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_status_text[value.intval]);
		#ifdef ODM_WT_EDIT
			/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190416, Add for ui_soc */
			else if (props[j] == POWER_SUPPLY_PROP_REAL_STATUS)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_status_text[value.intval]);
		#endif /* ODM_WT_EDIT */
			else if (props[j] == POWER_SUPPLY_PROP_CHARGE_TYPE)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_charge_type_text[value.intval]);
			else if (props[j] == POWER_SUPPLY_PROP_HEALTH)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_health_text[value.intval]);
			else if (props[j] == POWER_SUPPLY_PROP_TECHNOLOGY)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_technology_text[value.intval]);
			else if (props[j] == POWER_SUPPLY_PROP_CAPACITY_LEVEL)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_capacity_level_text[value.intval]);
			else if (props[j] == POWER_SUPPLY_PROP_TYPE ||
					props[j] == POWER_SUPPLY_PROP_REAL_TYPE)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_type_text[value.intval]);
			else if (props[j] == POWER_SUPPLY_PROP_SCOPE)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_scope_text[value.intval]);
			else if (props[j] == POWER_SUPPLY_PROP_TYPEC_MODE)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_usbc_text[value.intval]);
			else if (props[j] == POWER_SUPPLY_PROP_TYPEC_POWER_ROLE)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_usbc_pr_text[value.intval]);
			else if (props[j] == POWER_SUPPLY_PROP_TYPEC_SRC_RP)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_typec_src_rp_text[value.intval]);
			else if (props[j] == POWER_SUPPLY_PROP_DIE_HEALTH)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_health_text[value.intval]);
			else if (props[j] == POWER_SUPPLY_PROP_CONNECTOR_HEALTH)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_health_text[value.intval]);
			else if (props[j] >= POWER_SUPPLY_PROP_MODEL_NAME)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", value.strval);
			else if (props[j] == POWER_SUPPLY_PROP_CHARGE_COUNTER_EXT)
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%lld,", value.int64val);
			else
				fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%d,", value.intval);
		} else {
			return -EINVAL;
		}
	}

	return 0;
}

static int log_output_psy_head(struct power_supply *psy, enum power_supply_property *props, int len)
{
	int j = 0;

	//if (psy == NULL)
	//	return -EFAULT;

	if ((psy != NULL) && (fg_chg_log_buf_offset + strlen(psy->desc->name) + 3 < MAX_LOG_LEN))
		fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "-%s-,", psy->desc->name);
	else {
		fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "--,");
		//return -EINVAL;
	}

	for (j = 0; j < len; j++) {
		if (fg_chg_log_buf_offset + MAX_PROP_LEN < MAX_LOG_LEN) {
			fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "%s,", power_supply_name[props[j]]);
		} else {
			return -EINVAL;
		}
	}

	return 0;
}

static int chgr[] = {0x6, 0x7, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0x42, 0x51, 0x61, 0x63, 0x66, 0x70, 0x74, 0x7d, 0x7e, 0x7f};
static int otg[] = {0x7, 0xa, 0xb, 0x40, 0x51, 0x52};
static int batif[] = {0x6, 0x10, 0x70, 0x71, 0x86};
static int usb[] = {0x6, 0x7, 0x8, 0x9, 0x10, 0x40, 0x66, 0x70, 0x80};
static int misc[] = {0x10};
static int bmd[] = {0x44, 0x46};
static int qg[] = {0x8, 0x9};

#define CHGR_NAME	 "chgr"
#define OTG_NAME	 "otg"
#define BATIF_NAME	 "batif"
#define USBIN_NAME	 "usb"
#define DCIN_NAME	 "dc"
#define MISC_NAME	 "misc"
#define USB_PORT_NAME	 "usb_port"
#define BMD_NAME	 "bmd"
#define QG_NAME	 	 "qg"

#define OTG_BASE		0x1100
#define BMD_BASE		0x1A00
#define QG_BASE			0x4800

static int log_output_reg(char *reg_name, int reg_base, int *reg, int len)
{
	int j = 0;
	u8 val;
	int rc = 0;

	//if (g_smbchg_chg == NULL)
	//	return -EFAULT;

	fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "-%s-,", reg_name);

	for (j = 0; j < len; j++) {
		if (g_smbchg_chg == NULL) {
			pr_debug("chip reg map -g_smbchg_chg- was NULL!\n");
			fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, ",");
			continue;
		}
		if (fg_chg_log_buf_offset + MAX_PROP_LEN < MAX_LOG_LEN) {
			rc = smblib_read(g_smbchg_chg, reg_base + reg[j], &val);
			if (rc < 0)
				val = 0;
			fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "0x%x,", val);
		} else {
			return -EINVAL;
		}
	}

	return 0;
}

static int log_output_reg_head(char *reg_name, int reg_base, int *reg, int len)
{
	int j = 0;

	fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "-%s-,", reg_name);

	for (j = 0; j < len; j++) {
		if (fg_chg_log_buf_offset + MAX_PROP_LEN < MAX_LOG_LEN) {
			fg_chg_log_buf_offset += snprintf(fg_chg_log_buf + fg_chg_log_buf_offset, MAX_PROP_LEN, "0x%x,", reg_base + reg[j]);
		} else {
			return -EINVAL;
		}
	}

	return 0;
}

static void log_output(void)
{
	if (log_output_mode == 0)
		;
	else if (log_output_mode == 1)
		pr_info("%s\n", fg_chg_log_buf);
	else if (log_output_mode == 2)
		;

	fg_chg_log_buf[0] = '\0';
	//memset(fg_chg_log_buf, '\0', MAX_LOG_LEN);
	fg_chg_log_buf_offset = 0;
}

static ssize_t log_show(struct class *c, struct class_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	if ((g_smbchg_chg == NULL) || (log_output_mode != 2))
		return 0;

	// Main charger power_supply
	log_output_psy(g_smbchg_chg->bms_psy, qg_psy_props, ARRAY_SIZE(qg_psy_props));
	log_output_psy(g_smbchg_chg->batt_psy, smb5_batt_props, ARRAY_SIZE(smb5_batt_props));
	log_output_psy(g_smbchg_chg->usb_psy, smb5_usb_props, ARRAY_SIZE(smb5_usb_props));
	//log_output_psy(g_smbchg_chg->dc_psy, smb2_dc_props, ARRAY_SIZE(smb2_dc_props));
	log_output_psy(g_smbchg_chg->usb_main_psy, smb5_usb_main_props, ARRAY_SIZE(smb5_usb_main_props));
	log_output_psy(g_smbchg_chg->usb_port_psy, smb5_usb_port_props, ARRAY_SIZE(smb5_usb_port_props));

	// Parallel charger power_supply
	log_output_psy(g_smbchg_chg->pl.psy, smb138x_parallel_props, ARRAY_SIZE(smb138x_parallel_props));

	log_output_reg(CHGR_NAME, CHGR_BASE, chgr, ARRAY_SIZE(chgr));
	log_output_reg(OTG_NAME, OTG_BASE, otg, ARRAY_SIZE(otg));
	log_output_reg(BATIF_NAME, BATIF_BASE, batif, ARRAY_SIZE(batif));
	log_output_reg(USBIN_NAME, USBIN_BASE, usb, ARRAY_SIZE(usb));
	log_output_reg(MISC_NAME, MISC_BASE, misc, ARRAY_SIZE(misc));
	log_output_reg(BMD_NAME, BMD_BASE, bmd, ARRAY_SIZE(bmd));
	log_output_reg(QG_NAME, QG_BASE, qg, ARRAY_SIZE(qg));

	ret = snprintf(buf, PAGE_SIZE, "%s\n", fg_chg_log_buf);

	log_output();

	return ret;
}
static CLASS_ATTR_RO(log);

static ssize_t log_head_show(struct class *c, struct class_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	if (/*(g_smbchg_chg == NULL) || */(log_output_mode != 2))
		return 0;

	// Main charger power_supply
	log_output_psy_head(g_smbchg_chg ? g_smbchg_chg->bms_psy : NULL, qg_psy_props, ARRAY_SIZE(qg_psy_props));
	log_output_psy_head(g_smbchg_chg ? g_smbchg_chg->batt_psy : NULL, smb5_batt_props, ARRAY_SIZE(smb5_batt_props));
	log_output_psy_head(g_smbchg_chg ? g_smbchg_chg->usb_psy : NULL, smb5_usb_props, ARRAY_SIZE(smb5_usb_props));
	//log_output_psy_head(g_smbchg_chg ? g_smbchg_chg->dc_psy : NULL, smb2_dc_props, ARRAY_SIZE(smb2_dc_props));
	log_output_psy_head(g_smbchg_chg ? g_smbchg_chg->usb_main_psy : NULL, smb5_usb_main_props, ARRAY_SIZE(smb5_usb_main_props));
	log_output_psy_head(g_smbchg_chg ? g_smbchg_chg->usb_port_psy : NULL, smb5_usb_port_props, ARRAY_SIZE(smb5_usb_port_props));

	// Parallel charger power_supply
	log_output_psy_head(g_smbchg_chg ? g_smbchg_chg->pl.psy : NULL, smb138x_parallel_props, ARRAY_SIZE(smb138x_parallel_props));

	log_output_reg_head(CHGR_NAME, CHGR_BASE, chgr, ARRAY_SIZE(chgr));
	log_output_reg_head(OTG_NAME, OTG_BASE, otg, ARRAY_SIZE(otg));
	log_output_reg_head(BATIF_NAME, BATIF_BASE, batif, ARRAY_SIZE(batif));
	log_output_reg_head(USBIN_NAME, USBIN_BASE, usb, ARRAY_SIZE(usb));
	log_output_reg_head(MISC_NAME, MISC_BASE, misc, ARRAY_SIZE(misc));
	log_output_reg_head(BMD_NAME, BMD_BASE, bmd, ARRAY_SIZE(bmd));
	log_output_reg_head(QG_NAME, QG_BASE, qg, ARRAY_SIZE(qg));

	ret = snprintf(buf, PAGE_SIZE, "%s\n", fg_chg_log_buf);

	log_output();

	return ret;
}
static CLASS_ATTR_RO(log_head);

static struct attribute *log_output_attrs[] = {
	&class_attr_log.attr,
	&class_attr_log_head.attr,
	NULL,
};
ATTRIBUTE_GROUPS(log_output);

static void fg_chg_logout_work(struct work_struct *work)
{
	if (g_smbchg_chg == NULL)
		return ;

	// Main charger power_supply
	log_output_psy(g_smbchg_chg->bms_psy, qg_psy_props, ARRAY_SIZE(qg_psy_props));
	log_output_psy(g_smbchg_chg->batt_psy, smb5_batt_props, ARRAY_SIZE(smb5_batt_props));
	log_output_psy(g_smbchg_chg->usb_psy, smb5_usb_props, ARRAY_SIZE(smb5_usb_props));
	//log_output_psy(g_smbchg_chg->dc_psy, smb2_dc_props, ARRAY_SIZE(smb2_dc_props));
	log_output_psy(g_smbchg_chg->usb_main_psy, smb5_usb_main_props, ARRAY_SIZE(smb5_usb_main_props));
	log_output_psy(g_smbchg_chg->usb_port_psy, smb5_usb_port_props, ARRAY_SIZE(smb5_usb_port_props));

	// Parallel charger power_supply
	log_output_psy(g_smbchg_chg->pl.psy, smb138x_parallel_props, ARRAY_SIZE(smb138x_parallel_props));

	log_output_reg(CHGR_NAME, CHGR_BASE, chgr, ARRAY_SIZE(chgr));
	log_output_reg(OTG_NAME, OTG_BASE, otg, ARRAY_SIZE(otg));
	log_output_reg(BATIF_NAME, BATIF_BASE, batif, ARRAY_SIZE(batif));
	log_output_reg(USBIN_NAME, USBIN_BASE, usb, ARRAY_SIZE(usb));
	log_output_reg(MISC_NAME, MISC_BASE, misc, ARRAY_SIZE(misc));
	log_output_reg(BMD_NAME, BMD_BASE, bmd, ARRAY_SIZE(bmd));
	log_output_reg(QG_NAME, QG_BASE, qg, ARRAY_SIZE(qg));

	log_output();

	if (log_delay_ms > 0)
		schedule_delayed_work(&g_smbchg_chg->log_output_work, msecs_to_jiffies(log_delay_ms));
	else
		schedule_delayed_work(&g_smbchg_chg->log_output_work, msecs_to_jiffies(LOG_DELAY_MS));
}

static int fg_chg_logout_head(void)
{
	// Show psy name as space when pwy was NULL.
	//if (g_smbchg_chg @)
	//	return -EFAULT;

	// Main charger power_supply
	log_output_psy_head(g_smbchg_chg->bms_psy, qg_psy_props, ARRAY_SIZE(qg_psy_props));
	log_output_psy_head(g_smbchg_chg->batt_psy, smb5_batt_props, ARRAY_SIZE(smb5_batt_props));
	log_output_psy_head(g_smbchg_chg->usb_psy, smb5_usb_props, ARRAY_SIZE(smb5_usb_props));
	//log_output_psy_head(g_smbchg_chg->dc_psy, smb2_dc_props, ARRAY_SIZE(smb2_dc_props));
	log_output_psy_head(g_smbchg_chg->usb_main_psy, smb5_usb_main_props, ARRAY_SIZE(smb5_usb_main_props));
	log_output_psy_head(g_smbchg_chg->usb_port_psy, smb5_usb_port_props, ARRAY_SIZE(smb5_usb_port_props));

	// Parallel charger power_supply
	log_output_psy_head(g_smbchg_chg->pl.psy, smb138x_parallel_props, ARRAY_SIZE(smb138x_parallel_props));

	log_output_reg_head(CHGR_NAME, CHGR_BASE, chgr, ARRAY_SIZE(chgr));
	log_output_reg_head(OTG_NAME, OTG_BASE, otg, ARRAY_SIZE(otg));
	log_output_reg_head(BATIF_NAME, BATIF_BASE, batif, ARRAY_SIZE(batif));
	log_output_reg_head(USBIN_NAME, USBIN_BASE, usb, ARRAY_SIZE(usb));
	log_output_reg_head(MISC_NAME, MISC_BASE, misc, ARRAY_SIZE(misc));
	log_output_reg_head(BMD_NAME, BMD_BASE, bmd, ARRAY_SIZE(bmd));
	log_output_reg_head(QG_NAME, QG_BASE, qg, ARRAY_SIZE(qg));

	log_output();

	return 0;
}

int init_fg_chg_work(struct smb_charger *chg)
{
	int rc = 0;

	if (chg == NULL)
		return -EFAULT;

	g_smbchg_chg = chg;
	INIT_DELAYED_WORK(&g_smbchg_chg->log_output_work, fg_chg_logout_work);

	g_smbchg_chg->log_output_class.name = "wt_charger_log",
	g_smbchg_chg->log_output_class.owner = THIS_MODULE,
	g_smbchg_chg->log_output_class.class_groups = log_output_groups;

	rc = class_register(&g_smbchg_chg->log_output_class);
	if (rc < 0) {
		pr_err("couldn't register wt chg & battery log sysfs class, rc = %d\n", rc);
		return rc;
	}

	if (log_output_mode == 1) {
		fg_chg_logout_head();
		schedule_delayed_work(&g_smbchg_chg->log_output_work, msecs_to_jiffies(5000));
		//fg_chg_logout_work(&chg->log_output_work.work);
	}

	return 0;
}

int deinit_fg_chg_work(struct smb_charger *chg)
{
	if (g_smbchg_chg == NULL)
		return -EFAULT;

	class_unregister(&g_smbchg_chg->log_output_class);
	cancel_delayed_work_sync(&g_smbchg_chg->log_output_work);
	g_smbchg_chg = NULL;

	return 0;
}

#endif /* __WT_BATTERY_CHARGER_LOG_OUTPUT__ */
#endif /* ODM_WT_EDIT */