#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
//#include "netfileserver.h"
#include "libnetfiles.h"

#define SERV_TCP_PORT 8001
#define BACKLOG 5
#define THREAD_MAX 100

int readn(int fd, char * ptr, int nbytes) {
	int nleft, nread;

	nleft = nbytes;
	while (nleft > 0) {
		//nread = read(fd, ptr, nleft);
		nread = recv(fd, ptr, nbytes, 0);

		if (nread < 0) {
			return(nread);	//error
		} else if (nread == 0) {
			break;			//EOF
		}
		nleft -= nread;
		ptr +=nread;
	}
	return (nbytes-nleft);
}

int writen(int fd, char * ptr, int nbytes) {
	int nleft, nwritten;

	nleft = nbytes;
	while (nleft > 0) {
		nwritten = write(fd,ptr,nleft);
		if (nwritten <= 0) {
			// ERROR
			return (nwritten);
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (nbytes-nleft);
}

int doSomethingWithClientFD(int * sockfd) {
	//Sits on new socket to handle client library requests
	int i = 0;
	for (; i < 1; i++) {
		printf("doSomethingWithClientFD: %d\t", *sockfd);

		//char * buffer[256]; CURRENTLY UNUSED

		// Read command packet from socket
		//Command_packet packet;
		char * str = (char *)malloc(sizeof(char)*20);
		int nleft = readn(*sockfd, str, strlen(str));
		printf("Command_packet type: %d, %s\n", nleft, str);

		//switch on Command_packet type variable

		//read additional data if necessary 

		//execute command

		//populate return command packet with status 

		//write command packet to socket

		//write additional data if necessary 
	}
	return 1;
}

int main(int argc, char *argv[]) {

	// Declare socket descriptors and client length
	int sockfd, clientfd; 
	socklen_t clilen;

	// Check for input error
	if (argc < 2) {
   		printf("ERROR: no port provided\n");
  		exit(1);
 	}

 	// Open a TCP stream socket and error check
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Server: cannot open stream socket\n");
	}

	// Declare socket address struct
	struct sockaddr_in serv_addr, cli_addr;

	// Fill in struct with zeroes
	bzero((char *) &serv_addr, sizeof(serv_addr));

	// Initialize socket address
	serv_addr.sin_family = AF_INET;					// IPv4
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);	// Allows socket to bind to all available interfaces
	serv_addr.sin_port = htons(SERV_TCP_PORT);		// Set socket to wait on specified port

	// Binds server socket to a specific port to prepare for listen()
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Can't bind local address\n");
	}

	// Server waits for incoming connection requestions; sockfd will be the socket to satisfy these requests
	if (listen(sockfd, BACKLOG) < 0) {
		printf("Error in listen()\n");
	}

	// Initialize thread data
	pthread_t clientThreads[THREAD_MAX];
	int flag = 0;
	int i = 0;

	// Sits on accept, waiting for new clients
	while (i < THREAD_MAX) {
		printf("Listening...\n");

		// Initialize client socket size
		clilen = sizeof(cli_addr);

		// Accept client socket
		clientfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		// Error check accept()
		if (clientfd < 0) {
			printf("Server: accept() error");
		} else {
			printf("Received accept: blocking\n");
		}

		// Open thread for client
		if ((flag = pthread_create(&clientThreads[i], NULL, (void *)doSomethingWithClientFD, (int *)&sockfd))) {
			printf("Server: pthread_create() error");
		} else if (flag == 0) {
			printf("CONNECTION MADE\n");
			i++; 	
		}
	}

	int j;
	flag = 0;
	for (j = 0; j < i; i++) {

		// Join threads
		flag = pthread_join(clientThreads[j], NULL);

		// Erroring checking
		if (flag) {
			fprintf(stderr, "ERROR: pthread_join() exited with status %d\n", flag);
		}
	}
}






