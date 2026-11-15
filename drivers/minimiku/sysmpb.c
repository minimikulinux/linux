// SPDX-License-Identifier: GPL-2.0-or-later
/*
MiniMiku software notice:
ALL OF MINIMIKU LINUX IS FREE SOFTWARE
For more information, look at ./FREE.md 
 */
#include <linux/init.h>
#include <linux/kernel.h>

#define SYSMPB_VERSION "1.00"
#define MINIMIKU_HWNAME CONFIG_MINIMIKU_HWNAME

const char minimiku_hwname[] = MINIMIKU_HWNAME;

bool minimiku_autopanic;

static int __init minimiku_autopanic_setup(char *str)
{
    minimiku_autopanic = true;

    pr_info("MiniMiku: automatic panic enabled\n");

    return 0;
}

early_param("minimiku.autopanic", minimiku_autopanic_setup);


static int __init minimiku_sysmpb_init(void)
{
    pr_info("MiniMiku: system multipurpose builtin initialized\n");
    pr_info("MiniMiku: sysmpb version: %s\n", SYSMPB_VERSION);
    pr_info("MiniMiku: sysmpb: HWNAME:%s\n", minimiku_hwname);

    return 0;
}

core_initcall(minimiku_sysmpb_init);