#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "libnetfiles.h"

#define LOOP_BACK_ADDR "127.0.0.1"
#define SERVER_IP_ADDR "172.27.203.159"
#define STEVE_MACHINEIP "192.168.1.9"

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
 */
int main(int argc, char *argv[]) {

	// Declare and allocate memory for IP address of server
	//char * ip_str = (char *)malloc(sizeof(char)*50);

	// Initialize IP address of server
	//get_server_ip(ip_str);

	// Initialize server connection
	if ((netserverinit(STEVE_MACHINEIP)) < 0) {
		printf("Cannot connect\n");
		exit(0);
	}

	int fd = netopen("test.txt",0);

	printf ("about to write hellow world\n");
	netwrite(fd,"Hello World!", 12);
	printf("wrote hello world\n");

	netclose(fd);

	fd = netopen("test.txt",0);


	char buf[100];
	bzero(buf,100);

	int bytesRead = netread(fd,&buf,12);

	printf("Buffer: %s %d\n", buf,bytesRead);

	netclose(fd);

	// Free IP address of server
	//free(ip_str);

	// Call functions
	/*
	netopen(*pathname, flags);
	netclose(fd);
	netread(fildes, *buf, nbyte);
	netwrite(fildes, *buf, nbyte);
	*/

}