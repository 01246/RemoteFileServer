#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "netfileserver.h"

#define SERV_TCP_PORT 8001
#define SERV_TCP_PORT_STR "8001"
#define SERV_HOST_ADDR "127.0.0.1" // Loop back address

int sockfd;

// get sockaddr, IPv4 or IPv6:
void * get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* NETSERVERINIT
 *
 * Takes hostname and file connection mode and verifies that the host exists.
 * It makes sure your library attaches the file connection mode to every net file command a library sends.
 */
int netserverinit(char * hostname) {
	printf("%s\n", hostname);

    // Declare socket address struct
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN]; // 46

    // Fill in struct with zeroes
	bzero((char *)&hints, sizeof(hints));

	// Manually initialize the address information
    hints.ai_family = AF_UNSPEC;			// IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;		// Sets as TCP

    // Automatically initialize the address information from host
    if (getaddrinfo(hostname, SERV_TCP_PORT_STR, &hints, &servinfo) != 0) {
        printf("Client: Cannot get server address information\n");
        return -1;
    }

    // Loop through server information for the appropriate address information to start a socket
	for (p = servinfo; p != NULL; p = p->ai_next) {

		// Attempt to open socket with address information
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			printf("socket() failed\n");
			continue;
		}

		// Connect to the server
		printf("Connecting...");
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) {

			// Report error
			printf("Client: Cannot connect to the server\n");

			// Close socket
			close(sockfd);
			continue;
		}
		printf("Connected\n");

		// Successful connection
		break;
	}

	// Check if socket was not bound
	if (p == NULL) {
		printf("Client: cannot bind to socket\n");
	}

	// Get IP address from socket address
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("Client: Connected to %s\n", s);

	// Free server information
	freeaddrinfo(servinfo);

	// Declare buffer
	char buf[10];

	// Receive data from server
	if (recv(sockfd, buf, 10, 0) < 0) {
        printf("Client: cannot receive message\n");
        return -1;
    }

    // Print data recieved from the server
    printf("Server: %s\n", buf);

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