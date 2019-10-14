/**
 * Copyright 2008-2013 OPPO Mobile Comm Corp., Ltd, All rights reserved.
 * VENDOR_EDIT:
 * File:device_info.c
 * ModuleName :devinfo
 * Author : wangjc
 * Date : 2013-10-23
 * Version :1.0 2.0
 * Description :add interface to get device information.
 * History :
   <version >  <time>  <author>  <desc>
   1.0                2013-10-23        wangjc        init
   2.0          2015-04-13  hantong modify as platform device  to support diffrent configure in dts
   3.0          2018-10-30  Fanhong.Kong modify for 4.14
*/

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/soc/qcom/smem.h>
#include <soc/oppo/device_info.h>
#include <soc/oppo/oppo_project.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include "../../../fs/proc/internal.h"
#include <linux/uaccess.h>		/* For copy_to_user/put_user/... */
#define DEVINFO_NAME "devinfo"
#define INFO_BUF_LEN 64

static int mainboard_res = 0;

static struct of_device_id devinfo_id[] = {
        {.compatible = "oppo-devinfo", },
        {},
};

struct devinfo_data {
	struct platform_device *devinfo;
	struct pinctrl 	*pinctrl;
	struct pinctrl_state *hw_operator_gpio_sleep;
	struct pinctrl_state *hw_sub_gpio_sleep;
	struct pinctrl_state *hw_wlan_gpio_sleep;

	int hw_id0_gpio;
	int hw_id1_gpio;
	int hw_id2_gpio;
	int hw_id3_gpio;
	int sub_hw_id1;
	int sub_hw_id2;
	int ant_select_gpio;
	int wlan_hw_id1;
	int wlan_hw_id2;
};

static struct proc_dir_entry *parent = NULL;
struct proc_dir_entry *tp_entry;


static void *device_seq_start(struct seq_file *s, loff_t *pos)
{
        static unsigned long counter = 0;
        if (*pos == 0) {
                return &counter;
        } else {
                *pos = 0;
                return NULL;
        }
}

static void *device_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
        return NULL;
}

static void device_seq_stop(struct seq_file *s, void *v)
{
        return;
}

static int device_seq_show(struct seq_file *s, void *v)
{
        struct proc_dir_entry *pde = s->private;
        struct manufacture_info *info = pde->data;
        if (info) {
                seq_printf(s, "Device version:\t\t%s\nDevice manufacture:\t\t%s\n",
                         info->version,        info->manufacture);
                if(info->fw_path)
                        seq_printf(s, "Device fw_path:\t\t%s\n",
                                info->fw_path);
        }
        return 0;
}

static struct seq_operations device_seq_ops = {
        .start = device_seq_start,
        .next = device_seq_next,
        .stop = device_seq_stop,
        .show = device_seq_show
};

static int device_proc_open(struct inode *inode, struct file *file)
{
        int ret = seq_open(file, &device_seq_ops);
        pr_err("caven %s is called\n", __func__);
        if (!ret) {
                struct seq_file *sf = file->private_data;
                sf->private = PDE(inode);
        }
        return ret;
}
static const struct file_operations device_node_fops = {
        .read =  seq_read,
        .llseek = seq_lseek,
        .release = seq_release,
        .open = device_proc_open,
        .owner = THIS_MODULE,
};

int register_device_proc(char *name, char *version, char *manufacture)
{
        struct proc_dir_entry *d_entry;
        struct manufacture_info *info;

        if (!parent) {
                parent =  proc_mkdir("devinfo", NULL);
                if (!parent) {
                        pr_err("can't create devinfo proc\n");
                        return -ENOENT;
                }
        }

        info = kzalloc(sizeof*info, GFP_KERNEL);
        info->version = version;
        info->manufacture = manufacture;
        d_entry = proc_create_data(name, S_IRUGO, parent, &device_node_fops, info);
        if (!d_entry) {
                pr_err("create %s proc failed.\n", name);
                kfree(info);
                return -ENOENT;
        }
        return 0;
}

int register_tp_proc(char *name, char *version, char *manufacture ,char *fw_path)
{
	struct manufacture_info *info;
	if(!parent) {
		parent =  proc_mkdir ("devinfo", NULL);
		if(!parent) {
			pr_err("can't create devinfo proc\n");
			return -ENOENT;
		}
	}
	 info = kzalloc(sizeof*info, GFP_KERNEL);
     info->version = version;
     info->manufacture = manufacture;
	 info->fw_path = fw_path;
	 if (!tp_entry) {
		tp_entry = proc_create_data(name, S_IRUGO, parent, &device_node_fops, info);
		if (!tp_entry) {
			pr_err("create %s proc failed.\n", name);
			kfree(info);
			return -ENOENT;
		}
	}
     return 0;
}

