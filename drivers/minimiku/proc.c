// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <linux/errno.h>

#define MINIMIKU_PROC_DIR "MiniMiku"

static struct proc_dir_entry *minimiku_proc_dir;

static int minimiku_version_show(struct seq_file *m, void *v)
{
    seq_printf(m, "MiniMiku Linux\n");
    seq_printf(m, "Kernel: %s\n", utsname()->release);

    return 0;
}

static int minimiku_version_open(struct inode *inode, struct file *file)
{
    return single_open(file, minimiku_version_show, NULL);
}

static const struct proc_ops minimiku_version_ops = {
    .proc_open    = minimiku_version_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init minimiku_proc_init(void)
{
    minimiku_proc_dir = proc_mkdir(MINIMIKU_PROC_DIR, NULL);
    if (!minimiku_proc_dir)
        return -ENOMEM;

    if (!proc_create("version", 0444, minimiku_proc_dir,
                     &minimiku_version_ops)) {
        remove_proc_subtree(MINIMIKU_PROC_DIR, NULL);
        return -ENOMEM;
    }

    pr_info("MiniMiku: proc interface initialized\n");

    return 0;
}
early_initcall(minimiku_proc_init);