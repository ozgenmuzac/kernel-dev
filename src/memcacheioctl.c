#include<stdio.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<string.h>
#include<errno.h>

#include "test.h"

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

char m[1024];


int main() {
	int i;
	int n = sizeof(manls)/sizeof(manls[0]);
	int fd = open("/dev/memcache0",O_RDWR);
	int r;

	printf("minmsg: %d\n", ioctl(fd, MEMCACHE_IOCGETCACHE, (char *) &m));
	printf("%s\n", m);
	printf("Set: %d\n", ioctl(fd, MEMCACHE_IOCSETCACHE, "ozgen"));


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
