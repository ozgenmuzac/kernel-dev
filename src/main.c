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

int bufferlimit		= MEMCACHE_BUFFERLIMIT;
int numminors 		= MEMCACHE_DEVS;

module_param(bufferlimit, int, 0);
module_param(numminors, int, 0);
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
	if ( (filp->f_flags & O_ACCMODE) == O_RDONLY) 
		dev->num_readonly += 1;
	else
		dev->num_write_process += 1; 
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
	struct memcache_dev *dev;
	struct memcache_each_proc *ep;
	
	ep = filp->private_data;
	dev = ep->dev;

	if ( (filp->f_flags & O_ACCMODE) == O_RDONLY) 
		dev->num_readonly -= 1;
	else
		dev->num_write_process -= 1; 
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

/*	if (mutex_lock_interruptible(&dev->mutex))
	{
		return -ERESTARTSYS;
	}
*/

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

	if(readed_cache->actual_length <= ep->file_position)
	{
		ret_val = 0;
		goto err_mutex;
	}

	/*If the actual length is shorter than read value, shorten the length again */
	if(readed_cache->actual_length < (ep->file_position + count))
	{
		count = readed_cache->actual_length - ep->file_position;
	}
	printk(KERN_INFO "Copy to user count: %d\n", count);	
	if (copy_to_user (buf, readed_cache->data+ep->file_position, count)) 
	{
		ret_val=-EFAULT;
		goto err_mutex;
	}

	/* If all checks are passed, return count as readed bytes */
	ret_val = count;

err_mutex:
	//mutex_unlock(&dev->mutex);
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

	/*if (mutex_lock_interruptible(&dev->mutex))
	{
		return -ERESTARTSYS;
	}*/	


	/* If the message is longer than the max. length, truncate the message */
	if(ep->file_position + count > bufferlimit)
	{
		count = bufferlimit - ep->file_position;
	}

	cache = get_current_cache(ep);
	printk(KERN_INFO "Cache name write: %s\n", cache->cache_name);
	if(&cache->mutex == NULL) {
		printk(KERN_INFO "Cache mutex null!");
	}
	if (mutex_lock_interruptible(&(cache->mutex)))
	{
		return -ERESTARTSYS;
	}	
	printk(KERN_INFO "Write Count: %d\n", count);
	if(!cache)
	{
		mutex_unlock(&cache->mutex);
		return -ENOMEM;
	}

	if(!cache->data)
	{
		cache->data = kmalloc(bufferlimit, GFP_KERNEL);
	}

	if((ep->file_position + count) > bufferlimit)
	{
		mutex_unlock(&cache->mutex);
		return -EFBIG;
	}
	printk(KERN_INFO "Data before copy: %s\n", cache->data);	
	if (copy_from_user (cache->data+ep->file_position, buf, count)) 
	{
		mutex_unlock(&cache->mutex);
		kfree(cache->data);	
		return -EIO;
	}
	cache->actual_length = ep->file_position + count;
	ep->file_position += count;
	printk(KERN_INFO "Write data: %s\n", cache->data);
	printk(KERN_INFO "Actual length: %d\n", cache->actual_length);
	printk(KERN_INFO "EP file position: %d\n", ep->file_position);

	/* Unlock mutex and return the result */
	mutex_unlock(&cache->mutex);
	return count;
}


loff_t memcache_llseek (struct file *filp, loff_t off, int whence)
{
	struct memcache_each_proc *ep;
	struct memcache_dev *dev;
	struct memcache_cache *cache;
	int maxoffset = 0;
	//ssize_t ret_val;

	/* Get responsible character device that is set before */
	ep = filp->private_data;
	dev = ep->dev;
	cache = get_current_cache(ep);

	if (mutex_lock_interruptible(&dev->mutex))
	{
		return -ERESTARTSYS;
	}
	if((filp->f_flags & O_ACCMODE) == O_RDONLY)
		maxoffset = cache->actual_length;
	else
		maxoffset = bufferlimit;
	switch(whence) {
        	case 0: /* SEEK_SET */
			if(off > maxoffset)	
				ep->file_position = maxoffset;
			else
				ep->file_position = off;
                	break;

		case 1: /* SEEK_CUR */
			if((ep->file_position + off) > maxoffset)
				ep->file_position = maxoffset;
			else
				ep->file_position += off;
			break;

		case 2: /* SEEK_END */
			if((bufferlimit + off) > maxoffset)
				ep->file_position = maxoffset;
			else
				ep->file_position = bufferlimit + off;
			break;

		default: /* can't happen */
			return -EINVAL;
        }
        if (ep->file_position < 0) return -EINVAL;
        filp->f_pos = ep->file_position;
	printk(KERN_INFO "File position after seek: %d\n", ep->file_position);
	mutex_unlock(&dev->mutex);
        return ep->file_position;
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
					mutex_init(&(new_cache->mutex));
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
	minor_dev->num_readonly = 0;
	minor_dev->num_write_process = 0;
	mutex_init(&(minor_dev->cache->mutex));
	
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
