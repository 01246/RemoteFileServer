#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "netfileserver.h"
#include "libnetfiles.h"

#define SERV_TCP_PORT 8001
#define SERV_TCP_PORT_STR "8001"
#define BACKLOG 5
#define THREAD_MAX 100
#define LOOP_BACK_ADDR "127.0.0.1"

/* GET_SERVER_IP
 *
 * Fills in string with IPv4 address from getifaddrs()
 */
void get_server_ip(char * ip_str) {

	// Declare interface address structs
	struct ifaddrs *addrs, *temp;

	// Initialize addrs struct
	getifaddrs(&addrs);
	temp = addrs;

	// Traverse interface address linked list
	while (temp) {

		// Check if interface address is an IPv4
	    if (temp->ifa_addr && temp->ifa_addr->sa_family == AF_INET) {

	    	// Fill in temp socket address 
	        struct sockaddr_in *t_sockaddr = (struct sockaddr_in *)temp->ifa_addr;

	        // If temp socket address is not the LOOP_BACK_ADDR, copy into ip_str
	        if (strcmp(LOOP_BACK_ADDR, inet_ntoa(t_sockaddr->sin_addr)) != 0) {
	        	strcpy(ip_str, inet_ntoa(t_sockaddr->sin_addr));
	        	printf("%s\n", ip_str);
	        }
	    }

	    // Go to next interface address struct
    	temp = temp->ifa_next;
	}

	// Free interface address linked list
	freeifaddrs(addrs);
}

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
		printf("Server: Preparing to say hello\n");

		send(*sockfd, "Hello", 10, 0);

		// Read command packet from socket

		//Command_packet packet;

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

	// Declare and allocate memory for IP address of server
	char * ip_str = (char *)malloc(sizeof(char)*50);

	// Initialize IP address of server
	get_server_ip(ip_str);

	// Declare socket descriptors and client length
	int sockfd, cli_fd; 
	socklen_t cli_len;

	// Check for input error
	if (argc < 2) {
   		printf("ERROR: no port provided\n");
  		exit(1);
 	}

	// Declare address information struct
	struct addrinfo hints, cli_addr, *servinfo, *p;

	// Fill in struct with zeroes
	bzero((char *) &hints, sizeof(hints));

	// Manaually initialize address information struct
	hints.ai_family = AF_UNSPEC;					// IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;				// Sets as TCP

	// Automatically initialize address information
	if (getaddrinfo(ip_str, SERV_TCP_PORT_STR, &hints, &servinfo) != 0) {
		printf("Server: could not get address information\n");
		exit(1);
	}

	// Loop through server information for the appropriate address information to start a socket
	for (p = servinfo; p != NULL; p = p->ai_next) {

		// Attempt to open socket with address information
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			continue;
		}

		// Binds server socket to a specific port to prepare for listen()
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
			continue;
		}

		// Successful connection
		break;
	}

	// Check if socket was not bound
	if (p == NULL) {
		printf("Server: cannot bind to socket\n");
	}

	// Free server information
	freeaddrinfo(servinfo);

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
		cli_len = sizeof(cli_addr);

		// Accept client socket
		cli_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);

		// Error check accept()
		if (cli_fd < 0) {
			printf("Server: accept() error");
		} else {
			printf("Server: connection accepted\n");
		}

		// Open thread for client and pass in client file descriptor
		if ((flag = pthread_create(&clientThreads[i], NULL, (void *)doSomethingWithClientFD, (int *)&cli_fd))) {
			printf("Server: pthread_create() error");
		} else if (flag == 0) {
			printf("CONNECTION MADE\n");
			i++; 	
		}
	}

	// Join threads
	int j;
	for (j = 0, flag = 0; j < i; i++) {

		// Join threads
		flag = pthread_join(clientThreads[j], NULL);

		// Erroring checking
		if (flag) {
			fprintf(stderr, "ERROR: pthread_join() exited with status %d\n", flag);
		}
	}

	// Free IP address of server
	free(ip_str);
}






