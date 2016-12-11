#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "libnetfiles.h"
#include "netclient.h"

#define LOOP_BACK_ADDR "127.0.0.1"
#define SERVER_IP_ADDR "172.27.203.159"
#define STEVE_MACHINEIP "192.168.1.9"
#define ILAB_MACHINE_IP "128.6.13.174"
#define IP_SIZE 50
#define BUFFER_MAX 100

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

/* MAIN
 *
 * Open file, write message, close file
 * Open file, read message, close file
 */
int main(int argc, char *argv[]) {

	char filename[50];
	char message[BUFFER_MAX];

	strcpy(filename, argv[1]);
	strcpy(message, argv[2]);

	// Declare and allocate memory for IP address of server
	char * ip_str = (char *)malloc(sizeof(char)*IP_SIZE);

	// Initialize IP address of server
	get_server_ip(ip_str);

	// Initialize server connection
	if ((netserverinit(ILAB_MACHINE_IP)) < 0) {
		printf("Cannot connect\n");
		exit(0);
	}

	// Open file
	int fd = netopen(filename, 0);
	int fd2 = netopen(filename, 0);

	netwrite(fd2, message, strlen(message));

	char * buf = (char *)malloc(sizeof(char)*BUFFER_MAX);
	bzero(buf, BUFFER_MAX);

	netread(fd, buf, BUFFER_MAX);

	netclose(fd);
	netclose(fd2);

	// Free IP address of server
	free(ip_str);

	return 0;
}