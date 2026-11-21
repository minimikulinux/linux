// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/init.h>
#include <linux/kernel.h>

static bool minimiku_egg_enabled;

static int __init minimiku_egg_setup(char *str)
{
    minimiku_egg_enabled = true;

    pr_info("MiniMiku: Egg mode enabled!\n");

    return 0;
}

early_param("minimiku.egg", minimiku_egg_setup);