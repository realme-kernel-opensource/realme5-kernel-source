#ifdef ODM_WT_EDIT
#/* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190416, Add for charger log & charger monitor */
ccflags-y += -D__WT_BATTERY_CHARGER_LOG_OUTPUT__
obj-y += wt_charger_log.o
obj-y += charger_monitor.o
#endif #/* ODM_WT_EDIT */
obj-$(CONFIG_QPNP_FG_GEN3)     += qpnp-fg-gen3.o fg-memif.o fg-util.o
obj-$(CONFIG_QPNP_FG_GEN4)     += qpnp-fg-gen4.o fg-memif.o fg-util.o fg-alg.o  pmic-voter.o
obj-$(CONFIG_QPNP_QG)		+= qpnp-qg.o pmic-voter.o qg-util.o qg-soc.o qg-sdam.o qg-battery-profile.o qg-profile-lib.o fg-alg.o
obj-$(CONFIG_SMB1351_USB_CHARGER) += smb1351-charger.o pmic-voter.o battery.o
obj-$(CONFIG_SMB1355_SLAVE_CHARGER)   += smb1355-charger.o pmic-voter.o
obj-$(CONFIG_QPNP_SMB2)		+= step-chg-jeita.o battery.o qpnp-smb2.o smb-lib.o pmic-voter.o storm-watch.o
obj-$(CONFIG_SMB138X_CHARGER)	+= step-chg-jeita.o smb138x-charger.o smb-lib.o pmic-voter.o storm-watch.o battery.o
obj-$(CONFIG_QPNP_QNOVO)	+= qpnp-qnovo.o battery.o
obj-$(CONFIG_QPNP_QNOVO5)	+= qpnp-qnovo5.o battery.o
obj-$(CONFIG_QPNP_SMB5)		+= step-chg-jeita.o battery.o qpnp-smb5.o smb5-lib.o pmic-voter.o storm-watch.o schgm-flash.o
obj-$(CONFIG_SMB1390_CHARGE_PUMP)	+= smb1390-charger.o pmic-voter.o
obj-$(CONFIG_SMB1390_CHARGE_PUMP_PSY)	+= smb1390-charger-psy.o pmic-voter.o
