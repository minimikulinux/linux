// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>

#define MIKU_SYS_NAME "mmctl"

static const char miku_sys_info[] =
	"MiniMiku System Interface\n";

static ssize_t miku_sys_read(struct file *file,
			    char __user *buffer,
			    size_t count,
			    loff_t *offset)
{
	return simple_read_from_buffer(buffer, count, offset,
				     miku_sys_info,
				     sizeof(miku_sys_info) - 1);
}

static const struct file_operations miku_sys_fops = {
	.owner		= THIS_MODULE,
	.read		= miku_sys_read,
	.llseek		= default_llseek,
};

static struct miscdevice miku_sys_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= MIKU_SYS_NAME,
	.fops		= &miku_sys_fops,
	.mode		= 0444,
};

static int __init miku_sys_init(void)
{
	return misc_register(&miku_sys_device);
}

static void __exit miku_sys_exit(void)
{
	misc_deregister(&miku_sys_device);
}

module_init(miku_sys_init);
module_exit(miku_sys_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MiniMiku system device interface");