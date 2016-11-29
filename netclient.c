#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "libnetfiles.h"

int main(int argc, char *argv[]) {

	if ((netserverinit("127.0.0.1")) < 0) {
		printf("Cannot connect\n");
		exit(0);
	}

	// Call functions
	//netopen(*pathname, flags);
	//netclose(fd);
	//netread(fildes, *buf, nbyte);
	//netwrite(fildes, *buf, nbyte);

}