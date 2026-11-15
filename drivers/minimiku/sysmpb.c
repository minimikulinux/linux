// SPDX-License-Identifier: GPL-2.0-or-later
/*
MiniMiku software notice:
ALL OF MINIMIKU LINUX IS FREE SOFTWARE
For more information, look at ./FREE.md 
 */
#include <linux/init.h>
#include <linux/kernel.h>

#define SYSMPB_VERSION "1.00"

static int __init minimiku_sysmpb_init(void)
{
    pr_info("MiniMiku: system multipurpose builtin initialized\n");
    pr_info("MiniMiku: sysmpb version: %s\n", SYSMPB_VERSION);

    return 0;
}

core_initcall(minimiku_sysmpb_init);