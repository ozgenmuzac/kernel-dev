/**
 * main.c -- the memcache char module
 *
 * 2016 Ozgen Muzac
 * 
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form.
 */

#include "memcache.h"

int memcache_major_no;

int numminors 		= MEMCACHE_DEVS;
int bufferlimit		= MEMCACHE_BUFFERLIMIT;

module_param(numminors, int, 0);
module_param(bufferlimit, int, 0);
MODULE_AUTHOR("Ozgen Muzac");
MODULE_LICENSE("GPL");

struct memcache_dev *memcachedev_devices; 		/* allocated in memcache_init */

/****************** File Operations ******************/
/**
 * Opens a device
 */
int memcache_open (struct inode *inode, struct file *filp)
{
	struct memcache_dev *dev;
	char cache_name[] = ""; 

	/* Find the minor device */
	dev = container_of(inode->i_cdev, struct memcache_dev, cdev);

	/* Let the file pointer point to device data */
	dev->current_index = 0;
	dev->cache = kmalloc(sizeof(struct memcache_cache), GFP_KERNEL);
	dev->cache->cache_name = (char*)&cache_name;
	dev->cache->next = NULL;
	filp->private_data = dev;

	return 0;
}

/**
 *	Closes a device
 */
int memcache_release (struct inode *inode, struct file *filp)
{
	return 0;
}

/**
 *	Reads from a device
 */
ssize_t memcache_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct memcache_dev *dev;
	struct memcache_cache *readed_cache;
	size_t ret_val;
	
	/* Get responsible character device that is set before */
	dev = filp->private_data;

	if (mutex_lock_interruptible(&dev->mutex))
	{
		return -ERESTARTSYS;
	}

	/* If count is smaller than size of long, do not accept it */
	if(count < sizeof(long))
	{
		ret_val=-EIO;
		goto err_mutex;
	}

	readed_cache = get_current_cache(dev);
	if(!readed_cache)
	{
		/* If there is no message available, return 0 */
		ret_val = 0;
		goto err_mutex;
	}

	/*If the actual length is shorter than read value, shorten the length again */
	if(readed_cache->actual_length < count)
	{
		count = readed_cache->actual_length;
	}
	
	if (copy_to_user (buf, readed_cache->data, count)) 
	{
		ret_val=-EFAULT;
		goto err_clear;
	}

	/* If all checks are passed, return count as readed bytes */
	ret_val = count;

	/* Deallocate resources and unlock mutex */
err_clear:
	kfree(readed_cache->data);
	kfree(readed_cache);

err_mutex:
	mutex_unlock(&dev->mutex);
	return ret_val;
}

/**
 *	Writes into a device
 */
ssize_t memcache_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct memcache_dev *dev;
	struct memcache_cache *cache;
	ssize_t ret_val;

	/* Get responsible character device that is set before */
	dev = filp->private_data;

	if (mutex_lock_interruptible(&dev->mutex))
	{
		return -ERESTARTSYS;
	}	


	/* If the message is longer than the max. length, truncate the message */
	if(count > bufferlimit)
	{
		count = bufferlimit;
	}


	/* Alloc place for struct on heap, do not continue if it fails */
	cache = get_current_cache(dev);
	if(!cache)
	{
		mutex_unlock(&dev->mutex);
		return -ENOMEM;
	}

	/* Alloc place for message data. If fails, do not forget to deallocate heap! */
	cache->data = kmalloc(count, GFP_KERNEL);
	if(!cache->data)
	{
		mutex_unlock(&dev->mutex);
		return -ENOMEM;
	}

	/* If prio value is not fetched, do not forget to deallocate data and heap! */
	if (copy_from_user (cache->data, buf, count)) 
	{
		mutex_unlock(&dev->mutex);
		kfree(cache->data);	
		return -EIO;
	}
	cache->actual_length = count;

	/* Unlock mutex and return the result */
	mutex_unlock(&dev->mutex);
	return count;
}

/**
 *	Polls a device
 */
ssize_t memcache_poll (struct file *filp, const char __user *buf, size_t count,loff_t *f_pos)
{
	return (ssize_t) 0;
}


/**
 * The ioctl() implementation of device
 */
long memcache_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0l;
}

/****************** Kernel Module Operations ******************/
/**
 *	Module loader function
 */
int memcache_init(void)
{
	int result, i;
	dev_t major_dev;

	/* Create a device with automatic major number (first argument is 0) */
	major_dev = MKDEV(0, 0);

	/* Allocate new device region and link it with created device */
	result = alloc_chrdev_region(&major_dev, 0, numminors, MEMCACHE_NAME);
	if(result < 0)
	{
		return result;
	}

	/* Get the major number of new device */
	memcache_major_no = MAJOR(major_dev);
	
	/* Allocate heap for minor devices */
	memcachedev_devices = kmalloc(numminors*sizeof(struct memcache_dev), GFP_KERNEL);
	if(!memcachedev_devices)
	{
		unregister_chrdev_region(major_dev, numminors);
		return -ENOMEM;
	}

	/* Clear all bytes and initialize all minor devices*/
	memset(memcachedev_devices, 0, numminors*sizeof (struct memcache_dev));
	for(i = 0; i < numminors; i++)
	{
		mutex_init(&memcachedev_devices[i].mutex);
		setup_minor_cdev(memcachedev_devices + i, i);
	}
	printk(KERN_INFO "Memcache Major no: %d\n", memcache_major_no);
	
	return 0;
}

/**
 *	Module unloader function
 */
void memcache_exit(void)
{
	int i;

	/* 	Remove all character devices from kernel 
		Deallocate the resources allocated for minor devices */
	for (i = 0; i < numminors; i++) 
	{
		cdev_del(&memcachedev_devices[i].cdev);
	}

	/* Free allocated heap for all minor devices */
	kfree(memcachedev_devices);

	/* Unregister the major region for character device */
	unregister_chrdev_region(MKDEV (memcache_major_no, 0), numminors);
}

module_init(memcache_init);
module_exit(memcache_exit);

void setup_minor_cdev(struct memcache_dev *minor_dev, int index)
{
	int result, minor_dev_no;

	/* Create a minor device number */
	minor_dev_no = MKDEV(memcache_major_no, index);

	/* Set initial values on character device */
	cdev_init(&minor_dev->cdev, &memcachedev_fops);
	minor_dev->cdev.owner = THIS_MODULE;
	minor_dev->cdev.ops = &memcachedev_fops;
	minor_dev->current_index= 0;
	
	/* Try to add minor character device into kernel */
	result = cdev_add (&minor_dev->cdev, minor_dev_no, 1);
	if (result)
	{
		/* Fail gracefully */
		printk(KERN_NOTICE "Error %d adding memcache%d", result, index);
	}
	printk(KERN_INFO "Minor device is introduced: %d\n", minor_dev_no);
}



struct memcache_cache * get_current_cache(struct memcache_dev *dev)
{
	int i;
	struct memcache_cache *cache = dev->cache;
	for(i = 0;cache != NULL;i++)
	{
		if(i == dev->current_index)
			return cache;
		else
			cache = cache->next;
	}
	return NULL;
}
