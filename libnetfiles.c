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
#include "libnetfiles.h"

#define SERV_TCP_PORT 8001
#define SERV_TCP_PORT_STR "8001"
#define SERV_HOST_ADDR "127.0.0.1" // Loop back address

int sockfd;

/* GET_IN_ADDR
 *
 * 
 */
void * get_in_addr(struct sockaddr *sa) {

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

	return 0;
}

/* WRITECOMMAND
 *
 * Encodes a command from host to network and write across a socket.
 * Works in tandem with readCommand().
 */
void writeCommand(int sockfd, int type, int flag, int size, int status) {

	// Declare integer buffer
	int iBuf;

	// Write in type
	iBuf = htonl(type);
	writen(sockfd, (char *)&iBuf, 4);

	// Write in flag
	iBuf = htonl(flag);
	writen(sockfd, (char *)&iBuf, 4);

	// Write in size
	iBuf = htonl(size);
	writen(sockfd, (char *)&iBuf, 4);

	// Write in status
	iBuf = htonl(status);
	writen(sockfd, (char *)&iBuf, 4);

}

/* READCOMMAND
 *
 * Reads a command from a socket and fills in a packet struct.
 * Works in tandem with writeCommand().
 */
void * readCommand(int sockfd) {

	// Declare integer buffer
	int iBuf;

	// Allocate memory for command packet struct
	Command_packet * packet = (Command_packet *)malloc(sizeof(Command_packet));
	
	// Get type
	readn(sockfd, (char *)&iBuf, 4);
	packet->type = ntohl(iBuf);

	// Get flag
	readn(sockfd, (char *)&iBuf, 4);
	packet->flag = ntohl(iBuf);

	// Get size
	readn(sockfd, (char *)&iBuf, 4);
	packet->size = ntohl(iBuf);

	// Get status
	readn(sockfd, (char *)&iBuf, 4);
	packet->status = ntohl(iBuf);

	return (void *)packet;
}

/* READN
 *
 * Read n bytes from a file and return the number of bytes
 * successfully read.
 */
int readn(int fd, char * ptr, int nbytes) {
	
	// Declare and initialize counters
	int nleft, nread;
	nleft = nbytes;

	// Loop through reading bytes until EOF is found
	while (nleft > 0) {
		nread = read(fd, ptr, nleft);

		if (nread < 0) {			// Error
			return(nread);
		} else if (nread == 0) {	// EOF
			break;
		}
		nleft-=nread;
		ptr+=nread;
	}

	// Return the number of bytes successfully read
	return (nbytes-nleft);
}

/* WRITEN
 *
 * Write n bytes to a file from a string and return 
 * the number of bytes successfully writen.
 */
int writen(int fd, char * ptr, int nbytes) {

	// Declare and initialize counters
	int nleft, nwritten;
	nleft = nbytes;

	// Loop through writing bytes until all bytes are written
	while (nleft > 0) {
		nwritten = write(fd, ptr, nleft);

		if (nwritten < 1) {		// Error
			return (nwritten);
		}
		nleft-=nwritten;
		ptr+=nwritten;
	}

	// Return the number of bytes successfully written
	return (nbytes-nleft);
}

/* NETOPEN
 *
 * Returns file descriptor if pathname with appropriate access mode.
 */
int netopen(const char * pathname, int flags) {

	// Send command to server
	writeCommand(sockfd, 1, flags, strlen(pathname), 0);

	// Write the filename to the socket
	writen(sockfd, (char *)pathname, strlen(pathname));

	// Receive response from server
	Command_packet * cPack = (Command_packet *)readCommand(sockfd);

	// Return status received from server
	return cPack->status;
}

/* NETCLOSE
 *
 * Closes file.
 */
int netclose(int fd) {

	// Send command to server
	writeCommand(sockfd, 2, 0, 0, fd);

	// Receive response from server
	Command_packet * cPack = (Command_packet *)readCommand(sockfd);

	// Return status received from server
	return cPack->status;
}

/* NETREAD
 *
 * Returns the number of bytes successfully read from the file.
 */
ssize_t netread(int fd, void * buf, size_t nbyte) {

	// Send command to server
	writeCommand(sockfd, 3, 0, nbyte, fd);

	// Allocate memory for buffer
	buf = malloc(sizeof(nbyte));

	// Read character into buffer
	readn(sockfd, (char *)buf, nbyte);
	printf("Client: netread: %s %zd\n", buf, (size_t)nbyte);

	// Receive response from server
	Command_packet * packet = (Command_packet *)readCommand(sockfd);

	// Return size of read from server
	return packet->size;
}

/* NETWRITE
 *
 * Returns the number of bytes successfully written to the file.
 */
ssize_t netwrite(int fd, const void * buf, size_t nbyte) {

	// Send command to server
	writeCommand(sockfd, 4, 0, nbyte, fd);

	// Send buffer to be written to server
	writen(sockfd, (char *)buf, nbyte);
	printf("Client: netwrite: %s\n", buf);

	// Receive response from server
	Command_packet * cPack = (Command_packet *)readCommand(sockfd);

	// Return size of write from server
	return cPack->status;
}