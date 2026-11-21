#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/mlog.h>

#define MLOG_BUFFER_SIZE (64 * 1024)
#define MLOG_MAX_WRITE   4096
#define MLOG_FORMAT_SIZE 512

static char mlog_buffer[MLOG_BUFFER_SIZE];

static u64 mlog_head;

static DEFINE_SPINLOCK(mlog_lock);
static DECLARE_WAIT_QUEUE_HEAD(mlog_wait);

struct mlog_reader {
	u64 position;
	struct mutex lock;
};

static void mlog_append(const char *data, size_t len)
{
	unsigned long flags;
	size_t index;
	size_t first;

	if (!len)
		return;

	if (len > MLOG_BUFFER_SIZE) {
		data += len - MLOG_BUFFER_SIZE;
		len = MLOG_BUFFER_SIZE;
	}

	spin_lock_irqsave(&mlog_lock, flags);

	index = mlog_head % MLOG_BUFFER_SIZE;
	first = min_t(size_t, len, MLOG_BUFFER_SIZE - index);

	memcpy(mlog_buffer + index, data, first);

	if (len > first)
		memcpy(mlog_buffer, data + first, len - first);

	mlog_head += len;

	spin_unlock_irqrestore(&mlog_lock, flags);

	wake_up_interruptible(&mlog_wait);
}

int mlog_printk(const char *fmt, ...)
{
	va_list args;
	char buffer[MLOG_FORMAT_SIZE];
	int len;

	va_start(args, fmt);
	len = vscnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	mlog_append(buffer, len);

	return len;
}
EXPORT_SYMBOL_GPL(mlog_printk);

static bool mlog_has_data(struct mlog_reader *reader)
{
	unsigned long flags;
	bool available;

	spin_lock_irqsave(&mlog_lock, flags);
	available = reader->position < mlog_head;
	spin_unlock_irqrestore(&mlog_lock, flags);

	return available;
}

static int mlog_open(struct inode *inode, struct file *file)
{
	struct mlog_reader *reader;
	unsigned long flags;

	reader = kzalloc(sizeof(*reader), GFP_KERNEL);
	if (!reader)
		return -ENOMEM;

	mutex_init(&reader->lock);

	spin_lock_irqsave(&mlog_lock, flags);
	reader->position = mlog_head;
	spin_unlock_irqrestore(&mlog_lock, flags);

	file->private_data = reader;

	return 0;
}

static int mlog_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

static ssize_t mlog_read(struct file *file,
			 char __user *user_buffer,
			 size_t count,
			 loff_t *offset)
{
	struct mlog_reader *reader = file->private_data;
	unsigned long flags;
	char *buffer;
	u64 start;
	u64 available;
	size_t len;
	size_t capacity;
	size_t index;
	size_t first;
	int ret;

	if (!count)
		return 0;

	capacity = min_t(size_t, count, MLOG_BUFFER_SIZE);

	buffer = kmalloc(capacity, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	if (mutex_lock_interruptible(&reader->lock)) {
		kfree(buffer);
		return -ERESTARTSYS;
	}

	if (file->f_flags & O_NONBLOCK) {
		if (!mlog_has_data(reader)) {
			ret = -EAGAIN;
			goto out;
		}
	} else {
		ret = wait_event_interruptible(mlog_wait,
					       mlog_has_data(reader));
		if (ret)
			goto out;
	}

	spin_lock_irqsave(&mlog_lock, flags);

	start = reader->position;

	if (start < mlog_head - min_t(u64, mlog_head,
					     MLOG_BUFFER_SIZE))
		start = mlog_head - MLOG_BUFFER_SIZE;

	available = mlog_head - start;
	len = min_t(u64, capacity, available);

	index = start % MLOG_BUFFER_SIZE;
	first = min_t(size_t, len, MLOG_BUFFER_SIZE - index);

	memcpy(buffer, mlog_buffer + index, first);

	if (len > first)
		memcpy(buffer + first, mlog_buffer, len - first);

	spin_unlock_irqrestore(&mlog_lock, flags);

	if (copy_to_user(user_buffer, buffer, len)) {
		ret = -EFAULT;
		goto out;
	}

	reader->position = start + len;
	ret = len;

out:
	mutex_unlock(&reader->lock);
	kfree(buffer);

	return ret;
}

static ssize_t mlog_write(struct file *file,
			  const char __user *user_buffer,
			  size_t count,
			  loff_t *offset)
{
	char *buffer;
	size_t len;

	if (!count)
		return 0;

	len = min_t(size_t, count, MLOG_MAX_WRITE);

	buffer = kmalloc(len, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	if (copy_from_user(buffer, user_buffer, len)) {
		kfree(buffer);
		return -EFAULT;
	}

	mlog_append(buffer, len);

	kfree(buffer);

	return len;
}

static __poll_t mlog_poll(struct file *file, poll_table *wait)
{
	struct mlog_reader *reader = file->private_data;
	__poll_t mask = 0;

	poll_wait(file, &mlog_wait, wait);

	if (mlog_has_data(reader))
		mask |= EPOLLIN | EPOLLRDNORM;

	return mask;
}

static const struct file_operations mlog_fops = {
	.owner = THIS_MODULE,
	.open = mlog_open,
	.release = mlog_release,
	.read = mlog_read,
	.write = mlog_write,
	.poll = mlog_poll,
	.llseek = noop_llseek,
};

static struct miscdevice mlog_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mlog",
	.fops = &mlog_fops,
	.mode = 0644,
};

static int __init mlog_init(void)
{
	int ret;

	ret = misc_register(&mlog_device);
	if (ret)
		return ret;

	pr_info("MiniMiku: /dev/mlog initialized\n");

	return 0;
}

static void __exit mlog_exit(void)
{
	misc_deregister(&mlog_device);
}

module_init(mlog_init);
module_exit(mlog_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MiniMiku Linux");
MODULE_DESCRIPTION("MiniMiku independent kernel logging subsystem");