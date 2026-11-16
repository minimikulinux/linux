// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * MiniMiku Linux
 * DevRAM - Shared Memory RAM File
 */

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVRAM_INITIAL_SIZE (1ULL << 30)
#define DEVRAM_MAX_SIZE     (5ULL << 30)

#define DEVRAM_INITIAL_PAGES (DEVRAM_INITIAL_SIZE >> PAGE_SHIFT)
#define DEVRAM_MAX_PAGES     (DEVRAM_MAX_SIZE >> PAGE_SHIFT)

static struct page **devram_pages;
static DEFINE_MUTEX(devram_lock);

static struct page *devram_get_page(unsigned long index)
{
	struct page *page;

	if (index >= DEVRAM_MAX_PAGES)
		return NULL;

	mutex_lock(&devram_lock);

	page = devram_pages[index];

	if (!page) {
		page = alloc_page(GFP_KERNEL | __GFP_ZERO);

		if (page)
			devram_pages[index] = page;
	}

	mutex_unlock(&devram_lock);

	return page;
}

static ssize_t devram_read(struct file *file,
			   char __user *buf,
			   size_t count,
			   loff_t *ppos)
{
	size_t done = 0;

	if (*ppos < 0)
		return -EINVAL;

	if (*ppos >= DEVRAM_MAX_SIZE)
		return 0;

	count = min_t(u64, count, DEVRAM_MAX_SIZE - *ppos);

	while (done < count) {
		unsigned long index = *ppos >> PAGE_SHIFT;
		unsigned long offset = *ppos & (PAGE_SIZE - 1);
		size_t chunk = min_t(size_t, count - done,
				     PAGE_SIZE - offset);
		struct page *page = devram_get_page(index);
		void *mapping;
		unsigned long missing;

		if (!page) {
			missing = clear_user(buf + done, chunk);
		} else {
			mapping = kmap_local_page(page);

			missing = copy_to_user(buf + done,
					       (char *)mapping + offset,
					       chunk);

			kunmap_local(mapping);
		}

		if (missing)
			return done ? done : -EFAULT;

		done += chunk;
		*ppos += chunk;
	}

	return done;
}

static ssize_t devram_write(struct file *file,
			    const char __user *buf,
			    size_t count,
			    loff_t *ppos)
{
	size_t done = 0;

	if (*ppos < 0)
		return -EINVAL;

	if (*ppos >= DEVRAM_MAX_SIZE)
		return -ENOSPC;

	count = min_t(u64, count, DEVRAM_MAX_SIZE - *ppos);

	while (done < count) {
		unsigned long index = *ppos >> PAGE_SHIFT;
		unsigned long offset = *ppos & (PAGE_SIZE - 1);
		size_t chunk = min_t(size_t, count - done,
				     PAGE_SIZE - offset);
		struct page *page = devram_get_page(index);
		void *mapping;
		unsigned long missing;

		if (!page)
			return done ? done : -ENOMEM;

		mapping = kmap_local_page(page);

		missing = copy_from_user((char *)mapping + offset,
					 buf + done,
					 chunk);

		kunmap_local(mapping);

		if (missing)
			return done ? done : -EFAULT;

		done += chunk;
		*ppos += chunk;
	}

	return done;
}

static vm_fault_t devram_fault(struct vm_fault *vmf)
{
	unsigned long index = vmf->pgoff;
	struct page *page;

	if (index >= DEVRAM_MAX_PAGES)
		return VM_FAULT_SIGBUS;

	page = devram_get_page(index);

	if (!page)
		return VM_FAULT_OOM;

	return vmf_insert_page(vmf->vma, vmf->address, page);
}

static const struct vm_operations_struct devram_vm_ops = {
	.fault = devram_fault,
};

static int devram_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long pages =
		(vma->vm_end - vma->vm_start) >> PAGE_SHIFT;

	if (!(vma->vm_flags & VM_SHARED))
		return -EINVAL;

	if (vma->vm_pgoff > DEVRAM_MAX_PAGES ||
	    pages > DEVRAM_MAX_PAGES - vma->vm_pgoff)
		return -EINVAL;

	vm_flags_set(vma, VM_DONTEXPAND | VM_DONTDUMP);

	vma->vm_ops = &devram_vm_ops;

	return 0;
}

static loff_t devram_llseek(struct file *file,
			    loff_t offset,
			    int whence)
{
	return fixed_size_llseek(file, offset, whence,
				 DEVRAM_MAX_SIZE);
}

static const struct file_operations devram_fops = {
	.owner = THIS_MODULE,
	.read = devram_read,
	.write = devram_write,
	.mmap = devram_mmap,
	.llseek = devram_llseek,
};

static struct miscdevice devram_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "devram",
	.fops = &devram_fops,
	.mode = 0660,
};

static int __init devram_init(void)
{
	unsigned long i;
	int ret;

	devram_pages = kvcalloc(DEVRAM_MAX_PAGES,
				sizeof(*devram_pages),
				GFP_KERNEL);

	if (!devram_pages)
		return -ENOMEM;

	pr_info("MiniMiku DevRAM: Allocating initial 1 GiB\n");

	for (i = 0; i < DEVRAM_INITIAL_PAGES; i++) {
		devram_pages[i] = alloc_page(GFP_KERNEL | __GFP_ZERO);

		if (!devram_pages[i]) {
			pr_err("MiniMiku DevRAM: Initial allocation failed at page %lu\n",
			       i);
			ret = -ENOMEM;
			goto fail;
		}
	}

	ret = misc_register(&devram_device);

	if (ret)
		goto fail;

	pr_info("MiniMiku DevRAM: Initialized (1 GiB initial, 5 GiB maximum)\n");

	return 0;

fail:
	for (i = 0; i < DEVRAM_MAX_PAGES; i++) {
		if (devram_pages[i])
			__free_page(devram_pages[i]);
	}

	kvfree(devram_pages);
	devram_pages = NULL;

	return ret;
}

static void __exit devram_exit(void)
{
	unsigned long i;

	misc_deregister(&devram_device);

	for (i = 0; i < DEVRAM_MAX_PAGES; i++) {
		if (devram_pages[i])
			__free_page(devram_pages[i]);
	}

	kvfree(devram_pages);
	devram_pages = NULL;
}

module_init(devram_init);
module_exit(devram_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MiniMiku DevRAM shared memory device");
MODULE_AUTHOR("MiniMiku Linux");
