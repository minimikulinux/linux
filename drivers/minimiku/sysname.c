// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/errno.h>

#define MINIMIKU_PROC_DIR "MiniMiku"

static struct proc_dir_entry *sysname_file;

static int sysname_show(struct seq_file *m, void *v)
{
    seq_puts(m, "MiniMiku\n");
    return 0;
}

static int sysname_open(struct inode *inode, struct file *file)
{
    return single_open(file, sysname_show, NULL);
}

static const struct proc_ops sysname_ops = {
    .proc_open    = sysname_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init minimiku_sysname_init(void)
{
    struct proc_dir_entry *dir;

    dir = proc_mkdir(MINIMIKU_PROC_DIR, NULL);
    if (!dir)
        return -ENOMEM;

    sysname_file = proc_create("sysname", 0444, dir, &sysname_ops);
    if (!sysname_file) {
        remove_proc_subtree(MINIMIKU_PROC_DIR, NULL);
        return -ENOMEM;
    }

    pr_info("MiniMiku: sysname initialized\n");

    return 0;
}
device_initcall(minimiku_sysname_init);