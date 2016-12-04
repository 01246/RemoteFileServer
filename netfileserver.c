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
#include <errno.h>

#include "netfileserver.h"
#include "libnetfiles.h"

#define SERV_TCP_PORT 8001
#define SERV_TCP_PORT_STR "8001"
#define BACKLOG 5
#define THREAD_MAX 100
#define LOOP_BACK_ADDR "127.0.0.1"

Open_File_Data openFiles[100];
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



int doSomethingWithClientFD(int * sockfd) {
	//Sits on new socket to handle client library requests
	
	for (;;) {
		printf("Server: Preparing to say hello\n");

		//send(*sockfd, "Hello", 10, 0);
		//writen(*sockfd, "Hello Hello", strlen("Hello Hello"));
		Command_packet * cPtr = readCommand(*sockfd);
		switch(cPtr->type){
			case 1:{
				printf("1 Received %d %d %d %d\n", cPtr->type,cPtr->flag,cPtr->size,cPtr->status);
				char buf[300];
				bzero(buf,300);
				printf("About to read %d\n", cPtr->size);
				readn(*sockfd,buf,cPtr->size);
				printf("Read: %s\n", buf);

				FILE * filedes = fopen(buf,"a+");
				if (filedes == NULL)
				{				
					printf("ERROR can not Open file:-%s- %d\n", buf, errno);
				}
				printf("Opened file: \n");
				int i;
				for(i = 0; i<100; i++){
					if(openFiles[i].isActive==0){
						break;
					}
				}
				printf("chose location %d %zd\n", i, filedes);

				openFiles[i].fp = filedes;
				openFiles[i].isActive = 1;

				writeCommand(*sockfd,0,0,0,i);

			}
			break;

			case 2:{
				printf("2 Received %d %d %d %d\n", cPtr->type,cPtr->flag,cPtr->size,cPtr->status);
				int index = cPtr->status;

				int status = fclose(openFiles[index].fp);
				printf("netclose status %d %d\n", status, index);

				openFiles[index].isActive = 0;
				
				writeCommand(*sockfd,0,0,0,status);
			}
			break;

			case 3: {
				printf("3 Received %d %d %d %d\n", cPtr->type,cPtr->flag,cPtr->size,cPtr->status);
				FILE * filedes = openFiles[cPtr->status].fp;
				char * buf = malloc(cPtr->size);
				for(int i = 0; i < cPtr->size; i++){
					*buf = fgetc(filedes);
					buf++;
				}
				writen(*sockfd,buf, cPtr->size);
				writeCommand(*sockfd,0,0,cPtr->size,0);
			}
			break;

			case 4: {
				printf("4 Received %d %d %d %d\n", cPtr->type,cPtr->flag,cPtr->size,cPtr->status);
				int index = cPtr->status;
				FILE* filedes = openFiles[index].fp;
				printf("netwrite %zd %d \n", filedes, cPtr->size);
				char *buf = malloc(cPtr->size);
				bzero(buf,cPtr->size);

				readn(*sockfd,buf,cPtr->size);

				printf("netwrite buf %s \n", buf);

				for(int i = 0; i < cPtr->size; i++){
					fputc(*buf,filedes);
					printf("ferror %d \n", ferror(filedes));
					fflush(filedes);
					printf ("netwrite wrote a byte-%c-\n", *buf);
					buf++;
				}
				writeCommand(*sockfd,0,0,0,cPtr->size);
				//char buf[cPtr->size];

			}
			break;

			default: close(*sockfd);
			return 1;

		}
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

	
	for(int i = 0; i < 100; i++){
		//openFiles[i] = (Open_File_Data*)malloc(sizeof(Open_File_Data));
		openFiles[i].isActive = 0;
	}
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






