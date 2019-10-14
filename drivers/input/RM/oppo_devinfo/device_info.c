/***********************************************************
** Copyright (C), 2008-2016, OPPO Mobile Comm Corp., Ltd.
** ODM_WT_EDIT
** File: - devinfo.c
** Description: source  devinfo
**
** Version: 1.0
** Date : 2018/05/10
** Author: Ming.He@ODM_WT.BSP.Kernel.Driver
**
** ------------------------------- Revision History: -------------------------------
**  	<author>		    <data> 	         <version >	       <desc>
**       Ming.He@ODM_WT     2018/05/11       1.0               source  devinfo for factory mode and debug usage
**
****************************************************************/
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/uaccess.h>

#define ITEM_LENGTH 50

#define DEVINFO_PATH "devinfo_wt"




struct devinfo{
char device_name[ITEM_LENGTH];
char device_version [ITEM_LENGTH];
char device_manufacture[ITEM_LENGTH];
};
struct devinfo_tp_S{
	char version[ITEM_LENGTH];
	char manufacture[ITEM_LENGTH];
	char fw_path[ITEM_LENGTH];
};
struct devinfo_tp_S devinfo_tp = {"tp", "Null", "Null",};

struct devinfo g_devinfo_items[] = {
    {"Sensor_alsps", "Null", "Null",},      //alsps厂家和当前固件版本信息
    {"Sensor_gyro", "Null", "Null",},       //gyro厂家和当前固件版本信息
    {"Sensor_msensor", "Null", "Null",},    //msensor厂家和当前固件版本信息
    {"Sensor_accel", "Null", "Null",},      //accel厂家和当前固件版本信息
    {"Emmc", "Null", "Null",},              //emmc厂家和当前固件版本信息
    {"Battery", "Null", "Null",},          //battery厂家和当前固件版本信息
    {"Cam_b", "Null", "Null",},             //后置摄像头厂家和当前固件版本信息
    {"Cam_f", "Null", "Null",},             //前置摄像头厂家和当前固件版本信息
    {"Lcd", "Null", "Null",},               //lcd厂家和当前固件版本信息
    //{"Tp", "Null", "Null",},                //tp厂家和当前固件版本信息
    {"speaker_mainboard", "Null", "Null",},
    {"audio_mainboard", "Null", "Null",},
    {"wlan_resource", "Null", "Null",},
    {"emmc_verison", "Null", "Null",},
    {"RFverity", "Null", "Null",},
    {"PMIC632", "Null", "Null",},
    {"PMIC625", "Null", "Null",},
    /* Bin2.Zhang@ODM_WT.BSP.Charger.Basic.1941873, 20190530, Remove unsupport charger mode node */
    //{"vooc", "Null", "Null",},
    {"gauge", "Null", "Null",}
};

int devinfo_search_item(char *item_name)
{
    int i = 0;

    for(i = 0; i < ARRAY_SIZE(g_devinfo_items); i++)
    {
        if(strcmp(item_name, g_devinfo_items[i].device_name) == 0)
        {
            return i;
        }
    }

    return -1;
}

