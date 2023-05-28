/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Your Name Here"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    struct aesd_dev *dev;
    struct aesd_buffer_entry *entry;
    ssize_t read_offs;
    ssize_t  n_read, not_copied, to_read, just_copied, search_offset;
    int i;
    if (filp == NULL) return -EFAULT;
    dev = (struct aesd_dev*) filp->private_data; // circular buffer
    if (dev == NULL) return -EFAULT;
    if (mutex_lock_interruptible(&dev->lock)) {
	return -ERESTARTSYS;
    }
    search_offset = *f_pos;
    n_read = 0;
    for (i=0; i < 10; i++) {
	entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, search_offset, &read_offs);
	if (entry == NULL) break;
	to_read = entry->size - read_offs;
	if (to_read > count) to_read = count;
	not_copied = copy_to_user(buf + n_read, entry->buffptr + read_offs, to_read);
	just_copied = to_read - not_copied;
	n_read += just_copied;
	count -= just_copied;
	search_offset += just_copied;
	if (count == 0) break;
    }
    *f_pos += n_read;
    mutex_unlock(&dev->lock);
    return n_read;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    struct aesd_dev *dev;
    int rc;
    ssize_t n_written;
    if (filp == NULL) return -EFAULT;
    dev = (struct aesd_dev*) filp->private_data;
    if (mutex_lock_interruptible(&dev->lock)) {
	return -ERESTARTSYS;
    }
    if (dev->working.size == 0) {
	dev->working.buffptr = (char *) kmalloc(count, GFP_KERNEL);
    } else {
    	dev->working.buffptr = (char *) krealloc(dev->working.buffptr, dev->working.size + count, GFP_KERNEL);
    }
    if (dev->working.buffptr == NULL) {
    	n_written = -ENOMEM;
    } else {
    	rc = copy_from_user((void *) (dev->working.buffptr + dev->working.size), buf, count);
	n_written = count - rc;
	dev->working.size += n_written;
	if (dev->working.buffptr[dev->working.size-1] == '\n') {
	    if (dev->buffer.full) {
	    	kfree(dev->buffer.entry[dev->buffer.out_offs].buffptr);
	    }
	    aesd_circular_buffer_add_entry(&dev->buffer, &dev->working);
	    dev->working.buffptr = NULL;
	    dev->working.size = 0;
	}
    }
    mutex_unlock(&dev->lock);
    return n_written;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    mutex_init(&aesd_device.lock);
    aesd_circular_buffer_init(&aesd_device.buffer);
    aesd_device.working.size = 0;
    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    if (aesd_device.working.size > 0) {
    	kfree(aesd_device.working.buffptr);
	aesd_device.working.size = 0;
    }
    for(i=0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
    	if (aesd_device.buffer.entry[i].size > 0) {
	    kfree(aesd_device.buffer.entry[i].buffptr);
	    aesd_device.buffer.entry[i].size = 0;
	}
    }
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