int register_devinfo(char *name, struct manufacture_info *info)
{
	struct proc_dir_entry *d_entry;
	if(!parent) {
		parent =  proc_mkdir ("devinfo", NULL);
		if(!parent) {
			pr_err("can't create devinfo proc\n");
			return -ENOENT;
		}
	}

	d_entry = proc_create_data (name, S_IRUGO, parent, &device_node_fops, info);
	if(!d_entry) {
		pr_err("create %s proc failed.\n", name);
		return -ENOENT;
	}
	return 0;
}

static void dram_type_add(void)
{
        /*
        #if 0
        struct manufacture_info dram_info;
        int *p = NULL;
        #if 0
        p  = (int *)smem_alloc2(SMEM_DRAM_TYPE, 4);
        #else
        p  = (int *)smem_alloc(SMEM_DRAM_TYPE, 4, 0, 0);
        #endif

        if (p)
        {
                switch (*p){
                        case 0x01:
                                dram_info.version = "K3QF4F40BM-FGCF FBGA";
                                dram_info.manufacture = "SAMSUNG";
                                break;
                        case 0x06:
                                dram_info.version = "H9CKNNNCPTMRPR FBGA";
                                dram_info.manufacture = "HYNIX";
                                break;
                        default:
                                dram_info.version = "unknown";
                                dram_info.manufacture = "unknown";
                }

        }else{
                dram_info.version = "unknown";
                dram_info.manufacture = "unknown";

        }

        register_device_proc("ddr", dram_info.version, dram_info.manufacture);
        #endif
        */
}


static int get_ant_select_gpio(struct devinfo_data *devinfo_data)
{
        int ret = 0;
        struct device_node *np;
        pr_err("srd get_ant_select_gpio\n");
       if(devinfo_data != NULL && devinfo_data->devinfo != NULL) {
              np = devinfo_data->devinfo->dev.of_node;
        }
       else {
              pr_err("devinfo_data is NULL\n");
              return 0;
        }
        devinfo_data->ant_select_gpio = of_get_named_gpio(np, "Hw,ant_select-gpio", 0);

        if (devinfo_data->ant_select_gpio >= 0) {
                if (gpio_is_valid(devinfo_data->ant_select_gpio)) {
                        ret = gpio_request(devinfo_data->ant_select_gpio, "ant_select-gpio");
                                if (ret) {
                                        pr_err(" unable to request gpio [%d]\n", devinfo_data->ant_select_gpio);
                                }
                }
        } else {
                pr_err("devinfo_data->ant_select_gpio not specified\n");
        }

        return ret;
}

