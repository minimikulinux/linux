// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init minimiku_test_init(void)
{
    pr_info("MiniMiku: test module loaded\n");
    return 0;
}

static void __exit minimiku_test_exit(void)
{
    pr_info("MiniMiku: test module unloaded\n");
}

module_init(minimiku_test_init);
module_exit(minimiku_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MiniMiku");
MODULE_DESCRIPTION("MiniMiku test kernel module");