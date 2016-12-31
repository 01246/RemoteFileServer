#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "libnetfiles.h"

#define LOOP_BACK_ADDR "127.0.0.1"
#define IP_SIZE 50
#define IN_FILENAME_MAX 50
#define BUFFER_MAX 50

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
	        }
	    }

	    // Go to next interface address struct
    	temp = temp->ifa_next;
	}

	// Free interface address linked list
	freeifaddrs(addrs);
}

/* MAIN */
int main(int argc, char *argv[]) {

	// Declare buffers
	char ip_str[IP_SIZE];
	char filename[IN_FILENAME_MAX];
	char message[BUFFER_MAX];

	// Copy command line arguments to buffers
	strcpy(ip_str, argv[1]);
	strcpy(filename, argv[2]);
	strcpy(message, argv[3]);
	
	// Initialize server connection
	int sockfd = netserverinit(ip_str);

	// Open file
	int fd = netopen(filename, sockfd, O_RDWR);

	// Allocate size for buffer
	char * buf = (char *)malloc(sizeof(char)*BUFFER_MAX);
	int flag;

	if (fd > -1) {

		// Read from file
		netread(sockfd, fd, buf, BUFFER_MAX);

		// Write to file
		netwrite(sockfd, fd, message, BUFFER_MAX);

		// Close file
		flag = netclose(sockfd, fd);
	}

	// Free allocated memory
	free(buf);

	return 0;
}