static int get_hw_opreator_version(struct devinfo_data *devinfo_data)
{
        int hw_operator_name = 0;
        int ret;
        int id0 = -1;
        int id1 = -1;
        int id2 = -1;
        int id3 = -1;
        struct device_node *np;
        np = devinfo_data->devinfo->dev.of_node;
        if (!devinfo_data) {
                pr_err("devinfo_data is NULL\n");
                return 0;
        }
        devinfo_data->hw_id0_gpio = of_get_named_gpio(np, "Hw, operator-gpio0", 0);
        if (devinfo_data->hw_id0_gpio < 0) {
                pr_err("devinfo_data->hw_id0_gpio not specified\n");
        }
        devinfo_data->hw_id1_gpio = of_get_named_gpio(np, "Hw,operator-gpio1", 0);
        if (devinfo_data->hw_id1_gpio < 0) {
                pr_err("devinfo_data->hw_id1_gpio not specified\n");
        }
        devinfo_data->hw_id2_gpio = of_get_named_gpio(np, "Hw,operator-gpio2", 0);
        if (devinfo_data->hw_id2_gpio < 0) {
                pr_err("devinfo_data->hw_id2_gpio not specified\n");
        }
        devinfo_data->hw_id3_gpio = of_get_named_gpio(np, "Hw,operator-gpio3", 0);
        if (devinfo_data->hw_id3_gpio < 0) {
                pr_err("devinfo_data->hw_id3_gpio not specified\n");
        }
        devinfo_data->pinctrl = devm_pinctrl_get(&devinfo_data->devinfo->dev);
        if (IS_ERR_OR_NULL(devinfo_data->pinctrl)) {
                pr_err("%s:%d Getting pinctrl handle failed\n", __func__, __LINE__);
                return        OPERATOR_UNKOWN;
        }

        devinfo_data->hw_operator_gpio_sleep = pinctrl_lookup_state(devinfo_data->pinctrl, "hw_operator_gpio_sleep");
        if (IS_ERR_OR_NULL(devinfo_data->hw_operator_gpio_sleep)) {
                pr_err("%s:%d Failed to get the suspend state pinctrl handle\n", __func__, __LINE__);
                return        OPERATOR_UNKOWN;
        }
        /***Tong.han@Bsp.Group.Tp Added for Operator_Pcb detection***/
        if (devinfo_data->hw_id0_gpio >= 0) {
                ret = gpio_request(devinfo_data->hw_id0_gpio, "HW_ID0");
                if (ret) {
                        pr_err("unable to request gpio [%d]\n", devinfo_data->hw_id0_gpio);
                } else {
                        id0 = gpio_get_value(devinfo_data->hw_id0_gpio);
                }
        }
        if (devinfo_data->hw_id1_gpio >= 0) {
                ret = gpio_request(devinfo_data->hw_id1_gpio, "HW_ID1");
                if (ret) {
                        pr_err("unable to request gpio [%d]\n", devinfo_data->hw_id1_gpio);
                } else {
                        id1 = gpio_get_value(devinfo_data->hw_id1_gpio);
                }
        }
        if (devinfo_data->hw_id2_gpio >= 0) {
                ret = gpio_request(devinfo_data->hw_id2_gpio, "HW_ID2");
                if (ret) {
                        pr_err("unable to request gpio [%d]\n", devinfo_data->hw_id2_gpio);
                } else {
                        id2 = gpio_get_value(devinfo_data->hw_id2_gpio);
                }
        }
        if (devinfo_data->hw_id3_gpio >= 0) {
                ret = gpio_request(devinfo_data->hw_id3_gpio, "HW_ID3");
                if (ret) {
                        pr_err("unable to request gpio [%d]\n", devinfo_data->hw_id3_gpio);
                } else {
                        id3 = gpio_get_value(devinfo_data->hw_id3_gpio);
                }
        }
        pinctrl_select_state(devinfo_data->pinctrl, devinfo_data->hw_operator_gpio_sleep);
        pr_err("hw_operator_name [%d]\n", hw_operator_name);
        return hw_operator_name;
}

