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

/* NETSERVERINIT
 *
 * Takes hostname and file connection mode and verifies that the host exists.
 * It makes sure your library attaches the file connection mode to every net file command a library sends.
 */
int netserverinit(char * hostname) {

    // Declare socket address struct
    struct sockaddr_in serv_addr;

    // Fill in struct with zeroes
	bzero((char *)&serv_addr, sizeof(serv_addr));

	// Initialize socket address
	serv_addr.sin_family = AF_INET;						// Set protocol to IPv4
	serv_addr.sin_addr.s_addr = inet_addr(hostname);	// Convert address
	serv_addr.sin_port = htons(SERV_TCP_PORT);			// Host-to-network-short of port

	// Open a TCP stream socket and error check
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot open a stream socket\n");
		return -1;
	}

	// Connect to the server
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Cannot connect to the server\n");
		return -1;
	}

	// Function that for socket
	send(sockfd, "Hello, Ms. Lady!", 20, 0);

	// Close socket
	close(sockfd);

	return 0;

}

/* NETOPEN
 *
 * Returns file descriptor if pathname with appropriate access mode.
 */
int netopen(const char * pathname, int flags) {
	return 1;
}

/* NETCLOSE
 *
 * Closes file.
 */
int netclose(int fd) {
	return 1;
}

/* NETREAD
 *
 * Returns the number of bytes successfully read from the file.
 */
ssize_t netread(int fildes, void * buf, size_t nbyte) {
	return 1;
}

/* NETWRITE
 *
 * Returns the number of bytes successfully written to the file.
 */
ssize_t netwrite(int fildes, const void * buf, size_t nbyte) {
	return 1;
}