// SPDX-License-Identifier: GPL-2.0-or-later
/*
MiniMiku software notice:
ALL OF MINIMIKU LINUX IS FREE SOFTWARE
For more information, look at ./FREE.md 
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

// Version and hwname
#define SYSMPB_VERSION "1.00"

// If the hwname isn't set, set it to Unknown, should almost never be not set
#ifndef CONFIG_MINIMIKU_HWNAME
#define CONFIG_MINIMIKU_HWNAME "Unknown"
#endif

#define MINIMIKU_HWNAME CONFIG_MINIMIKU_HWNAME

const char minimiku_hwname[] = MINIMIKU_HWNAME;

// autopanic

bool minimiku_autopanic;

static int __init minimiku_autopanic_setup(char *str)
{
    // panic the system
    minimiku_autopanic = true;

    pr_info("MiniMiku: automatic panic enabled\n");

    return 0;
}

early_param("minimiku.autopanic", minimiku_autopanic_setup);

static int __init minimiku_autopanic_init(void)
{
    if (minimiku_autopanic)
        panic("MiniMiku: minimiku.autopanic requested a panic");

    return 0;
}
late_initcall(minimiku_autopanic_init);

// MiniMiku/sys/kernel/minimiku/hwname driver
static struct kobject *minimiku_kobj;

static ssize_t hwname_show(struct kobject *kobj,
               struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "%s\n", MINIMIKU_HWNAME);
}

static struct kobj_attribute hwname_attr =__ATTR_RO(hwname);

static int __init minimiku_sysfs_init(void)
{
    int ret;

    minimiku_kobj = kobject_create_and_add("minimiku", kernel_kobj);
    if (!minimiku_kobj)
        return -ENOMEM;

    ret = sysfs_create_file(minimiku_kobj, &hwname_attr.attr);
    if (ret) {
        kobject_put(minimiku_kobj);
        return ret;
    }

    return 0;
}
late_initcall(minimiku_sysfs_init);


// sysmpb startup
static int __init minimiku_sysmpb_init(void)
{
    pr_info("MiniMiku: system multipurpose builtin initialized\n");
    pr_info("MiniMiku: sysmpb version: %s\n", SYSMPB_VERSION);
    pr_info("MiniMiku: sysmpb: HWNAME:%s\n", minimiku_hwname);

    return 0;
}

core_initcall(minimiku_sysmpb_init);