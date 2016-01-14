#include<stdio.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<string.h>
#include<errno.h>

#include "pqdev.h"

char * manls[]={
"LS(1) User Commands LS(1)",
"NAME",
" ls - list directory contents",
"SYNOPSIS",
" ls [OPTION]... [FILE]...",
"DESCRIPTION",
" List information about the FILEs (the current directory by default).",
" Sort entries alphabetically if none of -cftuvSUX nor --sort is speci-",
" fied.",
" Mandatory arguments to long options are mandatory for short options",
" too.",
" -a, --all",
" do not ignore entries starting with .",
" -A, --almost-all",
" do not list implied . and ..",
" --author",
" with -l, print the author of each file",
" -b, --escape",
" print C-style escapes for nongraphic characters"};

struct mess {
	long pri;
	char body[1000];
};

int main() {
	int i;
	int n = sizeof(manls)/sizeof(manls[0]);
	struct mess m;
	int fd = open("/dev/pqdev0",O_RDWR);
	int r;

	printf("%d %d %d\n", _IOC_TYPE(PQDEV_IOCTSETMAXMSGS), 
		_IOC_NR(PQDEV_IOCTSETMAXMSGS),
		_IOC_SIZE(PQDEV_IOCTSETMAXMSGS)
		);
	printf("iocsetmaxmsgs: %d\n", ioctl(fd, PQDEV_IOCTSETMAXMSGS, 100));
	printf("%d\n", errno);
	perror("a");
	for (i = 0; i < 30; i++) {
		m.pri = i % 15 + 12;
		strncpy(m.body, manls[i%n], 1000);
		write(fd, &m, sizeof(long)+strlen(manls[i%n])+1);
	}
	printf("minpri: %d\n", ioctl(fd, PQDEV_IOCQMINPRI));
	printf("iochnummsg all: %d\n", ioctl(fd, PQDEV_IOCHNUMMSG, -1L));
	printf("iochnummsg(20): %d\n", ioctl(fd, PQDEV_IOCHNUMMSG, 20L));
	printf("minmsg: %d\n", ioctl(fd, PQDEV_IOCGMINMSG, (char *) &m));
	printf("%ld %s\n", m.pri, m.body);

	if (!fork()) {
		close(fd);
		int fd = open("/dev/pqdev0",O_RDWR);

		for (i = 0; i < 30; i++) {
			m.pri = i % 15 + 12;
			strncpy(m.body, manls[i%n], 1000);
			write(fd, &m, sizeof(long)+strlen(manls[i%n])+1);
			usleep(500000);
		}
		return 0;
	}

	/* BONUS 2:

	printf("setrpri=15\n", ioctl(fd, PQDEV_IOCTSETREADPRIO, 15));
	read(fd, &m, 1000);
	printf("%ld %s\n", m.pri, m.body);
	read(fd, &m, 1000);
	printf("%ld %s\n", m.pri, m.body);
	read(fd, &m, 1000);
	printf("%ld %s\n", m.pri, m.body);

	printf("setrpri=-20\n", ioctl(fd, PQDEV_IOCTSETREADPRIO, -20L));
	read(fd, &m, 1000);
	printf("%ld %s\n", m.pri, m.body);
	read(fd, &m, 1000);
	printf("%ld %s\n", m.pri, m.body);
	read(fd, &m, 1000);
	printf("%ld %s\n", m.pri, m.body);

	printf("setrpri=0\n", ioctl(fd, PQDEV_IOCTSETREADPRIO, 0));
	read(fd, &m, 1000);
	printf("%ld %s\n", m.pri, m.body);
	read(fd, &m, 1000);
	printf("%ld %s\n", m.pri, m.body);
	read(fd, &m, 1000);
	printf("%ld %s\n", m.pri, m.body);
	*/

	close(fd);
}
