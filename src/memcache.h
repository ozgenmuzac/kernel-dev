/* -*- C -*-
 * memcache.h -- definitions for the memcache char module
 *
 * 2016 Ozgen Muzac
 * 
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form.
 */

#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>		/* printk() */
#include <linux/slab.h>			/* kmalloc() */
#include <linux/fs.h>			/* everything about io */
#include <linux/errno.h>		/* error codes */
#include <linux/types.h>		/* size_t */
#include <asm/uaccess.h>

#define MEMCACHE_NAME		"memcache"

#define MEMCACHE_DEVS 		4 		/* number of minor devices */
#define MEMCACHE_BUFFERLIMIT	4096	/* maximum length of a message */

/********* Character Device Definitions *********/
struct memcache_cache
{
	int actual_length;
	char *data;
	char cache_name[1024];
	struct memcache_cache *next;
};

struct memcache_each_proc
{
	int current_index;
	int file_position;
	struct memcache_dev *dev;
};

struct memcache_dev 
{
	struct memcache_cache *cache;
	struct mutex mutex;
	struct cdev cdev;
};

/********* Ioctl Definitions *********/
#define MEMCACHE_IOC_MAGIC  		'm'
#define MEMCACHE_IOCCRESET	    	_IO(MEMCACHE_IOC_MAGIC, 0)
#define MEMCACHE_IOCGETCACHE    	_IOR(MEMCACHE_IOC_MAGIC, 1, int)
#define MEMCACHE_IOCSETCACHE 		_IOW(MEMCACHE_IOC_MAGIC, 2, int)
#define MEMCACHE_IOCTRUNC 		_IO(MEMCACHE_IOC_MAGIC, 3)
#define MEMCACHE_IOCQBUFSIZE 		_IO(MEMCACHE_IOC_MAGIC, 4)
#define MEMCACHE_IOCGTESTCACHE 		_IOR(MEMCACHE_IOC_MAGIC, 5, int)
#define MEMCACHE_IOC_MAXNR 		5

/********* File Operations *********/
int memcache_open (struct inode *inode, struct file *filp);
int memcache_release (struct inode *inode, struct file *filp);
ssize_t memcache_read (struct file *filp, char __user *buf, size_t count,loff_t *f_pos);
ssize_t memcache_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
ssize_t memcache_poll (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
long memcache_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);

struct file_operations memcachedev_fops = 
{
	.owner 			= THIS_MODULE,
	.read 			= memcache_read,
	.write 			= memcache_write,
	.unlocked_ioctl 	= memcache_ioctl,
	.open 			= memcache_open,
	.release 		= memcache_release,
	.poll 			= memcache_poll
};

extern struct file_operations memcachedev_fops;

/********* Prototype for module functions *********/
int memcache_init(void);
void memcache_exit(void);


void setup_minor_cdev(struct memcache_dev *minor_dev, int index);
struct memcache_cache * get_current_cache(struct memcache_each_proc *ep);
