#ifndef _LINUX_MINIMIKU_MLOG_H
#define _LINUX_MINIMIKU_MLOG_H

#include <linux/compiler_attributes.h>

int mlog_printk(const char *fmt, ...) __printf(1, 2);

#endif