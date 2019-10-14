/***********************************************************
** Copyright (C), 2008-2016, OPPO Mobile Comm Corp., Ltd.
** ODM_WT_EDIT
** File: - oppoversion.c
** Description: source  oppoversion
**
** Version: 1.0
** Date : 2018/05/10
** Author: Ming.He@ODM_WT.BSP.Kernel.Driver
**
** ------------------------------- Revision History: -------------------------------
**       <author>           <data>           <version >        <desc>
**       Ming.He@ODM_WT     2018/05/11       1.0               source  oppoversion for os usage
**
****************************************************************/
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/uaccess.h>

#define ITEM_LENGTH 50
#define OPPOVERSION_PATH "oppoVersion_wt"

struct oppoversion{
char name[ITEM_LENGTH];
char version [ITEM_LENGTH];
};

struct oppoversion g_oppoversion_items[] = {
    {"modemType", "Null"},
    {"ocp", "Null"},
    {"operatorName", "Null"},   //机器制式（比如亚太，欧洲，北美，射频使用）
    {"oppobootmode", "Null"},
    {"pcbVersion", "Null"},     //硬件板版本段代号（比如evt，dvt，pvt）
    {"prjVersion", "Null"},     //项目代号（区分各个项目使用，如2000,2001,2002就是一个项目，2021，2031,2041就是三个不同的项目）
    {"secureStage", "Null"},
    {"secureType", "Null"},
    {"serialID", "Null"},
};

int oppoversion_search_item(char *item_name)
{
    int i = 0;

    for(i = 0; i < ARRAY_SIZE(g_oppoversion_items); i++)
    {
        if(strcmp(item_name, g_oppoversion_items[i].name) == 0)
        {
            return i;
        }
    }

    return -1;
}

static ssize_t oppoversion_proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int item_index = -1;
    char oppoversion_temp[ITEM_LENGTH];

    if(*ppos)
        return 0;
    *ppos += count;

    item_index = oppoversion_search_item(file->f_path.dentry->d_iname);

    if(item_index < 0)
    {
        pr_err("[oppoversion][ERR]: oppoversion_proc_read %s fail\n", file->f_path.dentry->d_iname);
        return 0;
    }

    strncpy(oppoversion_temp, g_oppoversion_items[item_index].version, ITEM_LENGTH);

    if (copy_to_user(buf, oppoversion_temp, strlen(oppoversion_temp) + 1)) {
        pr_err("%s: copy to user error.", __func__);
        return -1;
    }

    return strlen(oppoversion_temp);
}

void oppoversion_info_set(char *name, char *version)
{
    int item_index = -1;

    item_index = oppoversion_search_item(name);

    if(item_index < 0)
    {
        pr_err("[oppoversion][ERR]: oppoversion_info_set %s fail\n", name);
        return;
    }

    memset(g_oppoversion_items[item_index].version, 0, ITEM_LENGTH);
    strncpy(g_oppoversion_items[item_index].version, version, ITEM_LENGTH);

    return;
}
EXPORT_SYMBOL_GPL(oppoversion_info_set);

static const struct file_operations oppoversion_fops =
{
    .write = NULL,
    .read  = oppoversion_proc_read,
    .owner = THIS_MODULE,
};

static int __init proc_oppoversion_init(void)
{
    int i;
    struct proc_dir_entry *oppoversion_dir;
    struct proc_dir_entry *oppoversion_item;

    oppoversion_dir = proc_mkdir(OPPOVERSION_PATH, NULL);
    if (!oppoversion_dir)
    {
        pr_err("[proc_oppoversion_init][ERR]: create %s dir fail\n", OPPOVERSION_PATH);
        return -1;
    }

    for(i = 0; i < ARRAY_SIZE(g_oppoversion_items); i++)
    {
        oppoversion_item = proc_create(g_oppoversion_items[i].name, 0444, oppoversion_dir, &oppoversion_fops);
        if (oppoversion_item == NULL)
            pr_err("[proc_oppoversion_init][ERR]: create %s fail\n", g_oppoversion_items[i].name);
    }
    return 0;
}
fs_initcall(proc_oppoversion_init);
