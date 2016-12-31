#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "netfileserver.h"
#include "libnetfiles.h"

/* GET_IN_ADDR
 *
 * Returns the correct IP address (IPv4 or IPv6)
 */
void * get_in_addr(struct sockaddr * sa) {

	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
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
    int sockfd, flag;

    // Fill in struct with zeroes
	bzero((char *)&hints, sizeof(hints));

	// Manually initialize the address information
    hints.ai_family = AF_UNSPEC;			// IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;		// Sets as TCP

    // Automatically initialize the address information from host
    if ((flag = getaddrinfo(hostname, SERV_TCP_PORT_STR, &hints, &servinfo)) != 0) {
        fprintf(stderr, "Client: %s\n", gai_strerror(flag));
        return -1;
    }

    // Loop through server information for the appropriate address information to start a socket
	for (p = servinfo; p != NULL; p = p->ai_next) {

		// Attempt to open socket with address information
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			perror("Client");
			continue;
		}

		// Connect to the server
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) {

			// Close socket
			close(sockfd);
			continue;
		}

		// Successful connection
		break;
	}

	// Check if socket was not bound
	if (p == NULL) {
		return -1;
	}

	// Get IP address from socket address
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));

	// Free server information
	freeaddrinfo(servinfo);

	return sockfd;
}

/* WRITECOMMAND
 *
 * Encodes a command from host to network and write across a socket.
 * Works in tandem with readCommand().
 */
void writeCommand(int s_fd, int type, int flag, int size, int status) {

	printf("writ: %d\n", s_fd);

	// Declare integer buffer
	int iBuf[4];

	// Write in packet
	iBuf[0] = htonl(type);
	iBuf[1] = htonl(flag);
	iBuf[2] = htonl(size);
	iBuf[3] = htonl(status);
	writen(s_fd, (char *)&iBuf, 16);
}

/* READCOMMANDSERVER
 *
 * Reads a command from a socket and fills in a packet struct.
 * Works in tandem with writeCommandServer().
 */
void readCommandServer(int sockfd, Command_packet * packet) {

	printf("readC %d %p\n", sockfd, packet);

	// Declare integer buffer
	int iBuf[4];

	// Read bytes
	readn(sockfd, (char *)&iBuf, 16);
	packet->type = ntohl(iBuf[0]);
	packet->flag = ntohl(iBuf[1]);
	packet->size = ntohl(iBuf[2]);
	packet->status = ntohl(iBuf[3]);
}

/* READCOMMAND
 *
 * Reads a command from a socket and fills in a packet struct.
 * Works in tandem with writeCommand().
 */
void * readCommand(int sockfd) {

	printf("read: %d\n", sockfd);

	// Allocate memory for command packet struct
	Command_packet * packet = (Command_packet *)malloc(sizeof(Command_packet));

	// Declare integer buffer
	int iBuf[4];

	// Read bytes
	readn(sockfd, (char *)(&iBuf[0]), 16);
	packet->type = ntohl(iBuf[0]);
	packet->flag = ntohl(iBuf[1]);
	packet->size = ntohl(iBuf[2]);
	packet->status = ntohl(iBuf[3]);
	
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
int netopen(const char * pathname, int sockfd, int flags) {

	printf("netopen\n");

	// Send command to server
	writeCommand(sockfd, 1, flags, strlen(pathname), 0);

	// Write the filename to the socket
	errno = 0;
	writen(sockfd, (char *)pathname, strlen(pathname));
	if (errno != 0) {
		perror("Client");
		errno = 0;
	}

	// Receive response from server
	Command_packet * cPack = (Command_packet *)readCommand(sockfd);

	// Get file descriptor index and free command packet
	errno = cPack->flag;
	int fd = cPack->status;
	free(cPack);

	if (fd < 0) {
		perror("Client");
	}

	// Return file descriptor index received from server
	return fd;
}

/* NETCLOSE
 *
 * Closes file.
 */
int netclose(int sockfd, int fd) {

	printf("netclose\n");

	// Send command to server
	writeCommand(sockfd, 2, 0, 0, fd);

	// Receive response from server
	Command_packet * cPack = (Command_packet *)readCommand(sockfd);

	// Get status and free command packet
	errno = 0;
	errno = cPack->flag;
	int stat = cPack->status;
	free(cPack);

	if (stat < 0) {
		perror("Client");
	}

	// Return status received from server
	return stat;
}

/* NETREAD
 *
 * Returns the number of bytes successfully read from the file.
 */
ssize_t netread(int sockfd, int fd, void * buf, size_t nbyte) {

	printf("netread\n");

	// Send command to server
	writeCommand(sockfd, 3, 0, nbyte, fd);

	// Read character into buffer
	errno = 0;
	readn(sockfd, (char *)buf, nbyte);
	if (errno != 0) {
		perror("Client");
		errno = 0;
	}

	// Receive response from server
	Command_packet * cPack = (Command_packet *)readCommand(sockfd);

	// Get status and free command packet
	int size = cPack->size;
	errno = cPack->flag;
	free(cPack);

	// Check if the number of bytes read is correct
	if (size != nbyte) {
		perror("Client");
	}

	// Return size of read from server
	return size;
}

/* NETWRITE
 *
 * Returns the number of bytes successfully written to the file.
 */
ssize_t netwrite(int sockfd, int fd, const void * buf, size_t nbyte) {

	printf("netwrite\n");

	// Send command to server
	writeCommand(sockfd, 4, 0, nbyte, fd);

	// Send buffer to be written to server
	errno = 0;
	writen(sockfd, (char *)buf, nbyte);
	if (errno != 0) {
		perror("Client");
		errno = 0;
	}

	// Receive response from server
	Command_packet * cPack = (Command_packet *)readCommand(sockfd);

	// Get status and free command packet
	int size = cPack->size;
	errno = cPack->flag;
	free(cPack);

	// Check if the number of bytes written is correct
	if (size != nbyte) {
		perror("Client");
	}

	// Return status received from server
	return size;
}