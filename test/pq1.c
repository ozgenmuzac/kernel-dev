#include<stdio.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<string.h>
#include<errno.h>

#include "pqdev.h"

char * manls[]={
"",
"Ana",
"Anannananana",
"Baaafafafafafafafa112",
"Baaafafafafafafafa1123",
"Baaafafafafafafafa11245",
"Baaafafafafafafafa1124552424141",
"List information about the FILEs (the current directory by default)."};

struct mess 
{
	long pri;
	char body[1000];
};

int main(int argc, char *argv[]) 
{
	int len;
	int i;
	int n = sizeof(manls)/sizeof(manls[0]);
	struct mess m;
	int fd = open("/dev/pqdev0",O_WRONLY);
	int r;

	for(i = 0; i < 8; i++)
	{
		m.pri = 100;
		sprintf(m.body, "%s", manls[i]);
		len = sizeof(long)+strlen(m.body)+1;
		r = write(fd, &m, len);
	}

	close(fd);

	fd = open("/dev/pqdev0",O_RDONLY);
	i = 0; 
	usleep(1000000);
	while ( i < 11 && (r = read(fd, &m, sizeof(m))) >0 ) 
	{
		printf("%ld %s\n", m.pri, m.body);
		usleep(100);
		i++;
	}

	close(fd);

	/*if (fork()) {
	for (i = 0; i < 50; i++) {
		m.pri = i % 15 + 100;
		sprintf(m.body, "%3d %s", i, manls[i%n]);
		write(fd, &m, sizeof(long)+strlen(m.body)+1);
	}

	/* phase 2, blocking read */
	/*for (i = 50; i < 80; i++) {
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
	}*/

	return 0;	
}