static void sub_mainboard_verify(struct devinfo_data *devinfo_data)
{
        int ret;
        int id1 = -1;
        int id2 = -1;
        static char temp_manufacture_sub[INFO_BUF_LEN] = {0};
        static char temp_speaker_manufacture_sub[INFO_BUF_LEN] = {0};
        struct device_node *np;
        struct manufacture_info mainboard_info;
        struct manufacture_info speaker_mainboard_info;
        if (!devinfo_data) {
                pr_err("devinfo_data is NULL\n");
                return;
        }
        np = devinfo_data->devinfo->dev.of_node;
        devinfo_data->sub_hw_id1 = of_get_named_gpio(np, "Hw,sub_hwid_1", 0);
        if (devinfo_data->sub_hw_id1 < 0) {
                pr_err("devinfo_data->sub_hw_id1 not specified\n");
        }
        devinfo_data->sub_hw_id2 = of_get_named_gpio(np, "Hw,sub_hwid_2", 0);
        if (devinfo_data->sub_hw_id2 < 0) {
                pr_err("devinfo_data->sub_hw_id2 not specified\n");
        }
        devinfo_data->pinctrl = devm_pinctrl_get(&devinfo_data->devinfo->dev);
        if (IS_ERR_OR_NULL(devinfo_data->pinctrl)) {
                pr_err("%s:%d Getting pinctrl handle failed\n", __func__, __LINE__);
                goto sub_mainboard_set;
        }
        devinfo_data->hw_sub_gpio_sleep = pinctrl_lookup_state(devinfo_data->pinctrl, "hw_sub_gpio_sleep");
        if (IS_ERR_OR_NULL(devinfo_data->hw_sub_gpio_sleep)) {
                pr_err("%s:%d Failed to get the suspend state pinctrl handle\n", __func__, __LINE__);
        }


        if (devinfo_data->sub_hw_id1 >= 0) {
                ret = gpio_request(devinfo_data->sub_hw_id1, "SUB_HW_ID1");
                if (ret) {
                        pr_err("unable to request gpio [%d]\n", devinfo_data->sub_hw_id1);
                } else {
                        id1 = gpio_get_value(devinfo_data->sub_hw_id1);
                }
        }
        if (devinfo_data->sub_hw_id2 >= 0) {
                ret = gpio_request(devinfo_data->sub_hw_id2, "SUB_HW_ID2");
                if (ret) {
                        pr_err("unable to request gpio [%d]\n", devinfo_data->sub_hw_id2);
                } else {
                        id2 = gpio_get_value(devinfo_data->sub_hw_id2);
                }
        }
sub_mainboard_set:
        mainboard_info.manufacture = temp_manufacture_sub;
        mainboard_info.version ="Qcom";
        speaker_mainboard_info.manufacture = temp_speaker_manufacture_sub;
        speaker_mainboard_info.version ="Qcom";
        switch (get_project()) {
        case OPPO_19021:
        {
                pr_err("id1 = %d, id2 = %d\n", id1, id2);
                if (id1 == 1) {
                        snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-audio-china", get_project());
                }
                else if (id1 == 0) {
                        snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-audio-oversea", get_project());
                }
                else
                        mainboard_info.manufacture = "sub-UNSPECIFIED";

                if (id2 == 1) {
                        snprintf(speaker_mainboard_info.manufacture, INFO_BUF_LEN, "%d-speaker-china", get_project());
                }
                else if (id2 == 0) {
                        snprintf(speaker_mainboard_info.manufacture, INFO_BUF_LEN, "%d-speaker-oversea", get_project());
                }
                else
                        speaker_mainboard_info.manufacture = "sub-UNSPECIFIED";

                register_device_proc("speaker_mainboard", speaker_mainboard_info.version, speaker_mainboard_info.manufacture);
                break;
        }
		case OPPO_18501:
        {
                pr_err("id1 = %d, id2 = %d\n", id1, id2);
                if (id1 == 0) {
                        snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-audio-china", get_project());
                }
                else if (id1 == 1) {
                        snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-audio-oversea", get_project());
                }
                else
                        mainboard_info.manufacture = "sub-UNSPECIFIED";

                if (id1 == 0) {
                        snprintf(speaker_mainboard_info.manufacture, INFO_BUF_LEN, "%d-speaker-china", get_project());
                }
                else if (id1 == 1) {
                        snprintf(speaker_mainboard_info.manufacture, INFO_BUF_LEN, "%d-speaker-oversea", get_project());
                }
                else
                        speaker_mainboard_info.manufacture = "sub-UNSPECIFIED";

                register_device_proc("speaker_mainboard", speaker_mainboard_info.version, speaker_mainboard_info.manufacture);
                break;
        }
		case OPPO_18503:
        {
                pr_err("id1 = %d, id2 = %d\n", id1, id2);
                if ((id1 == 0) && (id2 == 0) && (get_Modem_Version() == RF_VERSION__13)) {
                        snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "18316-all-match");
                }
                else if ((id1 == 1) && (id2 == 1) && (get_Modem_Version() == RF_VERSION__12)) {
                        snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "18315-indian-match");
                }
                else
			snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-%d-unmatch",id1, id2);

                register_device_proc("speaker_mainboard", speaker_mainboard_info.version, speaker_mainboard_info.manufacture);
                break;
        }
		case OPPO_11870:
		{
                pr_err("id1 = %d, id2 = %d\n", id1, id2);
                if ((id1 == 1) && (id2 == 1) && ((get_Modem_Version() == RF_VERSION__14) || (get_Modem_Version() == RF_VERSION__15))) {
                        snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "18005-all-match");
                }
                else
						snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-%d-%d-%d-unmatch", get_project(),get_Modem_Version(),id1, id2);

                register_device_proc("speaker_mainboard", speaker_mainboard_info.version, speaker_mainboard_info.manufacture);
                break;
        }
        default:
        {
                        snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-%d", get_project(), get_Operator_Version());
                        break;
        }
        }

        if (!IS_ERR_OR_NULL(devinfo_data->hw_sub_gpio_sleep)) {
                pinctrl_select_state(devinfo_data->pinctrl, devinfo_data->hw_sub_gpio_sleep);
        }
        register_device_proc("audio_mainboard", mainboard_info.version, mainboard_info.manufacture);
}