static ssize_t devinfo_proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int item_index = -1;
    char devinfo_temp[ITEM_LENGTH*3];

    if(*ppos)
        return 0;
    *ppos += count;

    item_index = devinfo_search_item(file->f_path.dentry->d_iname);

    if(item_index < 0)
    {
        pr_err("[devinfo][ERR]: devinfo_proc_read %s fail\n", file->f_path.dentry->d_iname);
        return 0;
    }

    snprintf(devinfo_temp, ITEM_LENGTH*3, "Device version: %s\n", g_devinfo_items[item_index].device_version);
    snprintf(devinfo_temp, ITEM_LENGTH*3, "%sDevice manufacture: %s\n", devinfo_temp, g_devinfo_items[item_index].device_manufacture);

    if (copy_to_user(buf, devinfo_temp, strlen(devinfo_temp) + 1)) {
        pr_err("%s: copy to user error.", __func__);
        return -1;
    }

    return strlen(devinfo_temp);
}
static ssize_t devinfo_proc_tp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	char devinfo_temp[ITEM_LENGTH*3];

    if(*ppos)
        return 0;
    *ppos += count;

    snprintf(devinfo_temp, ITEM_LENGTH*3, "Device version:\t\t%s\n", devinfo_tp.version);
    snprintf(devinfo_temp, ITEM_LENGTH*3, "%sDevice manufacture:\t\t%s\n", devinfo_temp,devinfo_tp.manufacture);
	snprintf(devinfo_temp, ITEM_LENGTH*3, "%sDevice fw_path:\t\t%s\n", devinfo_temp, devinfo_tp.fw_path);

    if (copy_to_user(buf, devinfo_temp, strlen(devinfo_temp) + 1)) {
        pr_err("%s: copy to user error.", __func__);
        return -1;
    }

    return strlen(devinfo_temp);
}
void devinfo_info_tp_set(char *version, char *manufacture, char *fw_path)
{

	memset(devinfo_tp.version, 0, ITEM_LENGTH);
	strncpy(devinfo_tp.version, version, ITEM_LENGTH);
	memset(devinfo_tp.manufacture, 0, ITEM_LENGTH);
	strncpy(devinfo_tp.manufacture, manufacture, ITEM_LENGTH);
	memset(devinfo_tp.manufacture, 0, ITEM_LENGTH);
	strncpy(devinfo_tp.manufacture, manufacture, ITEM_LENGTH);
	memset(devinfo_tp.fw_path, 0, ITEM_LENGTH);
	strncpy(devinfo_tp.fw_path, fw_path, ITEM_LENGTH);

    return;
}

void devinfo_info_set(char *name, char *version, char *manufacture)
{
    int item_index = -1;

    item_index = devinfo_search_item(name);

    if(item_index < 0)
    {
        pr_err("[devinfo][ERR]: devinfo_info_set %s fail\n", name);
        return;
    }

    memset(g_devinfo_items[item_index].device_version, 0, ITEM_LENGTH);
    strncpy(g_devinfo_items[item_index].device_version, version, ITEM_LENGTH);
    memset(g_devinfo_items[item_index].device_manufacture, 0, ITEM_LENGTH);
    strncpy(g_devinfo_items[item_index].device_manufacture, manufacture, ITEM_LENGTH);

    return;
}
EXPORT_SYMBOL_GPL(devinfo_info_set);

static const struct file_operations devinfo_fops =
{
    .write = NULL,
    .read  = devinfo_proc_read,
    .owner = THIS_MODULE,
};
static const struct file_operations devinfo_tp_fops =
{
    .write = NULL,
    .read  = devinfo_proc_tp_read,
    .owner = THIS_MODULE,
};

static int __init proc_devinfo_init(void)
{
    int i;
    struct proc_dir_entry *devinfo_dir;
    struct proc_dir_entry *devinfo_item;
	struct proc_dir_entry *devinfo_tp;

    devinfo_dir = proc_mkdir(DEVINFO_PATH, NULL);
    if (!devinfo_dir)
    {
        pr_err("[proc_devinfo_init][ERR]: create %s dir fail\n", DEVINFO_PATH);
        return -1;
    }

    for(i = 0; i < ARRAY_SIZE(g_devinfo_items); i++)
    {
        devinfo_item = proc_create(g_devinfo_items[i].device_name, 0444, devinfo_dir, &devinfo_fops);
        if (devinfo_item == NULL)
            pr_err("[proc_devinfo_init][ERR]: create %s fail\n", g_devinfo_items[i].device_name);
    }
	devinfo_tp = proc_create("tp", 0444, devinfo_dir, &devinfo_tp_fops);
	if (devinfo_tp == NULL)
		pr_err("[proc_devinfo_init][ERR]: create tp fail\n");
    return 0;
}
fs_initcall(proc_devinfo_init);
