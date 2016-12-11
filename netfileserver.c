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
#define OPEN_FILES_MAX 100

Open_File_Data openFiles[THREAD_MAX][OPEN_FILES_MAX];
int client_id = -1;

/* GET_SERVER_IP
 *
 * Fills in string with IPv4 address from getifaddrs().
 */
int get_server_ip(char * ip_str) {

	// Declare interface address structs
	struct ifaddrs *addrs, *temp;

	// Initialize addrs struct
	if (getifaddrs(&addrs) < 0) {
		perror("Server");
		return -1;
	}
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
	        	break;
	        }
	    }

	    // Go to next interface address struct
    	temp = temp->ifa_next;
	}

	// Free interface address linked list
	freeifaddrs(addrs);

	return 0;
}


/* EXECUTECLIENTCOMMANDS
 *
 * Take commands across the network and execute.
 */
int executeClientCommands(int * sockfd) {
	
	// Loop through incoming client commands
	while (1) {

		// Receive command packet from client
		Command_packet * cPtr = (Command_packet *)readCommand(*sockfd);

		// Declare and initialize buffers, counters, and file descriptors
		int i, status;
		int cli_id = client_id;
		int index = cPtr->status;
		char * buf = (char *)malloc(sizeof(char)*cPtr->size);
		FILE * fd;

		/*
		printf("Received type:%d, flag:%d, size:%d, status:%d\n", 
			cPtr->type, 
			cPtr->flag, 
			cPtr->size, 
			cPtr->status
		);
		*/

		switch (cPtr->type) {
			case 1: // Open

				// Zero buffer
				bzero(buf, cPtr->size);

				// Read filename
				readn(*sockfd, buf, cPtr->size);

				// Open file
				fd = fopen(buf, "a+");

				// Error check file
				if (fd == NULL) {				
					printf("ERROR cannot open file:-%s-%d\n", buf, errno);
				}

				// Loop through array to find first open index
				for (i = 0; i < OPEN_FILES_MAX; i++) {
					if (!openFiles[cli_id][i].isActive) {
						break;
					}
				}
				printf("Server: netopen  %d %zd %s index:%d\n", cli_id, (size_t)fd, buf, i);

				// Initialize open file array at corresponding index
				openFiles[cli_id][i].fp = fd;
				openFiles[cli_id][i].isActive = 1;

				// Send response to client
				writeCommand(*sockfd, 0, 0, 0, i);
				break;

			case 2: // Close

				// Close file
				status = fclose(openFiles[cli_id][index].fp);
				printf("Server: netclose %d %zd status:%d  index:%d\n", cli_id, (size_t)openFiles[cli_id][index].fp, status, index);

				// Set file as inactive in open file array
				openFiles[cli_id][index].isActive = 0;
				
				// Send message to client
				writeCommand(*sockfd, 0, 0, 0, status);
				break;

			case 3: // Read

				// Get file descriptor and jump to front of file
				fd = openFiles[cli_id][index].fp;
				fseek(fd, 0, SEEK_SET);
				printf("Server: netread  %d %zd size:%d\n", cli_id, (size_t)fd, cPtr->size);

				// Zero out buffer
				bzero(buf, cPtr->size);
				
				// Read one byte at time from file across the network
				for (i = 0; i < cPtr->size && !feof(fd); i++) {
					buf[i] = fgetc(fd);
				}

				// Send the bytes read to the client
				writen(*sockfd, buf, cPtr->size);

				// Send message to client
				writeCommand(*sockfd, 0, 0, cPtr->size, 0);
				break;

			case 4: // Write

				// Get file descriptor
				fd = openFiles[cli_id][index].fp;
				printf("Server: netwrite %d %zd size:%d\n", cli_id, (size_t)fd, cPtr->size);

				// Zero out buffer
				bzero(buf, cPtr->size);

				// Read the bytes to be written from the client
				readn(*sockfd, buf, cPtr->size);

				// Write one byte at time from file across the network
				for (i = 0; i < cPtr->size; i++, buf++) {
					fputc(*buf, fd);
					//printf("Server: netwrite wrote a byte-%c-%d\n", *buf, ferror(fd));
					fflush(fd);
				}

				// Send message to client
				writeCommand(*sockfd, 0, 0, 0, cPtr->size);
				break;

			default:
				close(*sockfd);
				return 1;
		}
	}
	return 1;
}

/* MAIN
 *
 * Initializes a server then waits for connections.
 * When a connection is made, a thread is created and 
 * the server continues to wait for more connections.
 */
int main(int argc, char *argv[]) {

	// Initialize openFiles array
	int i, j;
	for (i = 0; i < THREAD_MAX; i++) {
		for (j = 0; j < OPEN_FILES_MAX; j++) {
			openFiles[i][j].isActive = 0;
		}
	}

	// Declare and allocate memory for IP address of server
	char * ip_str = (char *)malloc(sizeof(char)*50);

	// Initialize IP address of server
	if (get_server_ip(ip_str) < 0) {
		return 0;
	}

	// Declare socket descriptors and client length
	int sockfd, cli_fd; 
	socklen_t cli_len;

	// Check for input error
	if (argc != 2) {
   		fprintf(stderr, "usage: %s <port>\n", argv[0]);
  		exit(1);
 	}

 	// Initialize port string
 	char port[5];
 	strcpy(port, argv[1]);

	// Declare address information struct
	struct addrinfo hints, cli_addr, *servinfo, *p;

	// Fill in struct with zeroes
	bzero((char *) &hints, sizeof(hints));

	// Manaually initialize address information struct
	hints.ai_family = AF_UNSPEC;					// IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;				// Sets as TCP

	// Automatically initialize address information
	if (getaddrinfo(ip_str, port, &hints, &servinfo) != 0) {
		perror("Server");
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
		perror("Server");
	}

	// Free server information
	freeaddrinfo(servinfo);

	// Server waits for incoming connection requestions; sockfd will be the socket to satisfy these requests
	if (listen(sockfd, BACKLOG) < 0) {
		perror("Server");
	}

	// Initialize thread data
	pthread_t clientThreads[THREAD_MAX];
	int flag = 0;
	i = 0;

	// Sits on accept, waiting for new clients
	while (i < THREAD_MAX) {
		printf("Listening...\n");

		// Initialize client socket size
		cli_len = sizeof(cli_addr);
		client_id++;

		// Accept client socket
		cli_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);

		// Error check accept()
		if (cli_fd < 0) {
			perror("Server");
		} else {
			printf("Server: Connection accepted\n");
		}

		// Open thread for client and pass in client file descriptor
		if ((flag = pthread_create(&clientThreads[i], NULL, (void *)executeClientCommands, (int *)&cli_fd))) {
			printf("Server: pthread_create() error");
		} else if (flag == 0) {
			i++; 	
		}
	}

	// Join threads
	for (j = 0, flag = 0; j < i; i++) {

		// Join threads
		flag = pthread_join(clientThreads[j], NULL);

		// Erroring checking
		if (flag) {
			fprintf(stderr, "Server: pthread_join() exited with status %d\n", flag);
		}
	}

	// Free IP address of server
	free(ip_str);

	return 0;
}