static void mainboard_verify(struct devinfo_data *devinfo_data)
{
        struct manufacture_info mainboard_info;
        int hw_opreator_version = 0;
        static char temp_manufacture[INFO_BUF_LEN] = {0};
        if (!devinfo_data) {
                pr_err("devinfo_data is NULL\n");
                return;
        }

        /***Tong.han@Bsp.Group.Tp Added for Operator_Pcb detection***/
        hw_opreator_version = get_hw_opreator_version(devinfo_data);
        /*end of Add*/
        mainboard_info.manufacture = temp_manufacture;
        switch (get_PCB_Version()) {
        case HW_VERSION__10:
                mainboard_info.version ="10";
                snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-T0", hw_opreator_version);
        /*                mainboard_info.manufacture = "SA(SB)";
*/
                break;
        case HW_VERSION__11:
                mainboard_info.version = "11";
                snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-SA", hw_opreator_version);
        /*                mainboard_info.manufacture = "SC";
*/
                        break;
        case HW_VERSION__12:
                mainboard_info.version = "12";
                snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-SA", hw_opreator_version);
        /*                mainboard_info.manufacture = "SD";
*/
                break;
        case HW_VERSION__13:
                mainboard_info.version = "13";
                snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-SB", hw_opreator_version);
        /*                mainboard_info.manufacture = "SE";
*/
                break;
        case HW_VERSION__14:
                mainboard_info.version = "14";
                snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-SC", hw_opreator_version);
        /*                mainboard_info.manufacture = "SF";
*/
                break;
        case HW_VERSION__15:
                mainboard_info.version = "15";
                snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-(T3-T4)", hw_opreator_version);
        /*                mainboard_info.manufacture = "T3-T4";
*/
                break;
        default:
                mainboard_info.version = "UNKOWN";
                snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-UNKOWN", hw_opreator_version);
        /*                mainboard_info.manufacture = "UNKOWN";
*/
        }
        register_device_proc("mainboard", mainboard_info.version, mainboard_info.manufacture);
}

/*#ifdef VENDOR_EDIT  Fanhong.Kong@ProDrv.CHG, modified 2014.4.13 for 14027*/
static void pa_verify(void)
{
        struct manufacture_info pa_info;
        switch (get_Modem_Version()) {
        case 0:
                pa_info.version = "0";
                pa_info.manufacture = "RFMD PA";
                break;
        case 1:
                pa_info.version = "1";
                pa_info.manufacture = "SKY PA";
                break;
        case 3:
                pa_info.version = "3";
                pa_info.manufacture = "AVAGO PA";
                break;
        default:
                pa_info.version = "UNKOWN";
                pa_info.manufacture = "UNKOWN";
        }
        register_device_proc("pa", pa_info.version, pa_info.manufacture);
}
/*#endif VENDOR_EDIT*/

