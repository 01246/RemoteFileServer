//Client Library 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "netfileserver.h"
#define SERV_TCP_PORT 8001
#define SERV_HOST_ADDR "127.0.0.1"

int sockfd;

int netserverinit(char * hostname){

    
    struct sockaddr_in serv_addr;

    /*
		Fill in the stucture "serv_addr" with the address of the server that we want to connect with 
    */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(hostname);
	serv_addr.sin_port = htons(SERV_TCP_PORT);

	/*
		Open a TCP stream socket
	*/

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("can not open a stream socket\n");
		return -1;
	}

	/*
		Connect to the server 
	*/
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		printf("Can not connect to the server\n");
		return -1;
	}

	// Function that for socket;

	return 0;

}

int netopen(const char *pathname, int flags){
	return 1;
}

 int netclose(int fd){
 	return 1;
 }

ssize_t netread(int fildes, void *buf, size_t nbyte){
	return 1;
}

ssize_t netwrite(int fildes, const void *buf, size_t nbyte){
	return 1;
}







