// SPDX-License-Identifier: GPL-2.0-or-later
// drivers/minimiku/sys.c --
// The stupidest file
// And also a multipurpose driver

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>

#define MINIMIKU_SYS_NAME "MiniMiku Linux"

static int __init minimiku_sys_init(void)
{
    pr_info("MiniMiku: system driver initialized\n");
    pr_info("MiniMiku: system name: %s\n", MINIMIKU_SYS_NAME);

    return 0;
}

static void __exit minimiku_sys_exit(void)
{
    pr_info("MiniMiku: system driver unloaded\n");
}

module_init(minimiku_sys_init);
module_exit(minimiku_sys_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MiniMiku Linux Developers");
MODULE_DESCRIPTION("MiniMiku multipurpose system driver");