/*#ifdef VENDOR_EDIT*/
/*rendong.shi@BSP.boot,2016/03/24,add for mainboard resource*/
static void wlan_resource_verify(struct devinfo_data *devinfo_data)
{
        int ret;
        int id1 = -1;
        int id2 = -1;
        static char temp_manufacture_wlan[INFO_BUF_LEN] = {0};
        struct device_node *np;
        struct manufacture_info mainboard_info;
        if (!devinfo_data) {
                pr_err("devinfo_data is NULL\n");
                return;
                }
        np = devinfo_data->devinfo->dev.of_node;
        devinfo_data->wlan_hw_id1 = of_get_named_gpio(np, "Hw,wlan_hwid_1", 0);
        if (devinfo_data->wlan_hw_id1 < 0) {
                pr_err("devinfo_data->wlan_hw_id1 not specified\n");
                }
        devinfo_data->wlan_hw_id2 = of_get_named_gpio(np, "Hw,wlan_hwid_2", 0);
        if (devinfo_data->wlan_hw_id2 < 0) {
                pr_err("devinfo_data->wlan_hw_id2 not specified\n");
                }
        devinfo_data->pinctrl = devm_pinctrl_get(&devinfo_data->devinfo->dev);
        if (IS_ERR_OR_NULL(devinfo_data->pinctrl)) {
                pr_err("%s:%d Getting pinctrl handle failed\n", __func__, __LINE__);
                goto wlan_resource_set;
                }
        devinfo_data->hw_wlan_gpio_sleep = pinctrl_lookup_state(devinfo_data->pinctrl, "hw_wlan_gpio_sleep");
        if (IS_ERR_OR_NULL(devinfo_data->hw_wlan_gpio_sleep)) {
                pr_err("%s:%d Failed to get the suspend state pinctrl handle\n", __func__, __LINE__);
                }
        if (devinfo_data->wlan_hw_id1 >= 0) {
                ret = gpio_request(devinfo_data->wlan_hw_id1, "WLAN_HW_ID1");
                if (ret) {
                        pr_err("unable to request gpio [%d]\n", devinfo_data->wlan_hw_id1);
                        } else {
                                id1 = gpio_get_value(devinfo_data->wlan_hw_id1);
                                }
                }
        if (devinfo_data->wlan_hw_id2 >= 0) {
                ret = gpio_request(devinfo_data->wlan_hw_id2, "WLAN_HW_ID2");
                if (ret) {
                        pr_err("unable to request gpio [%d]\n", devinfo_data->wlan_hw_id2);
                        } else {
                                id2 = gpio_get_value(devinfo_data->wlan_hw_id2);
                                }
                }
wlan_resource_set:
        mainboard_info.manufacture = temp_manufacture_wlan;
        mainboard_info.version ="Qcom";
        switch (get_project()) {
        default: {
                        snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-%d", get_project(), get_Operator_Version());
                        break;
                }
        }

        if (!IS_ERR_OR_NULL(devinfo_data->hw_wlan_gpio_sleep)) {
                pinctrl_select_state(devinfo_data->pinctrl, devinfo_data->hw_wlan_gpio_sleep);
                }
        register_device_proc("wlan_resource", mainboard_info.version, mainboard_info.manufacture);
}

static ssize_t mainboard_resource_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
        char page[256] = {0};
        int len = 0;
        len = sprintf(page, "%d", mainboard_res);

        if (len > *off) {
                len -= *off;
        }
        else
                len = 0;
        if (copy_to_user(buf, page, (len < count ? len : count))) {
                return -EFAULT;
        }
        *off += len < count ? len : count;
        return (len < count ? len : count);
}

struct file_operations mainboard_res_proc_fops = {
        .read = mainboard_resource_read_proc,
        .write = NULL,
};
/*#endif*/


static int devinfo_probe(struct platform_device *pdev)
{
        int ret = 0;
        struct devinfo_data *devinfo_data = NULL;
        struct proc_dir_entry *pentry;
        pr_err("this is project  %d \n", get_project());
        devinfo_data = kzalloc(sizeof(struct devinfo_data), GFP_KERNEL);
        if (devinfo_data == NULL) {
                pr_err("devinfo_data kzalloc failed\n");
                ret = -ENOMEM;
                return ret;
        }
        /*parse_dts*/
        devinfo_data->devinfo = pdev;
        /*end of parse_dts*/
        if (!parent) {
                parent =  proc_mkdir("devinfo_unused", NULL);
                if (!parent) {
                        pr_err("can't create devinfo proc\n");
                        ret = -ENOENT;
                }
        }
        sub_mainboard_verify(devinfo_data);
        wlan_resource_verify(devinfo_data);
        pentry = proc_create("wlan_res", S_IRUGO, NULL, &mainboard_res_proc_fops);
        if (!pentry) {
                pr_err("create prjVersion proc failed.\n");
        }


        return ret;
        get_ant_select_gpio(devinfo_data);
        mainboard_verify(devinfo_data);
        dram_type_add();
        return ret;
        /*Add devinfo for some devices*/
        pa_verify();
        /*end of Adding devinfo for some devices*/
}

static int devinfo_remove(struct platform_device *dev)
{
        remove_proc_entry(DEVINFO_NAME, NULL);
        return 0;
}

static struct platform_driver devinfo_platform_driver = {
        .probe = devinfo_probe,
        .remove = devinfo_remove,
        .driver = {
                .name = DEVINFO_NAME,
                .of_match_table = devinfo_id,
        },
};

//module_platform_driver(devinfo_platform_driver);
static int __init devinfo_init(void)
{
	return platform_driver_register(&devinfo_platform_driver);
}

static void __exit devinfo_exit(void)
{
        return platform_driver_unregister(&devinfo_platform_driver);
}

late_initcall(devinfo_init);
module_exit(devinfo_exit);

MODULE_DESCRIPTION("OPPO device info");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Wangjc <wjc@oppo.com>");
