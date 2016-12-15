#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "netfileserver.h"
#include "libnetfiles.h"

typedef struct thread_data {
	int * cli_fd;
	int client_id;
	Command_packet * cPtr;
} Thread_data;

int executeClientCommands(Thread_data * td);

#define SERV_TCP_PORT_STR "9000"
#define BACKLOG 5
#define THREAD_MAX 100
#define LOOP_BACK_ADDR "127.0.0.1"
#define OPEN_FILES_MAX 100

Open_File_Data openFiles[THREAD_MAX][OPEN_FILES_MAX];
int client_id = 0;
pthread_mutex_t m_lock = PTHREAD_MUTEX_INITIALIZER;

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
int executeClientCommands(Thread_data * td) {

	// Initialize socket and client ID
	int * sockfd = td->cli_fd;
	int client_id = td->client_id;
	Command_packet * cPtr = td->cPtr;

	// Loop through incoming client commands
	while (1) {

		// Receive command packet from client
		bzero(cPtr, sizeof(Command_packet));
		readCommandServer(*sockfd, cPtr);

		// Declare and initialize buffers, counters, and file descriptors
		int i, status, nbytes;
		int cli_id = client_id;
		int cmd_type = cPtr->type;
		int flag = cPtr->flag;
		int wr_size = cPtr->size;
		int fd_index = cPtr->status;

		if (fd_index < 0 || fd_index > OPEN_FILES_MAX-1) {
			printf("Server: file not active: [%d][%d]\n", cli_id, fd_index);
			writeCommand(*sockfd, 0, 9, -1, -1);
			break;
		}

		pthread_mutex_lock(&m_lock);
		char * buf = (char *)malloc(sizeof(char)*(wr_size+1));
		pthread_mutex_unlock(&m_lock);
		FILE * fd;

		printf("Server    type:%d, flag:%d, size:%d, status:%d\n\n", 
			cPtr->type, 
			cPtr->flag, 
			cPtr->size, 
			cPtr->status
		);

		switch (cmd_type) {
			case 1: // Open

				// Zero buffer
				bzero(buf, (wr_size+1));

				// Read filename
				nbytes = readn(*sockfd, buf, wr_size);

				// Error check readn
				if (nbytes != wr_size){
					printf("ERROR reading filename\n");
				}

				// Open file according to flag
				switch (flag) {
					case O_RDONLY:
						fd = fopen(buf, "r");
						break;
					case O_WRONLY:
						fd = fopen(buf, "w");
						break;
					case O_RDWR:
						fd = fopen(buf, "a+");
						break;
					default:
						fd = NULL;
						break;
				}
				
				// Error check file
				if (fd == NULL) {				
					printf("Server cannot open file: %s %d\n", buf, errno);
					writeCommand(*sockfd, 0, errno, 0, -1);
					free(buf);
					break;
				}

				// Loop through array to find first open fd_index
				for (i = 0; i < OPEN_FILES_MAX; i++) {
					if (!openFiles[cli_id][i].isActive) {
						break;
					}
				}
				printf("Server:   netopen  %d %zd %s fd_index:%d\n", cli_id, (size_t)fd, buf, i);

				// Initialize open file array at corresponding fd_index
				openFiles[cli_id][i].fp = fd;
				openFiles[cli_id][i].isActive = 1;

				// Send response to client
				writeCommand(*sockfd, 0, 0, 0, i);

				// Free buffer
				free(buf);

				break;

			case 2: // Close

				// Check if file is active
				if (!openFiles[cli_id][fd_index].isActive) {
					printf("Server: file not active: [%d][%d]\n", cli_id, fd_index);
					writeCommand(*sockfd, 0, 9, 0, -1);
					break;
				}	

				// Close active file
				status = fclose(openFiles[cli_id][fd_index].fp);
				printf("Server:   netclose %d %zd status:%d  fd_index:%d\n", 
					cli_id, (size_t)openFiles[cli_id][fd_index].fp, status, fd_index);

				// Set file as inactive in open file array
				openFiles[cli_id][fd_index].isActive = 0;
				
				// Send message to client
				writeCommand(*sockfd, 0, 0, 0, status);
				break;

			case 3: // Read

				// Check if file is active
				if (!openFiles[cli_id][fd_index].isActive) {
					printf("Server: file not active: [%d][%d]\n", cli_id, fd_index);
					writeCommand(*sockfd, 0, 9, -1, 0);
					break;
				}

				// Get file descriptor and jump to front of file
				fd = openFiles[cli_id][fd_index].fp;
				fseek(fd, 0, SEEK_SET);
				printf("Server:   netread  %d %zd size:%d\n", cli_id, (size_t)fd, wr_size);

				// Zero out buffer
				bzero(buf, wr_size+1);
				
				// Read one byte at time from file across the network
				for (i = 0; i < wr_size && !feof(fd); i++) {
					buf[i] = fgetc(fd);
				}

				// Send the bytes read to the client
				nbytes = writen(*sockfd, buf, wr_size);

				// Error check writen
				if (nbytes != wr_size){
					printf("ERROR writing buffer\n");
				}

				// Send message to client
				writeCommand(*sockfd, 0, 0, wr_size, 0);

				// Free buffer
				free(buf);

				break;

			case 4: // Write

				// Check if file is active
				if (!openFiles[cli_id][fd_index].isActive) {
					printf("Server: file not active: [%d][%d]\n", cli_id, fd_index);
					writeCommand(*sockfd, 0, 9, -1, 0);
					break;
				}

				// Get file descriptor
				fd = openFiles[cli_id][fd_index].fp;
				printf("Server:   netwrite %d %zd size:%d\n", cli_id, (size_t)fd, wr_size);

				// Zero out buffer
				bzero(buf, wr_size+1);

				// Read the bytes to be written from the client
				nbytes = readn(*sockfd, buf, wr_size);

				// Error check readn
				if (nbytes != wr_size){
					printf("ERROR writing buffer\n");
				}

				// Write one byte at time from file across the network
				for (i = 0; i < wr_size; i++) {
					fputc(buf[i], fd);
					fflush(fd);
				}

				// Send message to client
				errno = 0;
				printf("Server:   before write\n");
				writeCommand(*sockfd, 0, 0, wr_size, 0);
				printf("Server:   after write\n");
				perror("Server");

				// Free buffer
				free(buf);
				break;

			default:
				// tell client to close the socket
				printf("Server:   exit() %d\n", cli_id);
				free(buf);
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

	// Declare address information struct
	struct addrinfo hints, cli_addr, *servinfo, *p;

	// Fill in struct with zeroes
	bzero((char *) &hints, sizeof(hints));

	// Manaually initialize address information struct
	hints.ai_family = AF_UNSPEC;					// IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;				// Sets as TCP

	// Automatically initialize address information
	if (getaddrinfo(ip_str, SERV_TCP_PORT_STR, &hints, &servinfo) != 0) {
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
	Thread_data * td[THREAD_MAX];
	int flag = 0;
	i = 0;

	// Sits on accept, waiting for new clients
	while (i < THREAD_MAX) {
		printf("Listening...\n");

		// Initialize client socket size
		cli_len = sizeof(cli_addr);

		// Accept client socket
		cli_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);

		// Error check accept()
		if (cli_fd < 0) {
			perror("Server");
		} else {
			printf("Server: Connection accepted: %d\n", cli_fd);
			td[i] = (Thread_data *)malloc(sizeof(Thread_data));
			td[i]->cli_fd = &cli_fd;
			td[i]->client_id = client_id;
			td[i]->cPtr = (Command_packet *)malloc(sizeof(Command_packet));
		}

		// Open thread for client and pass in client file descriptor
		if ((flag = pthread_create(&clientThreads[i], NULL, (void *)executeClientCommands, (Thread_data *)td[i]))) {
			printf("Server: pthread_create() error");
		} else if (flag == 0) {
			i++;
			client_id++;
		}
	}

	// Join threads
	for (j = 0, flag = 0; j < i; j++) {

		// Join threads
		flag = pthread_join(clientThreads[j], NULL);
		free(td[i]->cPtr);
		free(td[i]);

		// Erroring checking
		if (flag) {
			fprintf(stderr, "Server: pthread_join() exited with status %d\n", flag);
		}
	}

	// Free IP address of server
	free(ip_str);

	return 0;
}