#include<stdio.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<poll.h>
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
	int i,j;
	int n = sizeof(manls)/sizeof(manls[0]);
	struct mess m;
	int r,nread = 2,nwrite = 0, nfd;
	int *fds;
	char fname[30];
	struct pollfd *pfds;

	if (argc > 3) {
		fprintf(stderr,"usage: %s [nread [nwrite]]\n", argv[0]);
		return 1;
	} else if (argc > 1)
		nread = atoi(argv[1]); 

	if (argc > 2)
		nwrite = atoi(argv[2]);

	nfd = nread+nwrite;

	if (nfd > 8) {
		fprintf(stderr,"too many devices: %d\n", nfd);
		return 1;

	}

	fds = malloc(sizeof(int)*(nread+nwrite));
	pfds = malloc(sizeof(struct pollfd)*(nread+nwrite));

	if (fork()) {
		for (i = 0; i < nfd ; i++)  {
			sprintf(fname, "/dev/pqdev%d", i);

			printf("Opening %s for %s\n", fname, (i < nread) ? "r":"w");
			fds[i] = open(fname,(i < nread) ? O_RDONLY:O_WRONLY);
			pfds[i].fd = fds[i];
			pfds[i].events =  (i < nread) ? (POLLIN):(POLLOUT);
		}
		r = 0;
	
		for (i = 0; i < 20*nfd ; i++)  {  /* poll 60 events */
			/*poll no timeout */
			if ((r = poll(pfds, nfd, -1)) <= 0  )
				break;
			for (j = 0; j < nfd; j++) {
				/* for all devices */
				if (j < nread) {	/* read event */
					if (pfds[j].revents & POLLIN) {
						r = read(fds[j], &m, sizeof(m));
						printf("RPOLL(%d): %ld %s\n", 
							j, m.pri, m.body);
					}
				} else {
					if (pfds[j].revents & POLLOUT) {
						m.pri = i ;
						sprintf(m.body, "%3d %s", i,
							manls[i%n]);
						printf("WPOLL(%d): %ld %s\n", 
							j, m.pri, m.body);
						write(fds[j], &m, sizeof(long)+strlen(m.body)+1);
					}

				}
			}
		}
		if (r) {
			printf("%d %d\n", r, errno);
		}
		for (i = 0; i < nfd ; i++)
			close(fds[i]);
		wait(&r);
	} else {
		for (i = 0; i < nfd; i++)  {
			sprintf(fname, "/dev/pqdev%d", i);
			/* reverse read/writes */
			fds[i] = open(fname,(i < nread) ? O_WRONLY:O_RDONLY);
		}
		r = 0;
		for (i = 0; i < 20 ; i++) {
			for (j = 0; j < nfd; j++) {
				if (j < nread) {	/* write event */
					m.pri = i ;
					sprintf(m.body, "%3d %s", i,
						manls[i%n]);
					write(fds[j], &m, sizeof(long)+strlen(m.body)+1);
				} else {
					r = read(fds[j], &m, sizeof(m));
					printf("CHILDR(%d): %ld %s\n", j,
						m.pri, m.body);
				}
				usleep(200000);
			}
		}

		return 0;
	}
	
}
