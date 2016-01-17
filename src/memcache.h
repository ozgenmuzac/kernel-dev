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
	char *cache_name;
	struct memcache_cache *next;
};

struct memcache_dev 
{
	struct memcache_cache *cache;
	struct mutex mutex;
	struct cdev cdev;
	int current_index;
};

/********* Ioctl Definitions *********/
#define PQDEV_IOC_MAGIC  		'Q'
#define PQDEV_IOCRESET    		_IO(PQDEV_IOC_MAGIC, 0)
#define PQDEV_IOCQMINPRI 		_IO(PQDEV_IOC_MAGIC, 1)
#define PQDEV_IOCHNUMMSG 		_IO(PQDEV_IOC_MAGIC, 2)
#define PQDEV_IOCTSETMAXMSGS 	_IO(PQDEV_IOC_MAGIC, 3)
#define PQDEV_IOCGMINMSG 		_IOR(PQDEV_IOC_MAGIC, 4, int)
#define PQDEV_IOC_MAXNR 		4

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
struct memcache_cache * get_current_cache(struct memcache_dev *dev);
