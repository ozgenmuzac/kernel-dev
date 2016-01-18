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
	struct memcache_each_proc *ep;

	/* Find the minor device */
	dev = container_of(inode->i_cdev, struct memcache_dev, cdev);

	/* Let the file pointer point to device data */
	ep = kmalloc(sizeof(struct memcache_each_proc), GFP_KERNEL);
	ep->dev = dev;
	ep->current_index = 0;
	ep->file_position = 0;
	filp->private_data = ep;

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
	struct memcache_each_proc *ep;
	struct memcache_cache *readed_cache;
	size_t ret_val;
	
	/* Get responsible character device that is set before */
	ep = filp->private_data;
	dev = ep->dev;

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
	if(dev->cache == NULL)
	{
		ret_val = 0;
		goto err_mutex;
	} 

	readed_cache = get_current_cache(ep);
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
	struct memcache_each_proc *ep;
	struct memcache_dev *dev;
	struct memcache_cache *cache;
	//ssize_t ret_val;

	/* Get responsible character device that is set before */
	ep = filp->private_data;
	dev = ep->dev;

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
	cache = get_current_cache(ep);
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
	int err = 0, ret = 0, i;
	struct memcache_each_proc *ep;
	struct memcache_dev *dev;
	struct memcache_cache *cache, *new_cache;

	if (_IOC_TYPE(cmd) != MEMCACHE_IOC_MAGIC) return -ENOTTY;

	if (_IOC_NR(cmd) > MEMCACHE_IOC_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if(err)
		return -EFAULT;

	ep = filp->private_data;
	dev = ep->dev;
	cache = dev->cache; 
	switch(cmd) {
		case MEMCACHE_IOCCRESET:
			printk(KERN_INFO "Reset\n");
			if (mutex_lock_interruptible(&dev->mutex))
			{
				return -ERESTARTSYS;
			}
			if(cache == NULL)
			{
				mutex_unlock(&dev->mutex);
				return 0;
			}
			for(;;) {
				struct memcache_cache *cache_next;
				cache_next = cache->next;
				kfree(cache);
				if(cache_next == NULL)
					break;
				cache = cache_next;
			}
			ep->current_index = 0;
			dev->cache = NULL;
			mutex_unlock(&dev->mutex);
			break;
		case MEMCACHE_IOCGETCACHE:
			printk(KERN_INFO "Current index: %d\n", ep->current_index);
			if (mutex_lock_interruptible(&dev->mutex))
			{
				return -ERESTARTSYS;
			}
			for(i = 0;;i++)
			{
				if(i == ep->current_index) {
					printk(KERN_INFO "IN IT!!\n");
					copy_to_user((void*)arg, (void*)cache->cache_name, strlen(cache->cache_name)+1);
					break;
				}
				cache = cache->next;
			}
			mutex_unlock(&dev->mutex);
			break;
		case MEMCACHE_IOCSETCACHE:
			printk(KERN_INFO "Set name: %s\n", (char*)arg);
			if (mutex_lock_interruptible(&dev->mutex))
			{
				return -ERESTARTSYS;
			}
			for(i = 0;;i++)
			{
				printk(KERN_INFO "Cache name: %s\n", cache->cache_name);
				if(strcmp(cache->cache_name, (char*)arg) == 0)//cache founded
				{
					printk(KERN_INFO "Cache founded: %d\n", i);
					ep->current_index = i;
					ep->file_position = 0;
					break;
				}
				else if(cache->next == NULL)
				{
					printk(KERN_INFO "Cache is adding: %d\n", i);
					new_cache = kmalloc(sizeof(struct memcache_cache), GFP_KERNEL);
					strcpy(new_cache->cache_name, (char*)arg);
					new_cache->actual_length = 0;
					new_cache->next = NULL;
					cache->next = new_cache;
					ep->file_position = 0;
					ep->current_index = i+1;
					break;
				}
				
				else
					cache = cache->next;
			}
			mutex_unlock(&dev->mutex);
			break;
		case MEMCACHE_IOCTRUNC:
			if (mutex_lock_interruptible(&dev->mutex))
			{
				return -ERESTARTSYS;
			}
			for(i = 0;;i++)
			{
				if(i == ep->current_index)
				{
					cache->data = NULL;
					cache->actual_length = 0;
					break;
				}
				else
					cache = cache->next;
			}
			mutex_unlock(&dev->mutex);
			break;
		case MEMCACHE_IOCQBUFSIZE:
			if (mutex_lock_interruptible(&dev->mutex))
			{
				return -ERESTARTSYS;
			}
			for(i =0;;i++)
			{
				if(i == ep->current_index)
				{
					printk(KERN_INFO "Buffer size of: %s\n", cache->cache_name);
					mutex_unlock(&dev->mutex);
					return cache->actual_length;
				}
				else
					cache = cache->next;
			}
			mutex_unlock(&dev->mutex);
			break;
		case MEMCACHE_IOCGTESTCACHE:
			if (mutex_lock_interruptible(&dev->mutex))
			{
				return -ERESTARTSYS;
			}
			for(i = 0;;i++)
			{
				if(cache == NULL)
				{
					ret = 1;
					break;
				}
				else if(strcmp(cache->cache_name, (char*)arg) == 0)
				{
					ret = 0;
					break;
				}
				else
					cache = cache->next;
			}
			mutex_unlock(&dev->mutex);
			break;
		default:
			return -ENOTTY;
	}
	
	return ret;
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
	//char cache_name = "\0";

	/* Create a minor device number */
	minor_dev_no = MKDEV(memcache_major_no, index);

	/* Set initial values on character device */
	cdev_init(&minor_dev->cdev, &memcachedev_fops);
	minor_dev->cdev.owner = THIS_MODULE;
	minor_dev->cdev.ops = &memcachedev_fops;
	minor_dev->cache = kmalloc(sizeof(struct memcache_cache), GFP_KERNEL);
	minor_dev->cache->cache_name[0] = '\0';
	
	/* Try to add minor character device into kernel */
	result = cdev_add (&minor_dev->cdev, minor_dev_no, 1);
	if (result)
	{
		/* Fail gracefully */
		printk(KERN_NOTICE "Error %d adding memcache%d", result, index);
	}
	printk(KERN_INFO "Minor device is introduced: %d\n", minor_dev_no);
}



struct memcache_cache * get_current_cache(struct memcache_each_proc *ep)
{
	int i;
	struct memcache_cache *cache = ep->dev->cache;
	for(i = 0;cache != NULL;i++)
	{
		if(i == ep->current_index)
			return cache;
		else
			cache = cache->next;
	}
	return NULL;
}
