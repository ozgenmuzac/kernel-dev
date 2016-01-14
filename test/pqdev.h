#include <linux/ioctl.h>
#define PQDEV_IOC_MAGIC 'Q'

#define PQDEV_IOCRESET 		_IO(PQDEV_IOC_MAGIC, 0)
#define PQDEV_IOCQMINPRI 	_IO(PQDEV_IOC_MAGIC, 1)
#define PQDEV_IOCHNUMMSG 	_IO(PQDEV_IOC_MAGIC, 2)
#define PQDEV_IOCTSETMAXMSGS 	_IO(PQDEV_IOC_MAGIC, 3)
#define PQDEV_IOCGMINMSG 	_IOR(PQDEV_IOC_MAGIC, 4, int)
#define PQDEV_IOCTSETREADPRIO	_IO(PQDEV_IOC_MAGIC, 5)
#define PQDEV_IOC_MAXNR 5



