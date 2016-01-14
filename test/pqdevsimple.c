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

int main(int argc, char *argv[]) {
	int i;
	int n = sizeof(manls)/sizeof(manls[0]);
	struct mess m;
	int fd = open("/dev/pqdev0",O_WRONLY);
	int r;

	if (fork()) {
	for (i = 0; i < 50; i++) {
		m.pri = i % 15 + 100;
		sprintf(m.body, "%3d %s", i, manls[i%n]);
		write(fd, &m, sizeof(long)+strlen(m.body)+1);
	}

	/* phase 2, blocking read */
	for (i = 50; i < 80; i++) {
		m.pri = i % 15 + 200;
		sprintf(m.body, "%3d %s", i, manls[i%n]);
		write(fd, &m, sizeof(long)+strlen(m.body)+1);
		usleep(250000);
	}
	close(fd);
	wait(&r);
	} else {
	close(fd);
	fd = open("/dev/pqdev0",O_RDONLY);
	i = 0; 
	usleep(1000000);
	while ( i < 80 && (r = read(fd, &m, sizeof(m))) >0 ) {
		printf("%ld %s\n", m.pri, m.body);
		usleep(100);
		i++;
	}
	perror("read");
	printf("%d\n", errno);
	close(fd);
	}
	
}
